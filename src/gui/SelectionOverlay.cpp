#include "SelectionOverlay.h"
#include <QPainter>
#include <QDebug>
#include <QApplication>
#include <QScreen>
#include <QRegion>

// SOP-Makros für transparente Fenster
#define TRANSPARENT_WINDOW_FLAGS (Qt::Window | Qt::X11BypassWindowManagerHint | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
#define TRANSPARENT_WINDOW_ATTRIBUTES() setAttribute(Qt::WA_TranslucentBackground)

SelectionOverlay::SelectionOverlay(QWidget *parent, bool outside)
    : QWidget(parent, TRANSPARENT_WINDOW_FLAGS) {
    TRANSPARENT_WINDOW_ATTRIBUTES();
    m_outside = outside;
    
    // Create diagonal pattern with piStudio blue (opaque RGB!)
    QImage image(16, 16, QImage::Format_RGB32);
    for(size_t j = 0; j < (size_t) image.height(); ++j) {
        uint32_t *row = (uint32_t*) image.scanLine(j);
        for(size_t i = 0; i < (size_t) image.width(); ++i) {
            // piStudio blue: #0093DD (uniform)
            row[i] = 0xff0093dd;
        }
    }
    m_texture = QPixmap::fromImage(image);
    
    UpdateMask();
}

void SelectionOverlay::SetRectangle(const QRect& r) {
    // SOP: Set geometry of frame window
    QRect rect = r.normalized();
    if(m_outside)
        rect.adjust(-BORDER_WIDTH, -BORDER_WIDTH, BORDER_WIDTH, BORDER_WIDTH);
    if(rect.isEmpty()) {
        hide();
    } else {
        setGeometry(rect);
        show();
    }
}

void SelectionOverlay::UpdateMask() {
    // SOP: Compositing support detection (static, evaluated once)
    static bool compositing = true;  // Assume compositing for now
    
    if(m_outside) {
        // Outside mode: Always frame mask
        setMask(QRegion(0, 0, width(), height()).subtracted(
            QRegion(BORDER_WIDTH, BORDER_WIDTH, width() - 2 * BORDER_WIDTH, height() - 2 * BORDER_WIDTH)));
        setWindowOpacity(0.5);
    } else {
        // Inside mode (rubber band selection)
        if(compositing) {
            clearMask();
            setWindowOpacity(0.25);
        } else {
            setMask(QRegion(0, 0, width(), height()).subtracted(
                QRegion(BORDER_WIDTH, BORDER_WIDTH, width() - 2 * BORDER_WIDTH, height() - 2 * BORDER_WIDTH)));
            setWindowOpacity(1.0);
        }
    }
}

void SelectionOverlay::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    m_texture.setDevicePixelRatio(devicePixelRatioF());
#endif
    
    painter.setPen(QColor(0, 0, 0, 128));
    painter.setBrush(Qt::NoBrush);
    
    // SOP: Draw tiled pattern everywhere!
    painter.drawTiledPixmap(0, 0, width(), height(), m_texture);
    
    // Draw frame outline
    if(m_outside) {
        painter.drawRect(QRectF((qreal) BORDER_WIDTH - 0.5,
                                (qreal) BORDER_WIDTH - 0.5,
                                (qreal) (width() - 2 * BORDER_WIDTH) + 1.0,
                                (qreal) (height() - 2 * BORDER_WIDTH) + 1.0));
    } else {
        painter.drawRect(QRectF(0.5, 0.5, (qreal) width() - 1.0, (qreal) height() - 1.0));
    }
}

void SelectionOverlay::setAspectRatio(double ratio) {
    aspectRatio = ratio;
}

QRect SelectionOverlay::getSelectedArea() const {
    // Compatibility: Not used anymore (MainWindow handles selection)
    return QRect();
}

void SelectionOverlay::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event);
    // SOP: Update mask when window is resized!
    UpdateMask();
}