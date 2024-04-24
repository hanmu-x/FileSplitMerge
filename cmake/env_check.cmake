set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)


# 指定 c++ 最高版本
check_include_file_cxx(any HAS_ANY)
check_include_file_cxx(string_view HAS_STRING_VIEW)
check_include_file_cxx(coroutine HAS_COROUTINE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# c++ 17 已经包含一些类型定义了,可能会导致重复定义
add_definitions("-D_HAS_STD_BYTE=0")
# 命令行解析库里面的std::max
add_definitions("-DNOMINMAX")
# 指定项目编码类型 unicode 不指定默认utf8 ???
add_definitions("-DUNICODE")
if (CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    set(IS_LINUX TRUE)
    set(PLATFORM_NAME "Linux")
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17  -lodbc -lpthread -fPIC -L. /usr/local/ssl/lib64/libssl.a /usr/local/ssl/lib64/libcrypto.a")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17  -lodbc -lpthread -fPIC -Wl,-Bstatic")


    set(dm_include_path /opt/dmdbms/include)
    set(dm_lib_path /opt/dmdbms/bin)
    add_definitions("-DDM8_ENABLED")
    link_directories("/opt/dmdbms/bin")
    include_directories(${dm_include_path})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17  -lodbc -lpthread -fPIC -Wall")
    set(STATIC_LIB_SUFFIX "a")
    set(DYNAMIC_LIB_SUFFIX "so")
elseif (CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    set(PLATFORM_NAME "Windows")
    set(IS_WINDOWS TRUE)
    add_definitions("-DTFF_INLINE_SPECIFIER=inline")
    add_definitions("-DIS_WIN32")
    add_definitions("-DWIN32_LEAN_AND_MEAN")
    add_compile_options(/wd4819 /wd4005 /wd4834)
    set(STATIC_LIB_SUFFIX "lib")
    set(DYNAMIC_LIB_SUFFIX "dll")

    if (MSVC)
        # Set cmake cxx flags.
        set(CMAKE_CXX_FLAGS_DEBUG "")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_DEBUG /MDd /Zi /Ob0  /Od /RTC1 /Gy /EHsc")

        set(CMAKE_CXX_FLAGS_MINSIZEREL "")
        set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MD /Zi /O1 /Ob2 /Oi /Os /D NDEBUG /GS- ")

        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MD /Zi /O2 /Ob1 /D NDEBUG ")

        set(CMAKE_CXX_FLAGS_RELEASE "")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /Zi /O2 /Ob1 /D NDEBUG ")
    endif (MSVC)

endif ()

# 检查系统位数
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_NAME "x64")
    message(STATUS "Target is 64 bits")
else ()
    set(ARCH_NAME "x32")
    message(STATUS "Target is 32 bits")
endif ()

##########################################
# 设置全局的可执行程序和链接库的生成路径.
##########################################
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/Bin/${PLATFORM_NAME}/${ARCH_NAME}/${CMAKE_BUILD_TYPE}")
set(LIBRARY_OUTPUT_PATH "${EXECUTABLE_OUTPUT_PATH}")