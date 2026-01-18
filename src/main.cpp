#include "Computer.hpp"
#include "CPU/Command.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <algorithm>
#include "CString/cstring_bridge.hpp"
#include "LazySequence/Sequence.h"
#include "LazySequence/LazySequence.h"
#include "VirtualFS/virtual_file_system.h"

// Компьютер = [CPU] + [Память] + [BIOS] + [Диск] + [ФС]

// Функция для разбиения строки на токены
std::vector<String*> splitCommandC(const std::string& input) {
    std::vector<String*> tokens;
    std::istringstream iss(input);
    std::string token;
    while (iss >> token) {
        tokens.push_back(cstring_bridge::makeString(token));
    }
    return tokens;
}

// Функция для вывода help
void printHelp() {
    std::cout << "\n=== SimpleVM Shell Commands ===" << std::endl;
    std::cout << "  help              - Show this help message" << std::endl;
    std::cout << "  status            - Show system status" << std::endl;
    std::cout << "  ls [path]         - List files in directory" << std::endl;
    std::cout << "  touch <file>      - Create empty file" << std::endl;
    std::cout << "  mkdir <dir>       - Create directory" << std::endl;
    std::cout << "  rm <file>         - Delete file" << std::endl;
    std::cout << "  mv <from> <to>    - Move/rename node in VFS" << std::endl;
    std::cout << "  find <name>       - Find files by name (without path)" << std::endl;
    std::cout << "  cpu status        - Show CPU status" << std::endl;
    std::cout << "  cpu step [n]      - Execute n CPU instructions (default: 1)" << std::endl;
    std::cout << "  cpu push <value>  - Push value onto CPU stack" << std::endl;
    std::cout << "  cpu pop           - Pop value from stack" << std::endl;
    std::cout << "  cpu stack         - Show stack contents" << std::endl;
    std::cout << "  mem info          - Show memory information" << std::endl;
    std::cout << "  disk info         - Show disk information" << std::endl;
    std::cout << "  poweroff          - Power off computer" << std::endl;
    std::cout << "  exit              - Exit program" << std::endl;
    std::cout << "================================\n" << std::endl;
}

static std::string normalizePath(std::string p) {
    if (p.empty()) return "/";
    if (p[0] != '/') p = "/" + p;
    while (p.size() > 1 && p.back() == '/') p.pop_back();
    return p;
}

// Функция для вывода состояния системы
void printStatus(Computer& computer) {
    std::cout << "\n=== System Status ===" << std::endl;
    std::cout << "Power: " << (computer.isPoweredOn() ? "ON" : "OFF") << std::endl;
    
    StackMachine& cpu = computer.getCPU();
    std::cout << "CPU Program Counter: " << cpu.getProgramCounter() << std::endl;
    std::cout << "CPU Stack Size: " << cpu.getStackSize() << std::endl;
    
    MemoryBlock& ram = computer.getRAM();
    std::cout << "RAM Blocks: " << ram.getTotalBlocks() << " (Block size: " << ram.getBlockSize() << " bytes)" << std::endl;
    
    HardDrive& hdd = computer.getHDD();
    std::cout << "HDD Blocks: " << hdd.getTotalBlocks() << " (Block size: " << hdd.getBlockSize() << " bytes)" << std::endl;
    std::cout << "====================\n" << std::endl;
}

// Функция для выполнения команды ls
void cmdLs(vfs::VirtualFileSystem& fs, const std::vector<String*>& args) {
    const std::string path_std = args.size() > 1 ? normalizePath(cstring_bridge::toStdString(args[1])) : "/";
    try {
        vfs::Node* n = fs.Resolve(path_std);
        if (!n) {
            std::cout << "Not found: " << path_std << "\n";
            return;
        }
        if (n->GetType() == vfs::NodeType::File) {
            auto* f = static_cast<vfs::FileNode*>(n);
            std::cout << "[F] " << f->GetName() << " -> " << f->GetPhysicalPath() << "\n";
            return;
        }
        auto* dir = static_cast<vfs::DirectoryNode*>(n);
        const auto& children = dir->GetChildren();
        if (children.empty()) {
            std::cout << "Directory is empty.\n";
            return;
        }
        for (const auto& ch : children) {
            const vfs::Node* child = ch.get();
            if (child->GetType() == vfs::NodeType::Directory) {
                std::cout << "[D] " << child->GetName() << "\n";
            } else {
                const auto* f = static_cast<const vfs::FileNode*>(child);
                std::cout << "[F] " << f->GetName() << " -> " << f->GetPhysicalPath() << "\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

void cmdMv(vfs::VirtualFileSystem& fs, const std::vector<String*>& args) {
    if (args.size() < 3) {
        std::cerr << "Usage: mv <from> <to>\n";
        return;
    }
    try {
        fs.Move(normalizePath(cstring_bridge::toStdString(args[1])),
                normalizePath(cstring_bridge::toStdString(args[2])));
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

void cmdFind(vfs::VirtualFileSystem& fs, const std::vector<String*>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: find <name>\n";
        return;
    }
    const std::string name = cstring_bridge::toStdString(args[1]);
    try {
        auto files = fs.FindFilesByName(name);
        if (files.empty()) {
            std::cout << "Not found\n";
            return;
        }
        for (auto* f : files) {
            std::cout << f->GetVirtualPath() << " -> " << f->GetPhysicalPath() << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

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
        
        vfs::VirtualFileSystem& fs = computer.getFileSystem();
        StackMachine& cpu = computer.getCPU();
        
        // Интерактивный цикл
        std::string input;
        while (true) {
            std::cout << "SimpleVM> ";
            std::getline(std::cin, input);
            
            if (input.empty()) continue;
            
            std::vector<String*> args = splitCommandC(input);
            if (args.empty()) continue;
            
            String* command = args[0];
            
            if (cstring_bridge::equalsLit(command, "exit") || cstring_bridge::equalsLit(command, "quit")) {
                std::cout << "Shutting down..." << std::endl;
                computer.powerOff();
                for (auto* a : args) cstring_bridge::destroyString(a);
                break;
            }
            else if (cstring_bridge::equalsLit(command, "help")) {
                printHelp();
            }
            else if (cstring_bridge::equalsLit(command, "status")) {
                printStatus(computer);
            }
            else if (cstring_bridge::equalsLit(command, "ls")) {
                cmdLs(fs, args);
            }
            else if (cstring_bridge::equalsLit(command, "touch")) {
                if (args.size() < 2) {
                    std::cerr << "Usage: touch <file>" << std::endl;
                } else {
                    try {
                        const std::string p = normalizePath(cstring_bridge::toStdString(args[1]));
                        fs.AttachFile(p, p);
                        std::cout << "File attached: " << p << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                    }
                }
            }
            else if (cstring_bridge::equalsLit(command, "mkdir")) {
                if (args.size() < 2) {
                    std::cerr << "Usage: mkdir <directory>" << std::endl;
                } else {
                    try {
                        const std::string p = normalizePath(cstring_bridge::toStdString(args[1]));
                        fs.MakeDirectory(p);
                        std::cout << "Directory created: " << p << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                    }
                }
            }
            else if (cstring_bridge::equalsLit(command, "rm")) {
                if (args.size() < 2) {
                    std::cerr << "Usage: rm <file>" << std::endl;
                } else {
                    try {
                        const std::string p = normalizePath(cstring_bridge::toStdString(args[1]));
                        fs.Remove(p);
                        std::cout << "Removed: " << p << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                    }
                }
            }
            else if (cstring_bridge::equalsLit(command, "mv")) {
                cmdMv(fs, args);
            }
            else if (cstring_bridge::equalsLit(command, "find")) {
                cmdFind(fs, args);
            }
            else if (cstring_bridge::equalsLit(command, "cpu")) {
                if (args.size() < 2) {
                    std::cerr << "Usage: cpu <status|step|push|pop|stack>" << std::endl;
                } else if (cstring_bridge::equalsLit(args[1], "status")) {
                    std::cout << "CPU Program Counter: " << cpu.getProgramCounter() << std::endl;
                    std::cout << "CPU Stack Size: " << cpu.getStackSize() << std::endl;
                    std::cout << "Stack Empty: " << (cpu.isStackEmpty() ? "Yes" : "No") << std::endl;
                } else if (cstring_bridge::equalsLit(args[1], "step")) {
                    int steps = 1;
                    if (args.size() > 2) {
                        try {
                            steps = std::stoi(cstring_bridge::toStdString(args[2]));
                        } catch (...) {
                            std::cerr << "Invalid number of steps" << std::endl;
                            continue;
                        }
                    }
                    for (int i = 0; i < steps; ++i) {
                        cpu.executeNext();
                    }
                    std::cout << "Executed " << steps << " instruction(s)" << std::endl;
                } else if (cstring_bridge::equalsLit(args[1], "push")) {
                    if (args.size() < 3) {
                        std::cerr << "Usage: cpu push <value>" << std::endl;
                    } else {
                        try {
                            int value = std::stoi(cstring_bridge::toStdString(args[2]));
                            cpu.push(value);
                            std::cout << "Pushed " << value << " to stack" << std::endl;
                        } catch (...) {
                            std::cerr << "Invalid value" << std::endl;
                        }
                    }
                } else if (cstring_bridge::equalsLit(args[1], "pop")) {
                    try {
                        int value = cpu.pop();
                        std::cout << "Popped: " << value << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                    }
                } else if (cstring_bridge::equalsLit(args[1], "stack")) {
                    std::cout << "Stack size: " << cpu.getStackSize() << std::endl;
                    // Примечание: для полного просмотра стека нужен дополнительный метод
                } else {
                    std::cerr << "Unknown CPU command: " << cstring_bridge::toStdString(args[1]) << std::endl;
                }
            }
            else if (cstring_bridge::equalsLit(command, "mem") || cstring_bridge::equalsLit(command, "memory")) {
                if (args.size() < 2 || !cstring_bridge::equalsLit(args[1], "info")) {
                    std::cerr << "Usage: mem info" << std::endl;
                } else {
                    MemoryBlock& ram = computer.getRAM();
                    std::cout << "RAM Information:" << std::endl;
                    std::cout << "  Total blocks: " << ram.getTotalBlocks() << std::endl;
                    std::cout << "  Block size: " << ram.getBlockSize() << " bytes" << std::endl;
                    std::cout << "  Total capacity: " << (ram.getTotalBlocks() * ram.getBlockSize()) << " bytes" << std::endl;
                }
            }
            else if (cstring_bridge::equalsLit(command, "disk")) {
                if (args.size() < 2 || !cstring_bridge::equalsLit(args[1], "info")) {
                    std::cerr << "Usage: disk info" << std::endl;
                } else {
                    HardDrive& hdd = computer.getHDD();
                    std::cout << "HDD Information:" << std::endl;
                    std::cout << "  Total blocks: " << hdd.getTotalBlocks() << std::endl;
                    std::cout << "  Block size: " << hdd.getBlockSize() << " bytes" << std::endl;
                    std::cout << "  Total capacity: " << (hdd.getTotalBlocks() * hdd.getBlockSize()) << " bytes" << std::endl;
                }
            }
            else if (cstring_bridge::equalsLit(command, "poweroff")) {
                computer.powerOff();
                std::cout << "Computer powered off." << std::endl;
            }
            else {
                std::cerr << "Unknown command: " << cstring_bridge::toStdString(command) << std::endl;
                std::cerr << "Type 'help' for available commands." << std::endl;
            }

            for (auto* a : args) cstring_bridge::destroyString(a);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
