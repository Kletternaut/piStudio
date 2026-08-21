#include "TabVisibilityService.h"
#include "TabRegistryService.h"

TabVisibilityService::TabVisibilityService(TabRegistryService *registryService, QObject *parent)
    : QObject(parent), m_registryService(registryService)
{
}

void TabVisibilityService::updateExpertTabVisibility()
{
    m_registryService->reorderAllTabs();
}

void TabVisibilityService::updateFocusTabVisibility()
{
    m_registryService->reorderAllTabs();
}

void TabVisibilityService::updateZoomTabVisibility()
{
    m_registryService->reorderAllTabs();
}

void TabVisibilityService::updateAudioTabVisibility()
{
    m_registryService->reorderAllTabs();
}

void TabVisibilityService::updateGstreamerTabVisibility()
{
    m_registryService->reorderAllTabs();
}

void TabVisibilityService::updateGstTabVisibility()
{
    m_registryService->reorderAllTabs();
}

void TabVisibilityService::updateInferenceTabVisibility()
{
    m_registryService->reorderAllTabs();
}

void TabVisibilityService::updateActionsTabVisibility()
{
    m_registryService->reorderAllTabs();
}

void TabVisibilityService::updateToolsTabVisibility()
{
    m_registryService->reorderAllTabs();
}

void TabVisibilityService::updateDebugTabVisibility()
{
    m_registryService->reorderAllTabs();
}
