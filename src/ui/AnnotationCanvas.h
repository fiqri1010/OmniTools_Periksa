#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <vector>
#include "../models/AnnotationPoint.h"
#include "../core/VisionEngine.h"

enum class CanvasMode {
    AddPoint,
    Pan
};

class AnnotationCanvas : public QGraphicsView {
    Q_OBJECT

public:
    explicit AnnotationCanvas(QWidget* parent = nullptr);
    ~AnnotationCanvas() override = default;

    bool loadImage(const QString& filePath);
    void clearAnnotations();
    void undoLastPoint();
    bool exportAnnotatedImage(const QString& outputPath);

    // Auto Detection
    int runAutoDetection(const AutoDetectParams& params);

    void setCanvasMode(CanvasMode mode);
    CanvasMode getCanvasMode() const { return currentMode; }

    void setCurrentColor(const QColor& color) { activeColor = color; }
    QColor getCurrentColor() const { return activeColor; }

    void setPointRadius(int radius) { currentRadius = radius; }
    int getPointRadius() const { return currentRadius; }

    int getPointCount() const { return static_cast<int>(points.size()); }
    bool hasImage() const { return !originalPixmap.isNull(); }
    QImage getCurrentQImage() const { return originalPixmap.toImage(); }

    void resetZoom();

signals:
    void pointCountChanged(int totalCount);
    void zoomChanged(double zoomPercent);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void addAnnotationAt(const QPointF& scenePos);
    void removeAnnotationNear(const QPointF& scenePos);
    void renderPointVisual(const AnnotationPoint& pt);
    void refreshAllVisuals();

    QGraphicsScene* scene;
    QGraphicsPixmapItem* pixmapItem;
    QPixmap originalPixmap;

    std::vector<AnnotationPoint> points;
    std::vector<QGraphicsItemGroup*> visualItems;

    CanvasMode currentMode = CanvasMode::AddPoint;
    QColor activeColor = QColor(0, 220, 130); // Vibrant Green
    int currentRadius = 14;

    bool isPanning = false;
    QPoint lastPanMousePos;
    double currentScale = 1.0;
};
