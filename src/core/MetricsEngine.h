#pragma once

#include "AppTypes.h"

#include <QVector>

#include <deque>

// 汇总实时鼠标输入，生成仪表盘指标和可持久化的测试汇总。
class MetricsEngine
{
public:
    MetricsEngine();

    // 按指定测试模式开始或结束一次录制会话。
    void setSessionState(bool active, TestMode mode);
    // 记录主窗口是否处于激活状态，用于反映后台录制状态。
    void setUiActive(bool active);
    // 更新实时快照中展示的最新设备信息。
    void updateDevice(const DeviceInfo& device);
    // 将一条原始鼠标采样纳入滚动指标和会话统计。
    void ingestSample(const MouseSample& sample);
    // 清空当前会话的累计统计，但保留实时监控状态。
    void resetSession();

    // 返回仪表盘使用的最新实时快照。
    LiveSnapshot snapshot() const;
    // 基于当前累计的录制状态生成一份已完成的会话汇总。
    SessionSummary buildSessionSummary(const QString& sessionId,
                                       TestMode mode,
                                       const QDateTime& startUtc,
                                       const QDateTime& endUtc,
                                       const DeviceInfo& device) const;

private:
    struct HzPoint
    {
        qint64 timestampUs = 0;
        double hz = 0.0;
    };

    LiveSnapshot m_snapshot;
    QVector<HzPoint> m_recentHz;
    QVector<HzPoint> m_sessionHz;
    std::deque<TimedPoint> m_recentTrajectory;
    QVector<QPoint> m_recentDeltas;
    qint64 m_lastTimestampUs = 0;
    QPoint m_lastPosition;
    bool m_hasLastSample = false;

    qint64 m_sessionStartUs = 0;
    bool m_sessionActive = false;
    qint64 m_sessionRawSampleCount = 0;
    qint64 m_sessionSampleCount = 0;
    double m_sessionSpeedSum = 0.0;
    double m_sessionPeakSpeed = 0.0;
    double m_sessionTotalDistance = 0.0;
    quint64 m_sessionLeftClicks = 0;
    quint64 m_sessionRightClicks = 0;
    quint64 m_sessionMiddleClicks = 0;
    quint64 m_sessionWheelEvents = 0;
    double m_sessionJitterAccum = 0.0;
    int m_sessionJitterSamples = 0;

    void pruneWindows(qint64 nowUs);
    static double averageForWindow(const QVector<HzPoint>& points, qint64 nowUs, qint64 windowUs);
    static double stddev(const QVector<HzPoint>& points, double mean);
    static double stabilityScore(double mean, double sd);
    void updateDerivedStats();
};
