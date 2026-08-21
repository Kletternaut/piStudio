#pragma once

#include "ITabPlugin.h"

class ToolsModule;

class ToolsTab : public QObject, public ITabPlugin
{
    Q_OBJECT
public:
    explicit ToolsTab(QObject *parent = nullptr);
    ~ToolsTab() override = default;

    QString tabName()     const override { return tr("Tools"); }
    int     tabPriority() const override { return 12; }
    QString settingKey()  const override { return "toolsTabEnabled"; }

    void initialize(const QString &tabGroup,
                    TabRegistryService *registry,
                    QList<CollapsibleHelper*> &helpers,
                    QWidget *parent) override;

    QWidget *tab() const override;
    void saveSettings(QSettings & /*settings*/) override {}
    void loadSettings(QSettings & /*settings*/) override {}

    ToolsModule *module() const { return m_module; }

private:
    ToolsModule *m_module = nullptr;
};
