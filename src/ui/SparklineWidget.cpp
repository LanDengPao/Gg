#include "SparklineWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QString>

namespace {
constexpr int kMinimumChartHeight = 160;
constexpr qreal kPanelRadius = 10.0;
constexpr qreal kPanelPadding = 12.0;
constexpr qreal kPlotLeftInset = 44.0;
constexpr qreal kPlotTopInset = 12.0;
constexpr qreal kPlotRightInset = 14.0;
constexpr qreal kPlotBottomInset = 28.0;
constexpr qreal kMarkerRadius = 3.5;

const QColor kPanelBackground(249, 251, 254);
const QColor kPanelBorder(216, 224, 235);
const QColor kGridColor(210, 219, 231);
const QColor kTextColor(107, 114, 128);
const QColor kLineColor(11, 87, 208);
const QColor kAxisColor(164, 174, 189);

QString formatYAxisValue(double value)
{
    return QString::number(value, 'f', value >= 100.0 ? 0 : 1);
}

QString formatElapsed(qint64 us)
{
    if (us <= 0) {
        return QStringLiteral("0");
    }

    const double seconds = static_cast<double>(us) / (1000.0 * 1000.0);
    return QString::number(seconds, 'f', seconds >= 10.0 ? 0 : 1);
}
}  // namespace

SparklineWidget::SparklineWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(kMinimumChartHeight);
}

void SparklineWidget::setValues(const QVector<TimedValue>& values)
{
    m_values = values;
    update();
}

void SparklineWidget::clear()
{
    m_values.clear();
    update();
}

void SparklineWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF panel = rect().adjusted(1.0, 1.0, -1.0, -1.0);
    const QRectF plot = panel.adjusted(kPanelPadding + kPlotLeftInset,
                                       kPanelPadding + kPlotTopInset,
                                       -(kPanelPadding + kPlotRightInset),
                                       -(kPanelPadding + kPlotBottomInset));

    painter.setPen(QPen(kPanelBorder, 1.0));
    painter.setBrush(kPanelBackground);
    painter.drawRoundedRect(panel, kPanelRadius, kPanelRadius);

    if (m_values.isEmpty()) {
        painter.setPen(kTextColor);
        painter.drawText(panel.toRect(), Qt::AlignCenter, tr("Waiting for mouse input data"));
        return;
    }
    if (plot.width() <= 0.0 || plot.height() <= 0.0) {
        return;
    }

    double minValue = m_values.first().value;
    double maxValue = minValue;
    qint64 minTimeUs = m_values.first().timestampUs;
    qint64 maxTimeUs = minTimeUs;
    for (const TimedValue& value : m_values) {
        minValue = qMin(minValue, value.value);
        maxValue = qMax(maxValue, value.value);
        minTimeUs = qMin(minTimeUs, value.timestampUs);
        maxTimeUs = qMax(maxTimeUs, value.timestampUs);
    }
    if (qFuzzyCompare(minValue, maxValue)) {
        maxValue += 1.0;
    }
    if (maxTimeUs <= minTimeUs) {
        maxTimeUs = minTimeUs + 1;
    }

    painter.setPen(QPen(kAxisColor, 1.0));
    painter.drawLine(plot.bottomLeft(), plot.topLeft());
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());

    painter.setPen(kTextColor);
    painter.drawText(QRectF(panel.left() + kPanelPadding,
                            panel.top() + kPanelPadding,
                            kPlotLeftInset,
                            16.0),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("Hz"));
    painter.drawText(QRectF(plot.left(),
                            plot.bottom() + 8.0,
                            plot.width(),
                            16.0),
                     Qt::AlignCenter,
                     QStringLiteral("t (s)"));

    for (int tick = 0; tick < 3; ++tick) {
        const qreal ratio = static_cast<qreal>(tick) / 2.0;
        const qreal y = plot.top() + ratio * plot.height();
        const double labelValue = maxValue - (maxValue - minValue) * ratio;
        painter.setPen(QPen(kGridColor, 1.0));
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        painter.setPen(kTextColor);
        painter.drawText(QRectF(panel.left() + kPanelPadding,
                                y - 10.0,
                                kPlotLeftInset - 8.0,
                                20.0),
                         Qt::AlignRight | Qt::AlignVCenter,
                         formatYAxisValue(labelValue));
    }

    for (int tick = 0; tick < 3; ++tick) {
        const qreal ratio = static_cast<qreal>(tick) / 2.0;
        const qreal x = plot.left() + ratio * plot.width();
        const qint64 labelTimeUs = minTimeUs + static_cast<qint64>((maxTimeUs - minTimeUs) * ratio);
        painter.setPen(QPen(kGridColor, 1.0));
        painter.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
        painter.setPen(kTextColor);
        painter.drawText(QRectF(x - 28.0,
                                plot.bottom() + 6.0,
                                56.0,
                                16.0),
                         Qt::AlignCenter,
                         formatElapsed(labelTimeUs - minTimeUs));
    }

    auto mapPoint = [&plot, minValue, maxValue, minTimeUs, maxTimeUs](const TimedValue& value) {
        const qreal x = plot.left() + (static_cast<qreal>(value.timestampUs - minTimeUs)
                                       / static_cast<qreal>(maxTimeUs - minTimeUs))
            * plot.width();
        const qreal y = plot.bottom()
            - (static_cast<qreal>(value.value - minValue) / static_cast<qreal>(maxValue - minValue)) * plot.height();
        return QPointF(x, y);
    };

    QPainterPath path;
    path.moveTo(mapPoint(m_values.first()));
    for (int i = 1; i < m_values.size(); ++i) {
        path.lineTo(mapPoint(m_values.at(i)));
    }

    painter.save();
    painter.setClipRect(plot.adjusted(-2.0, -2.0, 2.0, 2.0));
    painter.setPen(QPen(kLineColor, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path);

    const QPointF latestPoint = mapPoint(m_values.last());
    painter.setPen(QPen(kPanelBackground, 2.0));
    painter.setBrush(kLineColor);
    painter.drawEllipse(latestPoint, kMarkerRadius, kMarkerRadius);
    painter.restore();
}
