cmake_minimum_required(VERSION 3.14)

project(traffic-generator CXX)

find_package(PkgConfig REQUIRED)

set(3RD_PARTY_DIR ${CMAKE_SOURCE_DIR}/3rdparty)
set(BOOST_TARGET_DIR "${3RD_PARTY_DIR}/boost")
set(BOOST_INCLUDE_DIR "${BOOST_TARGET_DIR}")
set(BOOST_LIB_DIR "${BOOST_TARGET_DIR}/target/lib")
set(DPDK_TARGET_DIR "${3RD_PARTY_DIR}/dpdk/target")
set(FMT_TARGET_DIR "${3RD_PARTY_DIR}/fmt")
set(FMT_INCLUDE_DIR "${FMT_TARGET_DIR}/include")
set(FMT_LIB_DIR "${FMT_TARGET_DIR}/build")

execute_process(
    COMMAND ${CMAKE_COMMAND} ${3RD_PARTY_DIR}
    WORKING_DIRECTORY ${3RD_PARTY_DIR}
)
execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${3RD_PARTY_DIR}
)


add_custom_target(generate-version-info 
    ALL "${CMAKE_SOURCE_DIR}/generate_version_info.sh"
    BYPRODUCTS "version-info.cpp"
)


add_executable(traffic-generator
    main.cpp
    version_info.cpp
)

add_dependencies(traffic-generator
    generate-version-info
)

add_compile_options(-Wall -Wextra -Werror -std=c++23)

set(ENV{PKG_CONFIG_PATH} "${DPDK_TARGET_DIR}/lib/x86_64-linux-gnu/pkgconfig/")
pkg_search_module(LIBDPDK REQUIRED libdpdk)
target_compile_options(traffic-generator PRIVATE ${LIBDPDK_CFLAGS})

target_include_directories(traffic-generator 
    PRIVATE ${BOOST_INCLUDE_DIR}
    PRIVATE ${FMT_INCLUDE_DIR}
)

target_link_directories(traffic-generator 
    PRIVATE ${BOOST_LIB_DIR}
    PRIVATE ${FMT_LIB_DIR}
)

target_link_libraries(traffic-generator 
    PRIVATE ${LIBDPDK_LDFLAGS} -lnuma
    PRIVATE -lboost_program_options
    PRIVATE -lfmt
)

target_precompile_headers(traffic-generator
    PUBLIC precompiled.h
)
