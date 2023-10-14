cmake_minimum_required(VERSION 3.14)

project(traffic-generator CXX)

#find_package(PkgConfig REQUIRED)

set(3RD_PARTY_DIR ${CMAKE_SOURCE_DIR}/3rdparty)

execute_process(
    COMMAND ${CMAKE_COMMAND} ${3RD_PARTY_DIR}
    WORKING_DIRECTORY ${3RD_PARTY_DIR}
)
execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${3RD_PARTY_DIR}
)


#pkg_search_module(LIB_DPDK REQUIRED libdpdk)

add_executable(traffic-generator
    main.cpp)

add_compile_options(-Wall -Wextra -std=c++23)

#target_compile_options(traffic-generator LIBDPDK_CFLAGS_OTHER)
#target_link_libraries(traffic-generator LIBDPDK_LDFLAGS)

target_precompile_headers(traffic-generator
    PUBLIC precompiled.h)
