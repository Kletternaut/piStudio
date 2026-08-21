#ifndef COLLAPSIBLEHELPER_H
#define COLLAPSIBLEHELPER_H

#include <QGroupBox>
#include <QPushButton>
#include <QSettings>
#include "../utils/AppPaths.h"
#include <QPropertyAnimation>
#include <QEvent>
#include <QMap>
#include <QTimer>

/**
 * Helper class to make any QGroupBox collapsible
 * Usage: auto helper = CollapsibleHelper::makeCollapsible(myGroupBox, "UI/MyGroup");
 */
class CollapsibleHelper : public QObject {
    Q_OBJECT
    
signals:
    void expanded();  // Emitted when group is expanded
    void toggled();   // Emitted when group is toggled (collapsed or expanded)
    
public:
    static CollapsibleHelper* makeCollapsible(QGroupBox *groupBox, const QString &settingsKey) {
        return new CollapsibleHelper(groupBox, settingsKey, groupBox);
    }
    
    // Helper to make collapsible and connect to a slot
    template<typename Func>
    static CollapsibleHelper* makeCollapsible(QGroupBox *groupBox, const QString &settingsKey, 
                                             Func slot) {
        auto *helper = new CollapsibleHelper(groupBox, settingsKey, groupBox);
        QObject::connect(helper, &CollapsibleHelper::toggled, slot);
        return helper;
    }
    
    // Public accessors for toggle functionality
    QGroupBox* groupBox() const { return m_groupBox; }
    bool isCollapsed() const { return m_collapsed; }
    void toggleGroup() { toggle(); }
    
private:
    explicit CollapsibleHelper(QGroupBox *groupBox, const QString &settingsKey, QObject *parent = nullptr)
        : QObject(parent), m_groupBox(groupBox), m_settingsKey(settingsKey)
    {
        // Create collapse button
        m_button = new QPushButton("−", groupBox);
        m_button->setFixedSize(20, 20);
        m_button->setToolTip("Collapse/Expand group");
        m_button->setStyleSheet(
            "QPushButton { "
            "    border: 1px solid #3498db; "
            "    background: #f8f9fa; "
            "    font-weight: bold; "
            "    border-radius: 3px; "
            "} "
            "QPushButton:hover { background: #e0e0e0; }"
        );
        
        // Position button (will be adjusted in resizeEvent)
        positionButton();
        m_button->raise();
        
        // Connect button
        connect(m_button, &QPushButton::clicked, this, &CollapsibleHelper::toggle);
        
        // Install event filter to reposition button on resize
        groupBox->installEventFilter(this);
        
        // Load saved state (this will collapse if needed)
        loadState();
    }
    
    void storeVisibilityStates() {
        // Store current visibility state of all children
        m_visibilityStates.clear();
        for (auto *child : m_groupBox->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
            if (child != m_button) {
                m_visibilityStates[child] = child->isVisible();
            }
        }
    }
    
    void positionButton() {
        m_button->move(m_groupBox->width() - 28, 3);
    }
    
    void toggle() {
        m_collapsed = !m_collapsed;
        m_button->setText(m_collapsed ? "+" : "−");
        
        if (m_collapsed) {
            // Collapsing: Store current visibility, then hide all
            storeVisibilityStates();
            for (auto *child : m_groupBox->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
                if (child != m_button) {
                    child->setVisible(false);
                }
            }
            m_groupBox->setMaximumHeight(30);
        } else {
            // Expanding: Just show all widgets, let application logic control visibility
            // DON'T restore saved states - they might be outdated
            for (auto *child : m_groupBox->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
                if (child != m_button) {
                    child->setVisible(true);
                }
            }
            m_groupBox->setMaximumHeight(QWIDGETSIZE_MAX);
            
            // Emit signal so application can update visibility logic
            emit expanded();
        }
        
        saveState();
        
        // Emit toggled signal for auto-resize
        emit toggled();
    }
    
    void loadState() {
        if (m_settingsKey.isEmpty()) return;
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        m_collapsed = settings.value(m_settingsKey, false).toBool();
        if (m_collapsed) {
            m_button->setText("+");
            // Store visibility states BEFORE hiding
            // At this point, all widgets have their correct initial visibility from the UI setup
            storeVisibilityStates();
            // Now hide everything
            for (auto *child : m_groupBox->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
                if (child != m_button) {
                    child->setVisible(false);
                }
            }
            m_groupBox->setMaximumHeight(30);
        }
    }
    
    void saveState() {
        if (m_settingsKey.isEmpty()) return;
        QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
        settings.setValue(m_settingsKey, m_collapsed);
    }
    
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (watched == m_groupBox && event->type() == QEvent::Resize) {
            positionButton();
        }
        return QObject::eventFilter(watched, event);
    }
    
    QGroupBox *m_groupBox;
    QPushButton *m_button;
    QString m_settingsKey;
    bool m_collapsed = false;
    QMap<QWidget*, bool> m_visibilityStates;  // Store original visibility of children
};

#endif // COLLAPSIBLEHELPER_H