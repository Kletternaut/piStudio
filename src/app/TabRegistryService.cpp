#include "TabRegistryService.h"
#include <QSettings>
#include "../utils/AppPaths.h"
#include <algorithm>

TabRegistryService::TabRegistryService(QTabWidget *tabWidget, const QString &tabGroup, QObject *parent)
    : QObject(parent), m_tabWidget(tabWidget), m_tabGroup(tabGroup)
{
    // This is where we will register the tabs
}

void TabRegistryService::registerTab(QWidget *widget, const QString &name, int priority, const QString &settingKey, bool defaultEnabled)
{
    m_allTabs.append({priority, widget, name, settingKey, defaultEnabled});
}

void TabRegistryService::reorderAllTabs()
{
    if (!m_tabWidget) {
        return;
    }

    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);

    // This list should be populated from MainWindow
    // For now, we will leave it empty
    // m_allTabs = { ... };

    int currentIndex = m_tabWidget->currentIndex();
    QWidget *currentWidget = (currentIndex >= 0) ? m_tabWidget->widget(currentIndex) : nullptr;

    while (m_tabWidget->count() > 0) {
        m_tabWidget->removeTab(0);
    }

    QList<TabInfo> enabledTabs;
    for (const TabInfo &tabInfo : m_allTabs) {
        if (!tabInfo.widget) {
            continue;
        }

        bool enabled = true;
        if (!tabInfo.settingKey.isEmpty()) {
            enabled = settings.value(tabInfo.settingKey, tabInfo.defaultEnabled).toBool();
        }

        if (enabled) {
            enabledTabs.append(tabInfo);
        }
    }

    std::sort(enabledTabs.begin(), enabledTabs.end(),
              [](const TabInfo &a, const TabInfo &b) {
                  return a.priority < b.priority;
              });

    for (const TabInfo &tabInfo : enabledTabs) {
        m_tabWidget->addTab(tabInfo.widget, tabInfo.name);
    }

    settings.endGroup();

    if (currentWidget) {
        int newIndex = m_tabWidget->indexOf(currentWidget);
        if (newIndex >= 0) {
            m_tabWidget->setCurrentIndex(newIndex);
        }
    }
}
