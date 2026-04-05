cmake_minimum_required(VERSION 3.21)

project(Gg VERSION 0.2.0 LANGUAGES CXX)

include(cmake/config.cmake)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_subdirectory(src/core)
add_subdirectory(src/ui)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "src/ui/app/*.cpp")

add_executable(Gg WIN32)

target_sources(Gg PRIVATE ${APP_SOURCES})

set_target_properties(Gg PROPERTIES 
    AUTOMOC TRUE
    WIN32_EXECUTABLE TRUE
)

gg_configure_target(Gg)

target_include_directories(Gg PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/app
)

target_link_libraries(Gg PUBLIC 
    GgUi
    GgCore
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Setupapi
)

enable_testing()
