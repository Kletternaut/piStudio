#ifndef CUSTOMLINEEDIT_H
#define CUSTOMLINEEDIT_H

#include <QLineEdit>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>

class CustomLineEdit : public QLineEdit {
    Q_OBJECT

public:
    explicit CustomLineEdit(QWidget *parent = nullptr) : QLineEdit(parent) {}

signals:
    void doubleClicked(); // Signal for double-click
    void setAsDefaultRequested(); // Signal for "Set as Default" context menu action

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            emit doubleClicked();
        }
        QLineEdit::mouseDoubleClickEvent(event);
    }
    
    void contextMenuEvent(QContextMenuEvent *event) override {
        // Create custom context menu
        QMenu *menu = createStandardContextMenu();
        menu->addSeparator();
        
        QAction *setDefaultAction = menu->addAction("Set as Default");
        connect(setDefaultAction, &QAction::triggered, this, [this]() {
            emit setAsDefaultRequested();
        });
        
        menu->exec(event->globalPos());
        delete menu;
    }
};

#endif // CUSTOMLINEEDIT_H