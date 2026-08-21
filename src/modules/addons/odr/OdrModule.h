#pragma once

#include <QObject>
#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QProcess>
#include <QDateTime>
#include <functional>

#include "DetectionActionTypes.h"

class CollapsibleHelper;

class OdrModule : public QObject
{
    Q_OBJECT
public:
    explicit OdrModule(DetectionAction &detectionAction, QWidget *parentWidget);
    ~OdrModule();

    // Must be called once after construction to build the UI.
    // helpers: global collapsible list for toggle-all support
    // adjustWindowCallback: called when a collapsible group is toggled
    void setup(QList<CollapsibleHelper*> &helpers,
               std::function<void()> adjustWindowCallback);

    QWidget *tab() const { return m_tab; }

signals:
    // Emitted when a detection passes filter/cooldown and should trigger actions.
    void detectionToExecute(const QString &object, int confidence, const QString &fullDetection);

private slots:
    void startReceiver();
    void stopReceiver();
    void addDetectionToList(const QString &detection);

private:
    DetectionAction       &m_detectionAction;
    QWidget               *m_tab = nullptr;
    QWidget               *m_parentWidget = nullptr;

    // Receiver UI
    QLineEdit             *m_portInput = nullptr;
    QPushButton           *m_startButton = nullptr;
    QListWidget           *m_resultsList = nullptr;
    QCheckBox             *m_reportOnlyChanges = nullptr;
    QProcess              *m_process = nullptr;

    // State
    QString                m_lastDetectedObject;
};
