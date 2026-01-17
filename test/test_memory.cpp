#include "test_framework.hpp"
#include "../lib/Memory/MemoryBlock.hpp"
#include <vector>
#include <cstdint>

void test_memory_creation() {
    MemoryBlock mem(100, 64);
    ASSERT_EQ(100, mem.getTotalBlocks());
    ASSERT_EQ(64, mem.getBlockSize());
}

void test_memory_read_write() {
    MemoryBlock mem(10, 32);
    
    // Записываем данные в блок
    std::vector<uint8_t> data(32, 0xAA);
    mem.writeBlock(0, data);
    
    // Читаем данные
    std::vector<uint8_t> read_data = mem.readBlock(0);
    ASSERT_EQ(32, read_data.size());
    for (size_t i = 0; i < 32; ++i) {
        ASSERT_EQ(0xAA, read_data[i]);
    }
}

void test_memory_multiple_blocks() {
    MemoryBlock mem(5, 16);
    
    // Записываем разные данные в разные блоки
    for (size_t i = 0; i < 5; ++i) {
        std::vector<uint8_t> data(16, static_cast<uint8_t>(i));
        mem.writeBlock(i, data);
    }
    
    // Проверяем, что данные сохранились
    for (size_t i = 0; i < 5; ++i) {
        std::vector<uint8_t> read_data = mem.readBlock(i);
        ASSERT_EQ(16, read_data.size());
        ASSERT_EQ(static_cast<uint8_t>(i), read_data[0]);
    }
}

void test_memory_out_of_range_read() {
    MemoryBlock mem(10, 32);
    ASSERT_THROWS(mem.readBlock(10), std::out_of_range);
    ASSERT_THROWS(mem.readBlock(100), std::out_of_range);
}

void test_memory_out_of_range_write() {
    MemoryBlock mem(10, 32);
    std::vector<uint8_t> data(32, 0);
    ASSERT_THROWS(mem.writeBlock(10, data), std::out_of_range);
}

void test_memory_invalid_data_size() {
    MemoryBlock mem(10, 32);
    std::vector<uint8_t> wrong_size(16, 0);
    ASSERT_THROWS(mem.writeBlock(0, wrong_size), std::invalid_argument);
}

void test_memory_zero_initialization() {
    MemoryBlock mem(5, 16);
    std::vector<uint8_t> read_data = mem.readBlock(0);
    ASSERT_EQ(16, read_data.size());
    for (size_t i = 0; i < 16; ++i) {
        ASSERT_EQ(0, read_data[i]);
    }
}

int main() {
    TestFramework framework;
    
    framework.addTest("Memory creation", test_memory_creation);
    framework.addTest("Memory read/write", test_memory_read_write);
    framework.addTest("Memory multiple blocks", test_memory_multiple_blocks);
    framework.addTest("Memory out of range read", test_memory_out_of_range_read);
    framework.addTest("Memory out of range write", test_memory_out_of_range_write);
    framework.addTest("Memory invalid data size", test_memory_invalid_data_size);
    framework.addTest("Memory zero initialization", test_memory_zero_initialization);
    
    framework.runAll();
    return framework.getFailedCount() > 0 ? 1 : 0;
}

