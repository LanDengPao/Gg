#include "SparklineWidget.h"

#include <QPainter>

namespace {
constexpr int kMinimumChartHeight = 160;
constexpr qreal kPanelRadius = 10.0;
constexpr qreal kPanelPadding = 12.0;

const QColor kPanelBackground(249, 251, 254);
const QColor kPanelBorder(216, 224, 235);
const QColor kGridColor(210, 219, 231);
const QColor kTextColor(107, 114, 128);
const QColor kLineColor(11, 87, 208);
}  // namespace

SparklineWidget::SparklineWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(kMinimumChartHeight);
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

    const QRectF panel = rect().adjusted(1.0, 1.0, -1.0, -1.0);
    const QRectF content = panel.adjusted(kPanelPadding, kPanelPadding, -kPanelPadding, -kPanelPadding);

    painter.setPen(QPen(kPanelBorder, 1.0));
    painter.setBrush(kPanelBackground);
    painter.drawRoundedRect(panel, kPanelRadius, kPanelRadius);

    painter.setPen(QPen(kGridColor, 1.0));
    for (int i = 1; i < 4; ++i) {
        const qreal y = content.top() + (content.height() * i) / 4.0;
        painter.drawLine(QPointF(content.left(), y), QPointF(content.right(), y));
    }

    if (m_values.isEmpty()) {
        painter.setPen(kTextColor);
        painter.drawText(panel.toRect(), Qt::AlignCenter, tr("Waiting for mouse input data"));
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
        const double x = content.left() + content.width() * static_cast<double>(i) / qMax(1, m_values.size() - 1);
        const double ratio = (m_values.at(i) - minValue) / (maxValue - minValue);
        const double y = content.bottom() - ratio * content.height();
        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }

    painter.setPen(QPen(kLineColor, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path);
}
