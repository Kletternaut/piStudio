#ifndef SELECTIONOVERLAY_H
#define SELECTIONOVERLAY_H

#include <QWidget>
#include <QPixmap>

// SOP-style: Kleines bewegliches Fenster für visuellen Rahmen
// Mouse-Events werden von MainWindow verarbeitet!
class SelectionOverlay : public QWidget {
    Q_OBJECT

public:
    static constexpr int BORDER_WIDTH = 4;
    
    explicit SelectionOverlay(QWidget *parent = nullptr, bool outside = false);

    void SetRectangle(const QRect& r);  // SOP: Set position and size
    void setAspectRatio(double ratio);  // Set aspect ratio for selection
    QRect getSelectedArea() const;      // Compatibility (not used anymore)

signals:
    void overlayClosed();               // Compatibility signal (emitted by MainWindow)
    void selectionChanged(const QRect &selection);  // Compatibility signal (emitted by MainWindow)

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void UpdateMask();  // SOP-style dynamic masking
    
    bool m_outside;
    QPixmap m_texture;  // SOP pattern
    double aspectRatio = 0.0; // Seitenverhältnis (0.0 = frei)
};

#endif // SELECTIONOVERLAY_H