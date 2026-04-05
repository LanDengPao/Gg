#include "MetricsEngine.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <QtMath>
#include <algorithm>

namespace {
constexpr qint64 kOneSecondUs = 1000LL * 1000LL;
constexpr qint64 kFiveSecondsUs = 5LL * kOneSecondUs;
constexpr qint64 kThirtySecondsUs = 30LL * kOneSecondUs;
constexpr int kMaxHistoryPoints = 240;
constexpr int kMaxTrajectoryPoints = 320;
constexpr int kMaxRecentDeltas = 64;

// 限制实时图表与轨迹缓存规模，避免长时间监控时内存持续增长。
double clampScore(double value)
{
    return std::max(0.0, std::min(100.0, value));
}
}

MetricsEngine::MetricsEngine()
{
    m_snapshot.runStatus = SessionStatus::Monitoring;
    m_snapshot.uiActive = true;
}

void MetricsEngine::setSessionState(bool active, TestMode mode)
{
    m_sessionActive = active;
    m_snapshot.recording = active;
    m_snapshot.currentMode = mode;
    m_snapshot.runStatus = active ? SessionStatus::Recording : SessionStatus::Monitoring;
    if (active) {
        m_snapshot.recordingStartMs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
        resetSession();
    }
}

void MetricsEngine::setUiActive(bool active)
{
    m_snapshot.uiActive = active;
}

void MetricsEngine::updateDevice(const DeviceInfo& device)
{
    m_snapshot.device = device;
}

void MetricsEngine::resetSession()
{
    m_sessionStartUs = m_lastTimestampUs;
    m_sessionRawSampleCount = 0;
    m_sessionSampleCount = 0;
    m_sessionSpeedSum = 0.0;
    m_sessionPeakSpeed = 0.0;
    m_sessionTotalDistance = 0.0;
    m_sessionLeftClicks = 0;
    m_sessionRightClicks = 0;
    m_sessionMiddleClicks = 0;
    m_sessionWheelEvents = 0;
    m_sessionJitterAccum = 0.0;
    m_sessionJitterSamples = 0;
    m_sessionHz.clear();
}

void MetricsEngine::ingestSample(const MouseSample& sample)
{
    m_snapshot.position = sample.position;
    m_snapshot.delta = sample.delta;
    if (m_sessionActive) {
        ++m_sessionRawSampleCount;
    }

    qint64 dtUs = 0;
    if (m_hasLastSample) {
        dtUs = std::max<qint64>(0, sample.timestampUs - m_lastTimestampUs);
    } else {
        m_sessionStartUs = sample.timestampUs;
    }

    if (dtUs > 0) {
        // 由相邻采样的时间间隔换算瞬时轮询率。
        const double hz = 1000000.0 / static_cast<double>(dtUs);
        m_snapshot.currentHz = hz;
        m_recentHz.push_back({sample.timestampUs, hz});
        if (m_recentHz.size() > kMaxHistoryPoints * 4) {
            m_recentHz.remove(0, m_recentHz.size() - kMaxHistoryPoints * 4);
        }
        if (m_sessionActive) {
            m_sessionHz.push_back({sample.timestampUs, hz});
        }

        const double distance = qSqrt(static_cast<double>(sample.delta.x() * sample.delta.x() +
                                                           sample.delta.y() * sample.delta.y()));
        m_snapshot.currentSpeed = distance / (static_cast<double>(dtUs) / 1000000.0);
        if (m_sessionActive) {
            ++m_sessionSampleCount;
            m_sessionSpeedSum += m_snapshot.currentSpeed;
            m_sessionPeakSpeed = std::max(m_sessionPeakSpeed, m_snapshot.currentSpeed);
            m_sessionTotalDistance += distance;
        }
    }

    // 仅在发生位移时更新轨迹和抖动统计，避免静止数据污染预览。
    if (!sample.delta.isNull()) {
        m_recentTrajectory.push_back(QPointF(sample.position));
        if (m_recentTrajectory.size() > kMaxTrajectoryPoints) {
            m_recentTrajectory.remove(0, m_recentTrajectory.size() - kMaxTrajectoryPoints);
        }

        m_recentDeltas.push_back(sample.delta);
        if (m_recentDeltas.size() > kMaxRecentDeltas) {
            m_recentDeltas.remove(0, m_recentDeltas.size() - kMaxRecentDeltas);
        }

        if (m_recentDeltas.size() >= 2) {
            const QPoint& a = m_recentDeltas.at(m_recentDeltas.size() - 2);
            const QPoint& b = m_recentDeltas.at(m_recentDeltas.size() - 1);
            const double jitter = qSqrt(static_cast<double>((b.x() - a.x()) * (b.x() - a.x()) +
                                                            (b.y() - a.y()) * (b.y() - a.y())));
            if (m_sessionActive) {
                m_sessionJitterAccum += jitter;
                ++m_sessionJitterSamples;
            }
        }
    }

    if (sample.buttonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
        ++m_snapshot.leftClickCount;
        if (m_sessionActive) {
            ++m_sessionLeftClicks;
        }
    }
    if (sample.buttonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
        ++m_snapshot.rightClickCount;
        if (m_sessionActive) {
            ++m_sessionRightClicks;
        }
    }
    if (sample.buttonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) {
        ++m_snapshot.middleClickCount;
        if (m_sessionActive) {
            ++m_sessionMiddleClicks;
        }
    }
    if (sample.wheelDelta != 0) {
        ++m_snapshot.wheelEventCount;
        if (m_sessionActive) {
            ++m_sessionWheelEvents;
        }
    }

    m_lastTimestampUs = sample.timestampUs;
    m_lastPosition = sample.position;
    m_hasLastSample = true;

    pruneWindows(sample.timestampUs);
    updateDerivedStats();
}

LiveSnapshot MetricsEngine::snapshot() const
{
    return m_snapshot;
}

SessionSummary MetricsEngine::buildSessionSummary(const QString& sessionId,
                                                  TestMode mode,
                                                  const QDateTime& startUtc,
                                                  const QDateTime& endUtc,
                                                  const DeviceInfo& device) const
{
    SessionSummary summary;
    summary.sessionId = sessionId;
    summary.mode = mode;
    summary.status = SessionStatus::Completed;
    summary.startTimeUtc = startUtc;
    summary.endTimeUtc = endUtc;
    summary.durationMs = std::max<qint64>(0, startUtc.msecsTo(endUtc));
    summary.sampleCount = m_sessionRawSampleCount;
    summary.avgHz = averageForWindow(m_sessionHz, m_lastTimestampUs, std::numeric_limits<qint64>::max());
    summary.minHz = summary.avgHz;
    summary.maxHz = summary.avgHz;
    for (const HzPoint& point : m_sessionHz) {
        if (&point == &m_sessionHz.first()) {
            summary.minHz = point.hz;
            summary.maxHz = point.hz;
        } else {
            summary.minHz = std::min(summary.minHz, point.hz);
            summary.maxHz = std::max(summary.maxHz, point.hz);
        }
    }
    summary.stddevHz = stddev(m_sessionHz, summary.avgHz);
    summary.stabilityScore = stabilityScore(summary.avgHz, summary.stddevHz);
    summary.jitterValue = m_sessionJitterSamples > 0
        ? (m_sessionJitterAccum / static_cast<double>(m_sessionJitterSamples))
        : 0.0;
    summary.smoothnessScore = clampScore(100.0 - summary.jitterValue * 8.0);
    summary.peakSpeed = m_sessionPeakSpeed;
    summary.avgSpeed = m_sessionSampleCount > 0
        ? (m_sessionSpeedSum / static_cast<double>(m_sessionSampleCount))
        : 0.0;
    summary.totalDistance = m_sessionTotalDistance;
    summary.leftClickCount = m_sessionLeftClicks;
    summary.rightClickCount = m_sessionRightClicks;
    summary.middleClickCount = m_sessionMiddleClicks;
    summary.wheelEventCount = m_sessionWheelEvents;
    summary.device = device;
    return summary;
}

void MetricsEngine::pruneWindows(qint64 nowUs)
{
    // 实时统计仅保留最近 30 秒窗口，供仪表盘曲线和滚动指标使用。
    const qint64 cutoff = nowUs - kThirtySecondsUs;
    int removeCount = 0;
    while (removeCount < m_recentHz.size() && m_recentHz.at(removeCount).timestampUs < cutoff) {
        ++removeCount;
    }
    if (removeCount > 0) {
        m_recentHz.remove(0, removeCount);
    }

    QVector<double> values;
    values.reserve(std::min(kMaxHistoryPoints, m_recentHz.size()));
    const int startIndex = std::max(0, m_recentHz.size() - kMaxHistoryPoints);
    for (int i = startIndex; i < m_recentHz.size(); ++i) {
        values.push_back(m_recentHz.at(i).hz);
    }
    m_snapshot.pollingHistory = values;
    m_snapshot.trajectory = m_recentTrajectory;
}

double MetricsEngine::averageForWindow(const QVector<HzPoint>& points, qint64 nowUs, qint64 windowUs)
{
    if (points.isEmpty()) {
        return 0.0;
    }

    // 这里按采样间隔的倒数求平均，避免直接平均 Hz 对高频样本产生偏置。
    double inverseHzSum = 0.0;
    int count = 0;
    const qint64 cutoff = windowUs == std::numeric_limits<qint64>::max()
        ? std::numeric_limits<qint64>::min()
        : nowUs - windowUs;
    for (const HzPoint& point : points) {
        if (point.timestampUs >= cutoff && point.hz > 0.0) {
            inverseHzSum += 1.0 / point.hz;
            ++count;
        }
    }
    return (count > 0 && inverseHzSum > 0.0) ? (static_cast<double>(count) / inverseHzSum) : 0.0;
}

double MetricsEngine::stddev(const QVector<HzPoint>& points, double mean)
{
    if (points.isEmpty()) {
        return 0.0;
    }

    double accum = 0.0;
    for (const HzPoint& point : points) {
        const double diff = point.hz - mean;
        accum += diff * diff;
    }
    return qSqrt(accum / static_cast<double>(points.size()));
}

double MetricsEngine::stabilityScore(double mean, double sd)
{
    if (mean <= 0.0) {
        return 0.0;
    }
    return clampScore(100.0 - ((sd / mean) * 140.0));
}

void MetricsEngine::updateDerivedStats()
{
    // 从同一份最近窗口数据派生不同时间尺度下的统计指标。
    m_snapshot.avgHz1s = averageForWindow(m_recentHz, m_lastTimestampUs, kOneSecondUs);
    m_snapshot.avgHz5s = averageForWindow(m_recentHz, m_lastTimestampUs, kFiveSecondsUs);
    m_snapshot.avgHz30s = averageForWindow(m_recentHz, m_lastTimestampUs, kThirtySecondsUs);

    if (m_recentHz.isEmpty()) {
        m_snapshot.minHz = 0.0;
        m_snapshot.maxHz = 0.0;
        m_snapshot.stddevHz = 0.0;
        m_snapshot.stabilityScore = 0.0;
    } else {
        m_snapshot.minHz = m_recentHz.first().hz;
        m_snapshot.maxHz = m_recentHz.first().hz;
        for (const HzPoint& point : m_recentHz) {
            m_snapshot.minHz = std::min(m_snapshot.minHz, point.hz);
            m_snapshot.maxHz = std::max(m_snapshot.maxHz, point.hz);
        }
        m_snapshot.stddevHz = stddev(m_recentHz, m_snapshot.avgHz30s);
        m_snapshot.stabilityScore = stabilityScore(m_snapshot.avgHz30s, m_snapshot.stddevHz);
    }

    if (m_recentDeltas.size() >= 2) {
        double accum = 0.0;
        for (int i = 1; i < m_recentDeltas.size(); ++i) {
            const QPoint& a = m_recentDeltas.at(i - 1);
            const QPoint& b = m_recentDeltas.at(i);
            accum += qSqrt(static_cast<double>((b.x() - a.x()) * (b.x() - a.x()) +
                                               (b.y() - a.y()) * (b.y() - a.y())));
        }
        m_snapshot.jitterValue = accum / static_cast<double>(m_recentDeltas.size() - 1);
        m_snapshot.smoothnessScore = clampScore(100.0 - m_snapshot.jitterValue * 8.0);
    } else {
        m_snapshot.jitterValue = 0.0;
        m_snapshot.smoothnessScore = 0.0;
    }
}
