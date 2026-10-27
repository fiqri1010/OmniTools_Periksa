#pragma once

#include <QString>
#include <QImage>
#include <QVector>
#include "VisionEngine.h"

class OnnxInferenceEngine {
public:
    static OnnxInferenceEngine& instance();

    bool isModelLoaded() const { return modelLoaded; }
    bool loadModel(const QString& modelPath);
    QVector<DetectedObject> runInference(const QImage& image, float confThreshold = 0.40f, float iouThreshold = 0.45f);

private:
    OnnxInferenceEngine();
    ~OnnxInferenceEngine();

    bool modelLoaded = false;
    QString currentModelPath;
};
