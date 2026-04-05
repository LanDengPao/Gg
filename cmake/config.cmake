# config.cmake - 项目全局配置
# 统一管理编译选项、警告级别等

if(PROJECT_HAS_CONFIG)
    return()
endif()
set(PROJECT_HAS_CONFIG TRUE)

function(gg_configure_target target_name)
    target_compile_features(${target_name} PRIVATE cxx_std_23)

    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            /W4
            /permissive-
            /utf-8
            /GS
            /Zc:__cplusplus
            /Zc:strictStrings
            $<$<CONFIG:Debug>:/RTC1>
        )
    endif()
endfunction()

# Qt 自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 统一输出路径（在解决方案目录下）
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 多配置生成器（Visual Studio）的目录结构
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib/${OUTPUTCONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib/${OUTPUTCONFIG})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin/${OUTPUTCONFIG})
endforeach()
