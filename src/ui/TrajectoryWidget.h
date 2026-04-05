#pragma once

#include <QWidget>

class TrajectoryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrajectoryWidget(QWidget* parent = nullptr);

    void setPoints(const QVector<QPointF>& points);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<QPointF> m_points;
};
