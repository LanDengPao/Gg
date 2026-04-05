#pragma once

#include "AppTypes.h"

#include <QAbstractNativeEventFilter>
#include <QByteArray>
#include <QHash>

#include <functional>

class WinRawInputMouseSource : public QAbstractNativeEventFilter
{
public:
    using SampleHandler = std::function<void(const MouseSample& sample, const DeviceInfo& device)>;

    WinRawInputMouseSource();
    ~WinRawInputMouseSource() override;

    bool start(quintptr targetWindowId, QString* error = nullptr);
    void stop();
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
