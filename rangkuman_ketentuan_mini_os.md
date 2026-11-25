# Rangkuman Ketentuan Tugas Besar Mini-OS

**CPMK:** Mahasiswa mampu memahami struktur sistem operasi komputer, proses dan thread, penjadwalan CPU, sinkronisasi, manajemen memori, media penyimpanan, serta sistem keamanan dalam desain sistem operasi komputer.

---

## 1. Ketentuan Umum

* **Bentuk Sistem:** Harus berupa kernel sederhana yang dapat dijalankan di emulator (QEMU atau sejenisnya), **bukan** sekadar aplikasi simulasi yang berjalan di atas Linux/Windows.
* **Bahasa & Perkakas:** Bebas menggunakan bahasa pemrograman (C, Rust, Assembly, C++, dll.). Penggunaan Generative AI diperbolehkan dengan syarat **wajib didokumentasikan**.
* **Antarmuka (CLI):** Memiliki konsol/shell internal dengan **nama unik** dan menampilkan **logo ASCII** saat proses *booting*.
* **Mekanisme Penilaian:**
  * Demonstrasi sistem secara langsung.
  * Pengujian fungsionalitas fitur.
  * Verifikasi pemahaman individual (setiap anggota harus memahami arsitektur utama dan bagian yang dikembangkannya).

---

## 2. Ketentuan Minimum Fitur & Pengujian

Terdapat 8 komponen utama yang wajib diimplementasikan beserta syarat minimum dan skenario pengujiannya:

| No | Komponen | Syarat Minimum | Skenario Pengujian |
| :--- | :--- | :--- | :--- |
| **1** | **Bootstrapping & CLI Kernel** | Bootloader memuat kernel ke RAM dan menjalankan antarmuka CLI internal (menerima input keyboard / terminal serial QEMU). | OS berhasil *booting* di QEMU dan mampu mengeksekusi minimal **3 perintah CLI** buatan sendiri. |
| **2** | **Pengelolaan Proses & Thread** | Memiliki struktur data pengelolaan *thread* dan mekanisme pengelolaan status *thread*. | Menjalankan minimal **2 kernel thread** dengan tugas berbeda dan menampilkan perubahan statusnya (*New, Ready, Running, Waiting, Terminated*). |
| **3** | **Penjadwalan CPU Preemptive** | Menerapkan *Preemptive Scheduler* (misalnya: Round Robin atau Priority Scheduling). | Scheduler terbukti mengatur giliran eksekusi CPU secara nyata via **context switching** (bukan simulasi teks `printf`). |
| **4** | **Sinkronisasi** | Mengimplementasikan *mutex* atau *semaphore* untuk melindungi *shared data*. | Membandingkan operasi *shared counter* oleh 2 *thread* **tanpa sinkronisasi** (*race condition*) vs **dengan sinkronisasi** (data konsisten). |
| **5** | **Deadlock** | Mengimplementasikan *mutex/semaphore* dan memahami kondisi terjadinya *deadlock*. | Mendemonstrasikan 2 *thread* yang saling terblokir (*deadlock*) akibat mengambil 2 *mutex* secara berlawanan, lalu menjelaskan/menunjukkan solusinya (misal: pengurutan pengambilan *mutex*). |
| **6** | **Alokator Memori Dinamis** | Mengimplementasikan alokator memori sederhana yang mendukung alokasi dan pelepasan memori. | Menunjukkan perubahan kapasitas memori terpakai/tersedia, membuktikan memori dapat dilepas, dan menangani kondisi *Out of Memory* tanpa menyebabkan *kernel crash*. |
| **7** | **Virtual File System (VFS)** | Sistem berkas sederhana berbasis RAMDisk atau mekanisme setara. | Membuktikan operasi CRUD (**Create, Read, Update, Delete**) pada berkas teks. |
| **8** | **Proteksi & Hak Akses** | Pemisahan *user mode* & *kernel mode*, atau kontrol hak akses sederhana. | Membuktikan akses sah diterima dan akses terlarang ditolak (misal: akses terlarang ke memori kernel diblokir oleh sistem proteksi). |

---

## 3. Format Laporan Tugas Besar

* **Panjang Laporan:** Sekitar **10–15 halaman** (di luar lampiran).

### Struktur Bab:
* **Bab I: Pendahuluan**
  * Deskripsi singkat Mini-OS dan fitur utamanya.
  * Bahasa pemrograman, *toolchain*, dan lingkungan pengujian yang digunakan.
* **Bab II: Rancangan dan Implementasi**
  * Diagram arsitektur Mini-OS.
  * Struktur data dan algoritma yang digunakan.
  * Proses kerja dan mekanisme dari ke-8 komponen utama yang diimplementasikan.
* **Bab III: Pengujian dan Hasil**
  * Skenario pengujian untuk setiap komponen.
  * Bukti pengujian berupa *screenshot* atau *log* yang relevan.
  * Hasil pengujian, kendala yang dihadapi, serta tindakan perbaikannya.
* **Bab IV: Penutup**
  * Ringkasan komponen yang berhasil dan belum berhasil diimplementasikan.
  * Ringkasan hasil pengujian & pencapaian tujuan.
  * Keterbatasan Mini-OS dan rekomendasi pengembangan selanjutnya.

### Lampiran Wajib:
1. Tautan repositori atau berkas *source code*.
2. Petunjuk singkat kompilasi (*build*) dan menjalankan (*run*) Mini-OS.
3. Dokumentasi penggunaan AI dan kode dari pihak ketiga (jika ada).
4. Pembagian tugas masing-masing anggota kelompok.