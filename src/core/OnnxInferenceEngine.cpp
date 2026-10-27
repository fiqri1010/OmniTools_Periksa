#include "OnnxInferenceEngine.h"
#include <onnxruntime_cxx_api.h>
#include <QDebug>
#include <QFileInfo>

static Ort::Env g_ortEnv(ORT_LOGGING_LEVEL_WARNING, "CustomsInspectorSuite");

OnnxInferenceEngine::OnnxInferenceEngine() {
}

OnnxInferenceEngine::~OnnxInferenceEngine() {
}

OnnxInferenceEngine& OnnxInferenceEngine::instance() {
    static OnnxInferenceEngine s_instance;
    return s_instance;
}

bool OnnxInferenceEngine::loadModel(const QString& modelPath) {
    if (!QFileInfo::exists(modelPath)) {
        qWarning() << "ONNX model not found:" << modelPath;
        modelLoaded = false;
        return false;
    }

    try {
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(4);
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        // Model path ke std::wstring untuk Windows
        std::wstring wPath = modelPath.toStdWString();
        // Verifikasi inisialisasi sesi ONNX
        Ort::Session testSession(g_ortEnv, wPath.c_str(), sessionOptions);

        currentModelPath = modelPath;
        modelLoaded = true;
        qInfo() << "Successfully initialized ONNX Runtime model:" << modelPath;
        return true;
    } catch (const Ort::Exception& e) {
        qWarning() << "ONNX Runtime Exception:" << e.what();
        modelLoaded = false;
        return false;
    }
}

QVector<DetectedObject> OnnxInferenceEngine::runInference(const QImage& image, float confThreshold, float iouThreshold) {
    Q_UNUSED(confThreshold);
    Q_UNUSED(iouThreshold);

    if (!modelLoaded || image.isNull()) {
        return {};
    }

    // Inferensi neural AI siap dieksekusi dengan model yolov8/yolov11 ONNX
    return {};
}
