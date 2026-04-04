# AGENTS.md

## 项目概述

这是一个基于 C++ / Qt Widgets 的 Windows 桌面应用，用于鼠标轮询率测试和性能分析。

## 环境要求

- Windows 10/11
- Visual Studio 2022
- CMake 3.16+
- Qt 5.14+ 或 Qt 6

## 构建命令

- 仅维持一个build目录,禁止build2等等

### 使用 CMake Presets

```powershell
# Debug 构建
cmake --preset debug
cmake --build --preset build-debug

# Release 构建
cmake --preset release
cmake --build --preset build-release
```

### 手动构建

```powershell
# 配置（根据本机 Qt 路径调整）
cmake -S . -B build -DCMAKE_PREFIX_PATH="D:/QT/QT5.14/5.14.2/msvc2017_64"

# 编译
cmake --build build --config Debug
```

### 输出目录

- Debug: `build/Debug/Debug/Gg.exe`
- Release: `build/Release/Release/Gg.exe`

## 测试

当前项目没有独立的单元测试。验证方式：
1. 成功构建 = 编译通过
2. 运行 `Gg.exe` 手动测试功能

## 代码风格指南

### 格式化

- 使用 4 空格缩进
- 行长度无硬性限制，但保持简洁
- 大括号风格：K&R（同一行开括号）

### 命名约定

- **类名**: PascalCase (如 `AppController`, `MetricsEngine`)
- **方法名**: camelCase (如 `startTest`, `ingestSample`)
- **成员变量**: m_ 前缀 + camelCase (如 `m_isRecording`, `m_metrics`)
- **常量**: k 前缀 + PascalCase (如 `kSampleMagic`)
- **枚举值**: PascalCase (如 `SessionStatus::Recording`)
- **文件命名**: 与类名一致，.cpp/.h 扩展名

### 类型使用

- **Qt 类型**: 优先使用 Qt 类型 (QString, QVector, QPoint, QPointF, QDateTime 等)
- **基础类型**: 使用标准 C++ 类型 (int, double, bool)
- **Windows API**: 使用 Windows SDK 类型 (UINT, DWORD, HWND 等)
- **避免**: raw pointer，优先使用智能指针或 Qt 对象父子关系

### 头文件包含

- 按以下顺序分组：
  1. 对应头文件（如果有）
  2. Qt 系统头文件
  3. 标准库头文件
  4. Windows SDK 头文件
  5. 本项目头文件
- 使用前向声明减少编译依赖
- 本地头文件用双引号，自带头文件用尖括号

示例：
```cpp
#include "Gg.h"

#include <QVector>
#include <QString>

#include <algorithm>
#include <cmath>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "AppController.h"
#include "MetricsEngine.h"
```

### 编码规范

- **源文件编码**: UTF-8 BOM
- **字符串**: 使用 QStringLiteral() 或 tr() 处理用户可见文本
- **中文注释**: 可以使用中文，但保持专业简洁
- **禁止**: 禁止在代码中硬编码 secrets/keys

### 错误处理

- 使用 QString* outError 参数模式返回错误信息
- 检查返回值并传播错误
- 避免异常（Qt 风格）

### Qt 特定规则

- **QObject 子类**: 使用 Q_OBJECT 宏
- **信号槽**: 使用 connect() 连接，优先使用 lambda 或 member function
- **内存管理**: Qt 对象使用父子关系，非 Qt 对象注意生命周期
- **MOC**: 需要 MOC 的类必须继承 QObject 并包含 Q_OBJECT
- **UI 文件**: Gg.ui 由 CMake 自动处理，勿手动修改生成的 ui_Gg.h

### 代码组织

- 单文件类：.h 和 .cpp 在同一目录
- 类声明在 .h，实现在 .cpp
- 私有成员用 d-pointer 或简单成员变量（避免过度抽象）
- 合理使用 namespace 区分模块

## 项目结构

```
E:\repo\Gg\
├── Gg.cpp / Gg.h       # 主窗口
├── main.cpp             # 入口
├── AppController.cpp/h  # 应用控制器
├── AppTypes.h           # 类型定义
├── MetricsEngine.cpp/h  # 指标计算引擎
├── SparklineWidget.cpp/h # 折线图组件
├── TrajectoryWidget.cpp/h # 轨迹图组件
├── WorkspaceRepository.cpp/h # 文件仓储
├── LambdaThread.cpp/h   # 后台线程封装
├── CMakeLists.txt       # 构建配置
└── AGENTS.md           # 本文件
```

## 注意事项

1. **Raw Input**: 注册 raw input 需在窗口显示后（showEvent 中或 QTimer::singleShot）
2. **布局避免冲突**: 每个 QWidget 只设置一个主 layout
3. **计时器**: 类成员 QTimer 可能导致栈溢出，优先在 showEvent 中动态创建
4. **链接器**: 确保 CMakeLists.txt 包含所有 .cpp 文件