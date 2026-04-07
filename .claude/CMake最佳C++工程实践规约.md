# CMake 最佳 C++ 工程实践规约

本文是 **面向人阅读的详细参考文档**。

定位：

- [AGENTS_cmake.md](../AGENTS_cmake.md) 是给 AI 代理执行时遵守的**硬规范**
- 本文档是对这些规则的**展开说明、适用场景解释与实践参考**

如果两者出现表达差异，以 `AGENTS_cmake.md` 为准。

---

## 1. 为什么要先分 APP 和 SDK

很多 C++ 工程混乱，不是因为 CMake 不会写，而是因为一开始就没有先判断：

- 这是一个**自己内部使用的应用工程**？
- 还是一个**要给别人复用的库工程**？

这两类项目的目标不同，目录结构也应不同。

### APP 的目标

APP 关注的是：

- 开发效率
- 模块清晰
- 依赖简单
- 内部迭代快

APP 一般不需要对外承诺稳定 API，因此没有必要把“公开头 / 私有头”分成两套体系。

### SDK 的目标

SDK 关注的是：

- 对外接口稳定
- 安装与导出能力
- 外部项目可消费
- 内部实现可隐藏

因此 SDK 必须明确区分：

- 什么是对外 API
- 什么是内部实现细节

### 一句话判断

- **做 APP：头文件和源文件放一起。**
- **做 SDK：公开头放 include，私有头和实现放 src。**

---

## 2. APP 与 SDK 的适用场景

| 模式 | 更适合什么项目 | 目录核心特征 |
|------|----------------|--------------|
| APP | 桌面程序、CLI、服务端、内部工具、业务工程 | `.h/.cpp` 同目录 |
| SDK | 基础库、平台组件、对外分发库、公共模块 | `include/` 暴露公开头，`src/` 保存实现 |

### 什么时候别硬上 SDK 结构

如果项目只是：

- 自己团队内部使用
- 没有安装发布需求
- 不需要 `find_package()`
- 没有稳定公共 API

那就不要为了“看起来专业”强行做成 SDK。

### 什么时候应该认真做 SDK 结构

如果项目需要：

- 给别的项目复用
- 安装到系统或包管理器
- 生成 `Config.cmake`
- 导出 target
- 管理 ABI / API 边界

那就不要再沿用 APP 的随手式目录结构。

---

## 3. 现代 CMake 的核心思路

现代 CMake 的重点不是把命令写得花，而是把“**依赖、头文件、宏、编译选项**”都绑定到 target 上。

### 3.1 什么叫 Target-Based API

推荐使用：

```cmake
target_link_libraries(target PRIVATE xxx)
target_include_directories(target PUBLIC path)
target_compile_definitions(target PRIVATE USE_XXX=1)
target_compile_options(target PRIVATE -Wall)
```

这些写法的好处是：

- 作用域明确
- 谁依赖谁一眼能看懂
- 不容易污染整个工程
- 更适合多模块、大型工程

### 3.2 为什么不推荐全局写法

比如：

```cmake
include_directories(...)
link_libraries(...)
add_definitions(...)
```

问题在于：

- 所有模块都会受到影响
- 很难追踪某个头文件为什么“刚好能 include”
- 模块边界会越来越模糊
- 后期重构会非常痛苦

所以现代项目应把配置尽量挂在具体 target 上，而不是挂在全局。

---

## 4. PUBLIC / PRIVATE / INTERFACE 怎么理解

这是现代 CMake 最重要的概念之一。

### PRIVATE

只给当前 target 自己使用。

例如：

- 当前库内部用到某个第三方库
- 当前 target 的私有头路径
- 当前 target 的内部编译宏

### PUBLIC

当前 target 自己要用，依赖它的下游也要继承。

例如：

- 一个库的公开头目录
- 一个库对外 API 所依赖的头路径
- 下游也必须满足的编译特性

### INTERFACE

当前 target 不使用，但会传递给依赖它的下游。

常见于：

- header-only 库
- 纯接口 target
- 聚合型配置 target

### 判断口诀

- **只自己用：PRIVATE**
- **自己和下游都要用：PUBLIC**
- **自己不用，只给下游：INTERFACE**

---

## 5. 为什么不建议 `file(GLOB)`

很多人喜欢这样写：

```cmake
file(GLOB SRC "*.cpp" "*.h")
```

看起来省事，但长期来看问题很多：

- 新文件是否被自动纳入构建不够直观
- CMake 配置与真实源码列表脱节
- 代码审查时看不出模块到底包含哪些文件
- 大项目里容易让构建依赖关系变得隐蔽

因此，工程规范通常要求：

- 源码文件显式列出
- 模块边界可直接从 `CMakeLists.txt` 读出

这不是“保守”，而是为了工程可维护性。

---

## 6. APP 工程推荐写法

### 6.1 目录示例

```text
MyApp/
├── CMakeLists.txt
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
```

### 6.2 为什么 APP 里头源放一起更合理

因为 APP 的核心问题通常不是“对外 API 管理”，而是：

- 这个功能在哪
- 它依赖谁
- 修改时会影响哪里

把 `.h` 和 `.cpp` 放一起，可以明显降低定位和维护成本。

### 6.3 APP 里什么时候拆模块

可以拆模块的典型情况：

- 功能边界清晰
- 有独立依赖
- 能单独测试
- 被多个上层模块复用

不要为了“形式上模块化”把简单逻辑拆成很多库。

### 6.4 APP 示例 CMake

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

这里的含义很简单：

- `app_core` 这个模块的头文件就在当前目录
- 谁链接了 `app_core`，谁就能包含这里的公开头

---

## 7. SDK 工程推荐写法

### 7.1 目录示例

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
│   └── core/
│       ├── engine.cpp
│       └── engine_private.h
├── tests/
└── examples/
```

### 7.2 为什么 SDK 必须分公开头和私有头

因为 SDK 要解决的是“外部用户应该看到什么”。

如果不分层：

- 内部实现很容易泄漏出去
- API 边界会越来越不清晰
- 安装规则会混乱
- 后续版本兼容性难以管理

### 7.3 SDK 的 include 路径为什么这样写

常见写法：

```cmake
target_include_directories(mysdk
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
```

含义是：

- 构建阶段，对外公开头来自源码树中的 `include`
- 安装后，对外公开头来自安装目录下的 `include`
- 私有实现只在 `src` 内部可见

这是 SDK 与 APP 的根本差异之一。

---

## 8. 测试、示例、第三方代码怎么放

### tests/

推荐：

- 测试代码放单独的 `tests/`
- 测试直接链接正式 target
- 不要把测试辅助逻辑反向塞进正式模块

### examples/

推荐：

- SDK 才更常需要 `examples/`
- 示例应站在使用者视角
- 示例不应 include 私有头

### third_party/

推荐：

- vendor、git submodule、第三方源码单独放
- 主工程和第三方工程的 CMake 风格可以不同
- 不要因为第三方旧，就把主项目也降级为旧式写法

---

## 9. 常见误区

### 误区 1：把所有头文件都集中放到 include/

这只适合 SDK 的公开接口，不适合普通 APP。

### 误区 2：目录拆得越细越专业

错误。真正重要的是模块边界清楚，而不是目录层数多。

### 误区 3：所有编译选项都写到根 CMakeLists.txt

全局配置越多，后期模块隔离越难。

### 误区 4：`file(GLOB)` 更自动化所以更先进

工程可维护性比“少写两行”更重要。

### 误区 5：APP 也要像 SDK 一样导出一整套安装配置

没有对外分发需求时，这往往只是无效复杂度。

---

## 10. 给人的实用决策建议

如果你在新建一个 C++ 项目，可直接这样判断：

### 选 APP，如果你是：

- 做桌面程序
- 做命令行工具
- 做服务端
- 做内部业务系统
- 不打算给别人安装复用

### 选 SDK，如果你是：

- 做基础库
- 做公共组件
- 给多个项目复用
- 需要安装导出
- 需要稳定 API 边界

默认建议是：

- **能用 APP 解决，就不要提前做 SDK 化**
- **一旦明确是 SDK，就从一开始把公开边界建对**

---

## 11. 与 AGENTS_cmake.md 的分工

两份文档建议这样理解：

### [AGENTS_cmake.md](../AGENTS_cmake.md)

用途：

- 给 AI 代理直接执行
- 作为约束规则
- 决定生成代码时默认怎么做

特点：

- 结论明确
- 规则优先
- 偏执行导向

### 本文档

用途：

- 给人阅读
- 解释为什么这样规定
- 作为培训、沟通、评审参考

特点：

- 强调背景与取舍
- 便于团队统一理解
- 偏说明导向

---

## 12. 最后的建议

真正好的 CMake 工程，不在于命令多，而在于三件事：

1. **项目模式选对**
2. **模块边界清楚**
3. **配置作用域明确**

如果这三点做对了：

- APP 会保持简单直接
- SDK 会保持边界清晰
- AI 和人都更容易继续维护这个工程
