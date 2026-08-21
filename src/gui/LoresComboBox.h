#ifndef LORESCOMBOBOX_H
#define LORESCOMBOBOX_H

#include <QComboBox>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>

class LoresComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit LoresComboBox(QWidget *parent = nullptr);
    
    // Public interface methods
    bool isLoresEnabled() const;
    bool isParEnabled() const;
    int getWidth() const;
    int getHeight() const;
    
    void setLoresEnabled(bool enabled);
    void setParEnabled(bool enabled);
    void setWidth(int width);
    void setHeight(int height);
    void setMainResolution(const QString &resolution); // To read aspect ratio from main resolution
    void reset();

protected:
    void showPopup() override;
    void hidePopup() override;

signals:
    void loresConfigChanged();
    void parModeChanged(bool enabled);
    void dimensionsChanged(int width, int height);

private slots:
    void onParChanged(bool enabled);
    void onWidthChanged();
    void onHeightChanged();
    void updateDisplayText();
    void connectToResolutionSelector();
    void onResolutionChanged(const QString &resolution);

private:
    // UI elements for the popup
    QWidget *m_popupWidget;
    QCheckBox *m_parCheckbox;
    QLineEdit *m_widthInput;
    QLineEdit *m_heightInput;
    
    // Internal state
    bool m_loresEnabled;
    bool m_parEnabled;
    int m_width;
    int m_height;
    
    void setupInlineWidget();
    void calculateWidth(int height);
    void calculateHeight(int width);
    double getCurrentAspectRatio();
    QString getCurrentResolution();
};

#endif // LORESCOMBOBOX_H
