#pragma once

#include <QAction>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPoint>
#include <QScrollArea>
#include <QSize>
#include <QSlider>
#include <QString>
#include <QVBoxLayout>
#include <QTimer>
#include <QWidget>
#include <ZoomableLabel.h>

//Main GUI Window class
//holds all widgets and GUI logic
class MainWindow : public QMainWindow 
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openImage();
    void saveImage();
    void sliderValueChanged();
    void sendZoomEvent(const int delta);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    const QString imageDialogFilterText { "Images (*.png *.jpg *.bmp *.webp *.tiff)" };
    void setupMenu();
    void setupSlider(QSlider *slider, QLabel *label, const int value) const;
    void setupImageView();
    void setupMainWidget();
    void addSliderLayout(QVBoxLayout *mainLayout, QSlider *slider, QLabel *label);
    void updateImageView(const QImage& image, const bool resetScale);
    void performSharpening();

    QImage userImage, sharpenedImage;
    QSlider *sharpenStrength, *contrastAdaption;
    ZoomableLabel* imageView;
    QScrollArea* scrollArea;
    QLabel *sharpenStrengthLabel, *contrastAdaptionLabel;
    void* casObj;
    QAction *openImageAction, *saveImageAction;
    const QSize targetImageSize;
    bool userImageHasAlpha;
    QPoint lastMousePos;
    QTimer* throttleTimer;
};