cmake_minimum_required(VERSION 3.14)

project(3rdparty)

include(FetchContent)
include(ExternalProject)

set(CPU_TYPE "corei7")
set(3RD_PARTY_PREFIX_DIR "${CMAKE_SOURCE_DIR}")
set(3RD_PARTY_DOWNLOAD_DIR "${3RD_PARTY_PREFIX_DIR}")
set(3RD_PARTY_TMP_DIR "${3RD_PARTY_PREFIX_DIR}/tmp")

execute_process(
    COMMAND grep processor /proc/cpuinfo
    COMMAND wc -l
    OUTPUT_VARIABLE NUM_CPUS
)

################################################################################
# DPDK Libraries

set(DPDK_PREFIX_DIR "${3RD_PARTY_PREFIX_DIR}/dpdk")
set(DPDK_BUILD_DIR "${DPDK_PREFIX_DIR}/build")
set(DPDK_TARGET_DIR "${DPDK_PREFIX_DIR}/target")
ExternalProject_Add(
    dpdk

    GIT_REPOSITORY "https://github.com/DPDK/dpdk"
    GIT_TAG "v23.07"

    PREFIX ${3RD_PARTY_PREFIX_DIR}
    TMP_DIR ${3RD_PARTY_TMP_DIR}
    DOWNLOAD_DIR ${3RD_PARTY_DOWNLOAD_DIR}
    SOURCE_DIR ${DPDK_PREFIX_DIR}
    STAMP_DIR ${DPDK_PREFIX_DIR}-stamp
    #BINARY_DIR ${DPDK_PREFIX_DIR}-build

    CONFIGURE_COMMAND mkdir -p ${DPDK_BUILD_DIR} &&
        cd ${DPDK_BUILD_DIR} &&
        meson setup
        --prefix ${DPDK_TARGET_DIR}
        --buildtype release
        --default-library static
        --warnlevel 2
        -Denable_kmods=false
        -Denable_apps=dumpcap
        -Denable_drivers=net/bonding,net/e1000,net/i40e,net/ixgbe
        -Dmachine=${CPU_TYPE}

    BUILD_ALWAYS TRUE
    BUILD_IN_SOURCE TRUE
    BUILD_COMMAND cd ${DPDK_BUILD_DIR} && meson compile

    INSTALL_COMMAND cd ${DPDK_BUILD_DIR} && meson install
)
# Don't know how to prevent the generation of the .so files, so ...
ExternalProject_Add_Step(
    dpdk 
    remove_dpdk_so_files
    COMMAND find ${DPDK_TARGET_DIR} -name *.so -delete &&
            find ${DPDK_TARGET_DIR} -name *.so.* -delete
    DEPENDEES install
)

################################################################################
# Boost Libraries

set(BOOST_ARCHIVE "boost_1_84_0.tar.bz2")
set(BOOST_PREFIX_DIR "${3RD_PARTY_PREFIX_DIR}/boost")
set(BOOST_BUILD_DIR "${BOOST_PREFIX_DIR}/build")
set(BOOST_TARGET_DIR "${BOOST_PREFIX_DIR}/target")
ExternalProject_Add(
    boost

    URL 
        "https://boostorg.jfrog.io/artifactory/main/release/1.84.0/source/${BOOST_ARCHIVE}"
    DOWNLOAD_NO_EXTRACT TRUE

    PREFIX ${3RD_PARTY_PREFIX_DIR}
    TMP_DIR ${3RD_PARTY_TMP_DIR}
    DOWNLOAD_DIR ${3RD_PARTY_DOWNLOAD_DIR}
    SOURCE_DIR ${BOOST_PREFIX_DIR}
    STAMP_DIR ${BOOST_PREFIX_DIR}-stamp
    #BINARY_DIR ${BOOST_PREFIX_DIR}-build

    CONFIGURE_COMMAND 
        tar xf 
        ${3RD_PARTY_PREFIX_DIR}/${BOOST_ARCHIVE}
        -C ${BOOST_PREFIX_DIR} 
        --strip-components=1 &&
        ${BOOST_PREFIX_DIR}/bootstrap.sh

    BUILD_ALWAYS TRUE
    BUILD_IN_SOURCE TRUE
    #NO_COMPRESSION disables support for compression in boost::iostreams
    BUILD_COMMAND 
        ${BOOST_PREFIX_DIR}/b2 install
        --prefix=${BOOST_TARGET_DIR}
        --build-dir=${BOOST_BUILD_DIR}
        -sNO_COMPRESSION
        -j${NUM_CPUS}
        variant=release
        link=static
        runtime-link=shared
        threading=multi
        threadapi=pthread
        cflags="-march=${CPU_TYPE}"

    INSTALL_COMMAND echo "Installation done at ${BOOST_TARGET_DIR}"
)

################################################################################
# FMT Library

set(FMT_PREFIX_DIR "${3RD_PARTY_PREFIX_DIR}/fmt")
set(FMT_BUILD_DIR "${FMT_PREFIX_DIR}/build")
ExternalProject_Add(
    fmt

    GIT_REPOSITORY "https://github.com/fmtlib/fmt"
    GIT_TAG "10.1.1"

    PREFIX ${3RD_PARTY_PREFIX_DIR}
    TMP_DIR ${3RD_PARTY_TMP_DIR}
    DOWNLOAD_DIR ${3RD_PARTY_DOWNLOAD_DIR}
    SOURCE_DIR ${FMT_PREFIX_DIR}
    STAMP_DIR ${FMT_PREFIX_DIR}-stamp
    #BINARY_DIR ${FMT_PREFIX_DIR}-build

    CONFIGURE_COMMAND 
        mkdir -p ${FMT_BUILD_DIR} && cd ${FMT_BUILD_DIR} && cmake ../

    BUILD_ALWAYS TRUE
    BUILD_IN_SOURCE TRUE
    BUILD_COMMAND cd ${FMT_BUILD_DIR} && make

    INSTALL_COMMAND echo "Skip installation of the fmt library"
)
