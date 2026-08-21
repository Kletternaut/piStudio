#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>

class QTabWidget;
class QTextBrowser;
class QTableWidget;
class QLineEdit;
class QVBoxLayout;
class QTextEdit;
class QComboBox;
class QScrollArea;
class QGroupBox;

class HelpDialog : public QDialog {
    Q_OBJECT

public:
    explicit HelpDialog(QWidget *parent = nullptr, int initialTab = 0);
    ~HelpDialog() override = default;
    void scrollToEnhancedMode();
    static QString collectSystemInfo();

private:
    int m_initialTab;
    QTabWidget *m_tabWidget = nullptr;
    QScrollArea *m_guiScrollArea = nullptr;
    QGroupBox *m_enhancedGroup = nullptr;
    void setupUI();
    void createAppHelpTab(QTabWidget *tabWidget);
    void createRpicamAppsParametersTab(QTabWidget *tabWidget);
    void createSupportTab(QTabWidget *tabWidget);
};

#endif // HELPDIALOG_H
