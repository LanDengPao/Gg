cmake_minimum_required(VERSION 3.21)

project(Gg VERSION 0.2.0 LANGUAGES CXX)

include(cmake/config.cmake)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_subdirectory(lib/types)
add_subdirectory(lib/common)
add_subdirectory(lib/data)
add_subdirectory(lib/core)
add_subdirectory(lib/device)
add_subdirectory(lib/ui)

file(GLOB_RECURSE LIB_HEADERS CONFIGURE_DEPENDS "include/Gg/*.h")

add_executable(Gg WIN32)

target_sources(Gg PRIVATE
    "src/main.cpp"
    "src/Gg.cpp"
    "src/AppController.cpp"
    ${LIB_HEADERS}
)

set_target_properties(Gg PROPERTIES 
    AUTOMOC TRUE
    WIN32_EXECUTABLE TRUE
)

target_compile_features(Gg PRIVATE cxx_std_23)

target_include_directories(Gg PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Gg
)
target_include_directories(Gg PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(Gg PRIVATE 
    GgUi
    GgDevice
    GgCore
    GgData
    GgCommon
    GgTypes
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Setupapi
)

enable_testing()
