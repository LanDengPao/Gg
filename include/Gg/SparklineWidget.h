#pragma once

#include <QWidget>

class SparklineWidget : public QWidget
{
public:
    explicit SparklineWidget(QWidget* parent = nullptr);

    void setValues(const QVector<double>& values);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<double> m_values;
};
