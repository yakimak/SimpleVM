cmake_minimum_required(VERSION 3.10)

project(SimpleVM LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Библиотеки
add_library(cstring STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/string_utils.c)
add_library(LazySequence INTERFACE)

target_include_directories(LazySequence INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/LazySequence)
target_include_directories(cstring PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_include_directories(cstring PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/CString)
target_compile_features(LazySequence INTERFACE cxx_std_17)

# Исполняемый файл
add_executable(SimpleVM ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_include_directories(SimpleVM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(SimpleVM PUBLIC cstring LazySequence)

# Тесты
enable_testing()

# Общая библиотека для тестов (чтобы не дублировать код)
add_library(test_common INTERFACE)
target_include_directories(test_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_include_directories(test_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/test)
target_link_libraries(test_common INTERFACE cstring LazySequence)

# Функция для добавления теста с поддержкой multi-config
function(add_test_executable test_name source_file)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE test_common)
    
    # Добавляем тест
    # Для Visual Studio нужно указывать конфигурацию при запуске ctest -C Release
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# Тест памяти
add_test_executable(test_memory ${CMAKE_CURRENT_SOURCE_DIR}/test/test_memory.cpp)

# Тест процессора
add_test_executable(test_cpu ${CMAKE_CURRENT_SOURCE_DIR}/test/test_cpu.cpp)

# Тест диска
add_test_executable(test_disk ${CMAKE_CURRENT_SOURCE_DIR}/test/test_disk.cpp)

# Тест файловой системы
add_test_executable(test_filesystem ${CMAKE_CURRENT_SOURCE_DIR}/test/test_filesystem.cpp)

# Тест компьютера
add_test_executable(test_computer ${CMAKE_CURRENT_SOURCE_DIR}/test/test_computer.cpp)