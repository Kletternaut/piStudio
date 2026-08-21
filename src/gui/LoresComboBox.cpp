#include "LoresComboBox.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>
#include <QMainWindow>
#include <QRegExp>
#include <QTimer>

LoresComboBox::LoresComboBox(QWidget *parent)
    : QComboBox(parent)
    , m_popupWidget(nullptr)
    , m_parCheckbox(nullptr)
    , m_widthInput(nullptr)
    , m_heightInput(nullptr)
    , m_loresEnabled(true)  // Always enabled now
    , m_parEnabled(false)   // Not default enabled
    , m_width(0)
    , m_height(0)
{
    // Keep normal combobox appearance but disable dropdown
    setEditable(false);
    
    // Don't add any items to keep the combobox text area empty
    
    // Create inline widgets overlaying the combobox
    setupInlineWidget();
    
    // Connect to update display text
    updateDisplayText();
    
    // Find and connect to resolution selector changes (delayed to ensure parent is ready)
    QMetaObject::invokeMethod(this, &LoresComboBox::connectToResolutionSelector, Qt::QueuedConnection);
}

void LoresComboBox::setupInlineWidget()
{
    // Create widgets that overlay the combobox content
    m_parCheckbox = new QCheckBox("par", this);
    m_parCheckbox->setGeometry(5, 5, 65, 20);
    m_parCheckbox->setStyleSheet("QCheckBox { background: transparent; }");
    m_parCheckbox->setChecked(false);
    
    m_widthInput = new QLineEdit(this);
    m_widthInput->setPlaceholderText("Width");
    m_widthInput->setGeometry(75, 3, 60, 24);
    m_widthInput->setStyleSheet("QLineEdit { border: 1px solid #ccc; background: white; }");
    
    QLabel *xLabel = new QLabel("x", this);
    xLabel->setGeometry(140, 5, 10, 20);
    xLabel->setStyleSheet("QLabel { background: transparent; }");
    xLabel->setAlignment(Qt::AlignCenter);
    
    m_heightInput = new QLineEdit(this);
    m_heightInput->setPlaceholderText("Height");
    m_heightInput->setGeometry(155, 3, 60, 24);
    m_heightInput->setStyleSheet("QLineEdit { border: 1px solid #ccc; background: white; }");
    
    // Connect signals
    connect(m_parCheckbox, &QCheckBox::toggled, this, &LoresComboBox::onParChanged);
    connect(m_widthInput, &QLineEdit::textChanged, this, &LoresComboBox::onWidthChanged);
    connect(m_heightInput, &QLineEdit::textChanged, this, &LoresComboBox::onHeightChanged);
}

void LoresComboBox::showPopup()
{
    // Do nothing - we don't use popup anymore
}

void LoresComboBox::hidePopup()
{
    // Do nothing - we don't use popup anymore
}

void LoresComboBox::onParChanged(bool enabled)
{
    m_parEnabled = enabled;
    
    // Trigger recalculation if PAR is enabled
    if (enabled) {
        if (!m_widthInput->text().isEmpty()) {
            onWidthChanged();
        } else if (!m_heightInput->text().isEmpty()) {
            onHeightChanged();
        }
    }
    
    emit parModeChanged(enabled);
    emit loresConfigChanged();
}

void LoresComboBox::onWidthChanged()
{
    bool ok;
    int width = m_widthInput->text().toInt(&ok);
    if (ok && width > 0) {
        m_width = width;
        
        // Calculate height if PAR is enabled
        if (m_parEnabled) {
            calculateHeight(width);
        }
        
        emit dimensionsChanged(m_width, m_height);
    }
    updateDisplayText();
    emit loresConfigChanged();
}

void LoresComboBox::onHeightChanged()
{
    bool ok;
    int height = m_heightInput->text().toInt(&ok);
    if (ok && height > 0) {
        m_height = height;
        
        // Calculate width if PAR is enabled
        if (m_parEnabled) {
            calculateWidth(height);
        }
        
        emit dimensionsChanged(m_width, m_height);
    }
    updateDisplayText();
    emit loresConfigChanged();
}

void LoresComboBox::calculateWidth(int height)
{
    if (height > 0) {
        // Get main resolution for half-resolution calculation
        QString resolution = getCurrentResolution();
        QStringList parts = resolution.split("x");
        
        if (parts.size() == 2) {
            bool ok1, ok2;
            int mainWidth = parts[0].toInt(&ok1);
            int mainHeight = parts[1].toInt(&ok2);
            
            if (ok1 && ok2 && mainHeight > 0) {
                // Calculate width based on the ratio: height / mainHeight = width / mainWidth
                int calculatedWidth = static_cast<int>((double)height * mainWidth / mainHeight);
                if (calculatedWidth % 2 != 0) {
                    calculatedWidth += 1; // Make even
                }
                
                // Temporarily disconnect to avoid recursion
                disconnect(m_widthInput, &QLineEdit::textChanged, this, &LoresComboBox::onWidthChanged);
                m_widthInput->setText(QString::number(calculatedWidth));
                connect(m_widthInput, &QLineEdit::textChanged, this, &LoresComboBox::onWidthChanged);
                
                m_width = calculatedWidth;
                return;
            }
        }
        
        // Fallback to aspect ratio calculation
        double aspectRatio = getCurrentAspectRatio();
        int calculatedWidth = static_cast<int>(height * aspectRatio);
        if (calculatedWidth % 2 != 0) {
            calculatedWidth += 1; // Make even
        }
        
        // Temporarily disconnect to avoid recursion
        disconnect(m_widthInput, &QLineEdit::textChanged, this, &LoresComboBox::onWidthChanged);
        m_widthInput->setText(QString::number(calculatedWidth));
        connect(m_widthInput, &QLineEdit::textChanged, this, &LoresComboBox::onWidthChanged);
        
        m_width = calculatedWidth;
    }
}

void LoresComboBox::calculateHeight(int width)
{
    if (width > 0) {
        // Get main resolution for half-resolution calculation
        QString resolution = getCurrentResolution();
        QStringList parts = resolution.split("x");
        
        if (parts.size() == 2) {
            bool ok1, ok2;
            int mainWidth = parts[0].toInt(&ok1);
            int mainHeight = parts[1].toInt(&ok2);
            
            if (ok1 && ok2 && mainWidth > 0) {
                // Calculate height based on the ratio: width / mainWidth = height / mainHeight
                int calculatedHeight = static_cast<int>((double)width * mainHeight / mainWidth);
                if (calculatedHeight % 2 != 0) {
                    calculatedHeight += 1; // Make even
                }
                
                // Temporarily disconnect to avoid recursion
                disconnect(m_heightInput, &QLineEdit::textChanged, this, &LoresComboBox::onHeightChanged);
                m_heightInput->setText(QString::number(calculatedHeight));
                connect(m_heightInput, &QLineEdit::textChanged, this, &LoresComboBox::onHeightChanged);
                
                m_height = calculatedHeight;
                return;
            }
        }
        
        // Fallback to aspect ratio calculation
        double aspectRatio = getCurrentAspectRatio();
        int calculatedHeight = static_cast<int>(width / aspectRatio);
        if (calculatedHeight % 2 != 0) {
            calculatedHeight += 1; // Make even
        }
        
        // Temporarily disconnect to avoid recursion
        disconnect(m_heightInput, &QLineEdit::textChanged, this, &LoresComboBox::onHeightChanged);
        m_heightInput->setText(QString::number(calculatedHeight));
        connect(m_heightInput, &QLineEdit::textChanged, this, &LoresComboBox::onHeightChanged);
        
        m_height = calculatedHeight;
    }
}

void LoresComboBox::updateDisplayText()
{
    // Keep the combobox text empty so overlaid widgets are always visible
    setCurrentText("");
}

// Public interface methods
bool LoresComboBox::isLoresEnabled() const
{
    // Always return true if any configuration is set
    return m_parEnabled || m_width > 0 || m_height > 0;
}

bool LoresComboBox::isParEnabled() const
{
    return m_parEnabled;
}

int LoresComboBox::getWidth() const
{
    return m_width;
}

int LoresComboBox::getHeight() const
{
    return m_height;
}

void LoresComboBox::setLoresEnabled(bool enabled)
{
    // This method is kept for compatibility but doesn't do much now
    Q_UNUSED(enabled);
}

void LoresComboBox::setParEnabled(bool enabled)
{
    if (m_parCheckbox) {
        m_parCheckbox->setChecked(enabled);
    }
}

void LoresComboBox::setWidth(int width)
{
    if (m_widthInput) {
        m_widthInput->setText(QString::number(width));
    }
}

void LoresComboBox::setHeight(int height)
{
    if (m_heightInput) {
        m_heightInput->setText(QString::number(height));
    }
}

void LoresComboBox::reset()
{
    if (m_parCheckbox) m_parCheckbox->setChecked(false);
    if (m_widthInput) m_widthInput->clear();
    if (m_heightInput) m_heightInput->clear();
    
    m_loresEnabled = false;
    m_parEnabled = false;
    m_width = 0;
    m_height = 0;
    
    updateDisplayText();
}

double LoresComboBox::getCurrentAspectRatio()
{
    // Get the parent MainWindow and access its resolutionSelector
    QWidget *parentWidget = this->parentWidget();
    while (parentWidget && !qobject_cast<QMainWindow*>(parentWidget)) {
        parentWidget = parentWidget->parentWidget();
    }
    
    if (!parentWidget) {
        return 16.0 / 9.0; // Default fallback
    }
    
    // Look for QComboBox widgets that might be the resolutionSelector
    QList<QComboBox*> comboBoxes = parentWidget->findChildren<QComboBox*>();
    QComboBox *resolutionSelector = nullptr;
    
    // Find the resolution selector by looking for typical resolution patterns
    for (QComboBox *combo : comboBoxes) {
        if (combo == this) continue; // Skip ourself
        
        QString currentText = combo->currentText();
        if (currentText.contains(QRegExp("\\d+x\\d+"))) {
            resolutionSelector = combo;
            break;
        }
    }
    
    if (!resolutionSelector) {
        return 16.0 / 9.0; // Default fallback
    }
    
    QString resolution = resolutionSelector->currentText();
    if (resolution.isEmpty()) {
        return 16.0 / 9.0; // Default fallback
    }
    
    // Parse resolution string (e.g., "1920x1080")
    QStringList parts = resolution.split("x");
    if (parts.size() != 2) {
        return 16.0 / 9.0; // Default fallback
    }
    
    bool ok1, ok2;
    double width = parts[0].toDouble(&ok1);
    double height = parts[1].toDouble(&ok2);
    
    if (ok1 && ok2 && height > 0) {
        return width / height;
    }
    
    return 16.0 / 9.0; // Default fallback
}

void LoresComboBox::connectToResolutionSelector()
{
    // Get the parent MainWindow and access its resolutionSelector
    QWidget *parentWidget = this->parentWidget();
    while (parentWidget && !qobject_cast<QMainWindow*>(parentWidget)) {
        parentWidget = parentWidget->parentWidget();
    }
    
    if (!parentWidget) {
        return;
    }
    
    // Look for QComboBox widgets that might be the resolutionSelector
    QList<QComboBox*> comboBoxes = parentWidget->findChildren<QComboBox*>();
    
    // Find the resolution selector by looking for typical resolution patterns
    for (QComboBox *combo : comboBoxes) {
        if (combo == this) continue; // Skip ourself
        
        QString currentText = combo->currentText();
        if (currentText.contains(QRegExp("\\d+x\\d+"))) {
            // Connect to the resolution selector's change signal
            connect(combo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
                    this, &LoresComboBox::onResolutionChanged);
            break;
        }
    }
}

void LoresComboBox::onResolutionChanged(const QString &resolution)
{
    Q_UNUSED(resolution);
    
    // If PAR is enabled and we have values, recalculate
    if (m_parEnabled) {
        if (m_width > 0) {
            // Recalculate height based on new aspect ratio
            calculateHeight(m_width);
        } else if (m_height > 0) {
            // Recalculate width based on new aspect ratio  
            calculateWidth(m_height);
        }
    }
}

QString LoresComboBox::getCurrentResolution()
{
    // Get the parent MainWindow and access its resolutionSelector
    QWidget *parentWidget = this->parentWidget();
    while (parentWidget && !qobject_cast<QMainWindow*>(parentWidget)) {
        parentWidget = parentWidget->parentWidget();
    }
    
    if (!parentWidget) {
        return QString();
    }
    
    // Look for QComboBox widgets that might be the resolutionSelector
    QList<QComboBox*> comboBoxes = parentWidget->findChildren<QComboBox*>();
    
    // Find the resolution selector by looking for typical resolution patterns
    for (QComboBox *combo : comboBoxes) {
        if (combo == this) continue; // Skip ourself
        
        QString currentText = combo->currentText();
        if (currentText.contains(QRegExp("\\d+x\\d+"))) {
            return currentText;
        }
    }
    
    return QString();
}