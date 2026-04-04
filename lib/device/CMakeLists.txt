cmake_minimum_required(VERSION 3.21)

add_library(GgDevice STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/WinDeviceInfo.cpp
)
target_include_directories(GgDevice PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include/Gg>
    $<INSTALL_INTERFACE:include/Gg>
)
target_link_libraries(GgDevice PUBLIC 
    GgTypes
    Qt${QT_VERSION_MAJOR}::Core
    Setupapi
)
