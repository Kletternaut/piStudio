#ifndef CHECKABLECOMBOBOX_H
#define CHECKABLECOMBOBOX_H

#include <QComboBox>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QCheckBox>
#include <QListView>
#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>

class CheckableComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit CheckableComboBox(QWidget *parent = nullptr);
    
    void addCheckableItem(const QString &text, const QVariant &userData = QVariant());
    QStringList getCheckedItems() const;
    void setCheckedItems(const QStringList &items);
    void clearCheckedItems();
    
    void hidePopup() override;

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

signals:
    void checkedItemsChanged();

private slots:
    void onItemChanged(QStandardItem *item);
    void updateDisplayText();

private:
    QStandardItemModel *m_model;
    void setupModel();
    void addHeaderItem();
};

#endif // CHECKABLECOMBOBOX_H
