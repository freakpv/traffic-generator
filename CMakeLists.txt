cmake_minimum_required(VERSION 3.5)

project(traffic-generator CXX)

add_compile_options(-Wall -Wextra -std=c++23)

add_executable(traffic-generator
    main.cpp)

target_precompile_headers(traffic-generator
    PUBLIC precompiled.h)
