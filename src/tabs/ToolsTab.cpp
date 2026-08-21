#include "ToolsTab.h"
#include "../modules/addons/tools/ToolsModule.h"
#include "../app/TabRegistryService.h"

ToolsTab::ToolsTab(QObject *parent)
    : QObject(parent)
{
}

void ToolsTab::initialize(const QString &tabGroup,
                           TabRegistryService *registry,
                           QList<CollapsibleHelper*> &helpers,
                           QWidget *parent)
{
    m_module = new ToolsModule(tabGroup, parent);
    m_module->setup(helpers, [](){});

    registry->registerTab(m_module->tab(), tabName(), tabPriority(),
                           settingKey(), true);
}

QWidget *ToolsTab::tab() const
{
    return m_module ? m_module->tab() : nullptr;
}
