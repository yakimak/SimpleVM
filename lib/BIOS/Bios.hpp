#ifndef BIOS_HPP
#define BIOS_HPP

#include "Memory/MemoryBlock.hpp"
#include "Disk/HardDrive.hpp"
#include "CPU/StackMachine.hpp"
#include "VirtualFS/virtual_file_system.h"
#include <string>
#include <stdexcept>

/**
 * Класс Bios - базовая система ввода-вывода
 * Инициализирует систему при включении
 */
class Bios {
private:
    MemoryBlock* ram = nullptr;
    HardDrive* hdd = nullptr;
    StackMachine* cpu = nullptr;
    vfs::VirtualFileSystem* fs = nullptr;

    bool initialized = false;
    bool post_passed = false;

public:
    Bios() = default;

    void attach(MemoryBlock& memory,
                HardDrive& drive,
                StackMachine& processor,
                vfs::VirtualFileSystem& filesystem) {
        ram = &memory;
        hdd = &drive;
        cpu = &processor;
        fs = &filesystem;
    }

    // Простая проверка исправности самого BIOS.
    bool selfTest() const { return true; }

    void initializeSystems() {
        if (!ram || !hdd || !cpu || !fs) {
            throw std::runtime_error("BIOS: components are not attached");
        }
        // BIOS использует CPU в 16-битном режиме (упрощённая модель).
        cpu->setMode(StackMachine::Mode::BIOS16);
        initialized = true;
        post_passed = false;
    }

    // Power-On Self Test (POST): проверка всех компонентов.
    bool runPOST() {
        if (!initialized) {
            initializeSystems();
        }
        if (!ram || !hdd || !cpu || !fs) return false;
        post_passed = selfTest() &&
                      ram->selfTest() &&
                      hdd->selfTest() &&
                      cpu->selfTest() &&
                      fs->selfTest();
        return post_passed;
    }

    bool isInitialized() const { return initialized; }
    bool isPostPassed() const { return post_passed; }

    void reset() {
        ram = nullptr;
        hdd = nullptr;
        cpu = nullptr;
        fs = nullptr;
        initialized = false;
        post_passed = false;
    }
};

#endif // BIOS_HPP

