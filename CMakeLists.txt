cmake_minimum_required(VERSION 3.21)

# 项目元数据
project(Gg VERSION 0.2.0 LANGUAGES CXX)

# 加载全局配置
include(cmake/config.cmake)

# Qt 依赖查找
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# 主库源码
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# 头文件（用于 MOC 自动生成）
file(GLOB_RECURSE LIB_HEADERS CONFIGURE_DEPENDS "include/Gg/*.h")

# 主程序
add_executable(Gg WIN32 "src/main.cpp" ${LIB_SOURCES} ${LIB_HEADERS})

# C++ 标准
target_compile_features(Gg PRIVATE cxx_std_23)

# 头文件搜索路径（PUBLIC 让依赖也能使用）
target_include_directories(Gg PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(Gg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/Gg)

# 链接库
target_link_libraries(Gg PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets 
    Qt${QT_VERSION_MAJOR}::Core
    Setupapi
)

# Windows 特定设置
set_target_properties(Gg PROPERTIES WIN32_EXECUTABLE TRUE)

# 测试（当前未实际使用）
enable_testing()