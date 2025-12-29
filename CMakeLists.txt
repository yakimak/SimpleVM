cmake_minimum_required(VERSION 4.2.0)

project(Boot_loader LANGUAGES C CXX)

add_library(cstring STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/CString/string_utils.c)
add_library(LazySequence INTERFACE)

target_include_directories(LazySequence INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/LazySequence)

target_include_directories(cstring PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/CString)
target_compile_features(LazySequence INTERFACE cxx_std_17)

add_executable(BOOT_Loader ${CMAKE_CURRENT_SOURCE_DIR}/src/Computer/Computer.cpp)
target_link_libraries(BOOT_Loader PUBLIC cstring LazySequence)
