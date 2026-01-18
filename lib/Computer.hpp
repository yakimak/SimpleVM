#ifndef COMPUTER_HPP
#define COMPUTER_HPP

#include "CPU/StackMachine.hpp"
#include "Memory/MemoryBlock.hpp"
#include "BIOS/Bios.hpp"
#include "Disk/HardDrive.hpp"
#include "VirtualFS/virtual_file_system.h"
#include "LazySequence/Sequence.h"
#include "LazySequence/LazySequence.h"
#include "CPU/Command.hpp"
#include <string>
#include <vector>
#include <memory>
#include "CString/cstring_bridge.hpp"


//Главный класс компьютера
//Компьютер = [CPU] + [Память] + [BIOS] + [Диск] + [ФС]

class Computer {
private:
    std::unique_ptr<MemoryBlock> ram;
    std::unique_ptr<HardDrive> hdd;
    Bios bios;
    std::unique_ptr<vfs::VirtualFileSystem> filesystem;
    std::unique_ptr<StackMachine> cpu;
    bool powered_on;
    bool os_loaded;
    
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


        bootloader_stream = std::make_shared<LazySequence<Command>>(boot_commands.data(),
                                                                    (int)boot_commands.size());
    }

    static std::string normalizePath(std::string p) {
        if (p.empty()) return "/";
        if (p[0] != '/') p = "/" + p;
        while (p.size() > 1 && p.back() == '/') p.pop_back();
        return p;
    }

    void initializeSystemDirectories() {
        const std::vector<std::string> system_dirs = {
            "/bin", "/usr", "/usr/bin", "/usr/lib", "/lib", "/etc", "/home", "/tmp", "/var", "/var/log", "/boot"
        };
        for (const auto& dir : system_dirs) {
            // MakeDirectory создаёт только последнюю директорию; родитель создаётся автоматически.
            // Если директория уже есть, Resolve вернёт её — тогда пропускаем.
            const std::string p = normalizePath(dir);
            if (filesystem->Resolve(p) == nullptr) {
                filesystem->MakeDirectory(p);
            }
        }
    }

    bool vfsFileExists(const std::string& path) const {
        const std::string p = normalizePath(path);
        if (!filesystem) return false;
        vfs::Node* n = filesystem->Resolve(p);
        return n && n->GetType() == vfs::NodeType::File;
    }

    void bootloaderStage() {
        // Bootloader получает управление после BIOS POST.
        // Он переводит CPU в 32-bit режим, затем в 64-bit, повторно проверяет систему
        // и "загружает ОС" (в упрощённой модели — ставит флаг).
        if (!cpu || !ram || !hdd || !filesystem) {
            throw std::runtime_error("Bootloader: components are not initialized");
        }

        cpu->setMode(StackMachine::Mode::Protected32);
        cpu->setMode(StackMachine::Mode::Long64);

        const bool ok = ram->selfTest() && hdd->selfTest() && cpu->selfTest() && filesystem->selfTest();
        if (!ok) {
            throw std::runtime_error("Bootloader: self-test failed");
        }
        os_loaded = true;
    }

public:
    Computer()
        : ram(nullptr),
          hdd(nullptr),
          bios(),
          filesystem(nullptr),
          cpu(nullptr),
          powered_on(false),
          os_loaded(false),
          bootloader_stream(nullptr) {
    }

    ~Computer() = default;

    void powerOn() {
        powered_on = true;
        os_loaded = false;

        // BIOS инициализирует системы после включения (в этой модели —
        // компоненты создаёт Computer, а BIOS переводит CPU в 16-bit и делает POST).
        ram = std::make_unique<MemoryBlock>(1024, 64);
        hdd = std::make_unique<HardDrive>(4096, 512);
        filesystem = std::make_unique<vfs::VirtualFileSystem>();
        initializeSystemDirectories();

        createBootloader();
        cpu = std::make_unique<StackMachine>(*bootloader_stream);
        bios.attach(*ram, *hdd, *cpu, *filesystem);
        bios.initializeSystems();            // CPU = 16-bit
        if (!bios.runPOST()) {
            powered_on = false;
            throw std::runtime_error("BIOS POST failed");
        }

        // Передача управления bootloader
        bootloaderStage();
    }

    void powerOff() {
        powered_on = false;
        os_loaded = false;
        bios.reset();
        cpu.reset();
        filesystem.reset();
        hdd.reset();
        ram.reset();
    }

    bool isPoweredOn() const {
        return powered_on;
    }

    bool isOSLoaded() const {
        return os_loaded;
    }

    // CString-first API
    void loadOS(const String* os_name) {
        if (!powered_on) {
            throw std::runtime_error("Computer is not powered on");
        }
        
        // Загрузка ОС с диска
        if (!vfsFileExists(cstring_bridge::toStdString(os_name))) {
            throw std::runtime_error("OS file not found: " + cstring_bridge::toStdString(os_name));
        }
        
        // В упрощенной версии просто проверяем наличие файла
        // В реальной системе здесь была бы загрузка в память и передача управления
    }

    void runCommand(const String* command) {
        if (!powered_on) {
            throw std::runtime_error("Computer is not powered on");
        }
        
        // Оставлено пустым: интерактивная оболочка реализована в src/main.cpp
        (void)command;
    }

    void loadOS(const std::string& os_name) {
        String* tmp = cstring_bridge::makeString(os_name);
        loadOS(tmp);
        cstring_bridge::destroyString(tmp);
    }
    void runCommand(const std::string& command) {
        String* tmp = cstring_bridge::makeString(command);
        runCommand(tmp);
        cstring_bridge::destroyString(tmp);
    }

    // Геттеры для доступа к компонентам (для тестирования)
    MemoryBlock& getRAM() {
        if (!powered_on || !ram) throw std::runtime_error("RAM is not initialized");
        return *ram;
    }
    HardDrive& getHDD() {
        if (!powered_on || !hdd) throw std::runtime_error("HDD is not initialized");
        return *hdd;
    }
    vfs::VirtualFileSystem& getFileSystem() {
        if (!powered_on || !filesystem) throw std::runtime_error("FileSystem is not initialized");
        return *filesystem;
    }
    StackMachine& getCPU() {
        if (!powered_on || !cpu) throw std::runtime_error("CPU is not initialized");
        return *cpu;
    }
};

#endif // COMPUTER_HPP

