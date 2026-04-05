#include "TrajectoryWidget.h"

#include <QPainter>

namespace {
constexpr int kMinimumTrajectoryHeight = 200;
constexpr qreal kPanelRadius = 10.0;
constexpr qreal kPanelPadding = 12.0;

const QColor kPanelBackground(249, 251, 254);
const QColor kPanelBorder(216, 224, 235);
const QColor kTextColor(107, 114, 128);
const QColor kPathColor(22, 163, 74);
}  // namespace

TrajectoryWidget::TrajectoryWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(kMinimumTrajectoryHeight);
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

    const QRectF panel = rect().adjusted(1.0, 1.0, -1.0, -1.0);
    const QRectF frame = panel.adjusted(kPanelPadding, kPanelPadding, -kPanelPadding, -kPanelPadding);

    painter.setPen(QPen(kPanelBorder, 1.0));
    painter.setBrush(kPanelBackground);
    painter.drawRoundedRect(panel, kPanelRadius, kPanelRadius);

    if (m_points.size() < 2) {
        painter.setPen(kTextColor);
        painter.drawText(panel.toRect(), Qt::AlignCenter, tr("Move the mouse to preview the trajectory"));
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

    QPainterPath path;
    for (int i = 0; i < m_points.size(); ++i) {
        const QPointF& raw = m_points.at(i);
        const qreal x = frame.left() + ((raw.x() - minX) / (maxX - minX)) * frame.width();
        const qreal y = frame.top() + ((raw.y() - minY) / (maxY - minY)) * frame.height();
        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }

    painter.setPen(QPen(kPathColor, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path);
}
