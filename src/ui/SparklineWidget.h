#pragma once

#include <QWidget>

class SparklineWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SparklineWidget(QWidget* parent = nullptr);

    void setValues(const QVector<double>& values);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<double> m_values;
};
