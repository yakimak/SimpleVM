#ifndef COMPUTER_HPP
#define COMPUTER_HPP

#include "CPU/StackMachine.hpp"
#include "Memory/MemoryBlock.hpp"
#include "BIOS/Bios.hpp"
#include "Disk/HardDrive.hpp"
#include "FileSystem/SimpleFileSystem.hpp"
#include "LazySequence/SimpleLazySequence.hpp"
#include "CPU/Command.hpp"
#include <string>
#include <vector>
#include <memory>

/**
 * Главный класс компьютера
 * Компьютер = [CPU] + [Память] + [BIOS] + [Диск] + [ФС]
 */
class Computer {
private:
    MemoryBlock ram;
    HardDrive hdd;
    Bios bios;
    SimpleFileSystem filesystem;
    std::unique_ptr<StackMachine> cpu;
    bool powered_on;
    
    // Программа загрузки (bootloader)
    std::shared_ptr<LazySequence<Command>> bootloader_stream;
    
    void createBootloader() {
        // Создаем простой bootloader
        std::vector<Command> boot_commands = {
            Command(CommandType::PUSH, 1),
            Command(CommandType::PUSH, 2),
            Command(CommandType::ADD),
            Command(CommandType::HALT)
        };
        
        // Используем shared_ptr для общего состояния индекса
        auto boot_index = std::make_shared<size_t>(0);
        auto commands_ptr = std::make_shared<std::vector<Command>>(boot_commands);
        
        auto gen = [commands_ptr, boot_index]() -> Command {
            if (*boot_index < commands_ptr->size()) {
                return (*commands_ptr)[(*boot_index)++];
            }
            return Command(CommandType::HALT);
        };
        
        auto has_nxt = [commands_ptr, boot_index]() -> bool {
            return *boot_index < commands_ptr->size();
        };
        
        bootloader_stream = std::make_shared<LazySequence<Command>>(gen, has_nxt);
    }

public:
    Computer()
        : ram(1024, 64),  // 1024 блока по 64 байта каждый
          hdd(4096, 512), // 4096 блоков по 512 байт
          bios(ram, hdd),
          filesystem(hdd),
          powered_on(false),
          bootloader_stream(nullptr) {
        createBootloader();
        cpu = std::make_unique<StackMachine>(*bootloader_stream);
    }

    ~Computer() = default;

    void powerOn() {
        powered_on = true;
        bios.initialize();
        bios.loadBootloader();
    }

    void powerOff() {
        powered_on = false;
    }

    bool isPoweredOn() const {
        return powered_on;
    }

    void loadOS(const std::string& os_name) {
        if (!powered_on) {
            throw std::runtime_error("Computer is not powered on");
        }
        
        // Загрузка ОС с диска
        if (!filesystem.fileExists(os_name)) {
            throw std::runtime_error("OS file not found: " + os_name);
        }
        
        // В упрощенной версии просто проверяем наличие файла
        // В реальной системе здесь была бы загрузка в память и передача управления
    }

    void runCommand(const std::string& command) {
        if (!powered_on) {
            throw std::runtime_error("Computer is not powered on");
        }
        
        // Упрощенная реализация выполнения команд
        // В реальной системе здесь был бы интерпретатор команд ОС
        if (command == "ls") {
            // Список файлов
            std::vector<std::string> files = filesystem.listDirectory("/");
            // В реальной системе здесь был бы вывод
        }
    }

    // Геттеры для доступа к компонентам (для тестирования)
    MemoryBlock& getRAM() { return ram; }
    HardDrive& getHDD() { return hdd; }
    SimpleFileSystem& getFileSystem() { return filesystem; }
    StackMachine& getCPU() { return *cpu; }
};

#endif // COMPUTER_HPP

