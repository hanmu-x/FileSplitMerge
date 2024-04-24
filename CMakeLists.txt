cmake_minimum_required(VERSION 3.15)

project(silly_utils)

include(CheckIncludeFileCXX)

# 判断当前目录是否为 cmake的根目录,如果不是,则是被其他项目引用的,不需要编译测试,工具模块
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    set(SU_AS_PROJECT TRUE)
    set(SU_AS_LIB FALSE)
else ()
    set(SU_AS_PROJECT FALSE)
    set(SU_AS_LIB TRUE)
endif ()

if (SU_AS_PROJECT)

    # 设置默认程序根目录,程序中所有路径均参照此路径,方便调试和发布
    if (IS_DEBUG)
        add_definitions("-DDEFAULT_ROOT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
    else ()
        # add_definitions("-DDEFAULT_ROOT_DIR=\"\"")
        add_definitions("-DDEFAULT_ROOT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
    endif ()

    add_definitions("-DDEFAULT_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/data\"")
    set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")
    include(env_check)

endif ()


# 头文件
#include(silly_utils)
#include_directories("silly_utils/su")

# 添加目录
add_subdirectory(src)










