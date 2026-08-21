#include "ActionsTab.h"
#include "../app/TabRegistryService.h"

ActionsTab::ActionsTab(QObject *parent)
    : QObject(parent)
{
}

void ActionsTab::initialize(const QString &tabGroup,
                             TabRegistryService *registry,
                             QList<CollapsibleHelper*> &helpers,
                             QWidget *parent)
{
    m_module = new ActionsModule(tabGroup, parent);
    m_module->setup(helpers, [](){}, CameraInterface{});

    if (registry) {
        registry->registerTab(m_module->tab(), tabName(), tabPriority(),
                               settingKey(), false);
    }
}

QWidget *ActionsTab::tab() const
{
    return m_module ? m_module->tab() : nullptr;
}

void ActionsTab::saveSettings(QSettings &settings)
{
    if (m_module) m_module->saveSettings(settings);
}

void ActionsTab::loadSettings(QSettings &settings)
{
    if (m_module) m_module->loadSettings(settings);
}

void ActionsTab::setCameraInterface(const CameraInterface &iface)
{
    if (m_module) m_module->setCameraInterface(iface);
}
