#pragma once

#include "AppTypes.h"

#include <QQueue>
#include <QVector>

class MetricsEngine
{
public:
    MetricsEngine();

    void setSessionState(bool active, TestMode mode);
    void setUiActive(bool active);
    void updateDevice(const DeviceInfo& device);
    void ingestSample(const MouseSample& sample);
    void resetSession();

    LiveSnapshot snapshot() const;
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
    QVector<QPointF> m_recentTrajectory;
    QVector<QPoint> m_recentDeltas;
    qint64 m_lastTimestampUs = 0;
    QPoint m_lastPosition;
    bool m_hasLastSample = false;

    qint64 m_sessionStartUs = 0;
    bool m_sessionActive = false;
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
