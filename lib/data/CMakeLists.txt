cmake_minimum_required(VERSION 3.21)

add_library(GgData STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/WorkspaceRepository.cpp
)
target_include_directories(GgData PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include/Gg>
    $<INSTALL_INTERFACE:include/Gg>
)
target_link_libraries(GgData PUBLIC 
    GgTypes
    Qt${QT_VERSION_MAJOR}::Core
)
