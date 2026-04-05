#pragma once

#include "AppTypes.h"

#include <QAbstractNativeEventFilter>
#include <QByteArray>
#include <QHash>

#include <functional>

// 将 Windows 原始鼠标输入消息桥接为应用层可消费的采样数据。
class WinRawInputMouseSource : public QAbstractNativeEventFilter
{
public:
    // 每收到一条鼠标采样后回调，同时附带解析后的设备信息。
    using SampleHandler = std::function<void(const MouseSample& sample, const DeviceInfo& device)>;

    WinRawInputMouseSource();
    ~WinRawInputMouseSource() override;

    // 注册目标窗口，使其能够在后台接收鼠标原始输入。
    bool start(quintptr targetWindowId, QString* error = nullptr);
    void stop();
    // 安装处理已转换原始输入事件的回调。
    void setSampleHandler(SampleHandler handler);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;
#else
    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;
#endif

private:
    void handleRawInput(void* lParam);
    DeviceInfo resolveDeviceInfo(void* handle) const;
    static qint64 nowMicroseconds();

    QHash<quintptr, DeviceInfo> m_deviceCache;
    quintptr m_targetWindowId = 0;
    bool m_running = false;
    SampleHandler m_sampleHandler;
};
