#pragma once

#include "ITabPlugin.h"

class GStreamerModule;
class QComboBox;

class GStreamerTab : public QObject, public ITabPlugin
{
    Q_OBJECT
public:
    explicit GStreamerTab(QObject *parent = nullptr);
    ~GStreamerTab() override = default;

    QString tabName()     const override { return tr("GStreamer"); }
    int     tabPriority() const override { return 8; }
    QString settingKey()  const override { return "gstreamerTabEnabled"; }

    void initialize(const QString &tabGroup,
                    TabRegistryService *registry,
                    QList<CollapsibleHelper*> &helpers,
                    QWidget *parent) override;

    QWidget *tab() const override;
    void saveSettings(QSettings &settings) override;
    void loadSettings(QSettings &settings) override;

    void setResolutionSelector(QComboBox *selector);
    GStreamerModule *module() const { return m_module; }

private:
    GStreamerModule *m_module = nullptr;
};
