#pragma once

#include "AppTypes.h"

#include <QVector>

class WorkspaceRepository
{
public:
    WorkspaceRepository();

    QString rootPath() const;
    bool ensureWorkspace(QString* error = nullptr) const;
    QString sessionDirectory(const QString& sessionId) const;
    QString sessionMetaPath(const QString& sessionId) const;
    QString sessionSummaryPath(const QString& sessionId) const;
    QString sessionSamplesPath(const QString& sessionId) const;

    bool saveSessionRecord(const SessionRecord& record, QString* error = nullptr) const;
    bool saveSessionSummary(const SessionSummary& summary, QString* error = nullptr) const;
    QVector<SessionRecord> loadSessions() const;
    QVector<MouseSample> loadSamples(const QString& sessionId, QString* error = nullptr) const;
    bool exportSamples(const QString& sessionId, const QString& filePath, bool asJson, QString* error = nullptr) const;

private:
    QString m_rootPath;

    static bool writeJsonFile(const QString& path, const QJsonObject& obj, QString* error);
    static QJsonObject readJsonFile(const QString& path);
};
