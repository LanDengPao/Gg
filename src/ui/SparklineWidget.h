#pragma once

#include "AppTypes.h"

#include <QWidget>

// 绘制最近轮询率数据的紧凑折线图。
class SparklineWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SparklineWidget(QWidget* parent = nullptr);

    // 替换当前绘制序列并触发重绘。
    void setValues(const QVector<TimedValue>& values);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<TimedValue> m_values;
};
