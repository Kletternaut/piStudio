#ifndef TABVISIBILITYSERVICE_H
#define TABVISIBILITYSERVICE_H

#include <QObject>

class TabRegistryService;

class TabVisibilityService : public QObject
{
    Q_OBJECT
public:
    explicit TabVisibilityService(TabRegistryService *registryService, QObject *parent = nullptr);

public slots:
    void updateExpertTabVisibility();
    void updateFocusTabVisibility();
    void updateZoomTabVisibility();
    void updateAudioTabVisibility();
    void updateGstreamerTabVisibility();
    void updateGstTabVisibility();
    void updateInferenceTabVisibility();
    void updateActionsTabVisibility();
    void updateToolsTabVisibility();
    void updateDebugTabVisibility();

private:
    TabRegistryService *m_registryService;
};

#endif // TABVISIBILITYSERVICE_H
