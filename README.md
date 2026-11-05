# DIY Power Sequencer ESP32 ⚡

Proyek ini adalah sistem **Power Sequencer 8-Channel** berbasis mikrokontroler ESP32[cite: 1]. Sistem ini dirancang untuk menyalakan atau mematikan perangkat elektronik (seperti peralatan audio, server, atau perangkat berdaya tinggi) secara berurutan dengan jeda waktu tertentu.

## 🤔 Mengapa Membuat DIY Power Sequencer?

* **Mencegah MCB Turun (Jeglek):** Menyalakan banyak perangkat berdaya tinggi (seperti power amplifier, PC, atau lampu panggung) secara bersamaan akan menimbulkan lonjakan arus tarikan awal (*inrush current*) yang sangat besar. Sequencer mencegah hal ini dengan mendistribusikan beban nyala secara bertahap.
* **Melindungi Perangkat Audio:** Dalam dunia *sound system*, menghidupkan dan mematikan perangkat dengan urutan yang benar (misal: *mixer* dahulu, baru *amplifier*) sangat penting untuk mencegah suara *pop/thump* keras yang bisa merusak speaker.
* **Lebih Hemat Biaya (Cost-Effective):** Power sequencer komersial harganya bisa mencapai jutaan rupiah. Dengan ESP32 dan modul relay standar, Anda mendapatkan fungsionalitas kelas profesional dengan biaya yang jauh lebih murah!
* **Bisa Dikustomisasi:** Karena berbasis *open-source*, Anda bisa dengan mudah mengubah durasi jeda (delay)[cite: 1], menambah fitur kendali WiFi/Bluetooth, atau menyesuaikan logika kerja sesuai kebutuhan spesifik Anda.

## 🌟 Fitur Utama

* **Kontrol 8 Relay:** Mengendalikan hingga 8 channel relay dengan logika keluaran *Active High* (GPIO HIGH = ON, GPIO LOW = OFF)[cite: 1].
* **Kontrol Manual Individual:** Menyediakan 8 tombol fisik untuk menghidupkan/mematikan (*toggle*) masing-masing relay secara independen kapan saja saat sistem sedang *idle*[cite: 1].
* **Tombol Master Pintar:** Menahan tombol Master selama 3 detik akan memicu urutan (*sequence*) nyala/mati otomatis[cite: 1].
* **Pengecekan Status GPIO:** Saat tombol Master ditahan, sistem secara otomatis mengecek kondisi pin: jika semua relay mati, sistem akan menjalankan *Sequence ON* (hidup berurutan dari 1 sampai 8)[cite: 1]. Jika ada relay yang hidup, sistem memprioritaskan keamanan dengan menjalankan *Sequence OFF*[cite: 1].
* **Anti-Bouncing (Debounce):** Dilengkapi sistem *debounce* digital (30 ms) pada kode untuk memastikan tidak ada pembacaan tombol ganda (*false trigger*)[cite: 1].
* **Keamanan Startup:** Saat modul baru menyala, sistem diinisialisasi untuk menahan semua output relay dalam keadaan LOW (OFF) secara otomatis untuk mencegah alat menyala tiba-tiba[cite: 1].

## 🛠️ Perangkat Keras yang Dibutuhkan

1. Board Mikrokontroler ESP32 (misal: NodeMCU / DevKit V1)
2. Modul Relay 8-Channel (Tipe *Active High*)
3. 9x *Push Button* (8 untuk channel individual, 1 untuk Master)
4. Kabel jumper secukupnya
5. **Catatan Penting:** Gunakan *Power Supply* eksternal (biasanya 5V) untuk memberikan daya ke Modul Relay. Jangan mengambil daya kumparan relay (VCC/JD-VCC) langsung dari pin 5V/VIN ESP32 karena 8 relay yang menyala bersamaan dapat melebihi kapasitas arus regulator dan merusak board ESP32.

## 📌 Konfigurasi Pinout

Karena sistem menggunakan mode `INPUT_PULLUP`, semua *push button* dirangkai dengan menyambungkan salah satu kaki ke Pin GPIO ESP32 dan kaki satunya lagi langsung ke **GND** (Ground)[cite: 1].

| Channel | Pin Relay (OUTPUT)[cite: 1] | Pin Push Button (INPUT)[cite: 1] |
| :---: | :---: | :---: |
| **1** | GPIO 16 | GPIO 13 |
| **2** | GPIO 17 | GPIO 14 |
| **3** | GPIO 18 | GPIO 26 |
| **4** | GPIO 19 | GPIO 27 |
| **5** | GPIO 21 | GPIO 32 |
| **6** | GPIO 22 | GPIO 33 |
| **7** | GPIO 23 | GPIO 4 |
| **8** | GPIO 25 | GPIO 5 |

* **Tombol MASTER:** GPIO 15[cite: 1]

## 🚀 Cara Kerja Sistem

1. **Fase Startup / Idle:** Saat ESP32 dinyalakan, semua GPIO diset LOW. Sistem menunggu perintah dari tombol tanpa menjalankan aksi sekuensial apa pun[cite: 1].
2. **Operasi Manual:** Menekan tombol 1-8 dengan cepat akan membalikkan (*toggle*) status ON/OFF pada relay yang bersangkutan[cite: 1].
3. **Operasi Sekuensial (Sequence):**
   * Tahan tombol **Master (GPIO 15)** selama lebih dari **3 detik**[cite: 1].
   * ESP32 akan memberikan jeda **3,5 detik (3500ms)** antar setiap pergantian channel relay saat menyala (menuju HIGH) atau saat mati (menuju LOW)[cite: 1].
   * Anda bisa memantau status aktual logika program ini di Serial Monitor[cite: 1].

## 💻 Cara Instalasi & Upload Kode

1. Unduh atau *clone* repositori ini ke komputer Anda.
2. Buka file `power_sequencer_v5_active_high_gpio_check.ino` menggunakan perangkat lunak **Arduino IDE**[cite: 1].
3. Pastikan *Core/Board* ESP32 sudah terinstal di *Board Manager* Arduino IDE.
4. Pilih jenis board ESP32 yang sesuai pada menu **Tools > Board** (misal: "DOIT ESP32 DEVKIT V1").
5. Pilih *Port* komunikasi (COM) yang tepat.
6. Klik **Upload**.
7. Buka **Serial Monitor** dan atur baud rate ke `115200` untuk memantau log sistem (*debugging*)[cite: 1].

## 🔗 LINK SIMULASI
https://wokwi.com/projects/479804458908908545

---
*Diprogram menggunakan C++ / Arduino Framework untuk ESP32.*
