# AGENTS_cmake.md - 通用 CMake/C++ 工程规范

本文件供 AI 编码代理使用，用于约束 **CMake 写法、源码目录组织、模块边界** 与 **生成代码时的默认决策**。

目标：让 AI 在不同 C++ 项目中都能输出 **一致、可维护、可扩展、符合现代 CMake 思路** 的工程结构，而不是临时拼接脚本式 CMake。

---

## 1. 适用范围

本规范适用于绝大多数 CMake C++ 项目，尤其是：

- 桌面应用
- CLI 工具
- 服务端程序
- 内部业务工程
- 对外发布的 SDK / 组件库

本规范只定义两类工程模式，必须先判定再落地：

1. **业务项目（APP）**
2. **库项目（SDK）**

**禁止混用两套目录哲学。**

一句话原则：

- **做 APP：头文件和源文件放一起。**
- **做 SDK：公开头放 include，私有头和实现放 src。**

---

## 2. 先判定项目模式

### 2.1 模式选择标准

| 模式 | 适用场景 | 头文件布局 | 对外暴露接口 |
|------|----------|------------|--------------|
| **APP** | 自用应用、内部工具、服务、客户端、非对外发布工程 | `.h` / `.cpp` 同目录 | 通常无稳定公开 API |
| **SDK** | 对外发布库、组件、需被 `find_package` / `add_subdirectory` / 包管理器消费 | 公开头在 `include/<libname>/`，私有头在 `src/` | 有明确稳定边界 |

### 2.2 判定原则

如果项目满足下面任意一点，优先视为 **SDK**：

- 需要安装（`install()`）
- 需要导出 target
- 需要生成 `Config.cmake` / `Targets.cmake`
- 需要让外部项目通过 `find_package()` 使用
- 需要区分公开 API 与内部实现

否则，默认视为 **APP**。

### 2.3 AI 默认决策

当用户没有明确说明时：

- **内部业务工程 / 桌面应用 / 命令行工具 / 服务端程序** → 默认按 **APP** 处理
- **组件库 / SDK / 可复用基础库** → 默认按 **SDK** 处理

如果代码仓库已存在明确结构，**优先延续现有结构**，不要强行改造。

---

## 3. 通用原则

无论 APP 还是 SDK，都必须遵守下面规则。

### 3.1 只使用 Target-Based API

优先使用：

- `target_link_libraries()`
- `target_include_directories()`
- `target_compile_definitions()`
- `target_compile_options()`
- `target_compile_features()`
- `set_target_properties()`

避免或禁止：

- `include_directories()`
- `link_libraries()`
- `add_definitions()`
- 全局改写 `CMAKE_CXX_FLAGS`

### 3.2 明确依赖可见性

必须正确区分：

- `PRIVATE`：仅当前 target 自己使用
- `PUBLIC`：当前 target 与依赖它的下游都可见
- `INTERFACE`：当前 target 不编译该属性，只传递给下游

### 3.3 禁止偷懒式收集源码

禁止：

- `file(GLOB ...)`
- `aux_source_directory(...)`

要求：

- 源文件、头文件尽量**显式列出**
- 目录结构应能从 CMake 中直接读出

### 3.4 目录组织服从模块边界

- 一个功能模块对应一个目录
- 一个可独立复用的模块，通常对应一个 target
- CMake 分层要反映真实依赖，不要为拆分而拆分
- 不要制造没有意义的深层 `add_subdirectory()` 嵌套

### 3.5 优先局部配置，避免全局污染

推荐：

- 在具体 target 上设置 include、宏、编译选项、链接依赖

避免：

- 根目录一次性把所有头路径暴露给全项目
- 一个模块无意中依赖另一个模块的“顺便可见”头文件

### 3.6 不为“看起来高级”而过度设计

- 小项目不要强行拆几十个库
- 单次使用的代码不要过早抽象成公共模块
- 没有对外发布需求时，不必引入 SDK 式公开/私有头拆分

### 3.7 保持源码与构建描述同步

AI 修改工程时必须同步更新：

- 新增 `.cpp/.h` → 对应 target 的 `CMakeLists.txt`
- 新增模块目录 → 父目录 `add_subdirectory()`
- 新增链接依赖 → 对应 target 的 `target_link_libraries()`
- 新增公开头（SDK）→ `install()` / 导出配置

---

## 4. 业务项目（APP）规范

### 4.1 适用场景

适合：

- Qt / MFC / 控制台应用
- 后端服务
- 内部工具
- 不打算作为公共库发布的工程

### 4.2 目录规则

**核心原则：头文件与实现文件永远放在一起。**

推荐结构：

```text
Project/
├── CMakeLists.txt
├── CMakePresets.json
├── src/
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── common/
│   │   ├── CMakeLists.txt
│   │   ├── log.cpp
│   │   └── log.h
│   ├── core/
│   │   ├── CMakeLists.txt
│   │   ├── engine.cpp
│   │   └── engine.h
│   └── ui/
│       ├── CMakeLists.txt
│       ├── main_window.cpp
│       └── main_window.h
└── tests/
    ├── CMakeLists.txt
    └── test_xxx.cpp
```

### 4.3 APP 模式硬规则

- `.h` 和 `.cpp` 同目录
- 不单独建立全局 `include/` 来存放业务头文件
- 模块之间直接通过模块目录暴露头文件
- 每个模块目录可有自己的 `CMakeLists.txt`
- 主程序 target 负责汇总链接内部模块

### 4.4 APP 模式 CMake 参考

**根 CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src)
add_subdirectory(tests)
```

**src/CMakeLists.txt**

```cmake
add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(ui)

add_executable(my_app main.cpp)

target_link_libraries(my_app
  PRIVATE
    app_common
    app_core
    app_ui
)
```

**模块示例 `src/core/CMakeLists.txt`**

```cmake
add_library(app_core
  engine.cpp
  engine.h
)

target_include_directories(app_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
```

### 4.5 APP 模式下 AI 的默认行为

当用户让 AI 新增业务模块时，应默认：

1. 在 `src/<module>/` 下创建 `.cpp + .h`
2. 在该目录创建或更新 `CMakeLists.txt`
3. 在上层 `src/CMakeLists.txt` 增加 `add_subdirectory(<module>)`
4. 在最终可执行程序或上层模块上增加链接关系

---

## 5. 库项目（SDK）规范

### 5.1 适用场景

适合：

- 对外发布的组件库
- 需要稳定 API 的基础库
- 需要安装、导出、版本化、供外部消费的工程

### 5.2 目录规则

**核心原则：公开头与私有实现严格分离。**

推荐结构：

```text
MySDK/
├── CMakeLists.txt
├── cmake/
│   └── Config.cmake.in
├── include/mysdk/
│   ├── mysdk.h
│   └── core/
│       └── engine.h
├── src/
│   ├── CMakeLists.txt
│   ├── mysdk.cpp
│   ├── core/
│   │   ├── engine.cpp
│   │   └── engine_private.h
│   └── network/
│       ├── client.cpp
│       └── client_private.h
├── tests/
└── examples/
```

### 5.3 SDK 模式硬规则

- **公开头** 放 `include/<libname>/`
- **私有头** 和 `.cpp` 一起放 `src/`
- 外部用户只能依赖公开头
- `install()` 只安装公开头与公开 target
- 目录命名、导出 target、包配置命名要稳定

### 5.4 SDK 模式 CMake 参考

**根 CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.20)
project(MySDK VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(examples)
```

**src/CMakeLists.txt**

```cmake
add_library(mysdk
  mysdk.cpp
  core/engine.cpp
  network/client.cpp
)

add_library(MySDK::mysdk ALIAS mysdk)

target_include_directories(mysdk
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
```

**安装示例**

```cmake
install(TARGETS mysdk
  EXPORT MySDKTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/mysdk
  DESTINATION include
)
```

### 5.5 SDK 模式下 AI 的默认行为

当用户让 AI 新增对外 API 时，应默认：

1. 把公开声明放入 `include/<libname>/...`
2. 把实现与私有细节放入 `src/...`
3. 更新 `target_include_directories()` 与源码列表
4. 如涉及安装能力，同步更新 `install()`
5. 如涉及包分发，同步更新导出与 `Config.cmake` 相关配置

---

## 6. 测试、示例与第三方目录建议

### 6.1 tests/

- 测试代码放 `tests/`
- 测试 target 不应反向污染正式模块
- 优先链接被测 target，而不是复制源码

### 6.2 examples/

- 仅在 SDK 或具备演示价值的工程中使用
- 示例代码是消费者视角，不应依赖私有头

### 6.3 third_party/

- 第三方源码、子模块、vendor 代码单独放置
- 不要把第三方布局与主工程布局混在一起
- 不要因为第三方项目使用旧式 CMake，就把主工程也写成旧式

---

## 7. 现代 CMake 实践建议

### 7.1 标准与特性

优先：

```cmake
target_compile_features(mytarget PUBLIC cxx_std_17)
```

如果项目已统一采用根级设置，也可使用：

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

### 7.2 编译选项

推荐把警告、优化、特定编译器选项绑定到 target，而不是全局散射：

```cmake
target_compile_options(mytarget PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)
```

### 7.3 宏定义

使用：

```cmake
target_compile_definitions(mytarget PRIVATE MYTARGET_USE_FAST_PATH=1)
```

避免：

- `add_definitions(...)`
- 无边界的全局宏注入

### 7.4 生成器表达式

在安装接口、平台差异、配置差异场景下可使用生成器表达式，但不要滥用；可读性优先。

---

## 8. SDK 进阶实践

以下内容主要适用于 SDK，不强制用于普通 APP。

### 8.1 符号导出控制

共享库建议显式控制导出符号：

```cmake
include(GenerateExportHeader)
generate_export_header(mysdk
  EXPORT_FILE_NAME ${PROJECT_SOURCE_DIR}/include/mysdk/export.hpp
)
```

需要时可配合：

```cmake
set_target_properties(mysdk PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)
```

### 8.2 find_package 支持

如需被外部工程消费，应提供：

- `*Config.cmake`
- `*ConfigVersion.cmake`
- `*Targets.cmake`

示例：

```cmake
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/MySDKConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
```

### 8.3 静态 / 共享切换

如需支持切换，可通过项目选项驱动：

```cmake
option(MySDK_BUILD_SHARED_LIBS "Build shared library" ON)

if(DEFINED MySDK_BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ${MySDK_BUILD_SHARED_LIBS})
endif()
```

### 8.4 可选组件

推荐显式开关：

```cmake
option(MySDK_BUILD_TESTS "Build tests" OFF)
option(MySDK_BUILD_EXAMPLES "Build examples" OFF)
option(MySDK_BUILD_DOCS "Build docs" OFF)
```

---

## 9. AI 操作清单

AI 在修改 CMake/C++ 工程时，按下面顺序执行：

1. **先判断项目是 APP 还是 SDK**
2. **遵循现有目录风格，不擅自翻案**
3. **新增源码时同步更新对应 `CMakeLists.txt`**
4. **依赖只挂到真正需要的 target 上**
5. **检查可见性是否应为 PRIVATE / PUBLIC / INTERFACE**
6. **避免全局 include / link / definitions 污染**
7. **避免 file(GLOB) 与无边界脚本式 CMake**
8. **若是 SDK，检查公开头、安装、导出是否同步**

---

## 10. AI 禁止事项

AI 在编写或修改本类工程时，默认禁止：

- 禁止把 APP 项目强改成 SDK 结构
- 禁止把 SDK 项目强改成 APP 结构
- 禁止把所有头文件集中挪到一个公共目录，除非该项目本来就是 SDK
- 禁止使用 `include_directories()`、`link_libraries()`、`add_definitions()` 作为默认方案
- 禁止使用 `file(GLOB)`、`aux_source_directory()` 偷懒收集源码
- 禁止无理由引入多层级、无实际价值的 `add_subdirectory()` 嵌套
- 禁止因为“看起来规范”而额外创建不必要模块
- 禁止新增与用户当前模式不一致的目录结构

---

## 11. 给 AI 的简短执行口令

每次对话如需快速约束，可直接应用以下规则：

```text
以后所有 CMake/C++ 代码遵守以下规范：
先判断项目是 APP 还是 SDK；
APP 模式下 .h 和 .cpp 放一起；
SDK 模式下公开头放 include/<libname>/，私有头和实现放 src/；
优先使用 Target-Based API；
避免 include_directories、link_libraries、add_definitions、file(GLOB)；
所有源码和依赖尽量显式声明。
```

---

## 12. 最终总结

这份规范只有三条真正核心的落地原则：

1. **先分清 APP 还是 SDK，再决定目录结构**
2. **始终优先使用现代 Target-Based CMake**
3. **让目录、模块、依赖关系保持一致，不搞全局污染和伪抽象**

如果没有额外要求，AI 应优先生成：

- 对 APP：简单直接、模块清晰、头源同目录的工程
- 对 SDK：公开边界明确、可安装、可导出的工程
