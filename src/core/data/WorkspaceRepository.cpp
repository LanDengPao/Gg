#include "WorkspaceRepository.h"

#include <QCoreApplication>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>
#include <algorithm>

namespace {
constexpr quint32 kSampleMagic = 0x47475331;
constexpr quint16 kSampleVersion = 1;
}

WorkspaceRepository::WorkspaceRepository()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_rootPath = QDir(base.isEmpty() ? QCoreApplication::applicationDirPath() : base)
                     .filePath(QStringLiteral("workspace"));
}

QString WorkspaceRepository::rootPath() const
{
    return m_rootPath;
}

bool WorkspaceRepository::ensureWorkspace(QString* error) const
{
    QDir dir(m_rootPath);
    if (!dir.exists() && !QDir().mkpath(m_rootPath)) {
        if (error) {
            *error = uiText("Failed to create workspace: %1", "无法创建工作目录：%1").arg(m_rootPath);
        }
        return false;
    }

    if (!dir.exists(QStringLiteral("sessions")) && !dir.mkpath(QStringLiteral("sessions"))) {
        if (error) {
            *error = uiText("Failed to create the sessions directory.", "无法创建 sessions 目录");
        }
        return false;
    }
    return true;
}

QString WorkspaceRepository::sessionDirectory(const QString& sessionId) const
{
    return QDir(m_rootPath).filePath(QStringLiteral("sessions/%1").arg(sessionId));
}

QString WorkspaceRepository::sessionMetaPath(const QString& sessionId) const
{
    return QDir(sessionDirectory(sessionId)).filePath(QStringLiteral("session.json"));
}

QString WorkspaceRepository::sessionSummaryPath(const QString& sessionId) const
{
    return QDir(sessionDirectory(sessionId)).filePath(QStringLiteral("summary.json"));
}

QString WorkspaceRepository::sessionSamplesPath(const QString& sessionId) const
{
    return QDir(sessionDirectory(sessionId)).filePath(QStringLiteral("samples.bin"));
}

bool WorkspaceRepository::saveSessionRecord(const SessionRecord& record, QString* error) const
{
    if (!ensureWorkspace(error)) {
        return false;
    }
    if (!QDir().mkpath(record.sessionDir)) {
        if (error) {
            *error = uiText("Failed to create session directory: %1", "无法创建会话目录：%1").arg(record.sessionDir);
        }
        return false;
    }
    return writeJsonFile(sessionMetaPath(record.sessionId), toJson(record), error);
}

bool WorkspaceRepository::saveSessionSummary(const SessionSummary& summary, QString* error) const
{
    if (!ensureWorkspace(error)) {
        return false;
    }
    if (!QDir().mkpath(sessionDirectory(summary.sessionId))) {
        if (error) {
            *error = uiText("Failed to create session directory: %1", "无法创建会话目录：%1").arg(summary.sessionId);
        }
        return false;
    }
    return writeJsonFile(sessionSummaryPath(summary.sessionId), toJson(summary), error);
}

QVector<SessionRecord> WorkspaceRepository::loadSessions() const
{
    QVector<SessionRecord> result;
    QDir sessionsDir(QDir(m_rootPath).filePath(QStringLiteral("sessions")));
    if (!sessionsDir.exists()) {
        return result;
    }

    const QFileInfoList dirs = sessionsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    for (const QFileInfo& info : dirs) {
        const QJsonObject metaObj = readJsonFile(QDir(info.absoluteFilePath()).filePath(QStringLiteral("session.json")));
        if (metaObj.isEmpty()) {
            continue;
        }

        SessionRecord record = sessionRecordFromJson(metaObj);
        const QJsonObject summaryObj = readJsonFile(QDir(info.absoluteFilePath()).filePath(QStringLiteral("summary.json")));
        if (!summaryObj.isEmpty()) {
            record.summary = sessionSummaryFromJson(summaryObj);
        }
        result.push_back(record);
    }

    std::sort(result.begin(), result.end(), [](const SessionRecord& a, const SessionRecord& b) {
        return a.startTimeUtc > b.startTimeUtc;
    });
    return result;
}

QVector<MouseSample> WorkspaceRepository::loadSamples(const QString& sessionId, QString* error) const
{
    QVector<MouseSample> samples;
    QFile file(sessionSamplesPath(sessionId));
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = uiText("Failed to open sample file: %1", "无法打开采样文件：%1").arg(file.fileName());
        }
        return samples;
    }

    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    quint32 magic = 0;
    quint16 version = 0;
    in >> magic >> version;
    if (magic != kSampleMagic || version != kSampleVersion) {
        if (error) {
            *error = uiText("The sample file format is invalid.", "采样文件格式不正确");
        }
        return samples;
    }

    while (!in.atEnd()) {
        MouseSample sample;
        qint32 x = 0;
        qint32 y = 0;
        qint32 dx = 0;
        qint32 dy = 0;
        qint32 wheel = 0;
        quint16 buttonFlags = 0;
        quint8 eventType = 0;
        in >> sample.timestampUs >> x >> y >> dx >> dy >> wheel >> buttonFlags >> eventType;
        if (in.status() != QDataStream::Ok) {
            break;
        }
        sample.position = QPoint(x, y);
        sample.delta = QPoint(dx, dy);
        sample.wheelDelta = wheel;
        sample.buttonFlags = buttonFlags;
        sample.eventType = eventType;
        samples.push_back(sample);
    }
    return samples;
}

bool WorkspaceRepository::exportSamples(const QString& sessionId, const QString& filePath, bool asJson, QString* error) const
{
    const QVector<MouseSample> samples = loadSamples(sessionId, error);
    if (!error || error->isEmpty()) {
        if (asJson) {
            QJsonArray array;
            for (const MouseSample& sample : samples) {
                array.append(toJson(sample));
            }
            return writeJsonFile(filePath, QJsonObject{{"samples", array}}, error);
        }

        QSaveFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            if (error) {
                *error = uiText("Failed to write export file: %1", "无法写入导出文件：%1").arg(filePath);
            }
            return false;
        }
        QTextStream stream(&file);
        stream << "timestamp_us,x,y,dx,dy,wheel_delta,button_flags,event_type\n";
        for (const MouseSample& sample : samples) {
            stream << sample.timestampUs << ','
                   << sample.position.x() << ','
                   << sample.position.y() << ','
                   << sample.delta.x() << ','
                   << sample.delta.y() << ','
                   << sample.wheelDelta << ','
                   << sample.buttonFlags << ','
                   << static_cast<int>(sample.eventType) << '\n';
        }
        return file.commit();
    }
    return false;
}

bool WorkspaceRepository::writeJsonFile(const QString& path, const QJsonObject& obj, QString* error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = uiText("Failed to write JSON file: %1", "无法写入 JSON 文件：%1").arg(path);
        }
        return false;
    }
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (error) {
            *error = uiText("Failed to commit JSON file: %1", "提交 JSON 文件失败：%1").arg(path);
        }
        return false;
    }
    return true;
}

QJsonObject WorkspaceRepository::readJsonFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.isObject() ? doc.object() : QJsonObject{};
}
