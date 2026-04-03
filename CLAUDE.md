# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 交流与环境
- 所有交流使用中文。
- 当前主要开发环境是 Windows；优先使用适合 Windows 的命令和路径约定。

## 常用命令

### 构建
- Visual Studio 2022 打开解决方案：`Gg.sln`
- 命令行 Debug 构建：`msbuild Gg.sln /p:Configuration=Debug /p:Platform=x64`
- 命令行 Release 构建：`msbuild Gg.sln /p:Configuration=Release /p:Platform=x64`

### 运行
- 构建后的可执行文件：`x64/Debug/Gg.exe`

### 测试与检查
- 当前仓库没有独立的测试项目，也没有配置 lint。
- 没有“单测/单个测试”可运行；目前只能通过成功构建和手动运行应用做验证。

## 代码架构
- 这是一个基于 **Qt Widgets + C++ + Windows SDK** 的 Windows 桌面应用，当前代码还是骨架，目标产品见 `需求.md`：一个“鼠标实时数据监控 + 性能分析”工具。
- 应用入口在 `main.cpp`：创建 `QApplication`、显示主窗口 `Gg`，并启动一个后台 `LambdaThread`。这个线程里的 lambda 目前是空实现，说明“后台采集/计算”通道已经预留，但核心业务逻辑尚未接入。
- 主窗口封装在 `Gg.h` / `Gg.cpp`，实际界面结构来自 `Gg.ui`。当前 UI 只有 `QMainWindow` 基础骨架（central widget / menu bar / toolbar / status bar），还没有业务控件。
- `LambdaThread.h` / `LambdaThread.cpp` 提供了一个轻量线程封装：
  - 用 `QThread + QObject::moveToThread` 执行传入的 `std::function<void()>`
  - 支持线程命名
  - Windows 下会优先调用 `SetThreadDescription`，回退到调试器线程命名异常
  - 支持 `selfDelete(true)` 在线程结束后自删
  未来如果接入 Raw Input 采集、统计计算或持久化，优先复用这条“UI 线程 + 后台工作线程”的分层。
- `Gg.vcxproj` 使用 **Qt VS Tools / QtMsBuild** 管理 Qt 生成步骤：
  - `Gg.ui` 通过 UIC 生成 `ui_Gg.h`
  - `Gg.h`、`LambdaThread.h` 通过 MOC 生成元对象代码
  - `Gg.qrc` 通过 RCC 生成资源代码
  修改 UI、QObject 类或资源文件时，不需要手写生成文件，按正常 Qt 方式维护源文件即可。

## 构建与依赖注意事项
- 项目配置依赖 **MSVC v143**。
- `Gg.vcxproj` 里固定使用 Qt 安装名 `5.14.2_msvc2017_64`，并通过 `QtMsBuild` 导入 Qt 构建目标；如果本机 Qt VS Tools 或该 Qt kit 未配置，命令行/VS 构建会失败。
- Debug 配置启用了 `core/gui/widgets/charts`；Release 当前只声明了 `core/gui/widgets`。如果后续在 Release 里也使用 Qt Charts，需要同步项目配置。

## 当前实现状态
- 仓库里还没有 Raw Input、SQLite、图表展示、会话记录、导出等正式实现；这些能力目前只体现在 `需求.md` 的目标描述里。
- 因此后续开发时，优先把仓库看作“Qt 桌面壳 + 后台线程工具类”的起点，而不是已经成型的鼠标分析应用。