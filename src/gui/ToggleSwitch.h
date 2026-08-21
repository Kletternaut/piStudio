#pragma once
#include <QAbstractButton>
#include <QPainter>
#include <QMouseEvent>

// Simple toggle switch widget (QWidget-based, no QML needed)
// OFF = blue (#007acc), ON = red (#e74c3c) — same as Start/Stop button
class ToggleSwitch : public QAbstractButton
{
public:
    explicit ToggleSwitch(QWidget *parent = nullptr) : QAbstractButton(parent)
    {
        setCheckable(true);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    QSize sizeHint() const override { return QSize(42, 24); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const int w = width();
        const int h = height();
        const int r = h / 2;

        // Track
        QColor trackColor = isChecked() ? QColor("#e74c3c") : QColor("#007acc");
        p.setBrush(trackColor);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(0, 0, w, h, r, r);

        // Knob
        const int margin = 2;
        const int knobDiameter = h - 2 * margin;
        int knobX = isChecked() ? (w - knobDiameter - margin) : margin;
        p.setBrush(Qt::white);
        p.drawEllipse(knobX, margin, knobDiameter, knobDiameter);
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton) {
            setChecked(!isChecked());
            emit toggled(isChecked());
            update();
        }
    }

    void changeEvent(QEvent *e) override
    {
        QAbstractButton::changeEvent(e);
        update();
    }
};
