#ifndef COLLAPSIBLEGROUPBOX_H
#define COLLAPSIBLEGROUPBOX_H

#include <QWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QMouseEvent>

class ClickableLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ClickableLabel(const QString &text = "", QWidget *parent = nullptr);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
};

class CollapsibleGroupBox : public QWidget
{
    Q_OBJECT

public:
    explicit CollapsibleGroupBox(const QString &title = "", QWidget *parent = nullptr);
    void setContentLayout(QLayout *contentLayout);
    void setCollapsed(bool collapsed);
    bool isCollapsed() const;
    
    // Persistent state
    void setSettingsKey(const QString &key);
    QString settingsKey() const { return m_settingsKey; }

private slots:
    void toggle();

private:
    ClickableLabel *titleLabel;
    QWidget *contentArea;
    QPropertyAnimation *toggleAnimation;
    int collapsedHeight;
    int expandedHeight;
    bool m_collapsed;
    QString m_settingsKey;
    QString m_title;
    
    void updateTitleText();
    void loadState();
    void saveState();
};

#endif // COLLAPSIBLEGROUPBOX_H
