#ifndef TABREGISTRYSERVICE_H
#define TABREGISTRYSERVICE_H

#include <QObject>
#include <QTabWidget>
#include <QSettings>

class TabRegistryService : public QObject
{
    Q_OBJECT
public:
    struct TabInfo {
        int priority;
        QWidget *widget;
        QString name;
        QString settingKey;
        bool defaultEnabled;
    };

    explicit TabRegistryService(QTabWidget *tabWidget, const QString &tabGroup, QObject *parent = nullptr);

    void registerTab(QWidget *widget, const QString &name, int priority, const QString &settingKey, bool defaultEnabled);
    void reorderAllTabs();

private:
    QTabWidget *m_tabWidget;
    const QString &m_tabGroup;

    QList<TabInfo> m_allTabs;
};

#endif // TABREGISTRYSERVICE_H
