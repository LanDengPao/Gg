#include "WinRawInputMouseSource.h"

#include "WinDeviceInfo.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QVector>

#include <utility>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winuser.h>

#ifndef MOUSE_MOVE_ABSOLUTE
#define MOUSE_MOVE_ABSOLUTE 0x00000001
#endif

WinRawInputMouseSource::WinRawInputMouseSource() = default;

WinRawInputMouseSource::~WinRawInputMouseSource()
{
    stop();
}

bool WinRawInputMouseSource::start(quintptr targetWindowId, QString* error)
{
    if (m_running && m_targetWindowId == targetWindowId) {
        return true;
    }

    stop();
    if (targetWindowId == 0) {
        if (error) {
            *error = QCoreApplication::translate("WinRawInputMouseSource", "The target window is invalid.");
        }
        return false;
    }

    RAWINPUTDEVICE rawInputDevice;
    // 以 HID Generic Desktop/Mouse 注册原始输入，并允许窗口在后台继续接收事件。
    rawInputDevice.usUsagePage = 0x01;
    rawInputDevice.usUsage = 0x02;
    rawInputDevice.dwFlags = RIDEV_INPUTSINK;
    rawInputDevice.hwndTarget = reinterpret_cast<HWND>(targetWindowId);
    if (!RegisterRawInputDevices(&rawInputDevice, 1, sizeof(RAWINPUTDEVICE))) {
        if (error) {
            *error = QCoreApplication::translate("WinRawInputMouseSource", "Failed to register raw input.");
        }
        return false;
    }

    if (QCoreApplication* application = QCoreApplication::instance()) {
        application->installNativeEventFilter(this);
    }
    m_targetWindowId = targetWindowId;
    m_running = true;
    return true;
}

void WinRawInputMouseSource::stop()
{
    if (!m_running) {
        return;
    }

    if (QCoreApplication* application = QCoreApplication::instance()) {
        application->removeNativeEventFilter(this);
    }
    m_running = false;
    m_targetWindowId = 0;
}

void WinRawInputMouseSource::setSampleHandler(SampleHandler handler)
{
    m_sampleHandler = std::move(handler);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool WinRawInputMouseSource::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result)
#else
bool WinRawInputMouseSource::nativeEventFilter(const QByteArray& eventType, void* message, long* result)
#endif
{
    // 只处理目标窗口的 WM_INPUT，其他原生消息继续交给 Qt。
    Q_UNUSED(result);
    if (!m_running || eventType != "windows_generic_MSG") {
        return false;
    }

    MSG* msg = static_cast<MSG*>(message);
    if (msg->message != WM_INPUT || msg->hwnd != reinterpret_cast<HWND>(m_targetWindowId)) {
        return false;
    }

    handleRawInput(reinterpret_cast<void*>(msg->lParam));
    return false;
}

void WinRawInputMouseSource::handleRawInput(void* lParam)
{
    HRAWINPUT rawInputHandle = reinterpret_cast<HRAWINPUT>(lParam);

    UINT size = 0;
    // 先查询缓冲区大小，再读取完整的 RAWINPUT 数据。
    GetRawInputData(rawInputHandle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
    if (size == 0) {
        return;
    }

    QByteArray buffer(static_cast<int>(size), 0);
    if (GetRawInputData(rawInputHandle, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) != size) {
        return;
    }

    const RAWINPUT* raw = reinterpret_cast<const RAWINPUT*>(buffer.constData());
    if (raw->header.dwType != RIM_TYPEMOUSE) {
        return;
    }

    const RAWMOUSE& mouse = raw->data.mouse;
    const USHORT flags = mouse.usButtonFlags;
    // 过滤掉既没有位移也没有按键或滚轮变化的空事件。
    const bool hasMovement = mouse.lLastX != 0 || mouse.lLastY != 0 || (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0;
    if (!hasMovement && flags == 0) {
        return;
    }

    POINT point;
    GetCursorPos(&point);

    MouseSample sample;
    sample.timestampUs = nowMicroseconds();
    sample.position = QPoint(point.x, point.y);
    sample.delta = QPoint(mouse.lLastX, mouse.lLastY);
    sample.wheelDelta = (flags & RI_MOUSE_WHEEL) ? static_cast<short>(mouse.usButtonData) : 0;
    sample.buttonFlags = flags;
    sample.eventType = 0;
    if (flags & (RI_MOUSE_BUTTON_1_DOWN | RI_MOUSE_BUTTON_2_DOWN | RI_MOUSE_BUTTON_3_DOWN)) {
        sample.eventType = 1;
    } else if (flags & RI_MOUSE_WHEEL) {
        sample.eventType = 2;
    }

    const quintptr deviceKey = reinterpret_cast<quintptr>(raw->header.hDevice);
    // 设备信息解析代价较高，因此按原始设备句柄做缓存。
    if (!m_deviceCache.contains(deviceKey)) {
        m_deviceCache.insert(deviceKey, resolveDeviceInfo(raw->header.hDevice));
    }

    DeviceInfo device = m_deviceCache.value(deviceKey);
    device.connected = true;
    device.lastSeenUtc = QDateTime::currentDateTimeUtc();
    m_deviceCache.insert(deviceKey, device);

    if (m_sampleHandler) {
        m_sampleHandler(sample, device);
    }
}

DeviceInfo WinRawInputMouseSource::resolveDeviceInfo(void* handle) const
{
    DeviceInfo info;
    info.deviceId = QStringLiteral("mouse_%1").arg(reinterpret_cast<quintptr>(handle), 0, 16);
    info.connected = true;
    info.lastSeenUtc = QDateTime::currentDateTimeUtc();

    UINT deviceCount = 0;
    if (GetRawInputDeviceList(nullptr, &deviceCount, sizeof(RAWINPUTDEVICELIST)) == static_cast<UINT>(-1)) {
        return info;
    }

    QVector<RAWINPUTDEVICELIST> deviceList(static_cast<int>(deviceCount));
    GetRawInputDeviceList(deviceList.data(), &deviceCount, sizeof(RAWINPUTDEVICELIST));

    for (const RAWINPUTDEVICELIST& device : deviceList) {
        if (device.hDevice != handle) {
            continue;
        }

        UINT nameSize = 0;
        GetRawInputDeviceInfo(device.hDevice, RIDI_DEVICENAME, nullptr, &nameSize);
        if (nameSize == 0) {
            break;
        }

        QVector<wchar_t> nameBuffer(static_cast<int>(nameSize));
        GetRawInputDeviceInfo(device.hDevice, RIDI_DEVICENAME, nameBuffer.data(), &nameSize);

        const QString deviceName = QString::fromWCharArray(nameBuffer.constData());
        info.rawDeviceName = deviceName;
        populateFriendlyMouseName(deviceName, info);
        if (info.displayName.isEmpty()) {
            info.displayName = QStringLiteral("Mouse");
        }

        if (deviceName.contains(QStringLiteral("#VID_"), Qt::CaseInsensitive)) {
            const int vidIndex = deviceName.indexOf(QStringLiteral("#VID_"), 0, Qt::CaseInsensitive);
            const int pidIndex = deviceName.indexOf(QStringLiteral("#PID_"), 0, Qt::CaseInsensitive);
            if (vidIndex >= 0 && vidIndex + 9 <= deviceName.size()) {
                info.vendorId = QStringLiteral("0x%1").arg(deviceName.mid(vidIndex + 5, 4));
            }
            if (pidIndex >= 0 && pidIndex + 9 <= deviceName.size()) {
                info.productId = QStringLiteral("0x%1").arg(deviceName.mid(pidIndex + 5, 4));
            }
        }

        if (deviceName.contains(QStringLiteral("HID"), Qt::CaseInsensitive)) {
            info.connectionType = QStringLiteral("USB");
        }
        break;
    }

    if (info.displayName.isEmpty()) {
        info.displayName = QStringLiteral("Mouse");
    }
    return info;
}

qint64 WinRawInputMouseSource::nowMicroseconds()
{
    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;
    QueryPerformanceCounter(&counter);
    QueryPerformanceFrequency(&frequency);
    return (counter.QuadPart * 1000000) / frequency.QuadPart;
}
