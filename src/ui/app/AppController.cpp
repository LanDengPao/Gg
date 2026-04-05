#include "AppController.h"

#include <QDateTime>
#include <QDir>
#include <QUuid>

AppController::AppController(QObject* parent)
    : QObject(parent)
{
    m_inputSource.setSampleHandler([this](const MouseSample& sample, const DeviceInfo& device) {
        ingestSample(sample, device);
    });

    QString error;
    m_repository.ensureWorkspace(&error);
    reloadHistory();
    m_metrics.setSessionState(false, TestMode::PollingRate);
}

AppController::~AppController()
{
    m_inputSource.stop();
    if (m_isRecording) {
        QString ignored;
        stopTest(&ignored);
    }
    m_sampleWriter.close();
}

bool AppController::bindInputWindow(quintptr windowId, QString* error)
{
    return m_inputSource.start(windowId, error);
}

void AppController::setUiActive(bool active)
{
    if (m_metrics.snapshot().uiActive == active) {
        return;
    }
    m_metrics.setUiActive(active);
    emit stateChanged();
}

void AppController::ingestSample(const MouseSample& sample, const DeviceInfo& device)
{
    m_currentDevice = device;
    m_metrics.updateDevice(device);
    m_metrics.ingestSample(sample);

    if (m_isRecording) {
        QString ignored;
        m_sampleWriter.append(sample, &ignored);
    }
}

bool AppController::startTest(TestMode mode, QString* error)
{
    if (m_isRecording) {
        if (error) {
            *error = uiText("A test is already being recorded.", "当前已经有一个测试在录制中");
        }
        return false;
    }
    if (m_currentDevice.deviceId.isEmpty()) {
        if (error) {
            *error = uiText("No usable mouse device is currently detected.", "当前未识别到可用鼠标设备");
        }
        return false;
    }

    m_currentSession = {};
    m_currentSession.sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_currentSession.sessionDir = m_repository.sessionDirectory(m_currentSession.sessionId);
    m_currentSession.mode = mode;
    m_currentSession.status = SessionStatus::Recording;
    m_currentSession.startTimeUtc = QDateTime::currentDateTimeUtc();

    if (!QDir().mkpath(m_currentSession.sessionDir)) {
        if (error) {
            *error = uiText("Failed to create the test directory.", "无法创建测试目录");
        }
        return false;
    }

    if (!m_sampleWriter.start(m_repository.sessionSamplesPath(m_currentSession.sessionId), error)) {
        return false;
    }
    if (!m_repository.saveSessionRecord(m_currentSession, error)) {
        m_sampleWriter.close();
        return false;
    }

    m_isRecording = true;
    m_metrics.setSessionState(true, mode);
    emit stateChanged();
    return true;
}

bool AppController::stopTest(QString* error)
{
    if (!m_isRecording) {
        return true;
    }

    m_isRecording = false;
    m_sampleWriter.close();

    m_currentSession.endTimeUtc = QDateTime::currentDateTimeUtc();
    m_currentSession.durationMs = m_currentSession.startTimeUtc.msecsTo(m_currentSession.endTimeUtc);
    m_currentSession.sampleCount = m_sampleWriter.sampleCount();
    m_currentSession.status = SessionStatus::Completed;

    const SessionSummary summary = m_metrics.buildSessionSummary(m_currentSession.sessionId,
                                                                 m_currentSession.mode,
                                                                 m_currentSession.startTimeUtc,
                                                                 m_currentSession.endTimeUtc,
                                                                 m_currentDevice);
    m_currentSession.summary = summary;

    bool saveOk = true;
    if (!m_repository.saveSessionRecord(m_currentSession, error)) {
        m_currentSession.status = SessionStatus::Error;
        saveOk = false;
    }
    if (saveOk && !m_repository.saveSessionSummary(summary, error)) {
        m_currentSession.status = SessionStatus::Error;
        saveOk = false;
    }

    m_metrics.setSessionState(false, m_currentSession.mode);
    if (saveOk) {
        reloadHistory();
        emit historyChanged();
    }
    emit stateChanged();
    return saveOk;
}

LiveSnapshot AppController::snapshot() const
{
    return m_metrics.snapshot();
}

QVector<SessionRecord> AppController::history() const
{
    return m_history;
}

SessionRecord AppController::lastSession() const
{
    return m_history.isEmpty() ? SessionRecord{} : m_history.first();
}

QString AppController::workspacePath() const
{
    return m_repository.rootPath();
}

bool AppController::exportSession(const QString& sessionId, const QString& filePath, bool asJson, QString* error) const
{
    return m_repository.exportSamples(sessionId, filePath, asJson, error);
}

void AppController::reloadHistory()
{
    m_history = m_repository.loadSessions();
}
