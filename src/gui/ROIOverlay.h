#ifndef ROIOVERLAY_H
#define ROIOVERLAY_H

#include <QWidget>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QRect>
#include <QTimer>
#include <QRubberBand>
#include <QPixmap>

class ROIOverlay : public QWidget {
    Q_OBJECT

public:
    explicit ROIOverlay(QWidget *parent = nullptr);
    
    void setPreviewWindow(const QRect &previewRect);
    void setPreviewFromBoxInput(const QString &boxInput);
    QRect getROISelection() const;
    void clearSelection();
    bool isROIActive() const { return roiActive; }
    void findPreviewWindow();
    void startROISelection();
    void setAspectRatio(double ratio);  // Set aspect ratio for selection
    // setScreenshot() removed - using real transparency instead!

signals:
    void roiSelectionChanged(const QRect &roiRect);
    void roiSelectionFinished(double x, double y, double width, double height);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void updatePreviewSearch();

private:
    void updateMask();  // SOP-style dynamic masking

private:
    bool isSelecting;
    bool roiActive;
    bool waitingForFirstClick;
    QPoint startPoint;
    QRect selectionRect;
    QRect previewWindowRect;
    QTimer *previewSearchTimer;
    // m_screenshot removed - using real transparency instead!
    QRubberBand *rubberBand; // Overlay für die Auswahlrechteck
    double aspectRatio = 0.0; // Seitenverhältnis (0.0 = frei)
    
    QRect findQtPreviewWindow();
    void normalizeSelection();
};

#endif // ROIOVERLAY_H