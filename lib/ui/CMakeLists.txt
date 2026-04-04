cmake_minimum_required(VERSION 3.21)

add_library(GgUi STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/TrajectoryWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/SparklineWidget.cpp
)
target_include_directories(GgUi PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include/Gg>
    $<INSTALL_INTERFACE:include/Gg>
)
target_link_libraries(GgUi PUBLIC 
    GgTypes
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
)
