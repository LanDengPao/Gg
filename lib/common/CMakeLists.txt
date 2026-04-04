cmake_minimum_required(VERSION 3.21)

add_library(GgCommon STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/LambdaThread.cpp
)
target_include_directories(GgCommon PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include/Gg>
    $<INSTALL_INTERFACE:include/Gg>
)
target_link_libraries(GgCommon PUBLIC Qt${QT_VERSION_MAJOR}::Core)
