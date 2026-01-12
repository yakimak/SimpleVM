#ifndef BIOS_HPP
#define BIOS_HPP

#include "Memory/MemoryBlock.hpp"
#include "Disk/HardDrive.hpp"
#include <string>

/**
 * Класс Bios - базовая система ввода-вывода
 * Инициализирует систему при включении
 */
class Bios {
private:
    MemoryBlock& ram;
    HardDrive& hdd;
    bool initialized;

public:
    Bios(MemoryBlock& memory, HardDrive& drive)
        : ram(memory), hdd(drive), initialized(false) {}

    void initialize() {
        // Инициализация базовой системы
        // В реальной системе здесь была бы инициализация оборудования
        initialized = true;
    }

    void loadBootloader() {
        if (!initialized) {
            initialize();
        }
        // Загрузка загрузчика
        // В упрощенной версии просто устанавливаем начальное состояние
    }

    bool isInitialized() const { return initialized; }
};

#endif // BIOS_HPP

