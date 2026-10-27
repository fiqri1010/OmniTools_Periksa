#pragma once
#include <QPointF>
#include <QColor>
#include <QString>

struct AnnotationPoint {
    int id;
    QPointF pos;
    QColor color;
    QString label;
    int radius = 12;

    AnnotationPoint(int id = 0, QPointF pos = QPointF(), QColor color = QColor(0, 200, 83), QString label = "")
        : id(id), pos(pos), color(color), label(label) {}
};
