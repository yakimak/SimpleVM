#ifndef MEMORY_BLOCK_HPP
#define MEMORY_BLOCK_HPP

#include <vector>
#include <cstdint>
#include <stdexcept>
#include <string>

/**
 * Класс MemoryBlock - блочная память
 * M = {B_0, B_1, ..., B_{n-1}}
 * где каждый блок B_i — вектор байтов фиксированного размера
 */
class MemoryBlock {
private:
    std::vector<std::vector<uint8_t>> blocks;
    size_t block_size;
    size_t total_blocks;

public:
    MemoryBlock(size_t blocks_count, size_t block_size)
        : total_blocks(blocks_count), block_size(block_size) {
        blocks.resize(blocks_count, std::vector<uint8_t>(block_size, 0));
    }

    std::vector<uint8_t> readBlock(size_t block_id) {
        if (block_id >= total_blocks) {
            throw std::out_of_range("Invalid block_id: " + std::to_string(block_id));
        }
        return blocks[block_id];
    }

    void writeBlock(size_t block_id, const std::vector<uint8_t>& data) {
        if (block_id >= total_blocks) {
            throw std::out_of_range("Invalid block_id: " + std::to_string(block_id));
        }
        if (data.size() != block_size) {
            throw std::invalid_argument("Invalid data size: expected " + 
                                      std::to_string(block_size) + ", got " + 
                                      std::to_string(data.size()));
        }
        blocks[block_id] = data;
    }

    size_t getBlockSize() const { return block_size; }
    size_t getTotalBlocks() const { return total_blocks; }

    // Простая проверка исправности (self-test).
    // В реальной системе здесь мог бы быть тест чтения/записи блоков.
    bool selfTest() const {
        return total_blocks > 0 && block_size > 0 && blocks.size() == total_blocks;
    }
};

#endif // MEMORY_BLOCK_HPP

