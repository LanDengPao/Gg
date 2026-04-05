#pragma once

#include "AppTypes.h"

#include <QVector>

// 在应用工作区下持久化测试数据，并为界面重新加载历史记录。
class WorkspaceRepository
{
public:
    WorkspaceRepository();

    QString rootPath() const;
    // 按需创建工作区根目录和 sessions 子目录。
    bool ensureWorkspace(QString* error = nullptr) const;
    QString sessionDirectory(const QString& sessionId) const;
    QString sessionMetaPath(const QString& sessionId) const;
    QString sessionSummaryPath(const QString& sessionId) const;
    QString sessionSamplesPath(const QString& sessionId) const;

    bool saveSessionRecord(const SessionRecord& record, QString* error = nullptr) const;
    bool saveSessionSummary(const SessionSummary& summary, QString* error = nullptr) const;
    // Loads all saved sessions sorted from newest to oldest.
    QVector<SessionRecord> loadSessions() const;
    // Reads the binary sample stream written during recording.
    QVector<MouseSample> loadSamples(const QString& sessionId, QString* error = nullptr) const;
    // Exports a recorded session to either JSON or CSV.
    bool exportSamples(const QString& sessionId, const QString& filePath, bool asJson, QString* error = nullptr) const;

private:
    QString m_rootPath;

    static bool writeJsonFile(const QString& path, const QJsonObject& obj, QString* error);
    static QJsonObject readJsonFile(const QString& path);
};
