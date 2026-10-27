#include "AnnotationCanvas.h"
#include <QScrollBar>
#include <QPainter>
#include <cmath>

AnnotationCanvas::AnnotationCanvas(QWidget* parent)
    : QGraphicsView(parent)
{
    scene = new QGraphicsScene(this);
    setScene(scene);

    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setBackgroundBrush(QBrush(QColor(24, 26, 32))); // Dark sleek background

    pixmapItem = nullptr;
}

bool AnnotationCanvas::loadImage(const QString& filePath) {
    QPixmap pixmap(filePath);
    if (pixmap.isNull()) {
        return false;
    }

    originalPixmap = pixmap;
    points.clear();
    visualItems.clear();
    scene->clear();

    pixmapItem = scene->addPixmap(originalPixmap);
    pixmapItem->setZValue(0);
    scene->setSceneRect(0, 0, originalPixmap.width(), originalPixmap.height());

    resetZoom();
    emit pointCountChanged(0);
    return true;
}

void AnnotationCanvas::resetZoom() {
    resetTransform();
    currentScale = 1.0;
    if (pixmapItem) {
        fitInView(pixmapItem, Qt::KeepAspectRatio);
        // Calculate the scale after fitInView
        currentScale = transform().m11();
    }
    emit zoomChanged(currentScale * 100.0);
}

void AnnotationCanvas::wheelEvent(QWheelEvent* event) {
    if (!hasImage()) {
        QGraphicsView::wheelEvent(event);
        return;
    }

    const double factor = 1.15;
    if (event->angleDelta().y() > 0) {
        scale(factor, factor);
        currentScale *= factor;
    } else {
        scale(1.0 / factor, 1.0 / factor);
        currentScale /= factor;
    }
    emit zoomChanged(currentScale * 100.0);
}

void AnnotationCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton || 
       (event->button() == Qt::LeftButton && currentMode == CanvasMode::Pan)) {
        isPanning = true;
        lastPanMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (!hasImage()) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    QPointF scenePos = mapToScene(event->pos());

    if (event->button() == Qt::LeftButton && currentMode == CanvasMode::AddPoint) {
        // Ensure point is within image boundary
        if (scenePos.x() >= 0 && scenePos.x() <= originalPixmap.width() &&
            scenePos.y() >= 0 && scenePos.y() <= originalPixmap.height()) {
            addAnnotationAt(scenePos);
        }
        event->accept();
        return;
    }

    if (event->button() == Qt::RightButton) {
        // Right click removes point near click
        removeAnnotationNear(scenePos);
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void AnnotationCanvas::mouseMoveEvent(QMouseEvent* event) {
    if (isPanning) {
        QPoint delta = event->pos() - lastPanMousePos;
        lastPanMousePos = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void AnnotationCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (isPanning && (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton)) {
        isPanning = false;
        setCursor(currentMode == CanvasMode::Pan ? Qt::OpenHandCursor : Qt::CrossCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void AnnotationCanvas::setCanvasMode(CanvasMode mode) {
    currentMode = mode;
    if (currentMode == CanvasMode::Pan) {
        setCursor(Qt::OpenHandCursor);
    } else {
        setCursor(Qt::CrossCursor);
    }
}

void AnnotationCanvas::addAnnotationAt(const QPointF& scenePos) {
    int nextId = static_cast<int>(points.size()) + 1;
    AnnotationPoint pt(nextId, scenePos, activeColor);
    pt.radius = currentRadius;

    points.push_back(pt);
    renderPointVisual(pt);

    emit pointCountChanged(static_cast<int>(points.size()));
}

int AnnotationCanvas::runAutoDetection(const AutoDetectParams& params) {
    if (!hasImage()) return 0;

    QImage img = originalPixmap.toImage();
    QVector<DetectedObject> detected = VisionEngine::detectObjects(img, params);

    for (const auto& obj : detected) {
        int nextId = static_cast<int>(points.size()) + 1;
        AnnotationPoint pt(nextId, obj.center, activeColor);
        pt.radius = std::max(10, static_cast<int>(obj.radius * 0.45f));
        pt.label = obj.label;

        points.push_back(pt);
        renderPointVisual(pt);
    }

    emit pointCountChanged(static_cast<int>(points.size()));
    return static_cast<int>(detected.size());
}

void AnnotationCanvas::removeAnnotationNear(const QPointF& scenePos) {
    if (points.empty()) return;

    int closestIndex = -1;
    double minDistance = 25.0; // Click tolerance in image coordinates

    for (size_t i = 0; i < points.size(); ++i) {
        double dx = points[i].pos.x() - scenePos.x();
        double dy = points[i].pos.y() - scenePos.y();
        double dist = std::sqrt(dx * dx + dy * dy);
        if (dist < minDistance) {
            minDistance = dist;
            closestIndex = static_cast<int>(i);
        }
    }

    if (closestIndex != -1) {
        points.erase(points.begin() + closestIndex);
        // Renumber remaining points
        for (size_t i = 0; i < points.size(); ++i) {
            points[i].id = static_cast<int>(i) + 1;
        }
        refreshAllVisuals();
        emit pointCountChanged(static_cast<int>(points.size()));
    }
}

void AnnotationCanvas::undoLastPoint() {
    if (!points.empty()) {
        points.pop_back();
        if (!visualItems.empty()) {
            QGraphicsItemGroup* item = visualItems.back();
            scene->removeItem(item);
            delete item;
            visualItems.pop_back();
        }
        emit pointCountChanged(static_cast<int>(points.size()));
    }
}

void AnnotationCanvas::clearAnnotations() {
    points.clear();
    for (auto* item : visualItems) {
        scene->removeItem(item);
        delete item;
    }
    visualItems.clear();
    emit pointCountChanged(0);
}

void AnnotationCanvas::renderPointVisual(const AnnotationPoint& pt) {
    auto* group = new QGraphicsItemGroup();

    // Outer circle
    auto* circle = new QGraphicsEllipseItem(
        pt.pos.x() - pt.radius,
        pt.pos.y() - pt.radius,
        pt.radius * 2,
        pt.radius * 2
    );
    circle->setPen(QPen(QColor(255, 255, 255), 2.0));
    circle->setBrush(QBrush(pt.color));
    group->addToGroup(circle);

    // Number text
    auto* text = new QGraphicsTextItem(QString::number(pt.id));
    QFont font("Segoe UI", pt.radius > 16 ? 10 : 8, QFont::Bold);
    text->setFont(font);
    text->setDefaultTextColor(Qt::white);

    // Center text in circle
    QRectF textBounds = text->boundingRect();
    text->setPos(pt.pos.x() - textBounds.width() / 2.0, pt.pos.y() - textBounds.height() / 2.0);
    group->addToGroup(text);

    group->setZValue(10);
    scene->addItem(group);
    visualItems.push_back(group);
}

void AnnotationCanvas::refreshAllVisuals() {
    for (auto* item : visualItems) {
        scene->removeItem(item);
        delete item;
    }
    visualItems.clear();

    for (const auto& pt : points) {
        renderPointVisual(pt);
    }
}

bool AnnotationCanvas::exportAnnotatedImage(const QString& outputPath) {
    if (!hasImage()) return false;

    QPixmap result = originalPixmap.copy();
    QPainter painter(&result);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    for (const auto& pt : points) {
        // Draw circle
        painter.setPen(QPen(Qt::white, 2.5));
        painter.setBrush(QBrush(pt.color));
        painter.drawEllipse(pt.pos, pt.radius, pt.radius);

        // Draw centered number
        QFont font("Segoe UI", pt.radius > 16 ? 11 : 9, QFont::Bold);
        painter.setFont(font);
        painter.setPen(Qt::white);

        QRectF textRect(pt.pos.x() - pt.radius, pt.pos.y() - pt.radius, pt.radius * 2, pt.radius * 2);
        painter.drawText(textRect, Qt::AlignCenter, QString::number(pt.id));
    }

    painter.end();
    return result.save(outputPath);
}
