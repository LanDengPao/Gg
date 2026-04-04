#pragma once

#include <QWidget>

class TrajectoryWidget : public QWidget
{
public:
    explicit TrajectoryWidget(QWidget* parent = nullptr);

    void setPoints(const QVector<QPointF>& points);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<QPointF> m_points;
};
