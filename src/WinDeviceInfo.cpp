#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define INITGUID
#include <windows.h>
#include <ntddmou.h>
#include <setupapi.h>

#pragma comment(lib, "Setupapi.lib")

#include "WinDeviceInfo.h"

#include <QByteArray>
#include <QString>

namespace {
QString normalizeDevicePath(QString path)
{
    path = path.trimmed();
    if (path.startsWith(QStringLiteral("\\??\\"))) {
        path.replace(0, 4, QStringLiteral("\\\\?\\"));
    }
    return path.toLower();
}

QString extractVidPidKey(const QString& path)
{
    const QString upperPath = path.toUpper();
    const int vidIndex = upperPath.indexOf(QStringLiteral("VID_"));
    const int pidIndex = upperPath.indexOf(QStringLiteral("PID_"));
    if (vidIndex < 0 || pidIndex < 0 || vidIndex + 8 > upperPath.size() || pidIndex + 8 > upperPath.size()) {
        return QString();
    }

    return upperPath.mid(vidIndex, 8) + QLatin1Char('_') + upperPath.mid(pidIndex, 8);
}

QString readDeviceRegistryProperty(HDEVINFO deviceInfoSet, SP_DEVINFO_DATA& deviceInfoData, DWORD property)
{
    DWORD dataType = 0;
    DWORD requiredSize = 0;
    SetupDiGetDeviceRegistryPropertyW(deviceInfoSet,
                                      &deviceInfoData,
                                      property,
                                      &dataType,
                                      nullptr,
                                      0,
                                      &requiredSize);
    if (requiredSize == 0) {
        return QString();
    }

    QByteArray buffer(static_cast<int>(requiredSize), 0);
    if (!SetupDiGetDeviceRegistryPropertyW(deviceInfoSet,
                                           &deviceInfoData,
                                           property,
                                           &dataType,
                                           reinterpret_cast<PBYTE>(buffer.data()),
                                           requiredSize,
                                           nullptr)) {
        return QString();
    }

    return QString::fromWCharArray(reinterpret_cast<const wchar_t*>(buffer.constData())).trimmed();
}
}

bool populateFriendlyMouseName(const QString& rawDevicePath, DeviceInfo& info)
{
    const QString normalizedRawPath = normalizeDevicePath(rawDevicePath);
    const QString rawVidPidKey = extractVidPidKey(rawDevicePath);

    HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_MOUSE,
                                                  nullptr,
                                                  nullptr,
                                                  DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        return false;
    }

    bool found = false;
    for (DWORD index = 0;; ++index) {
        SP_DEVICE_INTERFACE_DATA interfaceData;
        interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        if (!SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, &GUID_DEVINTERFACE_MOUSE, index, &interfaceData)) {
            break;
        }

        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetailW(deviceInfoSet, &interfaceData, nullptr, 0, &requiredSize, nullptr);
        if (requiredSize == 0) {
            continue;
        }

        QByteArray detailBuffer(static_cast<int>(requiredSize), 0);
        auto* detailData = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(detailBuffer.data());
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        SP_DEVINFO_DATA deviceInfoData;
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        if (!SetupDiGetDeviceInterfaceDetailW(deviceInfoSet,
                                              &interfaceData,
                                              detailData,
                                              requiredSize,
                                              nullptr,
                                              &deviceInfoData)) {
            continue;
        }

        const QString devicePath = QString::fromWCharArray(detailData->DevicePath);
        const QString normalizedDevicePath = normalizeDevicePath(devicePath);
        const QString deviceVidPidKey = extractVidPidKey(devicePath);
        const bool pathMatched = normalizedDevicePath == normalizedRawPath;
        const bool vidPidMatched = !rawVidPidKey.isEmpty() && rawVidPidKey == deviceVidPidKey;
        if (!pathMatched && !vidPidMatched) {
            continue;
        }

        const QString friendlyName = readDeviceRegistryProperty(deviceInfoSet, deviceInfoData, SPDRP_FRIENDLYNAME);
        const QString deviceDesc = readDeviceRegistryProperty(deviceInfoSet, deviceInfoData, SPDRP_DEVICEDESC);
        const QString manufacturer = readDeviceRegistryProperty(deviceInfoSet, deviceInfoData, SPDRP_MFG);

        if (!friendlyName.isEmpty()) {
            info.displayName = friendlyName;
            info.productName = friendlyName;
        } else if (!deviceDesc.isEmpty()) {
            info.displayName = deviceDesc;
            info.productName = deviceDesc;
        }

        if (!manufacturer.isEmpty()) {
            info.manufacturer = manufacturer;
        }

        found = true;
        break;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return found;
}
