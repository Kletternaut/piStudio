#pragma once

#include "ITabPlugin.h"

class GstLaunchModule;

class GstLaunchTab : public QObject, public ITabPlugin
{
    Q_OBJECT
public:
    explicit GstLaunchTab(QObject *parent = nullptr);
    ~GstLaunchTab() override = default;

    QString tabName()     const override { return tr("GST"); }
    int     tabPriority() const override { return 9; }
    QString settingKey()  const override { return "gstTabEnabled"; }

    void initialize(const QString &tabGroup,
                    TabRegistryService *registry,
                    QList<CollapsibleHelper*> &helpers,
                    QWidget *parent) override;

    QWidget *tab() const override;
    void saveSettings(QSettings &settings) override;
    void loadSettings(QSettings &settings) override;

    GstLaunchModule *module() const { return m_module; }

private:
    GstLaunchModule *m_module = nullptr;
};
