#include "InferenceTab.h"
#include "../modules/addons/odr/OdrModule.h"
#include "../app/TabRegistryService.h"

InferenceTab::InferenceTab(QObject *parent)
    : QObject(parent)
{
}

void InferenceTab::initialize(const QString & /*tabGroup*/,
                               TabRegistryService *registry,
                               QList<CollapsibleHelper*> &helpers,
                               QWidget *parent)
{
    m_module = new OdrModule(m_detectionAction, parent);
    m_module->setup(helpers, nullptr);

    connect(m_module, &OdrModule::detectionToExecute,
            this, &InferenceTab::detectionToExecute);

    registry->registerTab(m_module->tab(), tabName(), tabPriority(),
                           settingKey(), false);
}

QWidget *InferenceTab::tab() const
{
    return m_module ? m_module->tab() : nullptr;
}
