#include "GstLaunchTab.h"
#include "../modules/streaming/GstLaunchModule.h"
#include "../app/TabRegistryService.h"

GstLaunchTab::GstLaunchTab(QObject *parent)
    : QObject(parent)
{
}

void GstLaunchTab::initialize(const QString &tabGroup,
                               TabRegistryService *registry,
                               QList<CollapsibleHelper*> &helpers,
                               QWidget *parent)
{
    m_module = new GstLaunchModule(tabGroup, parent);
    m_module->setup(helpers, [](){});

    registry->registerTab(m_module->tab(), tabName(), tabPriority(),
                           settingKey(), true);
}

QWidget *GstLaunchTab::tab() const
{
    return m_module ? m_module->tab() : nullptr;
}

void GstLaunchTab::saveSettings(QSettings & /*settings*/)
{
    // GstLaunchModule speichert intern via QSettings.
}

void GstLaunchTab::loadSettings(QSettings & /*settings*/)
{
    // GstLaunchModule laedt intern via QSettings.
}
