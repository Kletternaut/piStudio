#ifndef TOOLSMODULE_H
#define TOOLSMODULE_H

#include <QObject>
#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QSlider>
#include <QLabel>
#include <QProcess>
#include <QTime>
#include <functional>

class CollapsibleHelper;

class ToolsModule : public QObject
{
    Q_OBJECT
public:
    explicit ToolsModule(const QString &tabGroup, QObject *parent = nullptr);

    QWidget *tab() const { return m_tab; }
    void setup(QList<CollapsibleHelper *> &helpers, std::function<void()> adjustWindowCallback);

private slots:
    void browseInputDir();
    void browseOutputFile();
    void startVideoConversion();
    void stopVideoConversion();

private:
    QString m_tabGroup;
    QWidget *m_tab = nullptr;

    QLineEdit *m_inputDirInput = nullptr;
    QPushButton *m_inputDirBrowseButton = nullptr;
    QComboBox *m_patternSelector = nullptr;
    QComboBox *m_framerateSelector = nullptr;
    QComboBox *m_codecComboBox = nullptr;
    QComboBox *m_resizeSelector = nullptr;
    QSlider *m_qualitySlider = nullptr;
    QLabel *m_qualityLabel = nullptr;
    QComboBox *m_presetComboBox = nullptr;
    QLineEdit *m_outputFileInput = nullptr;
    QPushButton *m_outputFileBrowseButton = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QTextEdit *m_logOutput = nullptr;
    QProcess *m_conversionProcess = nullptr;
    int m_totalFrames = 0;
    QTime m_conversionStartTime;
};

#endif // TOOLSMODULE_H
