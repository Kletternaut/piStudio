#include "MainWindow.h"
#include "CollapsibleHelper.h"

// GetMousePhysicalCoordinates ist in MainWindow.cpp definiert (X11-Abhängigkeit dort)
QPoint GetMousePhysicalCoordinates();
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QScrollArea>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QProcess>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QTabWidget>
#include <QFrame>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
void MainWindow::StartGrabbing() {
    m_grabbing = true;
    m_selecting_window = false;  // Rectangle selection (not window)
    m_selection_started = false; // Wait for first mouse press!
    m_rubber_band_rect = QRect();

    // On Wayland, mouse grabbing may not work; the selection overlay serves as fallback
    bool isWayland = (qgetenv("XDG_SESSION_TYPE") == "wayland");

    // SOP-style: MainWindow goes to back, then grab mouse/keyboard
    if (!isWayland) {
        lower();
    }

    // Grab mouse and keyboard on MainWindow itself (like SOP does with PageInput!)
    // These may fail silently on Wayland – cursor tracking falls back to QCursor::pos()
    grabMouse(Qt::CrossCursor);
    grabKeyboard();
    setMouseTracking(true);

    if (isWayland) {
        qDebug() << "StartGrabbing: Wayland detected – using fallback cursor tracking";
    }
}

void MainWindow::StopGrabbing() {
    if (selectionOverlay) {
        selectionOverlay->hide();
    }

    // Release mouse/keyboard grab
    setMouseTracking(false);
    releaseKeyboard();
    releaseMouse();

    raise();
    activateWindow();

    m_grabbing = false;
}

void MainWindow::UpdateRubberBand() {
    if (selectionOverlay) {
        selectionOverlay->SetRectangle(m_rubber_band_rect);
    }
}

// SOP-style Mouse Event Handlers
void MainWindow::mousePressEvent(QMouseEvent *event) {
    if (m_grabbing) {
        if (event->button() == Qt::LeftButton) {
            // SOP: Use X11 XQueryPointer for real screen coordinates!
            QPoint mouse = GetMousePhysicalCoordinates();
            m_rubber_band_rect = QRect(mouse, mouse);
            m_selection_start = mouse;
            m_selection_started = true;  // Now selection has started!
            UpdateRubberBand();
        } else {
            // Right click or other button cancels
            StopGrabbing();
        }
        event->accept();
        return;
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (m_grabbing && m_selection_started) {
        // SOP: Use X11 XQueryPointer for real screen coordinates!
        QPoint mouse = GetMousePhysicalCoordinates();

        // Original 10-pixel rounding for consistent sizing
        QRect rawRect = QRect(m_selection_start, mouse).normalized();
        int x = (rawRect.x() / 10) * 10;
        int y = (rawRect.y() / 10) * 10;
        int width = ((rawRect.width() + 5) / 10) * 10;
        int height = ((rawRect.height() + 5) / 10) * 10;

        // Apply aspect ratio if set (original feature!)
        double aspectRatio = getCurrentVideoAspectRatio();
        if (aspectRatio > 0.0) {
            // Calculate height based on width and aspect ratio
            int calculatedHeight = ((int)(width / aspectRatio + 5) / 10) * 10;
            height = calculatedHeight;
        }

        m_rubber_band_rect = QRect(x, y, width, height);
        UpdateRubberBand();
        event->accept();
        return;
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    if (m_grabbing) {
        if (event->button() == Qt::LeftButton && m_selection_started) {
            // Only accept if selection was actually started
            QRect rawRect = m_rubber_band_rect.normalized();

            // Only accept if selection has meaningful size
            if (rawRect.width() > 5 && rawRect.height() > 5) {
                // Original 10-pixel rounding (already applied in mouseMoveEvent)
                // Just use the already-rounded values from m_rubber_band_rect
                int x = rawRect.x();
                int y = rawRect.y();
                int width = rawRect.width();
                int height = rawRect.height();

                // Update BoxInput with rounded values
                QString boxValue = QString("%1,%2,%3,%4")
                    .arg(x)
                    .arg(y)
                    .arg(width)
                    .arg(height);
                BoxInput->setText(boxValue);

                qDebug() << "Selection completed:" << boxValue;
            }

            StopGrabbing();
        }
        // Ignore release events before selection started (from double-click)
        event->accept();
        return;
    }
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (m_grabbing) {
        if (event->key() == Qt::Key_Escape) {
            StopGrabbing();
        }
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::initializeBoxInput() {
    // Placeholder and tooltip are set in the MainWindow constructor
    // (overlay toggle wording) – keep a single source of truth here.
    connect(BoxInput, &CustomLineEdit::doubleClicked, this, [this]() {
        // Set aspect ratio of current video resolution
        double aspectRatio = getCurrentVideoAspectRatio();
        selectionOverlay->setAspectRatio(aspectRatio);

        // SOP-style: Start grabbing instead of showing fullscreen overlay
        StartGrabbing();
    });

    // Connect right-click "Set as Default" action
    connect(BoxInput, &CustomLineEdit::setAsDefaultRequested, this, [this]() {
        QString currentValue = BoxInput->text();
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        settings.beginGroup(m_tabGroup);
        settings.setValue("Preview/CustomBoxInput", currentValue);
        settings.setValue("Preview/UseCustomGeometry", true); // Auto-enable custom geometry
        settings.endGroup();
        settings.sync();

        qDebug() << "Set custom preview geometry as default:" << currentValue;
        QMessageBox::information(this, "Preview Geometry",
            "Current preview geometry saved as default:\n" + currentValue +
            "\n\n'Use Custom Preview Geometry' has been automatically enabled.");
    });
}

double MainWindow::getCurrentVideoAspectRatio() const {
    QString resolution = resolutionSelector->currentText();

    // Parse Auflösung (z.B. "1920x1080")
    QStringList parts = resolution.split('x');
    if (parts.size() == 2) {
        bool widthOk, heightOk;
        int width = parts[0].toInt(&widthOk);
        int height = parts[1].toInt(&heightOk);

        if (widthOk && heightOk && height > 0) {
            double ratio = (double)width / (double)height;
            qDebug() << "Current video resolution:" << resolution << "- Aspect ratio:" << ratio;
            return ratio;
        }
    }

    // Fallback auf 16:9 wenn Parsing fehlschlägt
    qDebug() << "Could not parse resolution" << resolution << "- using 16:9 fallback";
    return 16.0 / 9.0;
}
void MainWindow::initializeSelectionOverlay() {
    selectionOverlay->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool); // Ohne Rahmen
    selectionOverlay->setAttribute(Qt::WA_TranslucentBackground); // Transparenter Hintergrund
    selectionOverlay->setAttribute(Qt::WA_ShowWithoutActivating); // Nicht den Fokus stehlen
    selectionOverlay->setWindowOpacity(0.01);
    connect(selectionOverlay, &SelectionOverlay::selectionChanged, this, &MainWindow::updateBoxInputFromSelection);
    connect(selectionOverlay, &SelectionOverlay::overlayClosed, this, [this]() {
        QRect selectedArea = selectionOverlay->getSelectedArea();
        if (selectedArea.width() <= 0 || selectedArea.height() <= 0) {
            QMessageBox::warning(this, "Ungültige Auswahl", "Bitte eine gültige Auswahl treffen.");
        }
    });
}

void MainWindow::initializeROIOverlay() {
    roiOverlay = new ROIOverlay(this);

    // Verbinde ROI-Overlay-Signale
    connect(roiOverlay, &ROIOverlay::roiSelectionFinished, this, [this](double x, double y, double width, double height) {
        QString roiText = QString("%1,%2,%3,%4")
            .arg(x, 0, 'f', 3)
            .arg(y, 0, 'f', 3)
            .arg(width, 0, 'f', 3)
            .arg(height, 0, 'f', 3);

        // Schreibe in das richtige Input-Feld basierend auf currentROITarget
        if (currentROITarget == ROISelectionTarget::ROI_INPUT) {
            roiInput->setText(roiText);
            updateROIResetButtonColor();
            appendLog("ROI selected: " + roiText);
        } else if (currentROITarget == ROISelectionTarget::AUTOFOCUS_WINDOW) {
            autofocusWindowInput->setText(roiText);
            updateResetButtonColor(resetAutofocusWindowButton, 1, 0); // Setze Button auf "rot" da nicht Default
            appendLog("Autofocus Window selected: " + roiText);
        }
    });
}

void MainWindow::setupInputLayout() {
    auto *inputLayout = new QHBoxLayout;
    inputLayout->addWidget(new QLabel(tr("App:")));
    inputLayout->addWidget(appSelector);
    inputLayout->addWidget(new QLabel(tr("Cam:")));
    inputLayout->addWidget(cameraSelector);
    inputLayout->addWidget(new QLabel(tr("Size:")));
    inputLayout->addWidget(resolutionSelector);
    inputLayout->addWidget(new QLabel(tr("fps:")));
    inputLayout->addWidget(framerateSelector);
    mainLayout->addLayout(inputLayout); // Nutze das Klassenmitglied mainLayout
}

void MainWindow::setupSliderLayout() {
    auto *sliderLayout = new QVBoxLayout;
    sliderLayout->addWidget(new QLabel(tr("Adjust Parameters:")));
    sliderLayout->addWidget(sharpnessSlider);
    // ...weitere Sliderelemente hier hinzufügen, falls benötigt...
}

// Fehlende Methoden wiederherstellen (Minimalimplementierung, damit der Linker sie findet)
void MainWindow::updateOverlayResetButtonColor(QPushButton *button) {
    if (!button) return;
    QString current = BoxInput->text();
    // Use getDefaultBoxInput() to respect custom geometry setting
    QString expected = getDefaultBoxInput();
    if (current != expected) {
        button->setStyleSheet("color: red;");
    } else {
        button->setStyleSheet("color: black;");
    }

    // Also update global reset button since preview position changed
    if (!isInitializing) {
        updateGlobalResetButtonColor();
    }
}

void MainWindow::updateOutputFileResetButtonColor() {
    if (!resetOutputFileButton) return;

    // Check all output file related settings
    bool hasNonDefaultValues = false;

    if (!outputFileName->text().isEmpty()) hasNonDefaultValues = true;
    if (autoNamingCheckbox->isChecked()) hasNonDefaultValues = true;
    if (timestampCheckbox->isChecked()) hasNonDefaultValues = true;

    if (hasNonDefaultValues) {
        resetOutputFileButton->setStyleSheet("color: red;");
    } else {
        resetOutputFileButton->setStyleSheet("color: black;");
    }
}

void MainWindow::updateROIResetButtonColor() {
    if (!roiResetButton) return;
    QString currentROI = roiInput->text();
    QString defaultROI = "0.0,0.0,1.0,1.0";
    if (currentROI != defaultROI && !currentROI.isEmpty()) {
        roiResetButton->setStyleSheet("color: red;");
    } else {
        roiResetButton->setStyleSheet("");
    }
}
