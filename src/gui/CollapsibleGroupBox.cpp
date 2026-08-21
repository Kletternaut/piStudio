#include "CollapsibleGroupBox.h"
#include <QFrame>
#include <QLabel>
#include <QCursor>
#include <QSettings>
#include "../utils/AppPaths.h"

// ClickableLabel Implementation
ClickableLabel::ClickableLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
    setCursor(Qt::PointingHandCursor);
}

void ClickableLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    QLabel::mousePressEvent(event);
}

void ClickableLabel::enterEvent(QEvent *event)
{
    // Hover styling handled by parent stylesheet
    QLabel::enterEvent(event);
}

void ClickableLabel::leaveEvent(QEvent *event)
{
    // Hover styling handled by parent stylesheet
    QLabel::leaveEvent(event);
}

// CollapsibleGroupBox Implementation
CollapsibleGroupBox::CollapsibleGroupBox(const QString &title, QWidget *parent)
    : QWidget(parent), m_collapsed(false), m_title(title)
{
    // Minimal widget styling - let QGroupBox style be applied from MainWindow
    setStyleSheet("");  // No styling, will inherit or be set externally
    
    // Create a frame for the border (like QGroupBox)
    auto *frameWidget = new QFrame(this);
    frameWidget->setObjectName("collapsibleFrame");
    frameWidget->setStyleSheet(
        "QFrame#collapsibleFrame {"
        "    border: 2px solid #3498db;"
        "    border-radius: 8px;"
        "    margin-top: 10px;"
        "    background-color: #f8f9fa;"
        "}"
    );
    
    // Title Label (clickable) - positioned like QGroupBox title
    titleLabel = new ClickableLabel("", this);
    titleLabel->setToolTip(tr("Click to expand/collapse"));
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: #333333;"
        "    font-weight: bold;"
        "    background-color: #f8f9fa;"
        "    border: none;"
        "    padding: 0 5px;"
        "}"
    );
    updateTitleText();
    
    // Content Area inside frame
    contentArea = new QWidget(frameWidget);
    contentArea->setStyleSheet("background-color: transparent; border: none;");
    
    // Frame layout
    auto *frameLayout = new QVBoxLayout(frameWidget);
    frameLayout->setContentsMargins(10, 15, 10, 10);
    frameLayout->setSpacing(5);
    frameLayout->addWidget(contentArea);
    
    // Main layout with title floating above frame
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(frameWidget);
    
    // Position title to overlap frame border (like QGroupBox)
    titleLabel->move(15, 0);
    titleLabel->raise();
    
    // Animation
    toggleAnimation = new QPropertyAnimation(contentArea, "maximumHeight", this);
    toggleAnimation->setDuration(200);
    
    // Connections
    connect(titleLabel, &ClickableLabel::clicked, this, &CollapsibleGroupBox::toggle);
    
    // Initial state
    collapsedHeight = 0;
    expandedHeight = 100; // Will be updated when content is set
}

void CollapsibleGroupBox::setContentLayout(QLayout *contentLayout)
{
    if (contentArea->layout()) {
        delete contentArea->layout();
    }
    
    contentArea->setLayout(contentLayout);
    
    // Important: Allow content to expand naturally
    contentArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    contentArea->setMaximumHeight(QWIDGETSIZE_MAX);  // No limit initially
    
    // Force layout to calculate size
    contentArea->adjustSize();
    contentArea->updateGeometry();
    
    // Calculate expanded height from actual content size
    expandedHeight = contentArea->sizeHint().height();
    if (expandedHeight < 50) expandedHeight = 100;  // Minimum height
    
    if (m_collapsed) {
        contentArea->setMaximumHeight(collapsedHeight);
        contentArea->setVisible(false);
    } else {
        contentArea->setMaximumHeight(expandedHeight);
        contentArea->setVisible(true);
    }
}

void CollapsibleGroupBox::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed) {
        return;
    }
    
    m_collapsed = collapsed;
    updateTitleText();
    saveState();  // Save state to settings
    
    if (collapsed) {
        toggleAnimation->setStartValue(expandedHeight);
        toggleAnimation->setEndValue(collapsedHeight);
        toggleAnimation->finished();
        connect(toggleAnimation, &QPropertyAnimation::finished, [this]() {
            contentArea->setVisible(false);
            disconnect(toggleAnimation, &QPropertyAnimation::finished, nullptr, nullptr);
        });
    } else {
        contentArea->setVisible(true);
        toggleAnimation->setStartValue(collapsedHeight);
        toggleAnimation->setEndValue(expandedHeight);
    }
    
    toggleAnimation->start();
}

bool CollapsibleGroupBox::isCollapsed() const
{
    return m_collapsed;
}

void CollapsibleGroupBox::toggle()
{
    setCollapsed(!m_collapsed);
}

void CollapsibleGroupBox::updateTitleText()
{
    QString arrow = m_collapsed ? "►" : "▼";
    titleLabel->setText(QString("%1 %2").arg(arrow).arg(m_title));
}

void CollapsibleGroupBox::setSettingsKey(const QString &key)
{
    m_settingsKey = key;
    loadState();  // Load immediately when key is set
}

void CollapsibleGroupBox::loadState()
{
    if (m_settingsKey.isEmpty()) {
        return;
    }
    
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    bool collapsed = settings.value(m_settingsKey, false).toBool();
    setCollapsed(collapsed);
}

void CollapsibleGroupBox::saveState()
{
    if (m_settingsKey.isEmpty()) {
        return;
    }
    
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.setValue(m_settingsKey, m_collapsed);
}
