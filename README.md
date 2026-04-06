# Gg

`Gg` 是一个面向 Windows 桌面的鼠标设备测试与分析工具，用于采集原始输入并实时展示轮询率、轨迹、速度、抖动等指标。

## 功能特性

- 轮询率测试，观察当前 Hz、窗口平均值、标准差和稳定度
- 轨迹稳定性测试，查看鼠标移动轨迹和抖动表现
- 基于 Windows Raw Input 的鼠标原始输入采集
- 历史记录浏览与多次测试结果对比
- 支持 `CSV` / `JSON` 导出原始采样数据
- Qt 官方翻译机制驱动的中英文界面切换
- 基于 `.ui + .qss` 的桌面界面结构，当前为现代 Windows 浅色风格

## 运行环境

- Windows 10 / 11
- Visual Studio 2022
- CMake 3.21+
- Qt 5.14.2+（需包含 `Widgets` 和 `LinguistTools`）

当前预设默认使用的 Qt 路径见 `CMakePresets.json`：

```json
"CMAKE_PREFIX_PATH": "D:/QT/QT5.14/5.14.2/msvc2017_64"
```

## 构建

### 使用 Preset

```powershell
# Debug
cmake --preset debug
cmake --build --preset build-debug

# Release
cmake --preset release
cmake --build --preset build-release
```

### 安装到本地目录

```powershell
# Debug 安装目录: build/install/Debug
cmake --build --preset install-debug

# Release 安装目录: build/install/Release
cmake --build --preset install-release
```

### 一键生成发布包

```powershell
cmake --preset release
cmake --build --preset package-release
```

输出文件：`build/packages/Gg-0.2.0-win64.zip`

### 输出目录

- Debug 可执行文件：`build/bin/Debug/Gg.exe`
- Release 可执行文件：`build/bin/Release/Gg.exe`
- 运行时翻译文件：`build/bin/<Config>/translations/Gg_zh_CN.qm`
- 安装目录：`build/install/<Config>/`
- 发布包目录：`build/packages/`

## 发布包说明

当前发布包面向 `Windows 10 / Windows 11`，目标是“最小依赖运行”：

- 包内包含应用本体 `Gg.exe`
- 包内包含 Qt 运行时和必要插件，由 `windeployqt` 自动部署
- 包内包含 MSVC 运行库，如 `vcruntime140.dll`、`msvcp140.dll`
- 不额外打包 Win10/Win11 已自带的 UCRT / `api-ms-win-*` 系统运行库

这意味着发布包默认假设目标机器是正常的 Windows 10/11 环境，不追求向更旧系统兼容。

## 开发提示

### 修改 `.ui` / `.qrc` 后

项目已开启 `AUTOUIC` / `AUTORCC`，重新构建即可：

```powershell
cmake --build --preset build-debug
```

### 修改翻译源文本后

先更新 `.ts`，再重新构建：

```powershell
& "D:/QT/QT5.14/5.14.2/msvc2017_64/bin/lupdate.exe" "E:/repo/Gg/src" -noobsolete -ts "E:/repo/Gg/src/ui/app/i18n/Gg_zh_CN.ts"
cmake --build --preset build-debug
```

### 修改打包相关配置后

建议重新配置再重新打包：

```powershell
cmake --preset release
cmake --build --preset package-release
```

## 项目结构

```text
Gg/
├── cmake/                    # 全局 CMake 配置
├── docs/                     # 架构与技术文档
├── src/
│   ├── core/                 # 核心逻辑、输入采集、存储、指标计算
│   └── ui/                   # 自定义绘图控件与应用层 UI
│       ├── app/              # 主窗口、UI 资源、QSS、翻译、图标
│       ├── SparklineWidget.* # 轮询率曲线控件
│       └── TrajectoryWidget.*# 轨迹预览控件
├── tests/                    # 预留测试目录
├── Build.md                  # 详细构建说明
├── CMakeLists.txt            # 根构建入口
└── CMakePresets.json         # Debug / Release 预设
```

## 技术栈

- C++23
- Qt Widgets
- Qt LinguistTools
- Windows Raw Input
- CMake

## 数据与存储

- 测试数据以“会话目录”方式持久化
- 原始采样、会话元信息、统计摘要分别保存
- 工作区由应用在本地自动创建和维护

## 文档

- 架构设计：`docs/技术/架构设计.md`
- 构建说明：`Build.md`

## 当前状态

- 主界面已采用 `.ui + .qss` 分离方式组织
- 样式已集中到 `src/ui/app/gg.qss`
- 中英文界面切换已接入 Qt `QTranslator`
- 应用窗口和可执行文件已配置图标资源

## 说明

当前仓库尚未配置完整自动化测试，`tests/` 目录暂为预留状态。
