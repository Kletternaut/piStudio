#include "GStreamerTab.h"
#include "../modules/streaming/GStreamerModule.h"
#include "../app/TabRegistryService.h"

#include <QComboBox>

GStreamerTab::GStreamerTab(QObject *parent)
    : QObject(parent)
{
}

void GStreamerTab::initialize(const QString &tabGroup,
                               TabRegistryService *registry,
                               QList<CollapsibleHelper*> &helpers,
                               QWidget *parent)
{
    m_module = new GStreamerModule(tabGroup, parent);
    m_module->setup(helpers, [](){}, nullptr);

    registry->registerTab(m_module->tab(), tabName(), tabPriority(),
                           settingKey(), true);
}

QWidget *GStreamerTab::tab() const
{
    return m_module ? m_module->tab() : nullptr;
}

void GStreamerTab::saveSettings(QSettings &settings)
{
    if (m_module) m_module->saveSettings(settings);
}

void GStreamerTab::loadSettings(QSettings &settings)
{
    if (m_module) m_module->loadSettings(settings);
}

void GStreamerTab::setResolutionSelector(QComboBox *selector)
{
    if (m_module) m_module->setResolutionSelector(selector);
}
