cmake_minimum_required(VERSION 3.21)

project(Gg VERSION 0.2.0 LANGUAGES CXX)

include(cmake/config.cmake)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

add_subdirectory(src/core)
add_subdirectory(src/ui)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    "src/ui/app/*.cpp"
    "src/ui/app/*.ui"
    "src/ui/app/*.qrc"
    "src/ui/app/*.rc"
)

set(TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/app/i18n/Gg_zh_CN.ts
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_add_translation(QM_FILES ${TS_FILES})
else()
    qt5_add_translation(QM_FILES ${TS_FILES})
endif()

add_executable(Gg WIN32)

target_sources(Gg PRIVATE ${APP_SOURCES})

add_custom_target(GgTranslations ALL DEPENDS ${QM_FILES})
add_dependencies(Gg GgTranslations)

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

add_custom_command(TARGET Gg POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Gg>/translations"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QM_FILES} "$<TARGET_FILE_DIR:Gg>/translations"
    VERBATIM
)

enable_testing()
