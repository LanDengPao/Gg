#pragma once

#include "MetricsEngine.h"
#include "SessionSampleWriter.h"
#include "WinRawInputMouseSource.h"
#include "WorkspaceRepository.h"

#include <QObject>
#include <QtGlobal>

// 为界面层协调原始输入采集、实时指标计算和会话持久化。
class AppController : public QObject
{
    Q_OBJECT

public:
    explicit AppController(QObject* parent = nullptr);
    ~AppController() override;

    // 将原始输入采集绑定到应用主窗口的原生句柄。
    bool bindInputWindow(quintptr windowId, QString* error = nullptr);
    // 更新界面是否处于前台，用于准确反映后台录制状态。
    void setUiActive(bool active);

    // 以指定测试模式启动一次新的录制。
    bool startTest(TestMode mode, QString* error = nullptr);
    // 结束当前录制，写入汇总结果并刷新历史记录。
    bool stopTest(QString* error = nullptr);

    LiveSnapshot snapshot() const;
    QVector<SessionRecord> history() const;
    SessionRecord lastSession() const;
    QString workspacePath() const;
    // 将指定会话导出为 JSON 或 CSV 文件。
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
