#include "test_framework.hpp"
#include "../lib/Computer.hpp"
#include <stdexcept>

void test_computer_creation() {
    Computer computer;
    ASSERT_FALSE(computer.isPoweredOn());
}

void test_computer_power_on_off() {
    Computer computer;
    
    ASSERT_FALSE(computer.isPoweredOn());
    computer.powerOn();
    ASSERT_TRUE(computer.isPoweredOn());
    computer.powerOff();
    ASSERT_FALSE(computer.isPoweredOn());
}

void test_computer_ram_access() {
    Computer computer;
    computer.powerOn();
    
    MemoryBlock& ram = computer.getRAM();
    ASSERT_EQ(1024, ram.getTotalBlocks());
    ASSERT_EQ(64, ram.getBlockSize());
}

void test_computer_hdd_access() {
    Computer computer;
    computer.powerOn();
    
    HardDrive& hdd = computer.getHDD();
    ASSERT_EQ(4096, hdd.getTotalBlocks());
    ASSERT_EQ(512, hdd.getBlockSize());
}

void test_computer_filesystem_access() {
    Computer computer;
    computer.powerOn();
    
    vfs::VirtualFileSystem& fs = computer.getFileSystem();
    vfs::Node* bin = fs.Resolve("/bin");
    ASSERT_TRUE(bin != nullptr);
    ASSERT_TRUE(bin->GetType() == vfs::NodeType::Directory);
}

void test_computer_cpu_access() {
    Computer computer;
    computer.powerOn();
    
    StackMachine& cpu = computer.getCPU();
    ASSERT_EQ(0, cpu.getProgramCounter());
    ASSERT_TRUE(cpu.isStackEmpty());
}

void test_computer_cpu_execution() {
    Computer computer;
    computer.powerOn();
    
    StackMachine& cpu = computer.getCPU();
    
    // Выполняем несколько команд из bootloader
    cpu.executeNext(); // PUSH 1
    ASSERT_EQ(1, cpu.getProgramCounter());
    ASSERT_EQ(1, cpu.getStackSize());
    
    cpu.executeNext(); // PUSH 2
    ASSERT_EQ(2, cpu.getProgramCounter());
    ASSERT_EQ(2, cpu.getStackSize());
    
    cpu.executeNext(); // ADD
    ASSERT_EQ(3, cpu.getProgramCounter());
    ASSERT_EQ(1, cpu.getStackSize());
    
    int result = cpu.pop();
    ASSERT_EQ(3, result); // 1 + 2 = 3
}

void test_computer_load_os_not_powered() {
    Computer computer;
    
    ASSERT_THROWS(computer.loadOS("os.bin"), std::runtime_error);
}

void test_computer_run_command_not_powered() {
    Computer computer;
    
    ASSERT_THROWS(computer.runCommand("ls"), std::runtime_error);
}

void test_computer_filesystem_operations() {
    Computer computer;
    computer.powerOn();
    
    vfs::VirtualFileSystem& fs = computer.getFileSystem();
    
    // Создаем файл
    fs.AttachFile("/test.txt", "/tmp/test.txt");
    ASSERT_TRUE(fs.Resolve("/test.txt") != nullptr);
    
    // Удаляем файл
    fs.Remove("/test.txt");
    ASSERT_TRUE(fs.Resolve("/test.txt") == nullptr);
}

int main() {
    TestFramework framework;
    
    framework.addTest("Computer creation", test_computer_creation);
    framework.addTest("Computer power on/off", test_computer_power_on_off);
    framework.addTest("Computer RAM access", test_computer_ram_access);
    framework.addTest("Computer HDD access", test_computer_hdd_access);
    framework.addTest("Computer filesystem access", test_computer_filesystem_access);
    framework.addTest("Computer CPU access", test_computer_cpu_access);
    framework.addTest("Computer CPU execution", test_computer_cpu_execution);
    framework.addTest("Computer load OS not powered", test_computer_load_os_not_powered);
    framework.addTest("Computer run command not powered", test_computer_run_command_not_powered);
    framework.addTest("Computer filesystem operations", test_computer_filesystem_operations);
    
    framework.runAll();
    return framework.getFailedCount() > 0 ? 1 : 0;
}

