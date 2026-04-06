#include "TrajectoryWidget.h"

#include <QLinearGradient>
#include <QPainter>
#include <QString>
#include <QtMath>

namespace {
constexpr int kMinimumTrajectoryHeight = 200;
constexpr qreal kPanelRadius = 10.0;
constexpr qreal kPanelPadding = 12.0;
constexpr qreal kLegendAreaHeight = 28.0;
constexpr qreal kLegendBarHeight = 8.0;
constexpr qreal kPathWidth = 2.5;
constexpr qreal kPointRadius = 1.75;
constexpr qreal kStartMarkerRadius = 2.5;
constexpr qreal kMinimumSegmentLengthSquared = 1.0;

const QColor kPanelBackground(249, 251, 254);
const QColor kPanelBorder(216, 224, 235);
const QColor kTextColor(107, 114, 128);
const QColor kLegendBorder(216, 224, 235);
const QColor kMarkerOutline(255, 255, 255, 220);

QString formatTimeOffset(qint64 us)
{
    if (us >= 1000LL * 1000LL) {
        const double seconds = static_cast<double>(us) / (1000.0 * 1000.0);
        return QString::number(seconds, 'f', seconds >= 10.0 ? 0 : 1) + QStringLiteral(" s");
    }
    if (us >= 1000LL) {
        const double milliseconds = static_cast<double>(us) / 1000.0;
        return QString::number(milliseconds, 'f', milliseconds >= 10.0 ? 0 : 1) + QStringLiteral(" ms");
    }
    return QString::number(us) + QStringLiteral(" us");
}

qreal clampRatio(qreal value)
{
    return qBound<qreal>(0.0, value, 1.0);
}

QColor invertedColor(const QColor& color)
{
    return QColor(255 - color.red(),
                  255 - color.green(),
                  255 - color.blue(),
                  color.alpha());
}
}  // namespace

TrajectoryWidget::TrajectoryWidget(QWidget* parent)
    : QWidget(parent)
    , m_colorTable({QColor(37, 99, 235), QColor(16, 185, 129), QColor(245, 158, 11), QColor(239, 68, 68)})
{
    setMinimumHeight(kMinimumTrajectoryHeight);
}

void TrajectoryWidget::setRenderData(const QVector<TimedPoint>& points,
                                     const QVector<QColor>& colors,
                                     DrawMode drawMode,
                                     ColorMode colorMode,
                                     int loopColorMapPointCount,
                                     bool hasCustomTimeRange,
                                     qint64 startUs,
                                     qint64 endUs)
{
    m_points = points;
    if (!colors.isEmpty()) {
        m_colorTable = colors;
    }
    m_drawMode = drawMode;
    m_colorMode = colorMode;
    m_loopColorMapPointCount = qMax(1, loopColorMapPointCount);
    m_hasCustomTimeRange = hasCustomTimeRange && endUs > startUs;
    m_timeRangeStartUs = m_hasCustomTimeRange ? startUs : 0;
    m_timeRangeEndUs = m_hasCustomTimeRange ? endUs : 0;
    update();
}

void TrajectoryWidget::setPoints(const QVector<TimedPoint>& points)
{
    m_points = points;
    update();
}

void TrajectoryWidget::setColorTable(const QVector<QColor>& colors)
{
    if (colors.isEmpty()) {
        return;
    }
    m_colorTable = colors;
    update();
}

void TrajectoryWidget::setDrawMode(DrawMode drawMode)
{
    m_drawMode = drawMode;
    update();
}

void TrajectoryWidget::setColorMode(ColorMode colorMode)
{
    m_colorMode = colorMode;
    update();
}

void TrajectoryWidget::setLoopColorMapPointCount(int pointCount)
{
    m_loopColorMapPointCount = qMax(1, pointCount);
    update();
}

void TrajectoryWidget::setTimeRange(qint64 startUs, qint64 endUs)
{
    if (endUs <= startUs) {
        clearTimeRange();
        return;
    }
    m_hasCustomTimeRange = true;
    m_timeRangeStartUs = startUs;
    m_timeRangeEndUs = endUs;
    update();
}

void TrajectoryWidget::clearTimeRange()
{
    m_hasCustomTimeRange = false;
    m_timeRangeStartUs = 0;
    m_timeRangeEndUs = 0;
    update();
}

void TrajectoryWidget::clear()
{
    m_points.clear();
    update();
}

void TrajectoryWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF panel = rect().adjusted(1.0, 1.0, -1.0, -1.0);
    const QRectF legendBarRect(panel.left() + kPanelPadding,
                               panel.top() + kPanelPadding,
                               panel.width() - kPanelPadding * 2.0,
                               kLegendBarHeight);
    const QRectF frame = panel.adjusted(kPanelPadding,
                                        kPanelPadding + kLegendAreaHeight,
                                        -kPanelPadding,
                                        -kPanelPadding);

    painter.setPen(QPen(kPanelBorder, 1.0));
    painter.setBrush(kPanelBackground);
    painter.drawRoundedRect(panel, kPanelRadius, kPanelRadius);

    if (m_points.isEmpty()) {
        painter.setPen(kTextColor);
        painter.drawText(panel.toRect(), Qt::AlignCenter, tr("Move the mouse to preview the trajectory"));
        return;
    }
    if (frame.width() <= 0.0 || frame.height() <= 0.0 || legendBarRect.width() <= 0.0) {
        return;
    }

    qreal minX = m_points.first().position.x();
    qreal maxX = minX;
    qreal minY = m_points.first().position.y();
    qreal maxY = minY;
    qint64 dataStartUs = m_points.first().timestampUs;
    qint64 dataEndUs = dataStartUs;
    for (const TimedPoint& point : m_points) {
        minX = qMin(minX, point.position.x());
        maxX = qMax(maxX, point.position.x());
        minY = qMin(minY, point.position.y());
        maxY = qMax(maxY, point.position.y());
        dataStartUs = qMin(dataStartUs, point.timestampUs);
        dataEndUs = qMax(dataEndUs, point.timestampUs);
    }
    if (qFuzzyCompare(minX, maxX)) {
        maxX += 1.0;
    }
    if (qFuzzyCompare(minY, maxY)) {
        maxY += 1.0;
    }

    const qint64 timeRangeStartUs = m_hasCustomTimeRange ? m_timeRangeStartUs : dataStartUs;
    const qint64 timeRangeEndUs = (m_hasCustomTimeRange && m_timeRangeEndUs > m_timeRangeStartUs)
        ? m_timeRangeEndUs
        : qMax(dataStartUs + 1, dataEndUs);
    const qint64 timeSpanUs = qMax<qint64>(1, timeRangeEndUs - timeRangeStartUs);
    const int effectiveLoopPointCount = qMin(qMax(1, m_loopColorMapPointCount), m_points.size());

    QLinearGradient legendGradient(legendBarRect.topLeft(), legendBarRect.topRight());
    if (m_colorTable.size() == 1) {
        legendGradient.setColorAt(0.0, m_colorTable.first());
        legendGradient.setColorAt(1.0, m_colorTable.first());
    } else {
        for (int i = 0; i < m_colorTable.size(); ++i) {
            const qreal ratio = static_cast<qreal>(i) / static_cast<qreal>(m_colorTable.size() - 1);
            legendGradient.setColorAt(ratio, m_colorTable.at(i));
        }
    }
    painter.setPen(QPen(kLegendBorder, 1.0));
    painter.setBrush(legendGradient);
    painter.drawRoundedRect(legendBarRect, kLegendBarHeight / 2.0, kLegendBarHeight / 2.0);

    painter.setPen(kTextColor);
    const QRectF leftLabelRect(legendBarRect.left(),
                               legendBarRect.bottom() + 4.0,
                               legendBarRect.width() / 2.0,
                               kLegendAreaHeight - kLegendBarHeight);
    const QRectF rightLabelRect(legendBarRect.center().x(),
                                legendBarRect.bottom() + 4.0,
                                legendBarRect.width() / 2.0,
                                kLegendAreaHeight - kLegendBarHeight);
    if (m_colorMode == ColorMode::Loop) {
        painter.drawText(leftLabelRect, Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("1"));
        painter.drawText(rightLabelRect, Qt::AlignRight | Qt::AlignVCenter, QString::number(effectiveLoopPointCount));
    } else {
        painter.drawText(leftLabelRect,
                         Qt::AlignLeft | Qt::AlignVCenter,
                         formatTimeOffset(qMax<qint64>(0, timeRangeStartUs - dataStartUs)));
        painter.drawText(rightLabelRect,
                         Qt::AlignRight | Qt::AlignVCenter,
                         formatTimeOffset(qMax<qint64>(0, timeRangeEndUs - dataStartUs)));
    }

    auto interpolateColor = [this](qreal ratio) {
        const qreal clampedRatio = clampRatio(ratio);
        if (m_colorTable.size() <= 1) {
            return m_colorTable.isEmpty() ? QColor(22, 163, 74) : m_colorTable.first();
        }

        const qreal scaledIndex = clampedRatio * static_cast<qreal>(m_colorTable.size() - 1);
        const int leftIndex = static_cast<int>(qFloor(scaledIndex));
        const int rightIndex = qMin(leftIndex + 1, m_colorTable.size() - 1);
        const qreal mix = scaledIndex - static_cast<qreal>(leftIndex);

        const QColor left = m_colorTable.at(leftIndex);
        const QColor right = m_colorTable.at(rightIndex);
        return QColor::fromRgbF(left.redF() + (right.redF() - left.redF()) * mix,
                                left.greenF() + (right.greenF() - left.greenF()) * mix,
                                left.blueF() + (right.blueF() - left.blueF()) * mix,
                                left.alphaF() + (right.alphaF() - left.alphaF()) * mix);
    };

    auto colorForPoint = [this, timeRangeStartUs, timeSpanUs, effectiveLoopPointCount, &interpolateColor](int index, qint64 timestampUs) {
        if (m_colorMode == ColorMode::Loop) {
            if (effectiveLoopPointCount <= 1) {
                return interpolateColor(0.0);
            }
            const int loopIndex = index % effectiveLoopPointCount;
            const qreal ratio = static_cast<qreal>(loopIndex) / static_cast<qreal>(effectiveLoopPointCount - 1);
            return interpolateColor(ratio);
        }

        const qreal ratio = static_cast<qreal>(timestampUs - timeRangeStartUs)
            / static_cast<qreal>(timeSpanUs);
        return interpolateColor(ratio);
    };

    auto mapPoint = [&frame, minX, maxX, minY, maxY](const TimedPoint& point) {
        const qreal x = frame.left() + ((point.position.x() - minX) / (maxX - minX)) * frame.width();
        const qreal y = frame.top() + ((point.position.y() - minY) / (maxY - minY)) * frame.height();
        return QPointF(x, y);
    };

    painter.save();
    painter.setClipRect(frame.adjusted(-kPathWidth, -kPathWidth, kPathWidth, kPathWidth));
    if (m_drawMode == DrawMode::Points) {
        painter.setPen(Qt::NoPen);
        for (int i = 0; i < m_points.size(); ++i) {
            const TimedPoint& point = m_points.at(i);
            painter.setBrush(colorForPoint(i, point.timestampUs));
            painter.drawEllipse(mapPoint(point), kPointRadius, kPointRadius);
        }
    } else {
        for (int i = 1; i < m_points.size(); ++i) {
            const QPointF from = mapPoint(m_points.at(i - 1));
            const QPointF to = mapPoint(m_points.at(i));
            const qreal dx = to.x() - from.x();
            const qreal dy = to.y() - from.y();
            if ((dx * dx + dy * dy) < kMinimumSegmentLengthSquared) {
                continue;
            }
            painter.setPen(QPen(colorForPoint(i, m_points.at(i).timestampUs),
                                kPathWidth,
                                Qt::SolidLine,
                                Qt::FlatCap,
                                Qt::RoundJoin));
            painter.drawLine(from, to);
        }
    }
    painter.restore();

    const QPointF startPoint = mapPoint(m_points.first());
    painter.setPen(QPen(kMarkerOutline, 1.0));
    painter.setBrush(invertedColor(colorForPoint(0, m_points.first().timestampUs)));
    painter.drawEllipse(startPoint, kStartMarkerRadius, kStartMarkerRadius);
}
