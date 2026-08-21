#include "ROIOverlay.h"
#include "../app/AppMeta.h"
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QWindow>
#include <QDebug>
#include <QKeyEvent>
#include <QBuffer>
#include <QPalette>
#include <QProcess>
#include <QRubberBand>
#include <QPixmap>

// Helper: detect native Wayland session (not XWayland)
static bool isNativeWaylandSession() {
    if (qgetenv("XDG_SESSION_TYPE") == "wayland") return true;
    if (!qgetenv("WAYLAND_DISPLAY").isEmpty()) return true;
    return false;
}

// Prüfe, ob der Desktop Compositor Transparenz unterstützt
bool checkCompositorTransparency() {
    // Prüfe, ob wir auf Wayland oder X11 laufen
    QString sessionType = qgetenv("XDG_SESSION_TYPE");
    
    if (sessionType == "wayland") {
        qDebug() << "ROI: Running on Wayland";
        // Wayland sollte normalerweise Transparenz unterstützen
        return true;
    } else {
        qDebug() << "ROI: Running on X11";
        // Prüfe, ob ein Compositor läuft
        QProcess compositor;
        compositor.start("pgrep", QStringList() << "-x" << "compositor|compton|picom|xcompmgr");
        compositor.waitForFinished(1000);
        
        if (compositor.exitCode() == 0) {
            qDebug() << "ROI: Compositor detected";
            return true;
        } else {
            qDebug() << "ROI: No compositor detected";
            return false;
        }
    }
}

ROIOverlay::ROIOverlay(QWidget *parent)
    : QWidget(nullptr, 
      Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
      | (isNativeWaylandSession() ? Qt::WindowFlags() : Qt::X11BypassWindowManagerHint))
    , isSelecting(false)
    , roiActive(false)
    , waitingForFirstClick(false)
    , previewSearchTimer(new QTimer(this))
{
    setAttribute(Qt::WA_TranslucentBackground);
    
    // SOP-style: Start hidden, will be shown when selection rectangle is set
    hide();
    
    setFocusPolicy(Qt::StrongFocus);
    previewSearchTimer->setInterval(500);
    connect(previewSearchTimer, &QTimer::timeout, this, &ROIOverlay::updatePreviewSearch);
    setGeometry(0, 0, 100, 100);
    hide();
}

// setScreenshot() removed - using real transparency instead!
// No more screenshot needed with Qt::WA_TranslucentBackground

void ROIOverlay::setPreviewWindow(const QRect &previewRect)
{
    previewWindowRect = previewRect;
    roiActive = true;
    
    // NUR Daten speichern - Geometrie wird in startROISelection() gesetzt
    qDebug() << "ROI: setPreviewWindow called with:" << previewRect;
}

void ROIOverlay::setPreviewFromBoxInput(const QString &boxInput)
{
    if (boxInput.isEmpty()) {
        qDebug() << "ROI: Empty box input";
        return;
    }
    
    QStringList coords = boxInput.split(',');
    if (coords.size() != 4) {
        qDebug() << "ROI: Invalid box input format:" << boxInput;
        return;
    }
    
    bool ok;
    int x = coords[0].toInt(&ok);
    if (!ok) return;
    int y = coords[1].toInt(&ok);
    if (!ok) return;
    int width = coords[2].toInt(&ok);
    if (!ok) return;
    int height = coords[3].toInt(&ok);
    if (!ok) return;
    
    // Korrigiere die Koordinaten für Menübalken und Rahmen
    // Y-Position: Addiere die Menübalken-Höhe basierend auf der Fenstergröße
    
    // Bei größeren Fenstern (x2-Modus) ist die Menübalken-Korrektur anders
    int menuBarHeight = 30; // Standard-Menübalken-Höhe
    
    // Dynamische Anpassung basierend auf der Fenstergröße
    if (height > 600) {
        // Großes Fenster (x2-Modus) - NOCH weniger Offset nötig (eine Fenstertitelhöhe weniger)
        menuBarHeight = 0;  // Keine zusätzliche Menübalken-Korrektur bei x2-Modus
    }
    
    y += menuBarHeight;
    
    // Breite: Addiere 1 Pixel für den fehlenden Rahmen
    width += 2;
    
    QRect previewRect(x, y, width, height);
    // Feinkorrektur: 1px nach rechts, 2px nach oben verschieben, 1px breiter nach rechts, 1px höher nach oben
    previewRect.translate(1, -2); // 1px nach rechts, 2px nach oben (kumulativ)
    previewRect.setWidth(previewRect.width() - 1); // Breite: war -2, jetzt -1 = +1px breiter nach rechts
    previewRect.setHeight(previewRect.height() + 1); // 1px höher nach oben
    qDebug() << "ROI: Setting preview window from BoxInput (corrected):" << previewRect;
    qDebug() << "ROI: Original coords:" << coords[0] << coords[1] << coords[2] << coords[3];
    qDebug() << "ROI: Corrected coords:" << x << y << width << height;
    
    setPreviewWindow(previewRect);
}

QRect ROIOverlay::getROISelection() const
{
    return selectionRect;
}

void ROIOverlay::clearSelection()
{
    selectionRect = QRect();
    isSelecting = false;
    roiActive = false;
    previewSearchTimer->stop();
    update();
}

void ROIOverlay::startROISelection()
{
    qDebug() << "=== ROI startROISelection ===";
    qDebug() << "ROI: previewWindowRect:" << previewWindowRect;
    
    // Reset selection state
    isSelecting = false;
    selectionRect = QRect();
    waitingForFirstClick = true;
    
    // WICHTIG: Alles VOR show() vorbereiten!
    // 1. Geometrie setzen (während Widget noch versteckt ist)
    setGeometry(previewWindowRect);
    
    bool wayland = isNativeWaylandSession();
    
    // 2. Fenster transparent machen – unterschiedliche Strategie je Platform
    if (wayland) {
        // Wayland: show full semi-transparent overlay so user can click on it.
        // grabMouse/grabKeyboard work only locally (within the overlay) on Wayland,
        // which is exactly what we want for ROI selection inside the preview.
        clearMask();
        setWindowOpacity(0.3);
        qDebug() << "ROI: Wayland mode - full overlay visible for click-through";
    } else {
        // X11: 1-pixel offscreen mask, entirely invisible.
        // Global grabMouse() captures all mouse events and routes them here.
        setMask(QRegion(-10, -10, 1, 1));
        setWindowOpacity(1.0);
        qDebug() << "ROI: X11 mode - invisible overlay with global grab";
    }
    
    // 3. Jetzt erst anzeigen
    show();
    raise();
    activateWindow();
    setFocus();
    
    // grabMouse/grabKeyboard: on X11 this gives global capture; on Wayland it is local-only
    grabMouse(Qt::CrossCursor);
    grabKeyboard();
    
    qDebug() << "ROI: Overlay shown at" << geometry();
    
    // WICHTIG: Automatische Fenstersuche DEAKTIVIERT
    // Bei Multi-Monitor-Setups (besonders RDP) kann die automatische Suche
    // zu falschen Koordinaten führen. Wir verwenden die bereits korrekten
    // Koordinaten aus setPreviewFromBoxInput()
    // previewSearchTimer->start();
    // findPreviewWindow();
    
    qDebug() << "ROI: Overlay positioned at" << geometry() << "- auto-search disabled";
}

void ROIOverlay::updatePreviewSearch()
{
    findPreviewWindow();
}

void ROIOverlay::findPreviewWindow()
{
    QRect foundWindow = findQtPreviewWindow();
    qDebug() << "ROI: findPreviewWindow() - foundWindow:" << foundWindow;
    qDebug() << "ROI: findPreviewWindow() - current previewWindowRect:" << previewWindowRect;
    if (!foundWindow.isEmpty() && foundWindow != previewWindowRect) {
        qDebug() << "ROI: AUTO-REPOSITIONING overlay from" << previewWindowRect << "to" << foundWindow;
        setPreviewWindow(foundWindow);
    } else if (!foundWindow.isEmpty()) {
        qDebug() << "ROI: Found window matches current position, no repositioning";
    } else {
        qDebug() << "ROI: No qt-preview window found";
    }
}

QRect ROIOverlay::findQtPreviewWindow()
{
    // Search for the qt-preview window
    // The qt-preview window typically has a recognizable title or properties
    
    qDebug() << "ROI: findQtPreviewWindow() - scanning windows...";
    const auto windows = QApplication::allWindows();
    qDebug() << "ROI: Total windows found:" << windows.size();
    
    for (QWindow* window : windows) {
        if (window && window->isVisible()) {
            QString title = window->title().toLower();
            qDebug() << "ROI: Window:" << title << "geometry:" << window->geometry() 
                     << "screen:" << (window->screen() ? window->screen()->name() : "null");
            
            // Detect the qt-preview window
            if (title.contains("preview") || title.contains("rpicam") || 
                (window->width() > 100 && window->height() > 100 && 
                 !title.contains(QLatin1String(AppMeta::NAME)))) {
                
                QRect windowRect(window->geometry());
                qDebug() << "ROI: Potential preview window found, original geometry:" << windowRect;
                
                // Convert to screen coordinates if necessary
                if (window->screen()) {
                    QRect screenGeom = window->screen()->geometry();
                    qDebug() << "ROI: Window screen geometry:" << screenGeom;
                    windowRect = QRect(
                        window->x() + screenGeom.x(),
                        window->y() + screenGeom.y(),
                        window->width(),
                        window->height()
                    );
                    qDebug() << "ROI: Converted to screen coordinates:" << windowRect;
                }
                return windowRect;
            }
        }
    }
    return QRect();
}

void ROIOverlay::updateMask() {
    if (!selectionRect.isNull() && isSelecting && selectionRect.width() > 1 && selectionRect.height() > 1) {
        // Rahmen um die Auswahl herum sichtbar machen
        QRect r = selectionRect.normalized();
        const int bw = 4;  // Border width
        QRegion outer(r.adjusted(-bw, -bw, bw, bw));
        QRegion inner(r);
        setMask(outer.subtracted(inner));  // Nur Rahmen sichtbar, Inhalt transparent
    } else if (isNativeWaylandSession() && waitingForFirstClick) {
        // Wayland: keep full window visible so user can click on it
        clearMask();
    } else {
        // Keine/kleine Auswahl: 1-Pixel Maske außerhalb = unsichtbar
        setMask(QRegion(-10, -10, 1, 1));
    }
}

void ROIOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    // Wayland: show subtle hint when waiting for first click
    if (isNativeWaylandSession() && waitingForFirstClick && roiActive) {
        QPainter p(this);
        // Semi-transparent blue tint to indicate selection mode
        p.fillRect(rect(), QColor(0, 147, 221, 60)); // app blue with low alpha
        p.setPen(QPen(QColor(0, 147, 221, 120), 2));
        p.drawRect(rect().adjusted(1, 1, -2, -2));
        // Draw hint text
        p.setPen(QColor(255, 255, 255, 180));
        QFont font = p.font();
        font.setPointSize(14);
        font.setBold(true);
        p.setFont(font);
        p.drawText(rect(), Qt::AlignCenter, tr("Click and drag to select ROI region"));
        return;
    }
    
    if (!selectionRect.isNull() && isSelecting) {
        QPainter p(this);
        
        // SOP-style: app blue for the frame
        static QPixmap pattern;
        if (pattern.isNull()) {
            QImage image(16, 16, QImage::Format_RGB32);
            for (int j = 0; j < image.height(); ++j) {
                uint32_t *row = (uint32_t*) image.scanLine(j);
                for (int i = 0; i < image.width(); ++i) {
                    // App blue: #0093DD (uniform)
                    row[i] = 0xff0093dd;
                }
            }
            pattern = QPixmap::fromImage(image);
        }
        
        // Tile the pattern over the whole window (masked to the frame)
        p.drawTiledPixmap(0, 0, width(), height(), pattern);
    }
}

void ROIOverlay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && roiActive) {
        // Event nur akzeptieren wenn innerhalb des Widget-Bereichs
        QPoint pos = event->pos();
        if (pos.x() >= 0 && pos.x() < width() && pos.y() >= 0 && pos.y() < height()) {
            waitingForFirstClick = false;
            isSelecting = true;
            startPoint = pos;
            selectionRect = QRect(startPoint, startPoint);
            
            // updateMask() zeigt Rahmen sobald Auswahl größer als 1x1
            updateMask();
            update();
        }
    }
}

void ROIOverlay::mouseMoveEvent(QMouseEvent *event)
{
    if (isSelecting && roiActive) {
        QPoint currentPoint = event->pos();
        currentPoint.setX(qBound(0, currentPoint.x(), width() - 1));
        currentPoint.setY(qBound(0, currentPoint.y(), height() - 1));
        selectionRect = QRect(startPoint, currentPoint).normalized();
        
        // Apply aspect ratio if set (like Preview Geometry)
        if (aspectRatio > 0.0) {
            int rectWidth = selectionRect.width();
            int calculatedHeight = static_cast<int>(rectWidth / aspectRatio);
            
            // Stelle sicher, dass die berechnete Höhe nicht über die Widget-Grenzen hinausgeht
            if (selectionRect.top() + calculatedHeight <= height()) {
                selectionRect.setHeight(calculatedHeight);
            } else {
                // Wenn die Höhe zu groß wäre, passe die Breite an
                int maxHeight = height() - selectionRect.top();
                int adjustedWidth = static_cast<int>(maxHeight * aspectRatio);
                selectionRect.setWidth(adjustedWidth);
                selectionRect.setHeight(maxHeight);
            }
        }
        
        updateMask();
        update();
        if (!selectionRect.isEmpty()) {
            emit roiSelectionChanged(selectionRect);
        }
    }
}

void ROIOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isSelecting) {
        isSelecting = false;
        normalizeSelection();
        releaseMouse();
        releaseKeyboard();
        updateMask();  // Update mask after selection complete
        update();
        if (!selectionRect.isEmpty() && roiActive) {
            double relX = (double)selectionRect.x() / width();
            double relY = (double)selectionRect.y() / height();
            double relW = (double)selectionRect.width() / width();
            double relH = (double)selectionRect.height() / height();
            emit roiSelectionFinished(relX, relY, relW, relH);
            hide();
        }
    }
}

void ROIOverlay::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        clearSelection();
        releaseMouse();
        releaseKeyboard();
        hide();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (!selectionRect.isEmpty() && roiActive) {
            normalizeSelection();
            double relX = (double)selectionRect.x() / width();
            double relY = (double)selectionRect.y() / height();
            double relW = (double)selectionRect.width() / width();
            double relH = (double)selectionRect.height() / height();
            emit roiSelectionFinished(relX, relY, relW, relH);
        }
        releaseMouse();
        releaseKeyboard();
        hide();
    }
    QWidget::keyPressEvent(event);
}

void ROIOverlay::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    this->move(previewWindowRect.topLeft());
    this->resize(previewWindowRect.size());
    
    // Leere Maske = komplett unsichtbar
    setMask(QRegion());
    setWindowOpacity(1.0);
    
    this->show();
    this->raise();
    this->activateWindow();
    update();
}

void ROIOverlay::setAspectRatio(double ratio) {
    aspectRatio = ratio;
    qDebug() << "ROI: Aspect ratio set to" << ratio;
}

void ROIOverlay::normalizeSelection()
{
    if (selectionRect.isEmpty()) return;
    
    // Stelle sicher, dass die Auswahl innerhalb der Widget-Grenzen liegt
    int x = qMax(0, selectionRect.x());
    int y = qMax(0, selectionRect.y());
    int right = qMin(width(), selectionRect.right());
    int bottom = qMin(height(), selectionRect.bottom());
    
    selectionRect = QRect(x, y, right - x, bottom - y);
    
    // Mindestgröße sicherstellen
    if (selectionRect.width() < 10) {
        selectionRect.setWidth(10);
    }
    if (selectionRect.height() < 10) {
        selectionRect.setHeight(10);
    }
    
    // Erneut prüfen, dass wir nicht über die Grenzen hinausgehen
    if (selectionRect.right() > width()) {
        selectionRect.moveRight(width());
    }
    if (selectionRect.bottom() > height()) {
        selectionRect.moveBottom(height());
    }
}