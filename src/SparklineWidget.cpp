#include "SparklineWidget.h"

#include <QPainter>

SparklineWidget::SparklineWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(160);
}

void SparklineWidget::setValues(const QVector<double>& values)
{
    m_values = values;
    update();
}

void SparklineWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(12, 19, 31));

    painter.setPen(QColor(37, 99, 235, 80));
    for (int i = 1; i < 4; ++i) {
        const int y = rect().top() + (rect().height() * i) / 4;
        painter.drawLine(rect().left() + 8, y, rect().right() - 8, y);
    }

    if (m_values.isEmpty()) {
        painter.setPen(QColor(203, 213, 225));
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("等待鼠标输入数据"));
        return;
    }

    double minValue = m_values.first();
    double maxValue = m_values.first();
    for (double value : m_values) {
        minValue = qMin(minValue, value);
        maxValue = qMax(maxValue, value);
    }
    if (qFuzzyCompare(minValue, maxValue)) {
        maxValue += 1.0;
    }

    QPainterPath path;
    for (int i = 0; i < m_values.size(); ++i) {
        const double x = 12.0 + (rect().width() - 24.0) * static_cast<double>(i) / qMax(1, m_values.size() - 1);
        const double ratio = (m_values.at(i) - minValue) / (maxValue - minValue);
        const double y = rect().bottom() - 12.0 - ratio * (rect().height() - 24.0);
        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }

    painter.setPen(QPen(QColor(14, 165, 233), 2.0));
    painter.drawPath(path);
}
