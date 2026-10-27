#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QComboBox>
#include <QSpinBox>
#include <QStatusBar>
#include "AnnotationCanvas.h"
#include "../core/VisionEngine.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onOpenImage();
    void onExportImage();
    void onExportCSV();
    void onUndo();
    void onClear();
    void onFitZoom();
    void onModeChanged(bool isPan);
    void onColorPresetChanged(int index);
    void onRadiusSliderChanged(int value);
    void onTemplateChanged(int index);
    void onRunAutoDetect();
    void updatePointCountDisplay(int count);
    void updateZoomDisplay(double zoomPercent);

private:
    void setupUI();
    void setupToolbar();
    void setupSidebar();
    void applyModernDarkTheme();

    AnnotationCanvas* canvas;

    // Sidebar Widgets
    QLabel* countDisplayLabel;
    QLabel* imageInfoLabel;
    QLabel* zoomLabel;
    QSlider* radiusSlider;
    QLabel* radiusValueLabel;
    QComboBox* colorPresetCombo;

    // Auto Detection Widgets
    QComboBox* templateCombo;
    QSpinBox* minRadiusSpin;
    QSpinBox* maxRadiusSpin;
    QSpinBox* minSpacingSpin;
    QSlider* sensitivitySlider;
    QLabel* sensitivityLabel;
    QPushButton* btnAutoCount;

    QPushButton* btnAddMode;
    QPushButton* btnPanMode;
};
