#pragma once

#include "AppTypes.h"

#include <QColor>
#include <QWidget>

// 绘制最近鼠标轨迹的归一化预览。
class TrajectoryWidget : public QWidget
{
    Q_OBJECT

public:
    enum class DrawMode
    {
        Path = 0,
        Points = 1
    };

    enum class ColorMode
    {
        TimeRange = 0,
        Loop = 1
    };

    explicit TrajectoryWidget(QWidget* parent = nullptr);

    // 一次性提交轨迹点和渲染配置，避免多次 update()。
    void setRenderData(const QVector<TimedPoint>& points,
                       const QVector<QColor>& colors,
                       DrawMode drawMode,
                       ColorMode colorMode,
                       int loopColorMapPointCount,
                       bool hasCustomTimeRange,
                       qint64 startUs,
                       qint64 endUs);
    // 替换当前绘制点集并触发重绘。
    void setPoints(const QVector<TimedPoint>& points);
    // 设置轨迹的时间映射色表，颜色会按时间线性插值。
    void setColorTable(const QVector<QColor>& colors);
    void setDrawMode(DrawMode drawMode);
    void setColorMode(ColorMode colorMode);
    void setLoopColorMapPointCount(int pointCount);
    // 使用指定时间范围做颜色映射，便于聚焦某个时间窗口。
    void setTimeRange(qint64 startUs, qint64 endUs);
    void clearTimeRange();
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<TimedPoint> m_points;
    QVector<QColor> m_colorTable;
    DrawMode m_drawMode = DrawMode::Path;
    ColorMode m_colorMode = ColorMode::TimeRange;
    int m_loopColorMapPointCount = 32;
    bool m_hasCustomTimeRange = false;
    qint64 m_timeRangeStartUs = 0;
    qint64 m_timeRangeEndUs = 0;
};
