# Build

本文说明当前 `Gg` 项目的构建、安装和打包方式，以及发布包的依赖策略。

---

## 1. 构建概览

当前项目使用：

- `CMake` 作为构建系统
- `Qt Widgets` 作为桌面 UI 框架
- `Qt LinguistTools` 生成翻译文件
- `Visual Studio 17 2022` 作为主要 Windows 生成器
- `C++23` 作为当前语言标准

最终产物包括：

- `GgCore`：核心静态库
- `GgUi`：UI 静态库
- `Gg`：Windows 图形界面程序
- `Gg_zh_CN.qm`：中文翻译文件
- `Gg-<version>-win64.zip`：发布压缩包

---

## 2. 构建入口

当前主要入口文件：

- 根构建文件：`CMakeLists.txt`
- 全局配置：`cmake/config.cmake`
- 核心模块：`src/core/CMakeLists.txt`
- UI 模块：`src/ui/CMakeLists.txt`
- 预设配置：`CMakePresets.json`

其中根 `CMakeLists.txt` 当前负责：

- 查找 Qt Widgets 和 LinguistTools
- 添加 `src/core` 与 `src/ui` 子目录
- 构建最终可执行程序 `Gg`
- 生成 `.qm` 翻译文件
- 配置 `install()` 规则
- 调用 `windeployqt` 部署 Qt 运行时
- 通过 `CPack` 生成 ZIP 发布包

---

## 3. 依赖要求

### 3.1 基础工具

- CMake 3.21+
- Visual Studio 2022
- Windows SDK

### 3.2 Qt

需要安装可被 CMake 发现的 Qt，至少包含：

- `Widgets`
- `LinguistTools`

当前预设默认使用：

```json
"CMAKE_PREFIX_PATH": "D:/QT/QT5.14/5.14.2/msvc2017_64"
```

根构建文件当前使用：

```cmake
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)
```

也就是说工程仍保留 Qt5 / Qt6 的查找兼容性，但当前预设实际指向 Qt 5.14.2。

---

## 4. 预设说明

当前 `CMakePresets.json` 已提供以下常用预设：

### 4.1 Configure Presets

- `debug`
- `release`

### 4.2 Build Presets

- `build-debug`
- `build-release`
- `install-debug`
- `install-release`
- `package-release`

其中：

- `build-*` 用于正常编译
- `install-*` 用于把程序安装到 `build/install/<Config>/`
- `package-release` 用于一键生成发布压缩包

---

## 5. 常用命令

### 5.1 Debug 构建

```powershell
cmake --preset debug
cmake --build --preset build-debug
```

### 5.2 Release 构建

```powershell
cmake --preset release
cmake --build --preset build-release
```

### 5.3 安装到本地目录

```powershell
cmake --build --preset install-debug
cmake --build --preset install-release
```

默认安装输出：

- `build/install/Debug/`
- `build/install/Release/`

### 5.4 一键打包发布包

```powershell
cmake --preset release
cmake --build --preset package-release
```

生成位置：

- `build/packages/Gg-0.2.0-win64.zip`

---

## 6. 输出目录

当前输出目录结构如下：

```text
build/
├── bin/
│   ├── Debug/
│   │   └── Gg.exe
│   └── Release/
│       └── Gg.exe
├── install/
│   ├── Debug/
│   └── Release/
├── lib/
│   ├── Debug/
│   └── Release/
└── packages/
    └── Gg-0.2.0-win64.zip
```

此外：

- 翻译文件会出现在 `build/bin/<Config>/translations/`
- 安装结果会出现在 `build/install/<Config>/`

---

## 7. Qt 自动处理

项目在 `cmake/config.cmake` 中开启了：

```cmake
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
```

因此：

- `.ui` 会自动走 `uic`
- `.qrc` 会自动走 `rcc`
- 含 `Q_OBJECT` 的类会自动走 `moc`

当前应用层中直接受益的文件包括：

- `src/ui/app/Gg.ui`
- `src/ui/app/Gg.qrc`

修改这些文件后，重新构建即可。

---

## 8. 翻译文件生成

当前项目使用 Qt 官方翻译机制。

翻译源文件：

- `src/ui/app/i18n/Gg_zh_CN.ts`

构建时会自动生成：

- `Gg_zh_CN.qm`

如果修改了源代码里的可翻译文本，建议先运行：

```powershell
& "D:/QT/QT5.14/5.14.2/msvc2017_64/bin/lupdate.exe" "E:/repo/Gg/src" -noobsolete -ts "E:/repo/Gg/src/ui/app/i18n/Gg_zh_CN.ts"
```

然后再重新构建或重新打包。

---

## 9. 安装与打包策略

### 9.1 install

当前根构建文件已配置 `install()` 规则，安装内容包括：

- `Gg.exe`
- `translations/Gg_zh_CN.qm`
- `README.md`
- MSVC 运行库

同时在 Windows 下会额外调用 `windeployqt`，自动补齐 Qt 运行时和必要插件。

### 9.2 package

当前项目已接入 `CPack`，并使用：

```cmake
set(CPACK_GENERATOR "ZIP")
```

因此 `package-release` 会直接生成 ZIP 发布包。

### 9.3 发布包依赖策略

当前发布包的目标是：

- 在 `Windows 10 / Windows 11` 上最小依赖运行

具体策略是：

- 打包 Qt 运行时与必要插件
- 打包 MSVC 运行库，如 `vcruntime140.dll`、`msvcp140.dll`
- 不再打包 Win10/Win11 已自带的 `ucrtbase.dll` 和 `api-ms-win-*` 系统运行库

这意味着当前发布包默认不追求向更旧 Windows 系统兼容。

---

## 10. 当前发布包内容

当前 `package-release` 生成的包中，通常可以看到以下内容：

- `Gg.exe`
- `Qt5Core.dll`
- `Qt5Gui.dll`
- `Qt5Widgets.dll`
- `platforms/qwindows.dll`
- `imageformats/*`
- `styles/qwindowsvistastyle.dll`
- `translations/Gg_zh_CN.qm`
- `vcruntime140.dll`
- `vcruntime140_1.dll`
- `msvcp140.dll`

这是一份可直接分发的 Release ZIP，而不是仅供开发机运行的裸可执行文件。

---

## 11. 常见问题

### 11.1 Qt 未被正确发现

表现：

- `find_package(QT ...)` 失败
- 找不到 `Qt5Widgets` 或 `Qt6Widgets`

检查方向：

- `CMAKE_PREFIX_PATH` 是否正确
- Qt 安装是否包含 `Widgets` 和 `LinguistTools`
- 当前生成器与 Qt 工具链是否匹配

### 11.2 `.ui` / `.qrc` 没有更新

检查方向：

- 是否已重新构建
- 文件是否被纳入当前 target
- 是否仍在使用旧生成目录中的缓存产物

### 11.3 打包后在目标机缺 DLL

优先检查：

- 是否使用 `package-release` 生成发布包，而不是直接拷贝 `build/bin/Release/Gg.exe`
- `windeployqt` 是否能在当前 Qt 安装中正常找到
- 目标机器是否为 Windows 10 / 11

### 11.4 `windeployqt` 警告 `VCINSTALLDIR is not set`

当前已通过安装 MSVC 运行库兜底，发布包仍可正常使用。

---

## 12. 总结

当前 `Gg` 的构建链路已经具备：

- 正常的 Debug / Release 构建
- 本地安装目录输出
- 基于 `windeployqt` 的 Qt 运行时部署
- 基于 `CPack` 的一键 ZIP 打包
- 面向 Windows 10 / 11 的最小依赖发布包策略

如果你的目标是直接生成可分发产物，优先使用：

```powershell
cmake --preset release
cmake --build --preset package-release
```
