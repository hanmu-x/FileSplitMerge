# 添加第三方依赖包
include(FetchContent)
# FetchContent_MakeAvailable was not added until CMake 3.14
if (${CMAKE_VERSION} VERSION_LESS 3.14)
    include(add_FetchContent_MakeAvailable.cmake)
endif ()
# file(MAKE_DIRECTORY ${SU_DIRECTORY})
set(SOURCE_DIR "${SU_DIRECTORY}")
set(SILLY_UTIL_TAG master)  # 指定版本
# set(SILLY_UTILS_GIT_URL  http://123.56.193.136:16080/douliyang/silly_utils.git)  # 指定git仓库地址,外网地址,速度可能比较慢
set(SILLY_UTILS_GIT_URL http://192.168.0.60/douliyang/silly_utils.git)  # 指定git仓库地址, 内网地址,内网开发用这个

FetchContent_Declare(
        silly_utils
        GIT_REPOSITORY ${SILLY_UTILS_GIT_URL}
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/silly_utils
)

FetchContent_MakeAvailable(silly_utils)