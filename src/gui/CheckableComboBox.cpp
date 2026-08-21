#include "CheckableComboBox.h"
#include <QStandardItemModel>
#include <QStandardItem>
#include <QListView>
#include <QEvent>
#include <QMouseEvent>
#include <QApplication>

CheckableComboBox::CheckableComboBox(QWidget *parent)
    : QComboBox(parent)
{
    setupModel();
}

void CheckableComboBox::setupModel()
{
    m_model = new QStandardItemModel(this);
    setModel(m_model);
    
    // Verwende eine QListView für bessere Kontrolle
    QListView *listView = new QListView(this);
    setView(listView);
    
    // Installiere Event-Filter für die ListView, um Popup-Schließung zu verhindern
    listView->viewport()->installEventFilter(this);
    
    // Verhindere die Standard-Selektion beim Klicken
    listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    connect(m_model, &QStandardItemModel::itemChanged, this, &CheckableComboBox::onItemChanged);
    
    // Füge inaktiven Header-Eintrag hinzu
    addHeaderItem();
    
    // Initialer Display-Text
    updateDisplayText();
}

void CheckableComboBox::addHeaderItem()
{
    QStandardItem *headerItem = new QStandardItem("Select options:");
    headerItem->setEnabled(false); // Inaktiv machen
    headerItem->setSelectable(false); // Nicht auswählbar
    headerItem->setFlags(Qt::ItemIsEnabled); // Nur "Enabled", aber nicht "Selectable"
    m_model->appendRow(headerItem);
}

void CheckableComboBox::addCheckableItem(const QString &text, const QVariant &userData)
{
    QStandardItem *item = new QStandardItem(text);
    item->setCheckable(true);
    item->setCheckState(Qt::Unchecked);
    item->setData(userData, Qt::UserRole);
    m_model->appendRow(item);
}

QStringList CheckableComboBox::getCheckedItems() const
{
    QStringList checkedItems;
    // Überspringe den ersten Eintrag (Header)
    for (int i = 1; i < m_model->rowCount(); ++i) {
        QStandardItem *item = m_model->item(i);
        if (item && item->checkState() == Qt::Checked) {
            checkedItems << item->data(Qt::UserRole).toString();
        }
    }
    return checkedItems;
}

void CheckableComboBox::setCheckedItems(const QStringList &items)
{
    // Überspringe den ersten Eintrag (Header)
    for (int i = 1; i < m_model->rowCount(); ++i) {
        QStandardItem *item = m_model->item(i);
        if (item) {
            QString userData = item->data(Qt::UserRole).toString();
            item->setCheckState(items.contains(userData) ? Qt::Checked : Qt::Unchecked);
        }
    }
    updateDisplayText();
}

void CheckableComboBox::clearCheckedItems()
{
    // Überspringe den ersten Eintrag (Header)
    for (int i = 1; i < m_model->rowCount(); ++i) {
        QStandardItem *item = m_model->item(i);
        if (item) {
            item->setCheckState(Qt::Unchecked);
        }
    }
    updateDisplayText();
}

void CheckableComboBox::onItemChanged(QStandardItem *item)
{
    Q_UNUSED(item);
    // Diese Methode wird durch Programm-Updates aufgerufen, nicht durch Benutzer-Klicks
    updateDisplayText();
    emit checkedItemsChanged();
}

void CheckableComboBox::updateDisplayText()
{
    QStringList checkedItems = getCheckedItems();
    
    if (checkedItems.isEmpty()) {
        setCurrentText("Select option:");
    } else {
        QString displayText = QString("Select option: %1 selected").arg(checkedItems.count());
        setCurrentText(displayText);
    }
    
    // Stelle sicher, dass der angezeigte Index auf den Header zeigt
    setCurrentIndex(0);
}

void CheckableComboBox::hidePopup()
{
    QComboBox::hidePopup();
    updateDisplayText();
}

bool CheckableComboBox::eventFilter(QObject *object, QEvent *event)
{
    // Verhindere das Schließen des Popups beim Klicken auf Checkboxen
    if (event->type() == QEvent::MouseButtonPress && object == view()->viewport()) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QModelIndex index = view()->indexAt(mouseEvent->pos());
        
        if (index.isValid()) {
            QStandardItem *item = m_model->itemFromIndex(index);
            if (item) {
                // Überspringe den Header-Eintrag (Index 0)
                if (index.row() == 0) {
                    return true; // Header nicht anklickbar
                }
                
                // Toggle den Checked-Status
                Qt::CheckState newState = (item->checkState() == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
                item->setCheckState(newState);
                
                updateDisplayText();
                emit checkedItemsChanged();
                
                return true; // Event wird nicht weitergeleitet, Popup bleibt offen
            }
        }
    }
    return QComboBox::eventFilter(object, event);
}

void CheckableComboBox::mousePressEvent(QMouseEvent *event)
{
    // Öffne das Popup, wenn es geschlossen ist
    if (!view()->isVisible()) {
        showPopup();
    }
}

void CheckableComboBox::wheelEvent(QWheelEvent *event)
{
    // Verhindere Scrollen, das die Auswahl ändern könnte
    event->ignore();
}
