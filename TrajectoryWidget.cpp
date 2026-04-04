#include "TrajectoryWidget.h"

#include <QPainter>

TrajectoryWidget::TrajectoryWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(200);
}

void TrajectoryWidget::setPoints(const QVector<QPointF>& points)
{
    m_points = points;
    update();
}

void TrajectoryWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(15, 23, 42));

    if (m_points.size() < 2) {
        painter.setPen(QColor(203, 213, 225));
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("移动鼠标后显示轨迹预览"));
        return;
    }

    qreal minX = m_points.first().x();
    qreal maxX = minX;
    qreal minY = m_points.first().y();
    qreal maxY = minY;
    for (const QPointF& point : m_points) {
        minX = qMin(minX, point.x());
        maxX = qMax(maxX, point.x());
        minY = qMin(minY, point.y());
        maxY = qMax(maxY, point.y());
    }
    if (qFuzzyCompare(minX, maxX)) {
        maxX += 1.0;
    }
    if (qFuzzyCompare(minY, maxY)) {
        maxY += 1.0;
    }

    const QRectF frame = rect().adjusted(12, 12, -12, -12);
    QPainterPath path;
    for (int i = 0; i < m_points.size(); ++i) {
        const QPointF& raw = m_points.at(i);
        const qreal x = frame.left() + ((raw.x() - minX) / (maxX - minX)) * frame.width();
        const qreal y = frame.bottom() - ((raw.y() - minY) / (maxY - minY)) * frame.height();
        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }

    painter.setPen(QPen(QColor(34, 197, 94), 2.0));
    painter.drawPath(path);
}
