cmake_minimum_required(VERSION 3.21)

add_library(GgCore STATIC
    types/AppTypes.h
    common/LambdaThread.h
    common/LambdaThread.cpp
    MetricsEngine.h
    MetricsEngine.cpp
    data/WorkspaceRepository.h
    data/WorkspaceRepository.cpp
    device/WinDeviceInfo.h
    device/WinDeviceInfo.cpp
)

target_include_directories(GgCore PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/types
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    ${CMAKE_CURRENT_SOURCE_DIR}/device
)
target_link_libraries(GgCore PUBLIC 
    Qt${QT_VERSION_MAJOR}::Core
    Setupapi
)
