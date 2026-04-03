# Gg

这是一个基于 C++ / Qt Widgets 的 Windows 桌面应用，当前已切换为纯 CMake 构建。

## 环境要求

- Windows
- Visual Studio 2022
- CMake 3.16 及以上
- Qt 5 或 Qt 6

当前仓库内的 [CMakePresets.json](E:\repo\Gg\CMakePresets.json) 已配置本机 Qt 路径：

```text
D:/QT/QT5.14/5.14.2/msvc2017_64
```

如果你的 Qt 安装目录不同，需要先修改该文件中的 `CMAKE_PREFIX_PATH`。

## 构建方式

### 使用 CMake Presets

配置 Debug：

```powershell
cmake --preset debug
```

编译 Debug：

```powershell
cmake --build --preset build-debug
```

配置 Release：

```powershell
cmake --preset release
```

编译 Release：

```powershell
cmake --build --preset build-release
```

### 不使用 Presets

也可以直接手动指定 Qt 路径：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="D:/QT/QT5.14/5.14.2/msvc2017_64"
cmake --build build --config Debug
```

## 输出目录

- Debug 可执行文件默认位于 `build/Debug/Debug/Gg.exe`
- Release 可执行文件默认位于 `build/Release/Release/Gg.exe`

## 说明

- 项目已启用 `CMAKE_AUTOMOC`
- 项目已启用 `CMAKE_AUTOUIC`
- 项目已启用 `CMAKE_AUTORCC`

因此 `Gg.h`、`Gg.ui`、`Gg.qrc` 等 Qt 相关文件会由 CMake 自动处理，无需再维护旧的 Visual Studio Qt 工程文件。
