# OmniTools_Periksa 🇮🇩
> **Customs Inspection Desktop Suite — Alat Bantu Cerdas Pemeriksa Bea Cukai**

Aplikasi desktop berkinerja tinggi berbasis **C++20 & Qt 6** yang dirancang khusus untuk memfasilitasi dan mempercepat proses pemeriksaan fisik peti kemas dan kargo impor/ekspor di lingkungan Direktorat Jenderal Bea dan Cukai (DJBC).

---

## 🌟 Fitur Utama

### 1. Point Annotation & Object Counting
- **12 Template Komoditas Pabean Lengkap**:
  - 🔘 Pipa Silinder / Besi Berongga
  - 🧵 Roll Kain / Tekstil Gulungan
  - ⚪ Granule / Biji Plastik / Pellet
  - 📦 Karton Box / Peti Kayu Kemasan
  - 🛢️ Drum Kimia / Tong Aspal / Barel
  - 🛞 Ban Kendaraan / Velg Roda
  - 🌾 Karung Beras / Tepung / Jumbo Bag
  - 🪵 Kayu Log / Gelondongan
  - 🧯 Tabung Gas Medis / Silinder LPG
  - ⚽ Benda Bulat / Buah Seragam
  - ⭕ Pola Lingkaran Kustom
  - 🔍 Objek Kontras Bebas (Custom Blob)
- **Mesin Auto-Count Cerdas**: Dilengkapi filter deduplikasi spasial (*Anti-Double Points*) untuk mencegah titik ganda dan kalkulasi Otsu thresholding adaptif.
- **Mode Manual & Koreksi**: Tambah titik (Klik Kiri), Hapus titik (Klik Kanan) dengan penomoran urut otomatis (*auto-renumbering*).
- **Multi-Kategori Warna**: Hijau (Kondisi Baik/Sesuai), Merah (Rusak/Selisih), Kuning (Perlu Pemeriksaan Ulang), Biru (Sampel Lab).
- **Ekspor Dokumen**: Simpan foto beranotasi resolusi tinggi dan rekapitulasi data hitungan ke format CSV.

### 2. Roadmap Pengembangan (Fase Lanjutan)
- [x] **Fase 1**: Fondasi GUI Qt 6 & Kanvas Anotasi Berperforma Tinggi.
- [x] **Fase 2**: Mesin Auto-Count Multi-Template & Anti-Duplikasi.
- [ ] **Fase 3**: Image Tools Suite (Batch Compress, Resize, Convert, Cut) & Offline PDF Editor Engine (Merge, Split, Rotate, Stamp).
- [ ] **Fase 4**: Deep Learning Vision (ONNX Runtime + YOLO C++) & Integrasi Gemini Vision API untuk pembuatan draf Laporan Hasil Pemeriksaan (LHP) otomatis.

---

## 🛠️ Persyaratan Lingkungan & Kompilasi

- **Sistem Operasi**: Windows 10 / 11 (64-bit)
- **Compiler**: MinGW-w64 GCC 15+ (C++20 standard)
- **Build System**: CMake 3.20+ & Ninja
- **Framework**: Qt 6.8.0 MinGW 64-bit (`Core`, `Gui`, `Widgets`)

---

## 📄 Lisensi
Hak Cipta © 2026. Dikembangkan untuk efisiensi pemeriksaan kepabeanan.
