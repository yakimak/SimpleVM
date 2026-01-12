#include "Computer.hpp"
#include <iostream>
#include <vector>
#include <memory>

// Компьютер = [CPU] + [Память] + [BIOS] + [Диск] + [ФС]

int main() {
    try {
        // Создаем компьютер
        Computer computer;
        
        // Включаем компьютер
        std::cout << "Powering on computer..." << std::endl;
        computer.powerOn();
        std::cout << "Computer is now powered on." << std::endl;
        
        // Создаем файловую систему и тестируем основные операции
        SimpleFileSystem& fs = computer.getFileSystem();
        
        // Показываем структуру системных директорий
        std::cout << "\nSystem directories structure:" << std::endl;
        std::vector<std::string> root_dirs = fs.listDirectory("/");
        for (const auto& dir : root_dirs) {
            std::cout << "  /" << dir << "/" << std::endl;
        }
        
        // Создаем файл в системной директории /home
        std::cout << "\nCreating file /home/test.txt..." << std::endl;
        fs.createFile("/home/test.txt");
        
        // Записываем данные в файл
        std::vector<uint8_t> test_data = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        auto data_ptr = std::make_shared<std::vector<uint8_t>>(test_data);
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
        fs.writeFile("/home/test.txt", write_stream);
        
        // Читаем файл
        std::cout << "Reading file /home/test.txt..." << std::endl;
        LazySequence<uint8_t> read_stream = fs.readFile("/home/test.txt");
        std::cout << "File content: ";
        while (read_stream.hasNext()) {
            std::cout << static_cast<char>(read_stream.next());
        }
        std::cout << std::endl;
        
        // Список файлов в директории
        std::cout << "\nListing directory /home:" << std::endl;
        std::vector<std::string> files = fs.listDirectory("/home");
        for (const auto& file : files) {
            std::cout << "  " << file << std::endl;
        }
        
        // Демонстрация работы с системными директориями
        std::cout << "\nSystem paths:" << std::endl;
        std::cout << "  System binaries: " << fs.getSystemBinPath() << std::endl;
        std::cout << "  User binaries: " << fs.getUserBinPath() << std::endl;
        std::cout << "  Libraries: " << fs.getSystemLibPath() << std::endl;
        std::cout << "  Config: " << fs.getConfigPath() << std::endl;
        std::cout << "  Home: " << fs.getHomePath() << std::endl;
        std::cout << "  Temp: " << fs.getTempPath() << std::endl;
        std::cout << "  Logs: " << fs.getLogPath() << std::endl;
        std::cout << "  Boot: " << fs.getBootPath() << std::endl;
        
        // Показываем структуру /usr
        std::cout << "\nListing /usr directory:" << std::endl;
        std::vector<std::string> usr_dirs = fs.listDirectory("/usr");
        for (const auto& dir : usr_dirs) {
            std::cout << "  " << dir << "/" << std::endl;
        }
        
        // Тестируем процессор
        std::cout << "\nTesting CPU (Stack Machine)..." << std::endl;
        StackMachine& cpu = computer.getCPU();
        
        // Выполняем несколько команд
        for (int i = 0; i < 3; ++i) {
            cpu.executeNext();
        }
        
        std::cout << "CPU program counter: " << cpu.getProgramCounter() << std::endl;
        std::cout << "CPU stack size: " << cpu.getStackSize() << std::endl;
        
        // Выключаем компьютер
        std::cout << "\nPowering off computer..." << std::endl;
        computer.powerOff();
        std::cout << "Computer is now powered off." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

