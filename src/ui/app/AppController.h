#pragma once

#include "MetricsEngine.h"
#include "WorkspaceRepository.h"

#include <QFile>
#include <QObject>

class AppController : public QObject
{
    Q_OBJECT

public:
    explicit AppController(QObject* parent = nullptr);
    ~AppController() override;

    void setUiActive(bool active);
    void ingestSample(const MouseSample& sample, const DeviceInfo& device);

    bool startTest(TestMode mode, QString* error = nullptr);
    bool stopTest(QString* error = nullptr);

    LiveSnapshot snapshot() const;
    QVector<SessionRecord> history() const;
    SessionRecord lastSession() const;
    QString workspacePath() const;
    bool exportSession(const QString& sessionId, const QString& filePath, bool asJson, QString* error = nullptr) const;
    void reloadHistory();

signals:
    void stateChanged();
    void historyChanged();

private:
    WorkspaceRepository m_repository;
    MetricsEngine m_metrics;
    QVector<SessionRecord> m_history;
    DeviceInfo m_currentDevice;
    SessionRecord m_currentSession;
    bool m_isRecording = false;
    QFile m_sampleFile;
    QDataStream* m_sampleStream = nullptr;

    void closeSampleStream();
};
