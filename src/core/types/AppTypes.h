#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QPoint>
#include <QString>
#include <QtGlobal>
#include <QVector>

enum class TestMode
{
    PollingRate,
    TrajectoryStability
};

enum class UiLanguage
{
    English,
    Chinese
};

enum class SessionStatus
{
    Idle,
    Monitoring,
    Recording,
    Completed,
    Error
};

struct DeviceInfo
{
    QString deviceId;
    QString displayName;
    QString vendorId = "N/A";
    QString productId = "N/A";
    QString manufacturer = "N/A";
    QString productName = "N/A";
    QString firmwareVersion = "N/A";
    QString connectionType = "N/A";
    QString batteryLevel = "N/A";
    QString rawDeviceName = "N/A";
    bool connected = false;
    QDateTime lastSeenUtc;
};

struct MouseSample
{
    qint64 timestampUs = 0;
    QPoint position;
    QPoint delta;
    int wheelDelta = 0;
    quint16 buttonFlags = 0;
    quint8 eventType = 0;
};

struct SessionSummary
{
    QString sessionId;
    TestMode mode = TestMode::PollingRate;
    SessionStatus status = SessionStatus::Idle;
    QDateTime startTimeUtc;
    QDateTime endTimeUtc;
    qint64 durationMs = 0;
    qint64 sampleCount = 0;
    double avgHz = 0.0;
    double minHz = 0.0;
    double maxHz = 0.0;
    double stddevHz = 0.0;
    double stabilityScore = 0.0;
    double jitterValue = 0.0;
    double smoothnessScore = 0.0;
    double peakSpeed = 0.0;
    double avgSpeed = 0.0;
    double totalDistance = 0.0;
    quint64 leftClickCount = 0;
    quint64 rightClickCount = 0;
    quint64 middleClickCount = 0;
    quint64 wheelEventCount = 0;
    DeviceInfo device;
};

struct SessionRecord
{
    QString sessionId;
    QString sessionDir;
    TestMode mode = TestMode::PollingRate;
    SessionStatus status = SessionStatus::Idle;
    QDateTime startTimeUtc;
    QDateTime endTimeUtc;
    qint64 durationMs = 0;
    qint64 sampleCount = 0;
    SessionSummary summary;
};

struct LiveSnapshot
{
    SessionStatus runStatus = SessionStatus::Monitoring;
    TestMode currentMode = TestMode::PollingRate;
    bool recording = false;
    bool uiActive = true;
    QString activeSessionId;
    qint64 recordingStartMs = 0;
    DeviceInfo device;
    QPoint position;
    QPoint delta;
    double currentHz = 0.0;
    double avgHz1s = 0.0;
    double avgHz5s = 0.0;
    double avgHz30s = 0.0;
    double minHz = 0.0;
    double maxHz = 0.0;
    double stddevHz = 0.0;
    double stabilityScore = 0.0;
    double currentSpeed = 0.0;
    double jitterValue = 0.0;
    double smoothnessScore = 0.0;
    quint64 leftClickCount = 0;
    quint64 rightClickCount = 0;
    quint64 middleClickCount = 0;
    quint64 wheelEventCount = 0;
    QVector<double> pollingHistory;
    QVector<QPointF> trajectory;
};

inline UiLanguage& appUiLanguageStorage()
{
    static UiLanguage language = UiLanguage::English;
    return language;
}

inline UiLanguage appUiLanguage()
{
    return appUiLanguageStorage();
}

inline void setAppUiLanguage(UiLanguage language)
{
    appUiLanguageStorage() = language;
}

inline QString uiText(const char* english, const char* chinese)
{
    return appUiLanguage() == UiLanguage::Chinese ? QString::fromUtf8(chinese)
                                                  : QString::fromUtf8(english);
}

inline QString testModeKey(TestMode mode)
{
    return mode == TestMode::PollingRate ? QStringLiteral("polling_rate")
                                         : QStringLiteral("trajectory_stability");
}

inline QString sessionStatusKey(SessionStatus status)
{
    switch (status) {
    case SessionStatus::Idle:
        return QStringLiteral("idle");
    case SessionStatus::Monitoring:
        return QStringLiteral("monitoring");
    case SessionStatus::Recording:
        return QStringLiteral("recording");
    case SessionStatus::Completed:
        return QStringLiteral("completed");
    case SessionStatus::Error:
        return QStringLiteral("error");
    }
    return QStringLiteral("unknown");
}

inline TestMode testModeFromStoredString(const QString& value)
{
    if (value == QStringLiteral("trajectory_stability")
        || value.contains(QStringLiteral("轨迹"))
        || value.compare(QStringLiteral("Trajectory Stability"), Qt::CaseInsensitive) == 0) {
        return TestMode::TrajectoryStability;
    }
    return TestMode::PollingRate;
}

inline SessionStatus sessionStatusFromStoredString(const QString& value)
{
    if (value == QStringLiteral("idle")
        || value == QStringLiteral("空闲")
        || value.compare(QStringLiteral("Idle"), Qt::CaseInsensitive) == 0) {
        return SessionStatus::Idle;
    }
    if (value == QStringLiteral("monitoring")
        || value == QStringLiteral("实时监控")
        || value.compare(QStringLiteral("Monitoring"), Qt::CaseInsensitive) == 0) {
        return SessionStatus::Monitoring;
    }
    if (value == QStringLiteral("recording")
        || value == QStringLiteral("测试录制中")
        || value.compare(QStringLiteral("Recording"), Qt::CaseInsensitive) == 0) {
        return SessionStatus::Recording;
    }
    if (value == QStringLiteral("completed")
        || value == QStringLiteral("已完成")
        || value.compare(QStringLiteral("Completed"), Qt::CaseInsensitive) == 0) {
        return SessionStatus::Completed;
    }
    if (value == QStringLiteral("error")
        || value == QStringLiteral("异常")
        || value.compare(QStringLiteral("Error"), Qt::CaseInsensitive) == 0) {
        return SessionStatus::Error;
    }
    return SessionStatus::Monitoring;
}

inline QString testModeToDisplayString(TestMode mode, UiLanguage language = appUiLanguage())
{
    if (language == UiLanguage::Chinese) {
        return mode == TestMode::PollingRate ? QStringLiteral("轮询率测试")
                                             : QStringLiteral("轨迹/稳定性测试");
    }
    return mode == TestMode::PollingRate ? QStringLiteral("Polling Rate")
                                         : QStringLiteral("Trajectory Stability");
}

inline QString sessionStatusToDisplayString(SessionStatus status, UiLanguage language = appUiLanguage())
{
    if (language == UiLanguage::Chinese) {
        switch (status) {
        case SessionStatus::Idle:
            return QStringLiteral("空闲");
        case SessionStatus::Monitoring:
            return QStringLiteral("实时监控");
        case SessionStatus::Recording:
            return QStringLiteral("测试录制中");
        case SessionStatus::Completed:
            return QStringLiteral("已完成");
        case SessionStatus::Error:
            return QStringLiteral("异常");
        }
        return QStringLiteral("未知");
    }

    switch (status) {
    case SessionStatus::Idle:
        return QStringLiteral("Idle");
    case SessionStatus::Monitoring:
        return QStringLiteral("Monitoring");
    case SessionStatus::Recording:
        return QStringLiteral("Recording");
    case SessionStatus::Completed:
        return QStringLiteral("Completed");
    case SessionStatus::Error:
        return QStringLiteral("Error");
    }
    return QStringLiteral("Unknown");
}

inline QString scoreLevelText(double score, UiLanguage language = appUiLanguage())
{
    if (language == UiLanguage::Chinese) {
        if (score >= 90.0) {
            return QStringLiteral("优秀");
        }
        if (score >= 75.0) {
            return QStringLiteral("良好");
        }
        if (score >= 60.0) {
            return QStringLiteral("一般");
        }
        return QStringLiteral("较差");
    }

    if (score >= 90.0) {
        return QStringLiteral("Excellent");
    }
    if (score >= 75.0) {
        return QStringLiteral("Good");
    }
    if (score >= 60.0) {
        return QStringLiteral("Fair");
    }
    return QStringLiteral("Poor");
}

inline QJsonObject toJson(const DeviceInfo& device)
{
    QJsonObject obj;
    obj["device_id"] = device.deviceId;
    obj["display_name"] = device.displayName;
    obj["vendor_id"] = device.vendorId;
    obj["product_id"] = device.productId;
    obj["manufacturer"] = device.manufacturer;
    obj["product_name"] = device.productName;
    obj["firmware_version"] = device.firmwareVersion;
    obj["connection_type"] = device.connectionType;
    obj["battery_level"] = device.batteryLevel;
    obj["raw_device_name"] = device.rawDeviceName;
    obj["connected"] = device.connected;
    obj["last_seen_utc"] = device.lastSeenUtc.toString(Qt::ISODate);
    return obj;
}

inline DeviceInfo deviceInfoFromJson(const QJsonObject& obj)
{
    DeviceInfo device;
    device.deviceId = obj["device_id"].toString();
    device.displayName = obj["display_name"].toString();
    device.vendorId = obj["vendor_id"].toString("N/A");
    device.productId = obj["product_id"].toString("N/A");
    device.manufacturer = obj["manufacturer"].toString("N/A");
    device.productName = obj["product_name"].toString("N/A");
    device.firmwareVersion = obj["firmware_version"].toString("N/A");
    device.connectionType = obj["connection_type"].toString("N/A");
    device.batteryLevel = obj["battery_level"].toString("N/A");
    device.rawDeviceName = obj["raw_device_name"].toString("N/A");
    device.connected = obj["connected"].toBool(false);
    device.lastSeenUtc = QDateTime::fromString(obj["last_seen_utc"].toString(), Qt::ISODate);
    return device;
}

inline QJsonObject toJson(const SessionRecord& record)
{
    QJsonObject obj;
    obj["session_id"] = record.sessionId;
    obj["mode"] = testModeKey(record.mode);
    obj["status"] = sessionStatusKey(record.status);
    obj["start_time_utc"] = record.startTimeUtc.toString(Qt::ISODate);
    obj["end_time_utc"] = record.endTimeUtc.toString(Qt::ISODate);
    obj["duration_ms"] = static_cast<qint64>(record.durationMs);
    obj["sample_count"] = static_cast<qint64>(record.sampleCount);
    obj["session_dir"] = record.sessionDir;
    return obj;
}

inline SessionRecord sessionRecordFromJson(const QJsonObject& obj)
{
    SessionRecord record;
    record.sessionId = obj["session_id"].toString();
    record.mode = testModeFromStoredString(obj["mode"].toString());
    record.status = sessionStatusFromStoredString(obj["status"].toString());
    record.startTimeUtc = QDateTime::fromString(obj["start_time_utc"].toString(), Qt::ISODate);
    record.endTimeUtc = QDateTime::fromString(obj["end_time_utc"].toString(), Qt::ISODate);
    record.durationMs = static_cast<qint64>(obj["duration_ms"].toDouble());
    record.sampleCount = static_cast<qint64>(obj["sample_count"].toDouble());
    record.sessionDir = obj["session_dir"].toString();
    return record;
}

inline QJsonObject toJson(const SessionSummary& summary)
{
    QJsonObject obj;
    obj["session_id"] = summary.sessionId;
    obj["mode"] = testModeKey(summary.mode);
    obj["status"] = sessionStatusKey(summary.status);
    obj["start_time_utc"] = summary.startTimeUtc.toString(Qt::ISODate);
    obj["end_time_utc"] = summary.endTimeUtc.toString(Qt::ISODate);
    obj["duration_ms"] = static_cast<qint64>(summary.durationMs);
    obj["sample_count"] = static_cast<qint64>(summary.sampleCount);
    obj["avg_hz"] = summary.avgHz;
    obj["min_hz"] = summary.minHz;
    obj["max_hz"] = summary.maxHz;
    obj["stddev_hz"] = summary.stddevHz;
    obj["stability_score"] = summary.stabilityScore;
    obj["jitter_value"] = summary.jitterValue;
    obj["smoothness_score"] = summary.smoothnessScore;
    obj["peak_speed"] = summary.peakSpeed;
    obj["avg_speed"] = summary.avgSpeed;
    obj["total_distance"] = summary.totalDistance;
    obj["left_click_count"] = static_cast<qint64>(summary.leftClickCount);
    obj["right_click_count"] = static_cast<qint64>(summary.rightClickCount);
    obj["middle_click_count"] = static_cast<qint64>(summary.middleClickCount);
    obj["wheel_event_count"] = static_cast<qint64>(summary.wheelEventCount);
    obj["device"] = toJson(summary.device);
    return obj;
}

inline SessionSummary sessionSummaryFromJson(const QJsonObject& obj)
{
    SessionSummary summary;
    summary.sessionId = obj["session_id"].toString();
    summary.mode = testModeFromStoredString(obj["mode"].toString());
    summary.status = sessionStatusFromStoredString(obj["status"].toString());
    summary.startTimeUtc = QDateTime::fromString(obj["start_time_utc"].toString(), Qt::ISODate);
    summary.endTimeUtc = QDateTime::fromString(obj["end_time_utc"].toString(), Qt::ISODate);
    summary.durationMs = static_cast<qint64>(obj["duration_ms"].toDouble());
    summary.sampleCount = static_cast<qint64>(obj["sample_count"].toDouble());
    summary.avgHz = obj["avg_hz"].toDouble();
    summary.minHz = obj["min_hz"].toDouble();
    summary.maxHz = obj["max_hz"].toDouble();
    summary.stddevHz = obj["stddev_hz"].toDouble();
    summary.stabilityScore = obj["stability_score"].toDouble();
    summary.jitterValue = obj["jitter_value"].toDouble();
    summary.smoothnessScore = obj["smoothness_score"].toDouble();
    summary.peakSpeed = obj["peak_speed"].toDouble();
    summary.avgSpeed = obj["avg_speed"].toDouble();
    summary.totalDistance = obj["total_distance"].toDouble();
    summary.leftClickCount = static_cast<quint64>(obj["left_click_count"].toDouble());
    summary.rightClickCount = static_cast<quint64>(obj["right_click_count"].toDouble());
    summary.middleClickCount = static_cast<quint64>(obj["middle_click_count"].toDouble());
    summary.wheelEventCount = static_cast<quint64>(obj["wheel_event_count"].toDouble());
    summary.device = deviceInfoFromJson(obj["device"].toObject());
    return summary;
}

inline MouseSample mouseSampleFromJson(const QJsonObject& obj)
{
    MouseSample sample;
    sample.timestampUs = static_cast<qint64>(obj["timestamp_us"].toDouble());
    sample.position = QPoint(obj["x"].toInt(), obj["y"].toInt());
    sample.delta = QPoint(obj["dx"].toInt(), obj["dy"].toInt());
    sample.wheelDelta = obj["wheel_delta"].toInt();
    sample.buttonFlags = static_cast<quint16>(obj["button_flags"].toInt());
    sample.eventType = static_cast<quint8>(obj["event_type"].toInt());
    return sample;
}

inline QJsonObject toJson(const MouseSample& sample)
{
    QJsonObject obj;
    obj["timestamp_us"] = static_cast<double>(sample.timestampUs);
    obj["x"] = sample.position.x();
    obj["y"] = sample.position.y();
    obj["dx"] = sample.delta.x();
    obj["dy"] = sample.delta.y();
    obj["wheel_delta"] = sample.wheelDelta;
    obj["button_flags"] = static_cast<int>(sample.buttonFlags);
    obj["event_type"] = static_cast<int>(sample.eventType);
    return obj;
}
