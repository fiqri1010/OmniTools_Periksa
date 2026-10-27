#include "VisionEngine.h"
#include <queue>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

QImage VisionEngine::toGrayscale(const QImage& img) {
    QImage rgb = img.convertToFormat(QImage::Format_RGB888);
    int width = rgb.width();
    int height = rgb.height();
    QImage gray(width, height, QImage::Format_Grayscale8);

    for (int y = 0; y < height; ++y) {
        const uchar* srcLine = rgb.constScanLine(y);
        uchar* dstLine = gray.scanLine(y);
        for (int x = 0; x < width; ++x) {
            int r = srcLine[x * 3];
            int g = srcLine[x * 3 + 1];
            int b = srcLine[x * 3 + 2];
            // Bobot persepsi mata manusia standar ITU-R BT.601
            dstLine[x] = static_cast<uchar>((r * 299 + g * 587 + b * 114) / 1000);
        }
    }
    return gray;
}

QImage VisionEngine::gaussianBlur(const QImage& gray) {
    int w = gray.width();
    int h = gray.height();
    QImage blur(w, h, QImage::Format_Grayscale8);

    // Filter Gaussian 3x3 tertimbang
    for (int y = 1; y < h - 1; ++y) {
        const uchar* prev = gray.constScanLine(y - 1);
        const uchar* curr = gray.constScanLine(y);
        const uchar* next = gray.constScanLine(y + 1);
        uchar* out = blur.scanLine(y);

        for (int x = 1; x < w - 1; ++x) {
            int sum = (prev[x - 1] + 2 * prev[x] + prev[x + 1])
                    + (2 * curr[x - 1] + 4 * curr[x] + 2 * curr[x + 1])
                    + (next[x - 1] + 2 * next[x] + next[x + 1]);
            out[x] = static_cast<uchar>(sum >> 4);
        }
    }
    return blur;
}

QImage VisionEngine::otsuBinarize(const QImage& gray, int manualBias) {
    int w = gray.width();
    int h = gray.height();
    int totalPixels = w * h;
    if (totalPixels == 0) return gray;

    // Hitung Histogram 256 bin
    int hist[256] = {0};
    for (int y = 0; y < h; ++y) {
        const uchar* line = gray.constScanLine(y);
        for (int x = 0; x < w; ++x) {
            hist[line[x]]++;
        }
    }

    // Algoritma Otsu untuk mencari ambang kontras optimal otomatis
    double sum = 0;
    for (int i = 0; i < 256; ++i) sum += i * hist[i];

    double sumB = 0;
    int wB = 0;
    double maxVariance = 0;
    int optimalThreshold = 128;

    for (int t = 0; t < 256; ++t) {
        wB += hist[t];
        if (wB == 0) continue;
        int wF = totalPixels - wB;
        if (wF == 0) break;

        sumB += static_cast<double>(t * hist[t]);
        double mB = sumB / wB;
        double mF = (sum - sumB) / wF;
        double variance = static_cast<double>(wB) * static_cast<double>(wF) * (mB - mF) * (mB - mF);

        if (variance > maxVariance) {
            maxVariance = variance;
            optimalThreshold = t;
        }
    }

    // Terapkan bias manual dari slider sensitivitas
    int finalThresh = std::clamp(optimalThreshold + (manualBias - 110) / 2, 20, 240);

    QImage binary(w, h, QImage::Format_Grayscale8);
    for (int y = 0; y < h; ++y) {
        const uchar* inLine = gray.constScanLine(y);
        uchar* outLine = binary.scanLine(y);
        for (int x = 0; x < w; ++x) {
            outLine[x] = (inLine[x] > finalThresh) ? 255 : 0;
        }
    }
    return binary;
}

QImage VisionEngine::sobelEdges(const QImage& gray, std::vector<float>& gradAngles) {
    int w = gray.width();
    int h = gray.height();
    QImage edges(w, h, QImage::Format_Grayscale8);
    edges.fill(0);
    gradAngles.assign(w * h, 0.0f);

    for (int y = 1; y < h - 1; ++y) {
        const uchar* prev = gray.constScanLine(y - 1);
        const uchar* curr = gray.constScanLine(y);
        const uchar* next = gray.constScanLine(y + 1);
        uchar* out = edges.scanLine(y);

        for (int x = 1; x < w - 1; ++x) {
            int gx = (prev[x + 1] - prev[x - 1]) + 2 * (curr[x + 1] - curr[x - 1]) + (next[x + 1] - next[x - 1]);
            int gy = (next[x - 1] - prev[x - 1]) + 2 * (next[x] - prev[x]) + (next[x + 1] - prev[x + 1]);

            int mag = static_cast<int>(std::sqrt(gx * gx + gy * gy));
            out[x] = static_cast<uchar>(std::clamp(mag, 0, 255));
            gradAngles[y * w + x] = std::atan2(static_cast<float>(gy), static_cast<float>(gx));
        }
    }
    return edges;
}

QVector<DetectedObject> VisionEngine::deduplicateObjects(const QVector<DetectedObject>& rawObjects, float minSpacing) {
    if (rawObjects.isEmpty()) return {};

    // Urutkan berdasarkan tingkat keyakinan (confidence) tertinggi
    QVector<DetectedObject> sorted = rawObjects;
    std::sort(sorted.begin(), sorted.end(), [](const DetectedObject& a, const DetectedObject& b) {
        return a.confidence > b.confidence;
    });

    QVector<DetectedObject> cleanList;
    for (const auto& candidate : sorted) {
        bool isDuplicate = false;
        // Jarak aman adaptif terhadap radius objek
        float safeDist = std::max(minSpacing, candidate.radius * 0.85f);

        for (const auto& existing : cleanList) {
            float dx = existing.center.x() - candidate.center.x();
            float dy = existing.center.y() - candidate.center.y();
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < safeDist) {
                isDuplicate = true;
                break;
            }
        }

        if (!isDuplicate) {
            cleanList.push_back(candidate);
        }
    }

    return cleanList;
}

QVector<DetectedObject> VisionEngine::detectCirclesAdaptive(const QImage& gray, const AutoDetectParams& params, const QString& label) {
    int w = gray.width();
    int h = gray.height();
    std::vector<float> gradAngles;
    QImage edges = sobelEdges(gaussianBlur(gray), gradAngles);

    int minR = std::max(6, params.minRadius);
    int maxR = std::min(std::min(w, h) / 2, params.maxRadius);
    if (minR >= maxR) maxR = minR + 8;

    // Resolusi akumulator presisi tinggi
    int scale = (w > 1600 || h > 1200) ? 2 : 1;
    int accW = w / scale;
    int accH = h / scale;
    std::vector<int> acc(accW * accH, 0);

    // Ambang sensitivitas tepi adaptif
    int edgeThresh = std::clamp(params.threshold / 2, 25, 120);
    int stepR = std::max(2, (maxR - minR) / 20);

    for (int y = 2; y < h - 2; y += 2) {
        const uchar* edgeLine = edges.constScanLine(y);
        for (int x = 2; x < w - 2; x += 2) {
            if (edgeLine[x] > edgeThresh) {
                float angle = gradAngles[y * w + x];
                float cosA = std::cos(angle);
                float sinA = std::sin(angle);

                for (int r = minR; r <= maxR; r += stepR) {
                    // Penelusuran dua arah (gradien positif & negatif)
                    int cx1 = static_cast<int>(x + r * cosA) / scale;
                    int cy1 = static_cast<int>(y + r * sinA) / scale;
                    if (cx1 >= 0 && cx1 < accW && cy1 >= 0 && cy1 < accH) acc[cy1 * accW + cx1]++;

                    int cx2 = static_cast<int>(x - r * cosA) / scale;
                    int cy2 = static_cast<int>(y - r * sinA) / scale;
                    if (cx2 >= 0 && cx2 < accW && cy2 >= 0 && cy2 < accH) acc[cy2 * accW + cx2]++;
                }
            }
        }
    }

    // Pencarian puncak lokal dengan Non-Maximum Suppression
    int voteThreshold = std::max(10, params.threshold / 8);
    QVector<DetectedObject> rawCandidates;
    float avgR = (minR + maxR) / 2.0f;

    for (int ay = 2; ay < accH - 2; ++ay) {
        for (int ax = 2; ax < accW - 2; ++ax) {
            int v = acc[ay * accW + ax];
            if (v >= voteThreshold) {
                bool isLocalMax = true;
                for (int dy = -1; dy <= 1 && isLocalMax; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (acc[(ay + dy) * accW + (ax + dx)] > v) {
                            isLocalMax = false;
                            break;
                        }
                    }
                }
                if (isLocalMax) {
                    DetectedObject obj;
                    obj.center = QPointF(ax * scale, ay * scale);
                    obj.radius = avgR;
                    obj.confidence = static_cast<float>(v);
                    obj.label = label;
                    rawCandidates.push_back(obj);
                }
            }
        }
    }

    return deduplicateObjects(rawCandidates, static_cast<float>(params.minDistance));
}

QVector<DetectedObject> VisionEngine::detectBlobsAdaptive(const QImage& gray, const AutoDetectParams& params, const QString& label, bool invertBinary) {
    int w = gray.width();
    int h = gray.height();

    QImage binary = otsuBinarize(gaussianBlur(gray), params.threshold);
    std::vector<int> labels(w * h, 0);
    int currentLabel = 0;

    struct Component {
        int count = 0;
        double sumX = 0, sumY = 0;
        int minX = 1e9, maxX = -1e9;
        int minY = 1e9, maxY = -1e9;
    };
    std::vector<Component> components;
    components.push_back(Component{}); // index 0 kosong

    int targetPixelVal = invertBinary ? 0 : 255;

    for (int y = 0; y < h; ++y) {
        const uchar* binLine = binary.constScanLine(y);
        for (int x = 0; x < w; ++x) {
            if (binLine[x] == targetPixelVal && labels[y * w + x] == 0) {
                currentLabel++;
                Component comp;

                std::queue<std::pair<int, int>> q;
                q.push({x, y});
                labels[y * w + x] = currentLabel;

                while (!q.empty()) {
                    auto [cx, cy] = q.front();
                    q.pop();

                    comp.count++;
                    comp.sumX += cx;
                    comp.sumY += cy;
                    comp.minX = std::min(comp.minX, cx);
                    comp.maxX = std::max(comp.maxX, cx);
                    comp.minY = std::min(comp.minY, cy);
                    comp.maxY = std::max(comp.maxY, cy);

                    static const int dx[] = {1, -1, 0, 0};
                    static const int dy[] = {0, 0, 1, -1};
                    for (int k = 0; k < 4; ++k) {
                        int nx = cx + dx[k];
                        int ny = cy + dy[k];
                        if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                            if (binary.scanLine(ny)[nx] == targetPixelVal && labels[ny * w + nx] == 0) {
                                labels[ny * w + nx] = currentLabel;
                                q.push({nx, ny});
                            }
                        }
                    }
                }
                components.push_back(comp);
            }
        }
    }

    QVector<DetectedObject> rawResults;
    for (size_t i = 1; i < components.size(); ++i) {
        const auto& c = components[i];
        if (c.count >= params.minArea && c.count <= params.maxArea) {
            int bw = c.maxX - c.minX + 1;
            int bh = c.maxY - c.minY + 1;
            float aspectRatio = static_cast<float>(std::min(bw, bh)) / static_cast<float>(std::max(bw, bh));

            // Periksa rasio aspek bentuk
            if (aspectRatio >= params.minCircularity) {
                double cx = c.sumX / c.count;
                double cy = c.sumY / c.count;
                float r = std::sqrt(static_cast<float>(c.count) / static_cast<float>(M_PI));

                DetectedObject obj;
                obj.center = QPointF(cx, cy);
                obj.radius = std::max(8.0f, r);
                obj.confidence = static_cast<float>(c.count);
                obj.label = label;
                rawResults.push_back(obj);
            }
        }
    }

    return deduplicateObjects(rawResults, static_cast<float>(params.minDistance));
}

QVector<DetectedObject> VisionEngine::detectBoxesAdaptive(const QImage& gray, const AutoDetectParams& params, const QString& label) {
    // Kardus / Peti Kayu memiliki kontur persegi panjang
    AutoDetectParams boxParams = params;
    boxParams.minCircularity = 0.40f; // Toleransi bentuk persegi/persegi panjang lebih longgar
    return detectBlobsAdaptive(gray, boxParams, label, true);
}

QVector<DetectedObject> VisionEngine::detectFabricRollsAdaptive(const QImage& gray, const AutoDetectParams& params) {
    // Deteksi tumpukan silinder rol tekstil
    return detectCirclesAdaptive(gray, params, "Roll Kain / Tekstil");
}

QVector<DetectedObject> VisionEngine::detectObjects(const QImage& inputImage, const AutoDetectParams& params) {
    if (inputImage.isNull()) return {};

    QImage gray = toGrayscale(inputImage);

    switch (params.type) {
        case DetectionTemplate::PipesSteelTubes:
            return detectCirclesAdaptive(gray, params, "Pipa / Silinder");

        case DetectionTemplate::FabricRolls:
            return detectFabricRollsAdaptive(gray, params);

        case DetectionTemplate::GranulesPellets: {
            AutoDetectParams gParams = params;
            gParams.minArea = 15.0f;
            gParams.maxArea = 5000.0f;
            return detectBlobsAdaptive(gray, gParams, "Granule / Pellet");
        }

        case DetectionTemplate::CartonsBoxes:
            return detectBoxesAdaptive(gray, params, "Karton / Peti Box");

        case DetectionTemplate::DrumsBarrels:
            return detectCirclesAdaptive(gray, params, "Drum / Barel");

        case DetectionTemplate::TiresWheels:
            return detectCirclesAdaptive(gray, params, "Ban / Velg");

        case DetectionTemplate::SacksBags:
            return detectBlobsAdaptive(gray, params, "Karung / Bag");

        case DetectionTemplate::TimberLogs:
            return detectCirclesAdaptive(gray, params, "Kayu Log");

        case DetectionTemplate::GasCylinders:
            return detectCirclesAdaptive(gray, params, "Tabung Gas");

        case DetectionTemplate::SpheresBalls:
            return detectCirclesAdaptive(gray, params, "Benda Bulat");

        case DetectionTemplate::CustomCircles:
            return detectCirclesAdaptive(gray, params, "Objek Lingkaran");

        case DetectionTemplate::CustomBlobs:
        default:
            return detectBlobsAdaptive(gray, params, "Objek Kontras");
    }
}
