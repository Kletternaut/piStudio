#pragma once

#include "ITabPlugin.h"
#include "../modules/addons/odr/DetectionActionTypes.h"

class OdrModule;

class InferenceTab : public QObject, public ITabPlugin
{
    Q_OBJECT
public:
    explicit InferenceTab(QObject *parent = nullptr);
    ~InferenceTab() override = default;

    QString tabName()     const override { return tr("ODR"); }
    int     tabPriority() const override { return 10; }
    QString settingKey()  const override { return "odrTabEnabled"; }

    void initialize(const QString &tabGroup,
                    TabRegistryService *registry,
                    QList<CollapsibleHelper*> &helpers,
                    QWidget *parent) override;

    QWidget *tab() const override;
    void saveSettings(QSettings & /*settings*/) override {}
    void loadSettings(QSettings & /*settings*/) override {}

    DetectionAction &detectionAction() { return m_detectionAction; }
    OdrModule *module() const { return m_module; }

signals:
    void detectionToExecute(const QString &object, int confidence, const QString &fullDetection);

private:
    DetectionAction m_detectionAction;
    OdrModule      *m_module = nullptr;
};
