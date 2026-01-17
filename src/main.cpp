#include "Computer.hpp"
#include "CPU/Command.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <algorithm>

// Компьютер = [CPU] + [Память] + [BIOS] + [Диск] + [ФС]

// Функция для разбиения строки на токены
std::vector<std::string> splitCommand(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// Function to display help
void printHelp() {
    std::cout << "\n=== SimpleVM Shell Commands ===" << std::endl;
    std::cout << "  help              - Show this help message" << std::endl;
    std::cout << "  status            - Show system status" << std::endl;
    std::cout << "  ls [path]         - List files in directory" << std::endl;
    std::cout << "  cat <file>        - Display file contents" << std::endl;
    std::cout << "  touch <file>      - Create empty file" << std::endl;
    std::cout << "  mkdir <dir>       - Create directory" << std::endl;
    std::cout << "  rm <file>         - Delete file" << std::endl;
    std::cout << "  echo <text> > <file> - Write text to file" << std::endl;
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
void cmdLs(SimpleFileSystem& fs, const std::vector<std::string>& args) {
    std::string path = args.size() > 1 ? args[1] : "/";
    if (path.back() != '/' && path != "/") path += "/";
    
    try {
        std::vector<std::string> entries = fs.listDirectory(path);
        if (entries.empty()) {
            std::cout << "Directory is empty." << std::endl;
        } else {
            for (const auto& entry : entries) {
                std::cout << "  " << entry << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// Функция для выполнения команды cat
void cmdCat(SimpleFileSystem& fs, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: cat <file>" << std::endl;
        return;
    }
    
    try {
        LazySequence<uint8_t> read_stream = fs.readFile(args[1]);
        while (read_stream.hasNext()) {
            std::cout << static_cast<char>(read_stream.next());
        }
        std::cout << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// Функция для выполнения команды echo
void cmdEcho(SimpleFileSystem& fs, const std::vector<std::string>& args) {
    if (args.size() < 4 || args[args.size() - 2] != ">") {
        std::cerr << "Usage: echo <text> > <file>" << std::endl;
        return;
    }
    
    // Собираем текст (все аргументы между "echo" и ">")
    std::string text;
    for (size_t i = 1; i < args.size() - 2; ++i) {
        if (i > 1) text += " ";
        text += args[i];
    }
    
    std::string filename = args.back();
    
    try {
        // Преобразуем текст в вектор байтов
        std::vector<uint8_t> data(text.begin(), text.end());
        auto data_ptr = std::make_shared<std::vector<uint8_t>>(data);
        auto index_ptr = std::make_shared<size_t>(0);
        
        auto write_gen = [data_ptr, index_ptr]() -> uint8_t {
            if (*index_ptr < data_ptr->size()) {
                return (*data_ptr)[(*index_ptr)++];
            }
            return 0;
        };
        auto write_has_next = [data_ptr, index_ptr]() -> bool {
            return *index_ptr < data_ptr->size();
        };
        
        LazySequence<uint8_t> write_stream(write_gen, write_has_next);
        fs.writeFile(filename, write_stream);
        std::cout << "Text written to " << filename << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
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
        
        SimpleFileSystem& fs = computer.getFileSystem();
        StackMachine& cpu = computer.getCPU();
        
        // Интерактивный цикл
        std::string input;
        while (true) {
            std::cout << "SimpleVM> ";
            std::getline(std::cin, input);
            
            if (input.empty()) continue;
            
            std::vector<std::string> args = splitCommand(input);
            if (args.empty()) continue;
            
            std::string command = args[0];
            
            if (command == "exit" || command == "quit") {
                std::cout << "Shutting down..." << std::endl;
                computer.powerOff();
                break;
            }
            else if (command == "help") {
                printHelp();
            }
            else if (command == "status") {
                printStatus(computer);
            }
            else if (command == "ls") {
                cmdLs(fs, args);
            }
            else if (command == "cat") {
                cmdCat(fs, args);
            }
            else if (command == "touch") {
                if (args.size() < 2) {
                    std::cerr << "Usage: touch <file>" << std::endl;
                } else {
                    try {
                        fs.createFile(args[1]);
                        std::cout << "File created: " << args[1] << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                    }
                }
            }
            else if (command == "mkdir") {
                if (args.size() < 2) {
                    std::cerr << "Usage: mkdir <directory>" << std::endl;
                } else {
                    try {
                        fs.createDirectory(args[1]);
                        std::cout << "Directory created: " << args[1] << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                    }
                }
            }
            else if (command == "rm") {
                if (args.size() < 2) {
                    std::cerr << "Usage: rm <file>" << std::endl;
                } else {
                    try {
                        fs.deleteFile(args[1]);
                        std::cout << "File deleted: " << args[1] << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                    }
                }
            }
            else if (command == "echo") {
                cmdEcho(fs, args);
            }
            else if (command == "cpu") {
                if (args.size() < 2) {
                    std::cerr << "Usage: cpu <status|step|push|pop|stack>" << std::endl;
                } else if (args[1] == "status") {
                    std::cout << "CPU Program Counter: " << cpu.getProgramCounter() << std::endl;
                    std::cout << "CPU Stack Size: " << cpu.getStackSize() << std::endl;
                    std::cout << "Stack Empty: " << (cpu.isStackEmpty() ? "Yes" : "No") << std::endl;
                } else if (args[1] == "step") {
                    int steps = 1;
                    if (args.size() > 2) {
                        try {
                            steps = std::stoi(args[2]);
                        } catch (...) {
                            std::cerr << "Invalid number of steps" << std::endl;
                            continue;
                        }
                    }
                    for (int i = 0; i < steps; ++i) {
                        cpu.executeNext();
                    }
                    std::cout << "Executed " << steps << " instruction(s)" << std::endl;
                } else if (args[1] == "push") {
                    if (args.size() < 3) {
                        std::cerr << "Usage: cpu push <value>" << std::endl;
                    } else {
                        try {
                            int value = std::stoi(args[2]);
                            cpu.push(value);
                            std::cout << "Pushed " << value << " to stack" << std::endl;
                        } catch (...) {
                            std::cerr << "Invalid value" << std::endl;
                        }
                    }
                } else if (args[1] == "pop") {
                    try {
                        int value = cpu.pop();
                        std::cout << "Popped: " << value << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                    }
                } else if (args[1] == "stack") {
                    std::cout << "Stack size: " << cpu.getStackSize() << std::endl;
                    // Примечание: для полного просмотра стека нужен дополнительный метод
                } else {
                    std::cerr << "Unknown CPU command: " << args[1] << std::endl;
                }
            }
            else if (command == "mem" || command == "memory") {
                if (args.size() < 2 || args[1] != "info") {
                    std::cerr << "Usage: mem info" << std::endl;
                } else {
                    MemoryBlock& ram = computer.getRAM();
                    std::cout << "RAM Information:" << std::endl;
                    std::cout << "  Total blocks: " << ram.getTotalBlocks() << std::endl;
                    std::cout << "  Block size: " << ram.getBlockSize() << " bytes" << std::endl;
                    std::cout << "  Total capacity: " << (ram.getTotalBlocks() * ram.getBlockSize()) << " bytes" << std::endl;
                }
            }
            else if (command == "disk") {
                if (args.size() < 2 || args[1] != "info") {
                    std::cerr << "Usage: disk info" << std::endl;
                } else {
                    HardDrive& hdd = computer.getHDD();
                    std::cout << "HDD Information:" << std::endl;
                    std::cout << "  Total blocks: " << hdd.getTotalBlocks() << std::endl;
                    std::cout << "  Block size: " << hdd.getBlockSize() << " bytes" << std::endl;
                    std::cout << "  Total capacity: " << (hdd.getTotalBlocks() * hdd.getBlockSize()) << " bytes" << std::endl;
                }
            }
            else if (command == "poweroff") {
                computer.powerOff();
                std::cout << "Computer powered off." << std::endl;
            }
            else {
                std::cerr << "Unknown command: " << command << std::endl;
                std::cerr << "Type 'help' for available commands." << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
