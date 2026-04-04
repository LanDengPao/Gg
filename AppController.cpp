#include "AppController.h"

#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QUuid>

namespace {
constexpr quint32 kSampleMagic = 0x47475331;
constexpr quint16 kSampleVersion = 1;
}

AppController::AppController(QObject* parent)
    : QObject(parent)
{
    QString error;
    m_repository.ensureWorkspace(&error);
    reloadHistory();
    m_metrics.setSessionState(false, TestMode::PollingRate);
}

AppController::~AppController()
{
    if (m_isRecording) {
        QString ignored;
        stopTest(&ignored);
    }
    closeSampleStream();
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

    if (m_isRecording && m_sampleStream) {
        (*m_sampleStream) << sample.timestampUs
                          << static_cast<qint32>(sample.position.x())
                          << static_cast<qint32>(sample.position.y())
                          << static_cast<qint32>(sample.delta.x())
                          << static_cast<qint32>(sample.delta.y())
                          << static_cast<qint32>(sample.wheelDelta)
                          << sample.buttonFlags
                          << sample.eventType;
        m_currentSession.sampleCount += 1;
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

    if (!m_repository.saveSessionRecord(m_currentSession, error)) {
        return false;
    }

    m_sampleFile.setFileName(m_repository.sessionSamplesPath(m_currentSession.sessionId));
    if (!m_sampleFile.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = uiText("Failed to create the sample file.", "无法创建采样文件");
        }
        return false;
    }

    closeSampleStream();
    m_sampleStream = new QDataStream(&m_sampleFile);
    m_sampleStream->setByteOrder(QDataStream::LittleEndian);
    (*m_sampleStream) << kSampleMagic << kSampleVersion;

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
    closeSampleStream();

    m_currentSession.endTimeUtc = QDateTime::currentDateTimeUtc();
    m_currentSession.durationMs = m_currentSession.startTimeUtc.msecsTo(m_currentSession.endTimeUtc);
    m_currentSession.status = SessionStatus::Completed;

    const SessionSummary summary = m_metrics.buildSessionSummary(m_currentSession.sessionId,
                                                                 m_currentSession.mode,
                                                                 m_currentSession.startTimeUtc,
                                                                 m_currentSession.endTimeUtc,
                                                                 m_currentDevice);
    m_currentSession.summary = summary;

    if (!m_repository.saveSessionRecord(m_currentSession, error)) {
        m_currentSession.status = SessionStatus::Error;
        return false;
    }
    if (!m_repository.saveSessionSummary(summary, error)) {
        m_currentSession.status = SessionStatus::Error;
        return false;
    }

    m_metrics.setSessionState(false, m_currentSession.mode);
    reloadHistory();
    emit historyChanged();
    emit stateChanged();
    return true;
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

void AppController::closeSampleStream()
{
    delete m_sampleStream;
    m_sampleStream = nullptr;
    if (m_sampleFile.isOpen()) {
        m_sampleFile.close();
    }
}
