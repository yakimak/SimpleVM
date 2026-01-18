#include <iostream>
#include <cstdlib>

// Заглушки для запуска всех тестов

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  SimpleVM Test Suite" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Для запуска всех тестов используйте
    // ctest или запустите каждый тест отдельно
    
    std::cout << "To run all tests, use:" << std::endl;
    std::cout << "  - Individual test executables" << std::endl;
    std::cout << "  - ctest (if configured in CMakeLists.txt)" << std::endl;
    std::cout << "\nAvailable test modules:" << std::endl;
    std::cout << "  - test_memory" << std::endl;
    std::cout << "  - test_cpu" << std::endl;
    std::cout << "  - test_disk" << std::endl;
    std::cout << "  - test_filesystem" << std::endl;
    std::cout << "  - test_computer" << std::endl;
    
    return 0;
}

