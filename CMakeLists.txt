cmake_minimum_required(VERSION 3.10)

project(SimpleVM LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Библиотеки
add_library(cstring STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/string_utils.cpp)
add_library(LazySequence INTERFACE)

target_include_directories(LazySequence INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/LazySequence)
target_include_directories(cstring PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_compile_features(LazySequence INTERFACE cxx_std_17)

# Исполняемый файл
add_executable(SimpleVM ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_include_directories(SimpleVM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(SimpleVM PUBLIC cstring LazySequence)
