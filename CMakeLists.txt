cmake_minimum_required(VERSION 3.14)

project(traffic-generator CXX)

find_package(PkgConfig REQUIRED)

set(3RD_PARTY_DIR ${CMAKE_SOURCE_DIR}/3rdparty)
set(BOOST_TARGET_DIR "${3RD_PARTY_DIR}/boost")
set(BOOST_INCLUDE_DIR "${BOOST_TARGET_DIR}/include")
set(BOOST_LIB_DIR "${BOOST_TARGET_DIR}/lib")
set(DPDK_TARGET_DIR "${3RD_PARTY_DIR}/dpdk/target")

execute_process(
    COMMAND ${CMAKE_COMMAND} ${3RD_PARTY_DIR}
    WORKING_DIRECTORY ${3RD_PARTY_DIR}
)
execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${3RD_PARTY_DIR}
)

add_executable(traffic-generator
    main.cpp)

add_compile_options(-Wall -Wextra -std=c++23)

set(ENV{PKG_CONFIG_PATH} "${DPDK_TARGET_DIR}/lib/x86_64-linux-gnu/pkgconfig/")
pkg_search_module(LIBDPDK REQUIRED libdpdk)
target_compile_options(traffic-generator PRIVATE ${LIBDPDK_CFLAGS})
target_link_options(traffic-generator PRIVATE ${LIBDPDK_LDFLAGS})

target_include_directories(traffic-generator PRIVATE ${BOOST_INCLUDE_DIR})

target_link_directories(traffic-generator PRIVATE ${BOOST_LIB_DIR})

target_precompile_headers(traffic-generator
    PUBLIC precompiled.h)
