#pragma once

#include <QImage>
#include <QVector>
#include <QPointF>
#include <QString>
#include <cmath>
#include <algorithm>
#include <vector>

enum class DetectionTemplate {
    PipesSteelTubes,      // 🔘 Pipa Silinder / Besi Berongga / Tabung Gas
    FabricRolls,          // 🧵 Roll Kain / Tekstil Gulungan / Karpet
    GranulesPellets,      // ⚪ Granule / Biji Plastik / Butiran Kimia / Pellet
    CartonsBoxes,         // 📦 Karton / Kardus Box / Peti Kayu Kemasan
    DrumsBarrels,         // 🛢️ Drum Kimia / Tong Aspal / Barel Cairan
    TiresWheels,          // 🛞 Ban Kendaraan / Velg / Karet Lingkar
    SacksBags,            // 🌾 Karung Beras / Tepung / Semen / Jumbo Bag
    TimberLogs,           // 🪵 Kayu Log / Kayu Bulat Gelondongan
    GasCylinders,         // 🧯 Tabung Gas Medis / Silinder LPG / APAR
    SpheresBalls,         // ⚽ Benda Bulat / Bola / Buah Impor Seragam
    CustomCircles,        // ⭕ Pola Lingkaran Kustom (Fleksibel)
    CustomBlobs           // 🔍 Deteksi Kontras Bebas (Kustom Objek Apapun)
};

struct AutoDetectParams {
    DetectionTemplate type = DetectionTemplate::PipesSteelTubes;
    int minRadius = 15;
    int maxRadius = 120;
    int threshold = 110;        // Sensitivitas deteksi (Edge / Binary threshold)
    int minDistance = 25;       // Jarak minimum antar objek (Mencegah titik ganda)
    float minCircularity = 0.55f; // Toleransi kebulatan (1.0 = lingkaran sempurna)
    float minArea = 80.0f;
    float maxArea = 150000.0f;
};

struct DetectedObject {
    QPointF center;
    float radius;
    float confidence;
    QString label;
};

class VisionEngine {
public:
    static QVector<DetectedObject> detectObjects(const QImage& inputImage, const AutoDetectParams& params);

private:
    static QVector<DetectedObject> detectCirclesAdaptive(const QImage& gray, const AutoDetectParams& params, const QString& label);
    static QVector<DetectedObject> detectBlobsAdaptive(const QImage& gray, const AutoDetectParams& params, const QString& label, bool invertBinary = false);
    static QVector<DetectedObject> detectBoxesAdaptive(const QImage& gray, const AutoDetectParams& params, const QString& label);
    static QVector<DetectedObject> detectFabricRollsAdaptive(const QImage& gray, const AutoDetectParams& params);

    static QImage toGrayscale(const QImage& img);
    static QImage gaussianBlur(const QImage& gray);
    static QImage otsuBinarize(const QImage& gray, int manualBias = 0);
    static QImage sobelEdges(const QImage& gray, std::vector<float>& gradAngles);

    // Filter deduplikasi cerdas (Menghilangkan titik double)
    static QVector<DetectedObject> deduplicateObjects(const QVector<DetectedObject>& rawObjects, float minSpacing);
};
