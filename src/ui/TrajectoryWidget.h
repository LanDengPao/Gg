#pragma once

#include <QWidget>

// 绘制最近鼠标轨迹的归一化预览。
class TrajectoryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrajectoryWidget(QWidget* parent = nullptr);

    // 替换当前绘制点集并触发重绘。
    void setPoints(const QVector<QPointF>& points);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<QPointF> m_points;
};
