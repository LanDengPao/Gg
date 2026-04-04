cmake_minimum_required(VERSION 3.21)

add_library(GgTypes INTERFACE)
target_include_directories(GgTypes INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include/Gg>
    $<INSTALL_INTERFACE:include/Gg>
)
target_link_libraries(GgTypes INTERFACE Qt${QT_VERSION_MAJOR}::Core)
