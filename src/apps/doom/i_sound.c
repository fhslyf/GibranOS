#include "doomtype.h"
#include "i_sound.h"
#include "sounds.h"
#include "w_wad.h"
#include "z_zone.h"
#include "../../drivers/io.h"
#include <stdio.h>
#include <stdint.h>

extern uint32_t DG_GetTicksMs(void);

int snd_sfxdevice = 3;
int snd_musicdevice = 3;
int snd_samplerate = 11025;
int snd_cachesize = 64 * 1024 * 1024;
int snd_maxslicetime_ms = 28;
char *snd_musiccmd = "";

#define SB_BASE        0x220
#define SB_MIXER_ADDR  (SB_BASE + 0x04)
#define SB_MIXER_DATA  (SB_BASE + 0x05)
#define SB_RESET       (SB_BASE + 0x06)
#define SB_READ_DATA   (SB_BASE + 0x0A)
#define SB_WRITE_DATA  (SB_BASE + 0x0C)
#define SB_READ_STATUS (SB_BASE + 0x0E)

#define DMA_BUF_ADDR   0x00070000
#define DMA_BUF_SIZE   4096
#define MAX_CHANNELS   8

typedef struct {
    const uint8_t* data;
    uint32_t length;
    uint32_t pos_fp;
    uint32_t step_fp;
    int vol;
    int active;
} sfx_channel_t;

static sfx_channel_t channels[MAX_CHANNELS];
static int sb16_ready = 0;
static uint16_t last_dma_pos = 0;
static uint16_t write_pos = 0;

static void opl_music_tick(void);

static void sb16_mixer_write(uint8_t reg, uint8_t val) {
    outb(SB_MIXER_ADDR, reg);
    outb(SB_MIXER_DATA, val);
}

static void sb16_write(uint8_t val) {
    for (int i = 0; i < 100000; i++) {
        if ((inb(SB_WRITE_DATA) & 0x80) == 0) break;
    }
    outb(SB_WRITE_DATA, val);
}

static int sb16_reset(void) {
    outb(SB_RESET, 1);
    for (volatile int i = 0; i < 2000; i++) (void)inb(SB_RESET);
    outb(SB_RESET, 0);

    for (int i = 0; i < 100000; i++) {
        if (inb(SB_READ_STATUS) & 0x80) {
            if (inb(SB_READ_DATA) == 0xAA) return 1;
        }
    }
    return 0;
}

static uint16_t dma_get_pos(void) {
    outb(0x0C, 0x00);
    uint8_t lo = inb(0x02);
    uint8_t hi = inb(0x02);
    uint16_t cur = ((uint16_t)hi << 8) | lo;
    return cur & (DMA_BUF_SIZE - 1);
}

static void mix_samples(int count) {
    volatile uint8_t* dma_buf = (volatile uint8_t*)DMA_BUF_ADDR;
    for (int i = 0; i < count; i++) {
        int mixed = 0;
        for (int c = 0; c < MAX_CHANNELS; c++) {
            if (!channels[c].active) continue;
            uint32_t idx = channels[c].pos_fp >> 16;
            if (idx >= channels[c].length) {
                channels[c].active = 0;
                continue;
            }
            int sample = (int)channels[c].data[idx] - 128;
            mixed += (sample * channels[c].vol) >> 7;
            channels[c].pos_fp += channels[c].step_fp;
        }
        mixed += 128;
        if (mixed < 0) mixed = 0;
        if (mixed > 255) mixed = 255;
        dma_buf[write_pos] = (uint8_t)mixed;
        write_pos = (write_pos + 1) & (DMA_BUF_SIZE - 1);
    }
}

void doom_sound_tick(void) {
    opl_music_tick();
    if (!sb16_ready) return;
    uint16_t cur = dma_get_pos();
    uint16_t consumed = (cur - last_dma_pos) & (DMA_BUF_SIZE - 1);
    if (consumed > 0) {
        last_dma_pos = cur;
        mix_samples((int)consumed);
    }
}

void I_InitSound(boolean use_sfx_prefix) {
    (void)use_sfx_prefix;
    for (int i = 0; i < MAX_CHANNELS; i++) channels[i].active = 0;

    volatile uint8_t* dma_buf = (volatile uint8_t*)DMA_BUF_ADDR;
    for (int i = 0; i < DMA_BUF_SIZE; i++) dma_buf[i] = 0x80;

    if (!sb16_reset()) {
        sb16_ready = 0;
        return;
    }

    // Buka penuh volume Master (0x22), Voice/PCM (0x04), dan FM Synth (0x26) di mixer SB16
    sb16_mixer_write(0x22, 0xFF);
    sb16_mixer_write(0x04, 0xFF);
    sb16_mixer_write(0x26, 0xFF);
    sb16_mixer_write(0x30, 0xF8);
    sb16_mixer_write(0x31, 0xF8);
    sb16_mixer_write(0x32, 0xF8);
    sb16_mixer_write(0x33, 0xF8);
    sb16_mixer_write(0x34, 0xF8);
    sb16_mixer_write(0x35, 0xF8);

    outb(0x0A, 0x05);
    outb(0x0C, 0x00);
    outb(0x0B, 0x59);
    outb(0x83, 0x07);
    outb(0x02, 0x00);
    outb(0x02, 0x00);
    outb(0x03, (DMA_BUF_SIZE - 1) & 0xFF);
    outb(0x03, ((DMA_BUF_SIZE - 1) >> 8) & 0xFF);
    outb(0x0A, 0x01);

    sb16_write(0xD1);
    sb16_write(0x41);
    sb16_write((11025 >> 8) & 0xFF);
    sb16_write(11025 & 0xFF);
    sb16_write(0xC6);
    sb16_write(0x00);
    uint16_t half = (DMA_BUF_SIZE / 2) - 1;
    sb16_write(half & 0xFF);
    sb16_write((half >> 8) & 0xFF);

    last_dma_pos = dma_get_pos();
    write_pos = (last_dma_pos + (DMA_BUF_SIZE / 2)) & (DMA_BUF_SIZE - 1);
    sb16_ready = 1;
    printf("[SFX] sb16_ready=%d\n", sb16_ready);
}

void I_ShutdownSound(void) {
    if (sb16_ready) {
        sb16_write(0xD0);
        sb16_write(0xD3);
        sb16_ready = 0;
    }
}

int I_GetSfxLumpNum(sfxinfo_t *sfxinfo) {
    char namebuf[9];
    if (sfxinfo->link) sfxinfo = sfxinfo->link;
    snprintf(namebuf, sizeof(namebuf), "ds%s", sfxinfo->name);
    return W_CheckNumForName(namebuf);
}

void I_UpdateSound(void) {
    doom_sound_tick();
}

void I_UpdateSoundParams(int channel, int vol, int sep) {
    (void)sep;
    if (channel >= 0 && channel < MAX_CHANNELS && channels[channel].active) {
        channels[channel].vol = vol;
    }
}

int I_StartSound(sfxinfo_t *sfxinfo, int channel, int vol, int sep) {
    (void)sep;
    if (!sb16_ready || channel < 0 || channel >= MAX_CHANNELS) return -1;

    char namebuf[9];
    if (sfxinfo->link) sfxinfo = sfxinfo->link;
    snprintf(namebuf, sizeof(namebuf), "ds%s", sfxinfo->name);
    int lump = W_CheckNumForName(namebuf);
    if (lump < 0) return -1;

    int lumplen = W_LumpLength(lump);
    if (lumplen <= 24) return -1;

    const uint8_t* raw = (const uint8_t*)W_CacheLumpNum(lump, PU_STATIC);
    uint16_t rate = (uint16_t)raw[2] | ((uint16_t)raw[3] << 8);
    uint32_t samples = (uint32_t)raw[4] | ((uint32_t)raw[5] << 8) |
                       ((uint32_t)raw[6] << 16) | ((uint32_t)raw[7] << 24);
    if (rate == 0) rate = 11025;
    if (samples > (uint32_t)(lumplen - 8)) samples = (uint32_t)(lumplen - 8);
    if (samples <= 16) return -1;

    channels[channel].active  = 0;
    channels[channel].data    = raw + 8;
    channels[channel].length  = samples;
    channels[channel].pos_fp  = 0;
    channels[channel].step_fp = (rate << 16) / 11025;
    channels[channel].vol     = vol;
    channels[channel].active  = 1;
    return channel;
}

void I_StopSound(int channel) {
    if (channel >= 0 && channel < MAX_CHANNELS) {
        channels[channel].active = 0;
    }
}

boolean I_SoundIsPlaying(int channel) {
    if (channel >= 0 && channel < MAX_CHANNELS) {
        return channels[channel].active != 0;
    }
    return false;
}

void I_PrecacheSounds(sfxinfo_t *sounds, int num_sounds) {
    (void)sounds; (void)num_sounds;
}

// ============================================================================
// DRIVER MUSIK OPL3 / ADLIB FM SYNTHESIZER (PORT 0x220 & 0x388) + GENMIDI + MUS
// ============================================================================

#define OPL_VOICES      18
#define GENMIDI_NUM     175

typedef struct __attribute__((packed)) {
    uint8_t tremolo;
    uint8_t attack;
    uint8_t sustain;
    uint8_t waveform;
    uint8_t scale;
    uint8_t level;
} genmidi_op_t;

typedef struct __attribute__((packed)) {
    genmidi_op_t modulator;
    uint8_t feedback;
    genmidi_op_t carrier;
    uint8_t unused;
    int16_t base_note_offset;
} genmidi_voice_t;

typedef struct __attribute__((packed)) {
    uint16_t flags;
    uint8_t fine_tuning;
    uint8_t fixed_note;
    genmidi_voice_t voices[2];
} genmidi_instr_t;

typedef struct {
    int active;
    int mus_ch;
    int note;
    int velocity;
    int instr_idx;
    uint8_t reg_b0;
    uint32_t age;
} opl_voice_t;

static const uint8_t opl_op_offset[9] = {
    0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x10, 0x11, 0x12
};

static const uint16_t opl_fnum[12] = {
    0x157, 0x16B, 0x181, 0x198, 0x1B0, 0x1CA,
    0x1E5, 0x202, 0x220, 0x241, 0x263, 0x287
};

static const genmidi_instr_t* genmidi_bank = NULL;
static opl_voice_t opl_voices[OPL_VOICES];
static uint8_t mus_ch_instr[16];
static uint8_t mus_ch_vol[16];
static int8_t  mus_ch_bend[16];

static const uint8_t* mus_data = NULL;
static int mus_len = 0;
static int mus_score_start = 0;
static int mus_pos = 0;
static int mus_playing = 0;
static int mus_paused = 0;
static int mus_looping = 0;
static int music_volume = 127;
static uint32_t mus_delay_ticks = 0;
static uint32_t last_mus_ms = 0;
static uint32_t voice_age_counter = 1;
static uint32_t total_notes_played = 0;

static void opl_write(int bank, uint8_t reg, uint8_t val) {
    uint16_t port = (bank == 0) ? 0x388 : 0x38A;
    outb(port, reg);
    for (int i = 0; i < 6; i++) (void)inb(0x388);
    outb(port + 1, val);
    for (int i = 0; i < 35; i++) (void)inb(0x388);
}

static void opl_reset(void) {
    sb16_mixer_write(0x26, 0xFF);
    sb16_mixer_write(0x34, 0xF8);
    sb16_mixer_write(0x35, 0xF8);

    opl_write(1, 0x05, 0x01); // Aktifkan OPL3 mode
    opl_write(0, 0x01, 0x20); // Aktifkan Waveform Select
    opl_write(0, 0xBD, 0x00); // Mode melodi standar
    for (int b = 0; b < 2; b++) {
        for (int r = 0x20; r <= 0xF5; r++) {
            opl_write(b, (uint8_t)r, 0x00);
        }
        for (int ch = 0; ch < 9; ch++) {
            opl_write(b, (uint8_t)(0xB0 + ch), 0x00);
        }
    }
    for (int v = 0; v < OPL_VOICES; v++) {
        opl_voices[v].active = 0;
        opl_voices[v].reg_b0 = 0;
    }
}

static uint8_t opl_calc_ksl_tl(uint8_t scale, uint8_t level, int vol_scale) {
    int tl = level & 0x3F;
    if (vol_scale >= 0) {
        int atten = 63 - ((63 - tl) * vol_scale) / 127;
        if (atten < 0) atten = 0;
        if (atten > 63) atten = 63;
        tl = atten;
    }
    return (uint8_t)((scale & 0xC0) | (tl & 0x3F));
}

static void opl_load_patch(int v, const genmidi_voice_t* gv, int ch_vol, int vel) {
    int bank = v / 9;
    int ch = v % 9;
    uint8_t op_mod = opl_op_offset[ch];
    uint8_t op_car = op_mod + 3;

    int eff_mus_vol = (music_volume > 0) ? (music_volume * 8) : 0;
    if (eff_mus_vol > 127) eff_mus_vol = 127;

    int total_vol = (ch_vol * vel * eff_mus_vol) / (127 * 127);
    if (total_vol > 127) total_vol = 127;

    opl_write(bank, 0x20 + op_mod, gv->modulator.tremolo);
    opl_write(bank, 0x60 + op_mod, gv->modulator.attack);
    opl_write(bank, 0x80 + op_mod, gv->modulator.sustain);
    opl_write(bank, 0xE0 + op_mod, gv->modulator.waveform & 0x07);

    opl_write(bank, 0x20 + op_car, gv->carrier.tremolo);
    opl_write(bank, 0x60 + op_car, gv->carrier.attack);
    opl_write(bank, 0x80 + op_car, gv->carrier.sustain);
    opl_write(bank, 0xE0 + op_car, gv->carrier.waveform & 0x07);

    opl_write(bank, 0xC0 + ch, (gv->feedback & 0x0F) | 0x30);

    int mod_vol = (gv->feedback & 0x01) ? total_vol : -1;
    opl_write(bank, 0x40 + op_mod, opl_calc_ksl_tl(gv->modulator.scale, gv->modulator.level, mod_vol));
    opl_write(bank, 0x40 + op_car, opl_calc_ksl_tl(gv->carrier.scale, gv->carrier.level, total_vol));
}

static void opl_set_freq(int v, int note, int offset, int bend, int key_on) {
    int bank = v / 9;
    int ch = v % 9;

    int final_note = note + offset;
    if (final_note < 0) final_note = 0;
    if (final_note > 127) final_note = 127;

    int block = (final_note / 12) - 1;
    int semi = final_note % 12;
    if (block < 0) { block = 0; semi = 0; }
    if (block > 7) { block = 7; semi = 11; }

    int fnum = (int)opl_fnum[semi] + (bend * 2);
    if (fnum < 0x100) fnum = 0x100;
    if (fnum > 0x3FF) fnum = 0x3FF;

    uint8_t b0 = (uint8_t)((key_on ? 0x20 : 0x00) | ((block & 0x07) << 2) | ((fnum >> 8) & 0x03));
    opl_voices[v].reg_b0 = b0;
    opl_write(bank, 0xA0 + ch, (uint8_t)(fnum & 0xFF));
    opl_write(bank, 0xB0 + ch, b0);
}

static int opl_alloc_voice(void) {
    for (int v = 0; v < OPL_VOICES; v++) {
        if (!opl_voices[v].active) return v;
    }
    int oldest_v = 0;
    uint32_t min_age = opl_voices[0].age;
    for (int v = 1; v < OPL_VOICES; v++) {
        if (opl_voices[v].age < min_age) {
            min_age = opl_voices[v].age;
            oldest_v = v;
        }
    }
    opl_write(oldest_v / 9, 0xB0 + (oldest_v % 9), opl_voices[oldest_v].reg_b0 & ~0x20);
    opl_voices[oldest_v].active = 0;
    return oldest_v;
}

static void mus_note_off(int mus_ch, int note) {
    for (int v = 0; v < OPL_VOICES; v++) {
        if (opl_voices[v].active && opl_voices[v].mus_ch == mus_ch && opl_voices[v].note == note) {
            opl_write(v / 9, 0xB0 + (v % 9), opl_voices[v].reg_b0 & ~0x20);
            opl_voices[v].active = 0;
        }
    }
}

static void mus_note_on(int mus_ch, int note, int vel) {
    if (!genmidi_bank) return;
    int instr_idx = (mus_ch == 15) ? (128 + note - 35) : mus_ch_instr[mus_ch];
    if (instr_idx < 0 || instr_idx >= GENMIDI_NUM) return;

    const genmidi_instr_t* ins = &genmidi_bank[instr_idx];
    int play_note = (ins->flags & 0x0001) ? ins->fixed_note : note;

    int v = opl_alloc_voice();
    opl_write(v / 9, 0xB0 + (v % 9), 0x00);

    opl_voices[v].active = 1;
    opl_voices[v].mus_ch = mus_ch;
    opl_voices[v].note = note;
    opl_voices[v].velocity = vel;
    opl_voices[v].instr_idx = instr_idx;
    opl_voices[v].age = voice_age_counter++;
    total_notes_played++;

    opl_load_patch(v, &ins->voices[0], mus_ch_vol[mus_ch], vel);
    opl_set_freq(v, play_note, ins->voices[0].base_note_offset, mus_ch_bend[mus_ch], 1);
}

static void mus_all_notes_off(void) {
    for (int v = 0; v < OPL_VOICES; v++) {
        if (opl_voices[v].active) {
            opl_write(v / 9, 0xB0 + (v % 9), opl_voices[v].reg_b0 & ~0x20);
            opl_voices[v].active = 0;
        }
    }
}

static void mus_step_events(void) {
    if (!mus_data || !mus_playing || mus_paused) return;

    while (mus_playing && mus_delay_ticks == 0) {
        if (mus_pos >= mus_len) {
            if (mus_looping) {
                mus_all_notes_off();
                mus_pos = mus_score_start;
                continue;
            } else {
                mus_playing = 0;
                mus_all_notes_off();
                return;
            }
        }

        uint8_t desc = mus_data[mus_pos++];
        int last = (desc >> 7) & 1;
        int ev_type = (desc >> 4) & 0x07;
        int ch = desc & 0x0F;

        if (ev_type == 0) {
            if (mus_pos < mus_len) {
                uint8_t note = mus_data[mus_pos++] & 0x7F;
                mus_note_off(ch, note);
            }
        } else if (ev_type == 1) {
            if (mus_pos < mus_len) {
                uint8_t b = mus_data[mus_pos++];
                uint8_t note = b & 0x7F;
                int vel = 100;
                if ((b & 0x80) && mus_pos < mus_len) {
                    vel = mus_data[mus_pos++] & 0x7F;
                }
                mus_note_on(ch, note, vel);
            }
        } else if (ev_type == 2) {
            if (mus_pos < mus_len) {
                uint8_t bend = mus_data[mus_pos++];
                mus_ch_bend[ch] = (int8_t)((int)bend - 128);
            }
        } else if (ev_type == 3) {
            if (mus_pos < mus_len) {
                uint8_t sys = mus_data[mus_pos++] & 0x7F;
                if (sys == 10 || sys == 11 || sys == 14) {
                    mus_all_notes_off();
                }
            }
        } else if (ev_type == 4) {
            if (mus_pos + 1 < mus_len) {
                uint8_t ctrl = mus_data[mus_pos++] & 0x7F;
                uint8_t val = mus_data[mus_pos++] & 0x7F;
                if (ctrl == 0) {
                    mus_ch_instr[ch] = val;
                } else if (ctrl == 3) {
                    mus_ch_vol[ch] = val;
                }
            }
        } else if (ev_type == 6) {
            if (mus_looping) {
                mus_all_notes_off();
                mus_pos = mus_score_start;
                continue;
            } else {
                mus_playing = 0;
                mus_all_notes_off();
                return;
            }
        }

        if (last) {
            uint32_t delay = 0;
            uint8_t b;
            do {
                if (mus_pos >= mus_len) break;
                b = mus_data[mus_pos++];
                delay = (delay << 7) | (b & 0x7F);
            } while (b & 0x80);
            mus_delay_ticks = delay;
        }
    }
}

static void opl_music_tick(void) {
    if (!mus_playing || mus_paused || !mus_data) return;
    uint32_t now = DG_GetTicksMs();
    if (last_mus_ms == 0) {
        last_mus_ms = now;
        mus_step_events();
        return;
    }
    uint32_t elapsed_ms = now - last_mus_ms;
    if (elapsed_ms == 0) return;
    last_mus_ms = now;

    uint32_t ticks = (elapsed_ms * 140) / 1000;
    if (ticks == 0) ticks = 1;

    while (ticks > 0 && mus_playing) {
        if (mus_delay_ticks > ticks) {
            mus_delay_ticks -= ticks;
            ticks = 0;
        } else {
            ticks -= mus_delay_ticks;
            mus_delay_ticks = 0;
            mus_step_events();
        }
    }
}

void I_InitMusic(void) {
    opl_reset();
    for (int i = 0; i < 16; i++) {
        mus_ch_instr[i] = 0;
        mus_ch_vol[i] = 100;
        mus_ch_bend[i] = 0;
    }
    int lump = W_CheckNumForName("GENMIDI");
    if (lump >= 0 && W_LumpLength(lump) >= 8 + (int)(GENMIDI_NUM * sizeof(genmidi_instr_t))) {
        const uint8_t* raw = (const uint8_t*)W_CacheLumpNum(lump, PU_STATIC);
        genmidi_bank = (const genmidi_instr_t*)(raw + 8);
    }
    printf("[MUSIC] Init genmidi=%d opl_stat=%x\n", genmidi_bank != NULL, inb(0x388));
}

void I_ShutdownMusic(void) {
    mus_playing = 0;
    opl_reset();
}

void I_SetMusicVolume(int volume) {
    music_volume = volume;
    if (music_volume == 0) mus_all_notes_off();
}

void I_PauseSong(void) {
    mus_paused = 1;
    mus_all_notes_off();
}

void I_ResumeSong(void) {
    mus_paused = 0;
    last_mus_ms = DG_GetTicksMs();
}

void *I_RegisterSong(void *data, int len) {
    if (!data || len < 16) return NULL;
    const uint8_t* p = (const uint8_t*)data;
    printf("[MUSIC] Register len=%d sig=%c%c%c\n", len, p[0], p[1], p[2]);
    if (p[0] != 'M' || p[1] != 'U' || p[2] != 'S' || p[3] != 0x1A) return NULL;

    mus_playing = 0;
    mus_all_notes_off();

    mus_data = p;
    mus_len = len;
    mus_score_start = (int)p[6] | ((int)p[7] << 8);
    mus_pos = mus_score_start;
    mus_delay_ticks = 0;
    return data;
}

void I_UnRegisterSong(void *handle) {
    (void)handle;
    mus_playing = 0;
    mus_all_notes_off();
    mus_data = NULL;
}

void I_PlaySong(void *handle, boolean looping) {
    if (!handle || !mus_data) return;
    for (int i = 0; i < 16; i++) {
        mus_ch_instr[i] = 0;
        mus_ch_vol[i] = 100;
        mus_ch_bend[i] = 0;
    }
    mus_pos = mus_score_start;
    mus_delay_ticks = 0;
    mus_looping = looping ? 1 : 0;
    mus_paused = 0;
    last_mus_ms = DG_GetTicksMs();
    mus_playing = 1;
    mus_step_events();
    printf("[MUSIC] Play notes=%u vol=%d\n", total_notes_played, music_volume);
}

void I_StopSong(void) {
    mus_playing = 0;
    mus_all_notes_off();
}

boolean I_MusicIsPlaying(void) {
    return mus_playing ? true : false;
}

void I_BindSoundVariables(void) {}
