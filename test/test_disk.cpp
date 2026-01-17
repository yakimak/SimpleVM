#include "test_framework.hpp"
#include "../lib/Disk/HardDrive.hpp"
#include <vector>
#include <cstdint>

void test_disk_creation() {
    HardDrive disk(100, 512);
    ASSERT_EQ(100, disk.getTotalBlocks());
    ASSERT_EQ(512, disk.getBlockSize());
}

void test_disk_write_read() {
    HardDrive disk(10, 64);
    
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05};
    std::vector<size_t> blocks = {0};
    
    disk.writeFile("test.txt", data, blocks);
    ASSERT_TRUE(disk.fileExists("test.txt"));
    
    std::vector<uint8_t> read_data = disk.readFile("test.txt");
    ASSERT_EQ(64, read_data.size()); // Читаем весь блок
    ASSERT_EQ(0x01, read_data[0]);
    ASSERT_EQ(0x02, read_data[1]);
    ASSERT_EQ(0x03, read_data[2]);
    ASSERT_EQ(0x04, read_data[3]);
    ASSERT_EQ(0x05, read_data[4]);
}

void test_disk_multiple_blocks() {
    HardDrive disk(10, 32);
    
    // Создаем данные, которые занимают 2 блока
    std::vector<uint8_t> data(50, 0xAA);
    std::vector<size_t> blocks = {0, 1};
    
    disk.writeFile("large.txt", data, blocks);
    ASSERT_TRUE(disk.fileExists("large.txt"));
    
    std::vector<uint8_t> read_data = disk.readFile("large.txt");
    ASSERT_EQ(64, read_data.size()); // 2 блока * 32 байта
    
    // Проверяем первые байты
    for (size_t i = 0; i < 32; ++i) {
        ASSERT_EQ(0xAA, read_data[i]);
    }
}

void test_disk_file_not_found() {
    HardDrive disk(10, 64);
    ASSERT_THROWS(disk.readFile("nonexistent.txt"), std::runtime_error);
}

void test_disk_delete_file() {
    HardDrive disk(10, 64);
    
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    std::vector<size_t> blocks = {0};
    
    disk.writeFile("temp.txt", data, blocks);
    ASSERT_TRUE(disk.fileExists("temp.txt"));
    
    disk.deleteFile("temp.txt");
    ASSERT_FALSE(disk.fileExists("temp.txt"));
    ASSERT_THROWS(disk.readFile("temp.txt"), std::runtime_error);
}

void test_disk_overwrite_file() {
    HardDrive disk(10, 64);
    
    std::vector<uint8_t> data1 = {0x01, 0x02};
    std::vector<uint8_t> data2 = {0xFF, 0xFE, 0xFD};
    std::vector<size_t> blocks = {0};
    
    disk.writeFile("file.txt", data1, blocks);
    disk.writeFile("file.txt", data2, blocks);
    
    std::vector<uint8_t> read_data = disk.readFile("file.txt");
    ASSERT_EQ(0xFF, read_data[0]);
    ASSERT_EQ(0xFE, read_data[1]);
    ASSERT_EQ(0xFD, read_data[2]);
}

void test_disk_insufficient_blocks() {
    HardDrive disk(10, 32);
    
    std::vector<uint8_t> data(100, 0xAA); // Нужно 4 блока
    std::vector<size_t> blocks = {0, 1}; // Выделено только 2
    
    ASSERT_THROWS(disk.writeFile("file.txt", data, blocks), std::runtime_error);
}

int main() {
    TestFramework framework;
    
    framework.addTest("Disk creation", test_disk_creation);
    framework.addTest("Disk write/read", test_disk_write_read);
    framework.addTest("Disk multiple blocks", test_disk_multiple_blocks);
    framework.addTest("Disk file not found", test_disk_file_not_found);
    framework.addTest("Disk delete file", test_disk_delete_file);
    framework.addTest("Disk overwrite file", test_disk_overwrite_file);
    framework.addTest("Disk insufficient blocks", test_disk_insufficient_blocks);
    
    framework.runAll();
    return framework.getFailedCount() > 0 ? 1 : 0;
}

