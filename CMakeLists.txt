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

if (DEFINED ENV{TGN_CHECK_3RD_PARTY})
    execute_process(
        COMMAND ${CMAKE_COMMAND} ${3RD_PARTY_DIR}
        WORKING_DIRECTORY ${3RD_PARTY_DIR}
    )
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ${3RD_PARTY_DIR}
    )
endif()

add_custom_target(generate-version-info 
    ALL "${CMAKE_SOURCE_DIR}/generate_version_info.sh"
    BYPRODUCTS "version-info.cxx"
)

file(GLOB PROJECT_SOURCES CONFIGURE_DEPENDS
     "*.cpp"
     "app/*.cpp"
     "app/priv/*.cpp"
     "gen/*.cpp"
     "gen/priv/*.cpp"
     "mgmt/*.cpp"
     "mgmt/priv/*.cpp"
     "put/*.cpp"
)

add_executable(traffic-generator
    ${PROJECT_SOURCES}
    version_info.cxx
)

add_dependencies(traffic-generator
    generate-version-info
)

set(ENV{PKG_CONFIG_PATH} "${DPDK_TARGET_DIR}/lib/x86_64-linux-gnu/pkgconfig/")
pkg_search_module(LIBDPDK REQUIRED libdpdk)

target_compile_options(traffic-generator 
    PRIVATE -Wall -Wextra -Werror -O3 -std=c++23
    PRIVATE ${LIBDPDK_CFLAGS}
)

target_link_options(traffic-generator 
    PRIVATE -static-libstdc++ -static-libgcc -pthread -rdynamic
)

target_include_directories(traffic-generator 
    PRIVATE ${BOOST_INCLUDE_DIR}
    PRIVATE ${FMT_INCLUDE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}
)

target_link_directories(traffic-generator 
    PRIVATE ${BOOST_LIB_DIR}
    PRIVATE ${FMT_LIB_DIR}
)

target_link_libraries(traffic-generator 
    PRIVATE ${LIBDPDK_STATIC_LDFLAGS} -lnuma -lpcap -lelf
    PRIVATE -lboost_json
    PRIVATE -lboost_program_options
    PRIVATE -lfmt
)

target_precompile_headers(traffic-generator
    PUBLIC precompiled.h
)
