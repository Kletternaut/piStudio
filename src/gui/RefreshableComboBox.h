#ifndef REFRESHABLECOMBOBOX_H
#define REFRESHABLECOMBOBOX_H

#include <QComboBox>
#include <functional>

class RefreshableComboBox : public QComboBox {
    Q_OBJECT

public:
    explicit RefreshableComboBox(QWidget *parent = nullptr)
        : QComboBox(parent) {}

    void setRefreshCallback(std::function<void()> callback) {
        m_refreshCallback = callback;
    }

protected:
    void showPopup() override {
        if (m_refreshCallback) {
            m_refreshCallback();
        }
        QComboBox::showPopup();
    }

private:
    std::function<void()> m_refreshCallback;
};

#endif // REFRESHABLECOMBOBOX_H