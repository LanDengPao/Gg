# Gg - 鼠标设备测试工具

Windows 桌面应用，用于测试和分析鼠标设备的性能指标。

## 功能特性

- **轮询率测试** - 检测鼠标报告率（125Hz/500Hz/1000Hz 等）
- **轨迹稳定性** - 分析鼠标移动的精度和稳定性
- **设备识别** - 自动识别连接的 HID 鼠标设备
- **数据记录** - 保存测试会话，支持 JSON 格式导出
- **历史对比** - 对比多次测试结果
- **双语支持** - 中文/英文界面

## 环境要求

- Windows 10/11
- Visual Studio 2022
- Qt 5.14+ (msvc2017_64)

## 构建

```bash
# Debug
cmake --preset debug
cmake --build --preset build-debug

# Release
cmake --preset release
cmake --build --preset build-release
```

输出: `build/bin/Debug/Gg.exe`

## 项目结构

```
Gg/
├── include/Gg/     # 头文件
├── src/           # 源文件
├── cmake/         # CMake 模块
└── build/        # 构建输出
```

## 技术栈

- C++23
- Qt Widgets
- Windows API (Raw Input)
