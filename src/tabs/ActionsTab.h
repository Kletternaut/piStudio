#pragma once

#include "ITabPlugin.h"
#include "../modules/addons/actions/ActionsModule.h"

class ActionsTab : public QObject, public ITabPlugin
{
    Q_OBJECT
public:
    explicit ActionsTab(QObject *parent = nullptr);
    ~ActionsTab() override = default;

    QString tabName()     const override { return tr("Actions"); }
    int     tabPriority() const override { return 22; }
    QString settingKey()  const override { return "actionsTabEnabled"; }

    void initialize(const QString &tabGroup,
                    TabRegistryService *registry,
                    QList<CollapsibleHelper*> &helpers,
                    QWidget *parent) override;

    QWidget *tab() const override;
    void saveSettings(QSettings &settings) override;
    void loadSettings(QSettings &settings) override;

    void setCameraInterface(const CameraInterface &iface);
    ActionsModule *module() const { return m_module; }

private:
    ActionsModule *m_module = nullptr;
};
