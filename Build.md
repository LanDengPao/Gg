# Build

本文说明当前 `Gg` 项目的构建方式、依赖要求、构建入口与常见输出结果。

---

## 1. 项目构建概览

当前项目使用：

- **CMake** 作为构建系统
- **Qt Widgets** 作为 UI 框架
- **MSVC / Visual Studio 生成器** 作为主要 Windows 构建环境
- **C++23** 作为当前语言标准

项目最终会生成：

- `GgCore`：核心静态库
- `GgUi`：UI 静态库
- `Gg`：Windows 图形界面可执行程序

---

## 2. 构建入口文件

当前构建入口如下：

- 根构建文件：[CMakeLists.txt](../CMakeLists.txt)
- 全局配置文件：[cmake/config.cmake](../cmake/config.cmake)
- 核心模块构建文件：[src/core/CMakeLists.txt](../src/core/CMakeLists.txt)
- UI 模块构建文件：[src/ui/CMakeLists.txt](../src/ui/CMakeLists.txt)

它们的职责分别是：

### 根 `CMakeLists.txt`

负责：

- 定义项目 `Gg`
- 引入全局配置
- 查找 Qt Widgets
- 添加 `src/core` 与 `src/ui` 子目录
- 创建最终可执行程序 `Gg`
- 链接 `GgUi`、`GgCore` 与 Qt 依赖

### `cmake/config.cmake`

负责：

- 设置 C++23
- 开启 Qt 自动 `moc` / `uic` / `rcc`
- 配置 MSVC 编译选项
- 配置输出目录

### `src/core/CMakeLists.txt`

负责构建静态库 `GgCore`，包含：

- 类型定义
- 通用线程工具
- 核心业务逻辑
- 数据访问逻辑
- 设备访问逻辑

### `src/ui/CMakeLists.txt`

负责构建静态库 `GgUi`，包含：

- 图形控件
- UI 相关类
- 对 `GgCore` 的界面层依赖

---

## 3. 依赖要求

要成功构建当前项目，至少需要以下环境：

### 3.1 基础工具

- CMake 3.21 或更高版本
- 支持 C++23 的编译器

### 3.2 Windows 构建环境

当前配置明显偏向 Windows：

- Visual Studio / MSVC
- Windows SDK
- `Setupapi` 系统库

### 3.3 Qt

需要安装可被 CMake 发现的 Qt：

- Qt5 或 Qt6
- 至少包含 `Widgets`
- 实际代码中也链接了 `Core`

根构建文件当前使用：

```cmake
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
```

说明项目兼容 Qt5 / Qt6 的查找方式，但当前重点仍是 Qt Widgets 桌面程序。

---

## 4. 当前构建流程

从逻辑上看，构建顺序如下：

1. CMake 读取根 [CMakeLists.txt](../CMakeLists.txt)
2. 引入 [cmake/config.cmake](../cmake/config.cmake)
3. 设置编译标准、Qt 自动处理和输出目录
4. 查找 Qt Widgets 依赖
5. 构建 `GgCore`
6. 构建 `GgUi`
7. 收集 `src/ui/app/*.cpp` 应用层源码
8. 生成最终可执行程序 `Gg`
9. 链接 Qt、`GgUi`、`GgCore`、`Setupapi`

---

## 5. 推荐构建方式

下面给出在 Windows 上较常见的构建方式。

### 5.1 使用 Visual Studio 生成器

```bash
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Debug
```

如需 Release：

```bash
cmake --build build --config Release
```

### 5.2 使用 Ninja（前提是环境已正确配置）

如果本机已正确配置 MSVC、Qt 和 Ninja，也可以：

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

但从当前配置看，项目更自然的目标环境仍是 Windows + MSVC。

---

## 6. 输出目录说明

[cmake/config.cmake](../cmake/config.cmake) 已统一设置输出目录：

- 静态库 / 动态库输出到 `build/lib/`
- 可执行文件输出到 `build/bin/`

对于多配置生成器（如 Visual Studio），会进一步分目录，例如：

```text
build/
├── bin/
│   ├── Debug/
│   │   └── Gg.exe
│   └── Release/
│       └── Gg.exe
└── lib/
    ├── Debug/
    └── Release/
```

这能避免不同配置的产物互相覆盖。

---

## 7. Qt 自动处理说明

项目在 [cmake/config.cmake](../cmake/config.cmake) 中开启了：

```cmake
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
```

这意味着：

- `QObject` / `Q_OBJECT` 相关代码会自动处理 `moc`
- `.ui` 文件会自动处理 `uic`
- `.qrc` 文件会自动处理 `rcc`

因此当前 `src/ui/app/` 中的：

- `Gg.ui`
- `Gg.qrc`

可以直接参与构建，无需再手工编写额外 Qt 生成命令。

---

## 8. 当前构建配置的特点与注意点

### 8.1 当前项目是 GUI 程序

根构建文件中：

```cmake
add_executable(Gg WIN32)
```

说明这是一个 Windows 图形界面程序，而不是普通控制台程序。

同时还设置了：

```cmake
set_target_properties(Gg PROPERTIES
    AUTOMOC TRUE
    WIN32_EXECUTABLE TRUE
)
```

与 Qt Widgets 项目定位一致。

### 8.2 当前应用层源码通过 GLOB 收集

根构建文件中目前使用：

```cmake
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "src/ui/app/*.cpp")
```

优点：

- 应用层新增 `.cpp` 时，维护成本较低

注意点：

- 源文件列表不够显式
- 长期维护时不如手工列出清晰
- 与当前文档中强调的显式声明原则并不完全一致

如果后续继续收紧工程规范，建议逐步改成显式 `target_sources()`。

### 8.3 `GgCore` 当前 include 暴露较宽

[src/core/CMakeLists.txt](../src/core/CMakeLists.txt) 中当前公开暴露了多个子目录：

- `types`
- `common`
- `data`
- `device`

这有利于当前快速开发，但也意味着模块边界还可以继续收紧。

后续如果核心层继续细分，可考虑按职责进一步缩小 include 暴露范围。

---

## 9. 当前没有完整测试目标

根构建文件已启用：

```cmake
enable_testing()
```

但当前仓库里还没有看到完整的 `tests/` 构建入口和测试 target。

这意味着：

- 工程已经预留测试能力
- 但测试体系目前尚未完全接入

后续若补测试，建议：

- 新建 `tests/`
- 为核心逻辑优先补测试
- 让测试直接链接 `GgCore` 或进一步拆分后的核心模块

---

## 10. 常见构建问题排查方向

如果构建失败，优先检查以下几点：

### 10.1 Qt 未被正确发现

表现：

- `find_package(QT ...)` 失败
- 找不到 `Qt5Widgets` 或 `Qt6Widgets`

检查方向：

- Qt 是否安装完整
- Qt 的 CMake package 路径是否已加入环境或显式指定
- 当前生成器和 Qt 编译器是否匹配

### 10.2 MSVC / Windows SDK 环境不完整

表现：

- 找不到编译器
- 找不到系统头或系统库
- `Setupapi` 链接失败

检查方向：

- Visual Studio C++ workload 是否安装
- Windows SDK 是否可用
- 是否在正确的开发者命令行环境中运行 CMake

### 10.3 Qt 自动生成文件异常

表现：

- `moc` / `uic` / `rcc` 相关错误
- `.ui` 或 `.qrc` 未被正确处理

检查方向：

- 文件是否已纳入 target
- Qt 包是否完整
- 头文件中的 Qt 宏和类声明是否正确

---

## 11. 总结

当前 `Gg` 项目的构建系统具备以下特点：

- 使用现代 CMake 组织模块
- 用 `config.cmake` 集中管理全局配置
- 用 `GgCore` 和 `GgUi` 进行初步模块拆分
- 以 Qt Widgets 为核心构建桌面应用
- 已具备继续完善测试和收紧模块边界的空间

如果你要快速理解构建链路，建议按下面顺序阅读：

1. [CMakeLists.txt](../CMakeLists.txt)
2. [cmake/config.cmake](../cmake/config.cmake)
3. [src/core/CMakeLists.txt](../src/core/CMakeLists.txt)
4. [src/ui/CMakeLists.txt](../src/ui/CMakeLists.txt)
