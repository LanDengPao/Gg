cmake_minimum_required(VERSION 3.21)

add_library(GgUi STATIC
    TrajectoryWidget.h
    TrajectoryWidget.cpp
    SparklineWidget.h
    SparklineWidget.cpp
)

gg_configure_target(GgUi)

target_include_directories(GgUi PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(GgUi PUBLIC 
    GgCore
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
)
