cmake_minimum_required(VERSION 3.21)

project(Gg VERSION 0.2.0 LANGUAGES CXX)

include(cmake/config.cmake)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install path prefix." FORCE)
endif()

if(MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
    include(InstallRequiredSystemLibraries)

    set(GG_MSVC_RUNTIME_LIBS)
    foreach(_runtime_lib IN LISTS CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        get_filename_component(_runtime_name "${_runtime_lib}" NAME)
        if(_runtime_name MATCHES "^api-ms-win-" OR _runtime_name STREQUAL "ucrtbase.dll")
            continue()
        endif()
        list(APPEND GG_MSVC_RUNTIME_LIBS "${_runtime_lib}")
    endforeach()
endif()

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

set(GG_QT_TOOL_HINTS)
foreach(_qt_prefix IN LISTS CMAKE_PREFIX_PATH)
    list(APPEND GG_QT_TOOL_HINTS "${_qt_prefix}/bin")
endforeach()

find_program(GG_WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt windeployqt.exe
    HINTS ${GG_QT_TOOL_HINTS}
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

install(TARGETS Gg
    RUNTIME DESTINATION .
)

install(FILES ${QM_FILES}
    DESTINATION translations
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    DESTINATION .
)

if(GG_MSVC_RUNTIME_LIBS)
    install(PROGRAMS ${GG_MSVC_RUNTIME_LIBS}
        DESTINATION .
    )
endif()

if(WIN32)
    if(NOT GG_WINDEPLOYQT_EXECUTABLE)
        message(WARNING "windeployqt was not found. Install/package output will not include Qt runtime files.")
    else()
        install(CODE "
            set(_gg_install_root \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}\")
            set(_gg_executable \"\${_gg_install_root}/Gg.exe\")
            if(EXISTS \"\${_gg_executable}\")
                set(_gg_deploy_args --dir \"\${_gg_install_root}\" --compiler-runtime --no-translations)
                if(CMAKE_INSTALL_CONFIG_NAME MATCHES \"^[Dd]ebug\$\")
                    list(APPEND _gg_deploy_args --debug)
                else()
                    list(APPEND _gg_deploy_args --release)
                endif()
                list(APPEND _gg_deploy_args \"\${_gg_executable}\")

                execute_process(
                    COMMAND \"${GG_WINDEPLOYQT_EXECUTABLE}\" \${_gg_deploy_args}
                    RESULT_VARIABLE _gg_deploy_result
                )
                if(NOT _gg_deploy_result EQUAL 0)
                    message(FATAL_ERROR \"windeployqt failed with exit code \${_gg_deploy_result}\")
                endif()
            endif()
        ")
    endif()
endif()

set(CPACK_GENERATOR "ZIP")
set(CPACK_MONOLITHIC_INSTALL ON)
set(CPACK_PACKAGE_NAME "Gg")
set(CPACK_PACKAGE_VENDOR "Gg")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "Gg-${PROJECT_VERSION}-win64")
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_BINARY_DIR}/packages")

include(CPack)

enable_testing()
