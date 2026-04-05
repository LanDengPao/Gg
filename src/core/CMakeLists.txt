cmake_minimum_required(VERSION 3.21)

add_library(GgCore STATIC
    types/AppTypes.h
    common/LambdaThread.h
    common/LambdaThread.cpp
    MetricsEngine.h
    MetricsEngine.cpp
    data/WorkspaceRepository.h
    data/WorkspaceRepository.cpp
    data/SessionSampleWriter.h
    data/SessionSampleWriter.cpp
    device/WinDeviceInfo.h
    device/WinDeviceInfo.cpp
    input/WinRawInputMouseSource.h
    input/WinRawInputMouseSource.cpp
)

gg_configure_target(GgCore)

target_include_directories(GgCore PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/types
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    ${CMAKE_CURRENT_SOURCE_DIR}/device
    ${CMAKE_CURRENT_SOURCE_DIR}/input
)
target_link_libraries(GgCore PUBLIC 
    Qt${QT_VERSION_MAJOR}::Core
    Setupapi
)
