#include "MainWindow.h"
#include <QToolBar>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QKeySequence>
#include <QShortcut>
#include <QTextStream>
#include <QDateTime>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QString::fromUtf8("Customs Inspector Suite — Tools Pemeriksa Bea Cukai (Fase 2: Smart Auto-Count Engine)"));
    resize(1380, 860);

    setupUI();
    setupToolbar();
    setupSidebar();
    applyModernDarkTheme();

    // Setup shortcuts
    new QShortcut(QKeySequence("Ctrl+O"), this, SLOT(onOpenImage()));
    new QShortcut(QKeySequence("Ctrl+S"), this, SLOT(onExportImage()));
    new QShortcut(QKeySequence("Ctrl+Z"), this, SLOT(onUndo()));

    statusBar()->showMessage(QString::fromUtf8("Siap. Pilih template deteksi lalu klik 'Hitung Otomatis' atau klik manual pada gambar."));
}

void MainWindow::setupUI() {
    canvas = new AnnotationCanvas(this);
    setCentralWidget(canvas);

    connect(canvas, &AnnotationCanvas::pointCountChanged, this, &MainWindow::updatePointCountDisplay);
    connect(canvas, &AnnotationCanvas::zoomChanged, this, &MainWindow::updateZoomDisplay);
}

void MainWindow::setupToolbar() {
    auto* toolbar = addToolBar("Aksi Utama");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(20, 20));

    auto* btnOpen = toolbar->addAction(QString::fromUtf8("📂 Buka Gambar (Ctrl+O)"));
    connect(btnOpen, &QAction::triggered, this, &MainWindow::onOpenImage);

    auto* btnExport = toolbar->addAction(QString::fromUtf8("💾 Simpan Gambar (Ctrl+S)"));
    connect(btnExport, &QAction::triggered, this, &MainWindow::onExportImage);

    auto* btnCSV = toolbar->addAction(QString::fromUtf8("📊 Ekspor CSV"));
    connect(btnCSV, &QAction::triggered, this, &MainWindow::onExportCSV);

    toolbar->addSeparator();

    auto* btnUndo = toolbar->addAction(QString::fromUtf8("↩ Undo Titik (Ctrl+Z)"));
    connect(btnUndo, &QAction::triggered, this, &MainWindow::onUndo);

    auto* btnClear = toolbar->addAction(QString::fromUtf8("🗑 Bersihkan Semua"));
    connect(btnClear, &QAction::triggered, this, &MainWindow::onClear);

    toolbar->addSeparator();

    auto* btnFit = toolbar->addAction(QString::fromUtf8("🔍 Sesuaikan Layar"));
    connect(btnFit, &QAction::triggered, this, &MainWindow::onFitZoom);
}

void MainWindow::setupSidebar() {
    auto* dock = new QDockWidget(QString::fromUtf8("Panel Deteksi & Statistik"), this);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    dock->setMinimumWidth(330);

    auto* container = new QWidget(dock);
    auto* layout = new QVBoxLayout(container);
    layout->setSpacing(12);
    layout->setContentsMargins(12, 12, 12, 12);

    // --- KARTU COUNTER (METRIK BESAR) ---
    auto* countBox = new QGroupBox(QString::fromUtf8("Total Objek Terhitung"), container);
    auto* countLayout = new QVBoxLayout(countBox);

    countDisplayLabel = new QLabel("0", countBox);
    countDisplayLabel->setAlignment(Qt::AlignCenter);
    QFont fontCount("Segoe UI", 36, QFont::Bold);
    countDisplayLabel->setFont(fontCount);
    countDisplayLabel->setStyleSheet("color: #00E676; padding: 4px;");
    countLayout->addWidget(countDisplayLabel);

    auto* countSub = new QLabel(QString::fromUtf8("Manual: Klik Kiri (+) | Klik Kanan (-)"), countBox);
    countSub->setAlignment(Qt::AlignCenter);
    countSub->setStyleSheet("color: #90A4AE; font-size: 11px;");
    countLayout->addWidget(countSub);
    layout->addWidget(countBox);

    // --- FITUR AUTO-COUNTING LENGKAP DENGAN DEDUPLIKASI CERDAS ---
    auto* autoBox = new QGroupBox(QString::fromUtf8("⚡ Auto-Count (Template Komoditas)"), container);
    auto* autoLayout = new QVBoxLayout(autoBox);

    autoLayout->addWidget(new QLabel(QString::fromUtf8("Pilih Template Barang:"), autoBox));
    templateCombo = new QComboBox(autoBox);
    templateCombo->addItem(QString::fromUtf8("🔘 Pipa Silinder / Besi Berongga"));
    templateCombo->addItem(QString::fromUtf8("🧵 Roll Kain / Tekstil Gulungan"));
    templateCombo->addItem(QString::fromUtf8("⚪ Granule / Biji Plastik / Pellet"));
    templateCombo->addItem(QString::fromUtf8("📦 Karton Box / Peti Kayu Kemasan"));
    templateCombo->addItem(QString::fromUtf8("🛢️ Drum Kimia / Tong Aspal / Barel"));
    templateCombo->addItem(QString::fromUtf8("🛞 Ban Kendaraan / Velg Roda"));
    templateCombo->addItem(QString::fromUtf8("🌾 Karung Beras / Tepung / Jumbo Bag"));
    templateCombo->addItem(QString::fromUtf8("🪵 Kayu Log / Gelondongan"));
    templateCombo->addItem(QString::fromUtf8("🧯 Tabung Gas Medis / Silinder LPG"));
    templateCombo->addItem(QString::fromUtf8("⚽ Benda Bulat / Buah Seragam"));
    templateCombo->addItem(QString::fromUtf8("⭕ Pola Lingkaran Kustom"));
    templateCombo->addItem(QString::fromUtf8("🔍 Objek Kontras Bebas (Custom Blob)"));
    autoLayout->addWidget(templateCombo);

    connect(templateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onTemplateChanged);

    // Rentang Radius Perkiraan
    auto* radiusRangeLayout = new QHBoxLayout();
    radiusRangeLayout->addWidget(new QLabel("Min R:", autoBox));
    minRadiusSpin = new QSpinBox(autoBox);
    minRadiusSpin->setRange(4, 300);
    minRadiusSpin->setValue(20);
    radiusRangeLayout->addWidget(minRadiusSpin);

    radiusRangeLayout->addWidget(new QLabel("Max R:", autoBox));
    maxRadiusSpin = new QSpinBox(autoBox);
    maxRadiusSpin->setRange(8, 600);
    maxRadiusSpin->setValue(90);
    radiusRangeLayout->addWidget(maxRadiusSpin);
    autoLayout->addLayout(radiusRangeLayout);

    // Jarak Minimal Antar Objek (Anti-Double Titik)
    auto* spacingLayout = new QHBoxLayout();
    spacingLayout->addWidget(new QLabel(QString::fromUtf8("Jarak Aman Anti-Double:"), autoBox));
    minSpacingSpin = new QSpinBox(autoBox);
    minSpacingSpin->setRange(5, 200);
    minSpacingSpin->setValue(30);
    minSpacingSpin->setSuffix(" px");
    spacingLayout->addWidget(minSpacingSpin);
    autoLayout->addLayout(spacingLayout);

    // Slider Sensitivitas
    auto* sensLayout = new QHBoxLayout();
    sensLayout->addWidget(new QLabel("Sensitivitas:", autoBox));
    sensitivitySlider = new QSlider(Qt::Horizontal, autoBox);
    sensitivitySlider->setRange(30, 220);
    sensitivitySlider->setValue(100);
    sensitivityLabel = new QLabel("100", autoBox);
    sensitivityLabel->setMinimumWidth(30);
    sensLayout->addWidget(sensitivitySlider);
    sensLayout->addWidget(sensitivityLabel);
    autoLayout->addLayout(sensLayout);

    connect(sensitivitySlider, &QSlider::valueChanged, [this](int val) {
        sensitivityLabel->setText(QString::number(val));
    });

    btnAutoCount = new QPushButton(QString::fromUtf8("🚀 Jalankan Auto-Count"), autoBox);
    btnAutoCount->setStyleSheet("background-color: #00C853; color: white; font-weight: bold; padding: 10px; font-size: 13px;");
    autoLayout->addWidget(btnAutoCount);
    layout->addWidget(autoBox);

    connect(btnAutoCount, &QPushButton::clicked, this, &MainWindow::onRunAutoDetect);

    // --- MODE KANVAS ---
    auto* modeBox = new QGroupBox(QString::fromUtf8("Mode Kursor"), container);
    auto* modeLayout = new QHBoxLayout(modeBox);

    btnAddMode = new QPushButton(QString::fromUtf8("📍 Tambah Titik"), modeBox);
    btnAddMode->setCheckable(true);
    btnAddMode->setChecked(true);

    btnPanMode = new QPushButton(QString::fromUtf8("✋ Geser / Pan"), modeBox);
    btnPanMode->setCheckable(true);

    modeLayout->addWidget(btnAddMode);
    modeLayout->addWidget(btnPanMode);
    layout->addWidget(modeBox);

    connect(btnAddMode, &QPushButton::clicked, [this]() {
        btnAddMode->setChecked(true);
        btnPanMode->setChecked(false);
        onModeChanged(false);
    });

    connect(btnPanMode, &QPushButton::clicked, [this]() {
        btnPanMode->setChecked(true);
        btnAddMode->setChecked(false);
        onModeChanged(true);
    });

    // --- KATEGORI WARNA TITIK ---
    auto* catBox = new QGroupBox(QString::fromUtf8("Kategori & Warna Titik"), container);
    auto* catLayout = new QVBoxLayout(catBox);

    colorPresetCombo = new QComboBox(catBox);
    colorPresetCombo->addItem(QString::fromUtf8("🟢 Kondisi Baik / Sesuai"));
    colorPresetCombo->addItem(QString::fromUtf8("🔴 Rusak / Cacat / Selisih"));
    colorPresetCombo->addItem(QString::fromUtf8("🟡 Perlu Pemeriksaan Ulang"));
    colorPresetCombo->addItem(QString::fromUtf8("🔵 Sampel Uji Laboratorium"));
    colorPresetCombo->addItem(QString::fromUtf8("🟣 Lain-lain / Tambahan"));
    catLayout->addWidget(colorPresetCombo);
    layout->addWidget(catBox);

    connect(colorPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onColorPresetChanged);

    // --- UKURAN BULATAN TITIK ---
    auto* sizeBox = new QGroupBox(QString::fromUtf8("Ukuran Nomor Visual"), container);
    auto* sizeLayout = new QVBoxLayout(sizeBox);

    auto* sliderLayout = new QHBoxLayout();
    radiusSlider = new QSlider(Qt::Horizontal, sizeBox);
    radiusSlider->setRange(6, 36);
    radiusSlider->setValue(14);

    radiusValueLabel = new QLabel("14 px", sizeBox);
    radiusValueLabel->setMinimumWidth(45);

    sliderLayout->addWidget(radiusSlider);
    sliderLayout->addWidget(radiusValueLabel);
    sizeLayout->addLayout(sliderLayout);
    layout->addWidget(sizeBox);

    connect(radiusSlider, &QSlider::valueChanged, this, &MainWindow::onRadiusSliderChanged);

    // --- INFORMASI DOKUMEN / GAMBAR ---
    auto* infoBox = new QGroupBox(QString::fromUtf8("Status Gambar"), container);
    auto* infoLayout = new QVBoxLayout(infoBox);

    imageInfoLabel = new QLabel(QString::fromUtf8("Belum ada gambar yang dimuat"), infoBox);
    imageInfoLabel->setWordWrap(true);
    imageInfoLabel->setStyleSheet("color: #CFD8DC; font-size: 11px;");
    infoLayout->addWidget(imageInfoLabel);

    zoomLabel = new QLabel("Zoom: 100%", infoBox);
    zoomLabel->setStyleSheet("color: #90A4AE; font-size: 11px;");
    infoLayout->addWidget(zoomLabel);
    layout->addWidget(infoBox);

    layout->addStretch();
    dock->setWidget(container);
    addDockWidget(Qt::RightDockWidgetArea, dock);
}

void MainWindow::onTemplateChanged(int index) {
    // Penyetelan parameter otomatis yang optimal sesuai jenis komoditas
    switch (index) {
        case 0: // Pipa Silinder
            minRadiusSpin->setValue(20);
            maxRadiusSpin->setValue(90);
            minSpacingSpin->setValue(35);
            sensitivitySlider->setValue(95);
            break;
        case 1: // Roll Kain
            minRadiusSpin->setValue(25);
            maxRadiusSpin->setValue(110);
            minSpacingSpin->setValue(40);
            sensitivitySlider->setValue(105);
            break;
        case 2: // Granule / Pellet
            minRadiusSpin->setValue(4);
            maxRadiusSpin->setValue(25);
            minSpacingSpin->setValue(12);
            sensitivitySlider->setValue(110);
            break;
        case 3: // Karton Box
            minRadiusSpin->setValue(30);
            maxRadiusSpin->setValue(150);
            minSpacingSpin->setValue(50);
            sensitivitySlider->setValue(90);
            break;
        case 4: // Drum / Barel
            minRadiusSpin->setValue(35);
            maxRadiusSpin->setValue(130);
            minSpacingSpin->setValue(55);
            sensitivitySlider->setValue(100);
            break;
        case 5: // Ban Kendaraan
            minRadiusSpin->setValue(40);
            maxRadiusSpin->setValue(160);
            minSpacingSpin->setValue(60);
            sensitivitySlider->setValue(100);
            break;
        case 6: // Karung / Sacks
            minRadiusSpin->setValue(30);
            maxRadiusSpin->setValue(140);
            minSpacingSpin->setValue(45);
            sensitivitySlider->setValue(90);
            break;
        case 7: // Kayu Log
            minRadiusSpin->setValue(25);
            maxRadiusSpin->setValue(120);
            minSpacingSpin->setValue(40);
            sensitivitySlider->setValue(100);
            break;
        case 8: // Tabung Gas Medis / LPG
            minRadiusSpin->setValue(15);
            maxRadiusSpin->setValue(80);
            minSpacingSpin->setValue(28);
            sensitivitySlider->setValue(95);
            break;
        case 9: // Benda Bulat
            minRadiusSpin->setValue(15);
            maxRadiusSpin->setValue(80);
            minSpacingSpin->setValue(25);
            sensitivitySlider->setValue(100);
            break;
        default:
            break;
    }
}

void MainWindow::onRunAutoDetect() {
    if (!canvas->hasImage()) {
        QMessageBox::warning(this, QString::fromUtf8("Belum Ada Gambar"), QString::fromUtf8("Buka foto kontainer / barang terlebih dahulu sebelum menjalankan auto-count."));
        return;
    }

    AutoDetectParams params;
    int idx = templateCombo->currentIndex();
    switch (idx) {
        case 0: params.type = DetectionTemplate::PipesSteelTubes; break;
        case 1: params.type = DetectionTemplate::FabricRolls; break;
        case 2: params.type = DetectionTemplate::GranulesPellets; break;
        case 3: params.type = DetectionTemplate::CartonsBoxes; break;
        case 4: params.type = DetectionTemplate::DrumsBarrels; break;
        case 5: params.type = DetectionTemplate::TiresWheels; break;
        case 6: params.type = DetectionTemplate::SacksBags; break;
        case 7: params.type = DetectionTemplate::TimberLogs; break;
        case 8: params.type = DetectionTemplate::GasCylinders; break;
        case 9: params.type = DetectionTemplate::SpheresBalls; break;
        case 10: params.type = DetectionTemplate::CustomCircles; break;
        default: params.type = DetectionTemplate::CustomBlobs; break;
    }

    params.minRadius = minRadiusSpin->value();
    params.maxRadius = maxRadiusSpin->value();
    params.threshold = sensitivitySlider->value();
    params.minDistance = minSpacingSpin->value();

    statusBar()->showMessage(QString::fromUtf8("Sedang memproses deteksi otomatis dengan Vision Engine..."));
    int detectedCount = canvas->runAutoDetection(params);

    if (detectedCount > 0) {
        QMessageBox::information(this, QString::fromUtf8("Auto-Count Sukses"),
            QString::fromUtf8("Berhasil mendeteksi %1 objek secara akurat tanpa duplikasi!\nAnda dapat menambah atau menghapus titik secara manual jika diperlukan.").arg(detectedCount));
        statusBar()->showMessage(QString::fromUtf8("Deteksi selesai: %1 objek baru ditambahkan.").arg(detectedCount), 6000);
    } else {
        QMessageBox::information(this, QString::fromUtf8("Hasil Deteksi"),
            QString::fromUtf8("Tidak ada objek yang sesuai dengan parameter saat ini.\nCoba sesuaikan batas Min/Max Radius atau geser slider Sensitivitas."));
        statusBar()->showMessage(QString::fromUtf8("Deteksi selesai: 0 objek ditemukan."), 4000);
    }
}

void MainWindow::onExportCSV() {
    if (canvas->getPointCount() == 0) {
        QMessageBox::information(this, QString::fromUtf8("Perhatian"), QString::fromUtf8("Belum ada titik objek untuk diekspor ke CSV."));
        return;
    }

    QString defaultName = "Rekapitulasi_Hitung_Objek.csv";
    QString path = QFileDialog::getSaveFileName(this, QString::fromUtf8("Ekspor Rekap CSV"), defaultName, "CSV File (*.csv)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "No,Kategori,Waktu_Pemeriksaan\n";
        QString timeNow = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        for (int i = 1; i <= canvas->getPointCount(); ++i) {
            out << i << ",Objek Terhitung," << timeNow << "\n";
        }
        file.close();
        QMessageBox::information(this, QString::fromUtf8("Sukses"), QString::fromUtf8("Data rekapitulasi berhasil disimpan ke:\n%1").arg(path));
    }
}

void MainWindow::onOpenImage() {
    QString filter = QString::fromUtf8("File Gambar (*.jpg *.jpeg *.png *.webp *.bmp *.tiff);;Semua File (*.*)");
    QString path = QFileDialog::getOpenFileName(this, QString::fromUtf8("Pilih Foto Pemeriksaan Kontainer"), "", filter);
    if (path.isEmpty()) return;

    if (canvas->loadImage(path)) {
        QFileInfo fi(path);
        imageInfoLabel->setText(QString::fromUtf8("File: %1\nLokasi: %2").arg(fi.fileName(), fi.absolutePath()));
        statusBar()->showMessage(QString::fromUtf8("Berhasil memuat gambar: %1").arg(fi.fileName()), 5000);
    } else {
        QMessageBox::warning(this, QString::fromUtf8("Gagal Memuat"), QString::fromUtf8("Format gambar tidak didukung atau file rusak."));
    }
}

void MainWindow::onExportImage() {
    if (!canvas->hasImage()) {
        QMessageBox::information(this, QString::fromUtf8("Perhatian"), QString::fromUtf8("Muat gambar dan buat titik anotasi terlebih dahulu sebelum mengekspor."));
        return;
    }

    QString filter = QString::fromUtf8("PNG Image (*.png);;JPEG Image (*.jpg)");
    QString defaultName = "Laporan_Anotasi_Hitung.png";
    QString path = QFileDialog::getSaveFileName(this, QString::fromUtf8("Simpan Foto Beranotasi"), defaultName, filter);
    if (path.isEmpty()) return;

    if (canvas->exportAnnotatedImage(path)) {
        QMessageBox::information(this, QString::fromUtf8("Sukses"), QString::fromUtf8("Foto hasil perhitungan berhasil disimpan ke:\n%1").arg(path));
        statusBar()->showMessage(QString::fromUtf8("Ekspor berhasil: %1").arg(path), 5000);
    } else {
        QMessageBox::critical(this, QString::fromUtf8("Gagal"), QString::fromUtf8("Gagal menyimpan file gambar."));
    }
}

void MainWindow::onUndo() {
    canvas->undoLastPoint();
}

void MainWindow::onClear() {
    if (canvas->getPointCount() > 0) {
        auto res = QMessageBox::question(this, QString::fromUtf8("Konfirmasi"),
            QString::fromUtf8("Hapus semua %1 titik anotasi yang sudah dibuat?").arg(canvas->getPointCount()),
            QMessageBox::Yes | QMessageBox::No);
        if (res == QMessageBox::Yes) {
            canvas->clearAnnotations();
        }
    }
}

void MainWindow::onFitZoom() {
    canvas->resetZoom();
}

void MainWindow::onModeChanged(bool isPan) {
    canvas->setCanvasMode(isPan ? CanvasMode::Pan : CanvasMode::AddPoint);
    statusBar()->showMessage(isPan ? QString::fromUtf8("Mode Geser (Pan) aktif.") : QString::fromUtf8("Mode Tambah Titik aktif."));
}

void MainWindow::onColorPresetChanged(int index) {
    QColor color;
    switch (index) {
        case 0: color = QColor(0, 230, 118); break; // Hijau
        case 1: color = QColor(255, 23, 68);  break; // Merah
        case 2: color = QColor(255, 214, 0);  break; // Kuning
        case 3: color = QColor(0, 176, 255);  break; // Biru
        default: color = QColor(213, 0, 249); break; // Ungu
    }
    canvas->setCurrentColor(color);
}

void MainWindow::onRadiusSliderChanged(int value) {
    canvas->setPointRadius(value);
    radiusValueLabel->setText(QString::number(value) + " px");
}

void MainWindow::updatePointCountDisplay(int count) {
    countDisplayLabel->setText(QString::number(count));
    statusBar()->showMessage(QString::fromUtf8("Total titik terhitung: %1").arg(count));
}

void MainWindow::updateZoomDisplay(double zoomPercent) {
    zoomLabel->setText(QString::fromUtf8("Zoom: %1%").arg(static_cast<int>(zoomPercent)));
}

void MainWindow::applyModernDarkTheme() {
    setStyleSheet(R"(
        QMainWindow {
            background-color: #121418;
            color: #E0E0E0;
        }
        QToolBar {
            background-color: #1E222A;
            border-bottom: 1px solid #2C3240;
            padding: 4px;
            spacing: 6px;
        }
        QToolButton {
            background-color: transparent;
            color: #E0E0E0;
            padding: 6px 12px;
            border-radius: 4px;
            font-size: 13px;
            font-weight: 500;
        }
        QToolButton:hover {
            background-color: #2D3442;
        }
        QToolButton:pressed {
            background-color: #3B4456;
        }
        QDockWidget {
            color: #ECEFF1;
            font-weight: bold;
            font-size: 13px;
        }
        QDockWidget::title {
            background-color: #1E222A;
            padding: 8px;
            border-bottom: 1px solid #2C3240;
        }
        QGroupBox {
            background-color: #1A1D24;
            border: 1px solid #2D3442;
            border-radius: 8px;
            margin-top: 14px;
            padding-top: 14px;
            color: #90A4AE;
            font-weight: bold;
            font-size: 12px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 12px;
            padding: 0 4px;
        }
        QPushButton {
            background-color: #2A303C;
            color: #ECEFF1;
            border: 1px solid #3A4252;
            padding: 8px 14px;
            border-radius: 6px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #373F4F;
        }
        QPushButton:checked {
            background-color: #1976D2;
            border-color: #42A5F5;
            color: white;
        }
        QComboBox, QSpinBox {
            background-color: #2A303C;
            color: #ECEFF1;
            border: 1px solid #3A4252;
            padding: 6px 10px;
            border-radius: 6px;
        }
        QComboBox QAbstractItemView {
            background-color: #222731;
            color: #ECEFF1;
            selection-background-color: #1976D2;
        }
        QSlider::groove:horizontal {
            height: 6px;
            background: #2D3442;
            border-radius: 3px;
        }
        QSlider::sub-page:horizontal {
            background: #00E676;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #FFFFFF;
            border: 1px solid #B0BEC5;
            width: 16px;
            margin-top: -5px;
            margin-bottom: -5px;
            border-radius: 8px;
        }
        QStatusBar {
            background-color: #181B22;
            color: #90A4AE;
            border-top: 1px solid #252A36;
        }
    )");
}
