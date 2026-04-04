cmake_minimum_required(VERSION 3.16)

project(Gg VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    Gg.cpp
    Gg.h
    Gg.qrc
    Gg.ui
    main.cpp
    AppController.cpp
    AppController.h
    AppTypes.h
    MetricsEngine.cpp
    MetricsEngine.h
    SparklineWidget.cpp
    SparklineWidget.h
    TrajectoryWidget.cpp
    TrajectoryWidget.h
    WinDeviceInfo.cpp
    WinDeviceInfo.h
    WorkspaceRepository.cpp
    WorkspaceRepository.h
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(Gg
        WIN32
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(Gg WIN32 ${PROJECT_SOURCES})
endif()

target_link_libraries(Gg PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Setupapi
)

if(MSVC)
    target_compile_options(Gg PRIVATE /W3 /permissive- /MP /utf-8)
endif()

set_target_properties(Gg PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Gg)
endif()
