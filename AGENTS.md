# AGENTS.md

## 项目定位
这是一个基于 C++17 / Qt Widgets / Windows API 的桌面应用，目标是做鼠标轮询率测试、轨迹预览和性能分析。
当前主干代码包含：
- `Gg` 主窗口与多页面 UI
- `AppController` 会话控制与导出
- `MetricsEngine` 实时统计计算
- `WorkspaceRepository` 工作区持久化
- `WinDeviceInfo` 基于 SetupAPI 的鼠标信息补全

## 代理工作约定
- 与用户交流默认使用中文。
- 默认使用 Windows 风格命令和路径。
- 只维护一个 `build/` 目录，不要创建 `build2/`、`cmake-build-debug/` 之类的新构建目录。
- 先做最小正确修改，不要顺手重构整仓或大规模改格式。
- 不要编辑 CMake/Qt 生成物，如 `ui_*.h`、`moc_*.cpp`、`qrc_*.cpp`、`build/` 下文件。

## 现有规则文件
- 仓库中存在 `CLAUDE.md`，其中有效信息已吸收进本文件：中文交流、Windows 优先、当前无 lint/单测。
- 未发现 `.cursorrules`。
- 未发现 `.cursor/rules/`。
- 未发现 `.github/copilot-instructions.md`。

## 仓库结构
- `main.cpp`: Qt 应用入口。
- `Gg.h` / `Gg.cpp`: 主窗口、页面构建、Raw Input 接入、UI 刷新。
- `AppController.*`: 测试开始/结束、采样写盘、历史读取、导出。
- `MetricsEngine.*`: Hz、速度、抖动、稳定度等实时统计。
- `WorkspaceRepository.*`: `workspace/sessions/<session_id>` 下的 JSON/BIN 持久化。
- `AppTypes.h`: 枚举、结构体、`uiText()`、JSON 辅助函数。
- `SparklineWidget.*` / `TrajectoryWidget.*`: 自绘图表控件。
- `WinDeviceInfo.*`: Windows 设备路径、VID/PID、友好名称解析。
- `Gg.ui`: 已加入 CMake AUTOUIC，但当前主窗口实际由 `Gg::buildUi()` 代码构建；改主界面先看 `Gg.cpp`，不要假设 `.ui` 是真实来源。

## 环境要求
- Windows 10/11、Visual Studio 2022、CMake 3.16+、Qt 5.14+ 或 Qt 6。
- `CMakePresets.json` 当前把 `CMAKE_PREFIX_PATH` 指向 `D:/QT/QT5.14/5.14.2/msvc2017_64`。
- 如果本机 Qt 路径不同，先改 `CMakePresets.json` 或在手动配置时覆盖 `-DCMAKE_PREFIX_PATH`。

## 构建命令
推荐优先使用 CMake Presets：

```powershell
cmake --preset debug
cmake --build --preset build-debug
cmake --preset release
cmake --build --preset build-release
```

只重编译主目标时可用：

```powershell
cmake --build --preset build-debug --target Gg
cmake --build --preset build-release --target Gg
```

手动配置方式：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="D:/QT/QT5.14/5.14.2/msvc2017_64"
cmake --build build --config Debug --target Gg
cmake --build build --config Release --target Gg
```

## 运行命令
当前工作区里实际产物路径是 `build/Debug/Gg.exe` 和 `build/Release/Gg.exe`。

```powershell
.\build\Debug\Gg.exe
.\build\Release\Gg.exe
```

- 旧文档里出现过 `build/Debug/Debug/Gg.exe` 之类路径；以当前 `build/` 目录实际布局为准。

## 测试与 lint
- 当前仓库没有配置 lint 工具。
- 当前仓库没有 `enable_testing()` / `add_test()`，也没有独立单元测试目标。
- 因此当前不存在“单个测试”可运行。

当前可执行的验证方式：

```powershell
cmake --build --preset build-debug --target Gg
.\build\Debug\Gg.exe
```

如果只是想确认 CTest 状态，可以运行：

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

- 目前它会报告没有测试，这是正常现象。
- 如果未来补上 CTest，单个测试的标准命令是 `ctest --test-dir build -C Debug -R <test-name> --output-on-failure`。

## 代码风格总则
- 使用 4 空格缩进。
- 大括号使用 K&R 风格。
- 不强制极短行，但保持表达紧凑、易扫读。
- 优先做局部修改，避免无意义的文件级格式化。
- 头文件用 `#pragma once`。
- 倾向把小型工具函数放在匿名命名空间中。

## include / import 约定
- `.cpp` 文件优先包含自己的头文件。
- 现有 include 顺序并不完全统一；修改老文件时优先保持局部既有顺序，除非你正在顺手修正明显问题。
- 新代码建议顺序：本文件头、Qt 头、标准库头、Windows SDK 头、项目内其他头。
- 需要包含 `windows.h` 时，先定义 `WIN32_LEAN_AND_MEAN` 与 `NOMINMAX`。
- 头文件尽量用前向声明减少编译依赖。
- 项目内头文件使用双引号，系统/Qt 头文件使用尖括号。

## 命名约定
- 类名：`PascalCase`。
- 方法、普通函数、局部变量：`camelCase`。
- 成员变量：`m_` 前缀。
- 常量：`k` 前缀 + `PascalCase`。
- 枚举类型使用 `enum class`，枚举值使用 `PascalCase`。
- 文件名与核心类名保持一致，使用 `.h` / `.cpp`。

## 类型与数据结构
- UI、字符串、时间、容器优先使用 Qt 类型，如 `QString`、`QVector`、`QDateTime`、`QPointF`。
- 纯数值和算法场景使用标准 C++ 基础类型；Windows API 边界使用 `HWND`、`DWORD`、`UINT` 等类型。
- 当前代码广泛使用值语义结构体，新增状态优先考虑值类型而不是堆分配对象。
- QWidget/QObject 生命周期优先依赖 Qt 父子关系管理。
- `Gg` 中大量控件成员是裸指针，但默认由 Qt parent 托管；不要无故引入另一套所有权模型。

## 字符串、本地化与序列化
- 固定字符串优先使用 `QStringLiteral()`。
- 面向用户的中英文文案优先走 `uiText(english, chinese)`，保持双语切换行为一致。
- 时间序列化统一使用 `Qt::ISODate`。
- JSON 字段名保持小写蛇形，如 `session_id`、`avg_hz`。
- 二进制采样文件继续沿用 `QDataStream::LittleEndian`、`kSampleMagic`、`kSampleVersion`。
- 保持现有文件编码；仓库里部分源文件带 UTF-8 BOM，改动时不要无意破坏编码。

## 错误处理
- 不要引入异常流；当前工程基本不用 `throw/try/catch`。
- 失败路径优先返回 `bool`，并通过 `QString* error` 输出错误信息。
- 先检查前置条件，尽早返回。
- 写文件优先使用 `QSaveFile`，提交失败要向上返回错误。
- 新错误信息应保持专业、简洁，并与现有中英文提示风格一致。

## Qt / Widgets 约定
- 只有需要信号槽、元对象能力时才在类里加 `Q_OBJECT`。
- 信号槽连接优先使用函数指针语法或简短 lambda。
- 每个 `QWidget` 只设置一个主 layout，不要重复 `setLayout()`。
- 当前界面主题是深色风格；修改 UI 时尽量延续现有视觉语言。
- `SparklineWidget` 与 `TrajectoryWidget` 使用自绘，改动时注意空数据态文案和抗锯齿设置。
- `Gg.ui` 虽在 CMake 里，但当前主窗口真实结构来自 `Gg::buildUi()`；改主界面先看 `Gg.cpp`。
- 不要编辑 `ui_*.h`、`moc_*.cpp`、`qrc_*.cpp` 等生成文件。

## Windows / Raw Input 约定
- Raw Input 注册必须发生在窗口已经显示、拥有真实 HWND 之后。
- 当前实现是在 `Gg::showEvent()` 首次触发后用 `QTimer::singleShot(0, ...)` 调 `registerRawInput()`；扩展相关逻辑时沿用这个时序。
- `nativeEvent()` 负责接收 `WM_INPUT`，不要把这条链路拆散到多个不相关位置。
- 设备名补全走 `WinDeviceInfo.cpp` 的 SetupAPI 逻辑；改设备识别前先理解 VID/PID 与路径匹配策略。

## 持久化与工作区约定
- 工作区根目录来自 `QStandardPaths::AppDataLocation` 下的 `workspace/`。
- 会话目录格式是 `workspace/sessions/<session_id>/`。
- 当前核心文件名固定为 `session.json`、`summary.json`、`samples.bin`。
- 不要随意修改这些文件名、字段名、magic/version，除非同步处理读取兼容性。

## 编辑建议
- 不要因为看到旧说明就假设它仍然正确，先以当前源码和 `build/` 产物为准。
- `CLAUDE.md` 中关于旧 `Gg.sln`、旧输出路径和“UI 主要来自 Gg.ui”的描述已经部分过时；优先相信当前 CMake 与源码实现。
- 增加新 `.cpp` 文件时，记得同步加入 `CMakeLists.txt` 的 `PROJECT_SOURCES`。
- 不要为了“统一风格”而大面积重排 include、翻译文案或改动序列化字段。

## 变更后自检

```powershell
cmake --build --preset build-debug --target Gg
.\build\Debug\Gg.exe
```

重点手动确认：
- 应用能启动。
- 主窗口能显示。
- Raw Input 没有因为注册时机错误而失效。
- 历史记录、导出、图表或设备信息相关改动没有明显回归。
