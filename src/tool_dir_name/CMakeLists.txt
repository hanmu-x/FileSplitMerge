cmake_minimum_required (VERSION 3.15)

project(tool_lib_name CXX)
foreach(dirname ${CMAKE_CURRENT_SOURCE_DIR})
    file(GLOB_RECURSE SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            "${dirname}/*.h"
            "${dirname}/*.hpp"
            "${dirname}/*.c"
            "${dirname}/*.cpp"
            )
endforeach()



add_library(${PROJECT_NAME} STATIC ${SRC})


find_package(jsoncpp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC JsonCpp::JsonCpp)


#target_link_libraries(${PROJECT_NAME} PUBLIC sutils)
