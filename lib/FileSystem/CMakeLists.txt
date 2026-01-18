cmake_minimum_required(VERSION 3.10)
project(lab2_histogram_search_comparison LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lab2
    src/main.cpp
)

target_compile_features(lab2 PRIVATE cxx_std_17)

target_include_directories(lab2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/LibrarySequence
)


