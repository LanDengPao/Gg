cmake_minimum_required(VERSION 3.21)

add_library(GgCore STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/MetricsEngine.cpp
)
target_include_directories(GgCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include/Gg>
    $<INSTALL_INTERFACE:include/Gg>
)
target_link_libraries(GgCore PUBLIC 
    GgTypes
    Qt${QT_VERSION_MAJOR}::Core
)
