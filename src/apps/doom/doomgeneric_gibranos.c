#include "doomgeneric.h"
#include "doomkeys.h"
#include "d_event.h"
#include "i_sound.h"
#include "../../drivers/io.h"
#include "../../arch/i386/timer.h"
#include <stdint.h>

static uint32_t get_ticks_ms(void) { return timer_get_ticks() * 10; }

static uint16_t tty1_text_backup[80 * 25];
static uint8_t vga_font_backup[8192];
static volatile int doom_running = 0;
static void* doom_exit_jmp[5];

static uint8_t mouse_bytes[3];
static int mouse_cycle = 0;

static void ps2_wait_write(void) {
    for (int i = 0; i < 100000; i++) {
        if ((inb(0x64) & 0x02) == 0) return;
    }
}

static void ps2_wait_read(void) {
    for (int i = 0; i < 100000; i++) {
        if ((inb(0x64) & 0x01) != 0) return;
    }
}

static void ps2_mouse_write(uint8_t val) {
    ps2_wait_write();
    outb(0x64, 0xD4);
    ps2_wait_write();
    outb(0x60, val);
}

static void ps2_mouse_init(void) {
    mouse_cycle = 0;
    while (inb(0x64) & 0x01) (void)inb(0x60);
    ps2_wait_write();
    outb(0x64, 0xA8);
    ps2_wait_write();
    outb(0x64, 0x20);
    ps2_wait_read();
    uint8_t status = inb(0x60);
    status |=  0x01;
    status &= ~0x20;
    status &= ~0x02;
    ps2_wait_write();
    outb(0x64, 0x60);
    ps2_wait_write();
    outb(0x60, status);
    ps2_mouse_write(0xF6);
    for (int i = 0; i < 1000; i++) if (inb(0x64) & 0x01) { (void)inb(0x60); break; }
    ps2_mouse_write(0xF4);
    for (int i = 0; i < 1000; i++) if (inb(0x64) & 0x01) { (void)inb(0x60); break; }
}

static void vga_save_font(void) {
    outb(0x3C4, 0x02); outb(0x3C5, 0x04);
    outb(0x3C4, 0x04); outb(0x3C5, 0x07);
    outb(0x3CE, 0x04); outb(0x3CF, 0x02);
    outb(0x3CE, 0x05); outb(0x3CF, 0x00);
    outb(0x3CE, 0x06); outb(0x3CF, 0x00);
    volatile uint8_t* vga_plane2 = (volatile uint8_t*)0xA0000;
    for (int i = 0; i < 8192; i++) vga_font_backup[i] = vga_plane2[i];
}

static void vga_restore_font(void) {
    outb(0x3C4, 0x02); outb(0x3C5, 0x04);
    outb(0x3C4, 0x04); outb(0x3C5, 0x07);
    outb(0x3CE, 0x05); outb(0x3CF, 0x00);
    outb(0x3CE, 0x06); outb(0x3CF, 0x00);
    volatile uint8_t* vga_plane2 = (volatile uint8_t*)0xA0000;
    for (int i = 0; i < 8192; i++) vga_plane2[i] = vga_font_backup[i];

    outb(0x3C4, 0x02); outb(0x3C5, 0x03);
    outb(0x3C4, 0x04); outb(0x3C5, 0x02);
    outb(0x3CE, 0x04); outb(0x3CF, 0x00);
    outb(0x3CE, 0x05); outb(0x3CF, 0x10);
    outb(0x3CE, 0x06); outb(0x3CF, 0x0E);
}

static void vga_enter_mode13h(void) {
    __asm__ volatile("cli");

    volatile uint16_t* vga_text = (volatile uint16_t*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) tty1_text_backup[i] = vga_text[i];
    vga_save_font();

    outb(0x3C2, 0x63);
    static const uint8_t seq[5] = {0x03, 0x01, 0x0F, 0x00, 0x0E};
    for (uint8_t i = 0; i < 5; i++) { outb(0x3C4, i); outb(0x3C5, seq[i]); }
    outb(0x3D4, 0x11); outb(0x3D5, inb(0x3D5) & 0x7F);
    static const uint8_t crtc[25] = {
        0x5F,0x4F,0x50,0x82,0x54,0x80,0xBF,0x1F,0x00,0x41,0x00,0x00,0x00,
        0x00,0x00,0x00,0x9C,0x0E,0x8F,0x28,0x40,0x96,0xB9,0xA3,0xFF
    };
    for (uint8_t i = 0; i < 25; i++) { outb(0x3D4, i); outb(0x3D5, crtc[i]); }
    static const uint8_t gc[9] = {0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0F,0xFF};
    for (uint8_t i = 0; i < 9; i++) { outb(0x3CE, i); outb(0x3CF, gc[i]); }
    (void)inb(0x3DA);
    static const uint8_t ac[21] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,
        0x0D,0x0E,0x0F,0x41,0x00,0x0F,0x00,0x00
    };
    for (uint8_t i = 0; i < 21; i++) { outb(0x3C0, i); outb(0x3C0, ac[i]); }
    outb(0x3C0, 0x20);

    outb(0x3C8, 0);
    for (int i = 0; i < 256; i++) {
        outb(0x3C9, ((i >> 5) & 0x07) * 9);
        outb(0x3C9, ((i >> 2) & 0x07) * 9);
        outb(0x3C9, (i & 0x03) * 21);
    }

    volatile uint8_t* vga_mem = (volatile uint8_t*)0xA0000;
    for (int i = 0; i < 320 * 200; i++) vga_mem[i] = 0x00;

    __asm__ volatile("sti");
}

static void vga_exit_mode13h(void) {
    __asm__ volatile("cli");
    outb(0x61, inb(0x61) & 0xFC);

    outb(0x3C2, 0x67);
    static const uint8_t seq[5] = {0x03, 0x00, 0x03, 0x00, 0x02};
    for (uint8_t i = 0; i < 5; i++) { outb(0x3C4, i); outb(0x3C5, seq[i]); }
    outb(0x3D4, 0x11); outb(0x3D5, inb(0x3D5) & 0x7F);
    static const uint8_t crtc[25] = {
        0x5F,0x4F,0x50,0x82,0x55,0x81,0xBF,0x1F,0x00,0x4F,0x0D,0x0E,0x00,
        0x00,0x00,0x50,0x9C,0x0E,0x8F,0x28,0x1F,0x96,0xB9,0xA3,0xFF
    };
    for (uint8_t i = 0; i < 25; i++) { outb(0x3D4, i); outb(0x3D5, crtc[i]); }
    static const uint8_t gc[9] = {0x00,0x00,0x00,0x00,0x00,0x10,0x0E,0x00,0xFF};
    for (uint8_t i = 0; i < 9; i++) { outb(0x3CE, i); outb(0x3CF, gc[i]); }
    (void)inb(0x3DA);
    static const uint8_t ac[21] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x14,0x07,0x38,0x39,0x3A,0x3B,0x3C,
        0x3D,0x3E,0x3F,0x0C,0x00,0x0F,0x08,0x00
    };
    for (uint8_t i = 0; i < 21; i++) { outb(0x3C0, i); outb(0x3C0, ac[i]); }
    outb(0x3C0, 0x20);

    static const uint8_t ega_dac[16][3] = {
        {0,0,0},    {0,0,42},   {0,42,0},   {0,42,42},
        {42,0,0},   {42,0,42},  {42,21,0},  {42,42,42},
        {21,21,21}, {21,21,63}, {21,63,21}, {21,63,63},
        {63,21,21}, {63,21,63}, {63,63,21}, {63,63,63}
    };
    static const uint8_t ega_idx[16] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x14,0x07,
        0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F
    };
    for (int i = 0; i < 16; i++) {
        outb(0x3C8, ega_idx[i]);
        outb(0x3C9, ega_dac[i][0]);
        outb(0x3C9, ega_dac[i][1]);
        outb(0x3C9, ega_dac[i][2]);
    }

    vga_restore_font();
    volatile uint16_t* vga_text = (volatile uint16_t*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) vga_text[i] = tty1_text_backup[i];

    __asm__ volatile("sti");
}

void DG_Init(void) {
    vga_enter_mode13h();
    ps2_mouse_init();
    doom_running = 1;
}

void DG_DrawFrame(void) {
    I_UpdateSound();
    volatile uint8_t* vga_mem = (volatile uint8_t*)0xA0000;
    for (int y = 0; y < 200; y++) {
        uint32_t* src_row = &DG_ScreenBuffer[(y * 2) * DOOMGENERIC_RESX];
        for (int x = 0; x < 320; x++) {
            uint32_t px = src_row[x * 2];
            uint8_t r = (px >> 21) & 0x07;
            uint8_t g = (px >> 13) & 0x07;
            uint8_t b = (px >> 6)  & 0x03;
            vga_mem[y * 320 + x] = (r << 5) | (g << 2) | b;
        }
    }
}

uint32_t DG_GetTicksMs(void) {
    return get_ticks_ms();
}

void DG_SleepMs(uint32_t ms) {
    uint32_t target = get_ticks_ms() + ms;
    while (get_ticks_ms() < target) {
        __asm__ volatile("hlt");
    }
}

int DG_GetKey(int* pressed, unsigned char* key) {
    while ((inb(0x64) & 0x01) != 0) {
        uint8_t status = inb(0x64);
        uint8_t data = inb(0x60);

        if (status & 0x20) {
            if (mouse_cycle == 0) {
                if (data & 0x08) {
                    mouse_bytes[0] = data;
                    mouse_cycle = 1;
                }
            } else if (mouse_cycle == 1) {
                mouse_bytes[1] = data;
                mouse_cycle = 2;
            } else if (mouse_cycle == 2) {
                mouse_bytes[2] = data;
                mouse_cycle = 0;

                int buttons = mouse_bytes[0] & 0x07;
                int dx = (int)mouse_bytes[1] - ((mouse_bytes[0] & 0x10) ? 256 : 0);

                event_t ev;
                ev.type  = ev_mouse;
                ev.data1 = buttons;
                ev.data2 = dx * 3;
                ev.data3 = 0;
                D_PostEvent(&ev);
            }
            continue;
        }

        if (data == 0xE0 || data == 0xE1) continue;

        *pressed = (data & 0x80) ? 0 : 1;
        uint8_t code = data & 0x7F;

        switch (code) {
            case 0x01: *key = KEY_ESCAPE;     break;
            case 0x1C: *key = KEY_ENTER;      break;
            case 0x0F: *key = KEY_TAB;        break;
            case 0x39: *key = KEY_USE;        break;
            case 0x1D: *key = KEY_FIRE;       break;
            case 0x2A:
            case 0x36: *key = KEY_RSHIFT;     break;
            case 0x11:
            case 0x48: *key = KEY_UPARROW;    break;
            case 0x1F:
            case 0x50: *key = KEY_DOWNARROW;  break;
            case 0x1E:
            case 0x4B: *key = KEY_LEFTARROW;  break;
            case 0x20:
            case 0x4D: *key = KEY_RIGHTARROW; break;
            case 0x02: *key = '1'; break;
            case 0x03: *key = '2'; break;
            case 0x04: *key = '3'; break;
            case 0x05: *key = '4'; break;
            case 0x06: *key = '5'; break;
            case 0x07: *key = '6'; break;
            case 0x08: *key = '7'; break;
            case 0x15: *key = 'y'; break;
            case 0x31: *key = 'n'; break;
            default: continue;
        }
        return 1;
    }
    return 0;
}

void DG_SetWindowTitle(const char* title) { (void)title; }

void exit(int status) {
    (void)status;
    __builtin_longjmp(doom_exit_jmp, 1);
}

void gibranos_launch_doom(void) {
    __asm__ volatile("sti");

    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2);
    cr0 &= ~(1 << 3);
    cr0 |=  (1 << 1);
    __asm__ volatile("mov %0, %%cr0; fninit" :: "r"(cr0));

    if (__builtin_setjmp(doom_exit_jmp) == 0) {
        char* argv[] = { "doom", "-iwad", "doom1.wad", "-mb", "12", NULL };
        doomgeneric_Create(5, argv);
        while (doom_running) {
            doomgeneric_Tick();
        }
    }
    doom_running = 0;
    vga_exit_mode13h();
}
