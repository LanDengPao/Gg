#pragma once

#include "AppTypes.h"

// 根据原始输入设备路径补全更友好的鼠标名称和厂商信息。
bool populateFriendlyMouseName(const QString& rawDevicePath, DeviceInfo& info);
