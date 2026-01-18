#include "Computer.hpp"
#include "Monitor/Monitor.hpp"

#include <iostream>

int main() {
    try {
        // Создаем компьютер
        Computer computer;
        
        // Включаем компьютер
        std::cout << "=== SimpleVM - Virtual Machine Emulator ===" << std::endl;
        std::cout << "Powering on computer..." << std::endl;
        computer.powerOn();
        std::cout << "Computer is now powered on." << std::endl;
        std::cout << "Type 'help' for available commands." << std::endl;
        std::cout << "==========================================\n" << std::endl;

        return monitor::Monitor::Run(computer);
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }
}
