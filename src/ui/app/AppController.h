#pragma once

#include "MetricsEngine.h"
#include "SessionSampleWriter.h"
#include "WinRawInputMouseSource.h"
#include "WorkspaceRepository.h"

#include <QObject>
#include <QtGlobal>

class AppController : public QObject
{
    Q_OBJECT

public:
    explicit AppController(QObject* parent = nullptr);
    ~AppController() override;

    bool bindInputWindow(quintptr windowId, QString* error = nullptr);
    void setUiActive(bool active);

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
    SessionSampleWriter m_sampleWriter;
    WinRawInputMouseSource m_inputSource;
    bool m_isRecording = false;

    void ingestSample(const MouseSample& sample, const DeviceInfo& device);
};
