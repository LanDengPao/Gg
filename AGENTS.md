# AGENTS.md - Gg 项目开发指南

本文件供 AI 编码代理使用，提供构建、编码规范和项目结构指导。

项目源码组织与架构设计：AGENTS_architecture.md
项目配置与构建规范：AGENTS_cmake.md
代码规范：代码生成规范.md


中文交流环境
---

## 1. 构建命令

### CMake Presets
```bash
# Debug 配置
cmake --preset debug
cmake --build --preset build-debug

# Release 配置
cmake --preset release
cmake --build --preset build-release

# 清理构建
Remove-Item -Recurse -Force build/
```

### 直接构建
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

---

## 2. 项目结构

```
E:\repo\Gg\
├── CMakeLists.txt           # 根级构建配置（Target-Based）
├── CMakePresets.json        # 开发/发布预设
├── cmake/
│   └── config.cmake         # 全局编译选项
├── include/Gg/              # 公共头文件（9个.h）
│   ├── AppTypes.h          # 类型定义
│   ├── AppController.h     # 控制器
│   ├── Gg.h                # 主窗口
│   └── ...
└── src/                    # 源文件（.cpp/.ui/.qrc）
    ├── main.cpp
    └── ...
```

---

## 3. 代码风格指南

### 3.1 头文件
- 使用 `#pragma once` 而非 include guard
- 按标准库、第三方库、项目顺序分组
- Qt 头文件使用全路径: `#include <QString>`

### 3.2 命名约定
| 类型 | 规则 | 示例 |
|------|------|------|
| 类/结构体 | PascalCase | `AppController`, `DeviceInfo` |
| 函数/方法 | camelCase | `startTest()`, `ingestSample()` |
| 成员变量 | m_前缀 + camelCase | `m_controller`, `m_isRecording` |
| 常量 | k前缀 + camelCase | `kSampleMagic` |
| 枚举值 | PascalCase | `TestMode::PollingRate` |

### 3.3 类型系统
- 优先使用强类型：`enum class` 而非 `enum`
- Qt 类型：`QString`, `QVector`, `QPoint`, `qint64`
- 禁止 `int` 用于明确宽度的场景（如文件句柄）

### 3.4 格式化
- 大括号：同一行开
- 缩进：4空格
- 行长度：无硬限制，控制在120字符内
- 指针/引用：`*` 和 `&` 靠近类型名

### 3.5 导入规则
```cpp
// 正确
#include "AppController.h"
#include <QString>
#include <QVector>

// 错误
#include "QString"  // 不是本地文件
#include <AppController.h>  // 不是系统头
```

### 3.6 Qt 特定规则
- Q_OBJECT 宏必须在类的 private/protected 区
- 信号/槽使用 `signals:` 和 `public slots:` 或现代语法
- 使用 `emit` 显式发送信号

---

## 4. 错误处理

### 4.1 错误返回模式
```cpp
bool doSomething(QString* error = nullptr) {
    if (failed) {
        if (error) {
            *error = "具体错误信息";
        }
        return false;
    }
    return true;
}
```

### 4.2 异常使用
- 禁止使用 C++ 异常（项目未启用）
- 使用错误码模式或 Qt 信号槽传播错误

---

## 5. CMake 规范（Target-Based）

### 5.1 依赖管理
```cmake
# 正确：使用 Target-Based API
target_link_libraries(Gg PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_include_directories(Gg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/Gg)

# 错误：全局污染
link_libraries(...)
include_directories(...)
```

### 5.2 可见性
- `PRIVATE`: 仅当前 Target 使用
- `PUBLIC`: 当前和依赖方都需要
- `INTERFACE`: 仅依赖方使用（头文件库）

### 5.3 C++ 标准
- 使用 `target_compile_features(Gg PRIVATE cxx_std_23)`
- 避免 `CMAKE_CXX_STANDARD` 全局变量

---

## 6. 测试

项目当前未配置测试框架。如需添加，推荐：
- 单元测试: Qt Test (`find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test)`)
- 测试目录: `tests/` 子目录

---

## 7. Git 提交规范

- 使用英文提交信息
- 格式: `<type>: <description>`
- Type: `feat`, `fix`, `refactor`, `docs`, `build`
- 示例: `feat: add polling rate detection`

---

## 8. 注意事项

- 本项目为 Windows 桌面应用（Qt Widgets）
- 构建输出: `build/bin/Debug/Gg.exe`
- 禁止引入运行时依赖（除 Qt 外）
- 禁止修改 `.gitignore` 中的构建产物规则
