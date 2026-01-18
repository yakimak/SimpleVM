#ifndef HARD_DRIVE_HPP
#define HARD_DRIVE_HPP

#include "Memory/MemoryBlock.hpp"
#include "LazySequence/Sequence.h"
#include "LazySequence/LazySequence.h"
#include "CString/cstring_bridge.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

// имитация жесткого диска
class HardDrive {
private:
    MemoryBlock storage;
    // структура мапа имени файла на список блоков, которые он занимает
    std::unordered_map<std::string, std::vector<size_t>> file_blocks;

public:
    HardDrive(size_t total_blocks, size_t block_size)
        : storage(total_blocks, block_size) {}

        
    void writeFile(const String* filename,
                   const std::vector<uint8_t>& data,
                   const std::vector<size_t>& allocated_blocks) {
        writeFile(cstring_bridge::toStdString(filename), data, allocated_blocks);
    }

    // Записать данные в файл, используя предварительно выделенные блоки
    void writeFile(const std::string& filename, 
                   const std::vector<uint8_t>& data,
                   const std::vector<size_t>& allocated_blocks) {
        size_t block_size = storage.getBlockSize();
        size_t needed_blocks = (data.size() + block_size - 1) / block_size;

        if (allocated_blocks.size() < needed_blocks) {
            throw std::runtime_error("Not enough allocated blocks for file: " + filename);
        }

        // Освобождаем старые блоки, если файл существует
        if (file_blocks.find(filename) != file_blocks.end()) {
            file_blocks.erase(filename);
        }

        // Записываем данные в выделенные блоки
        for (size_t i = 0; i < allocated_blocks.size(); ++i) {
            size_t offset = i * block_size;
            if (offset >= data.size()) {
                break;  // Все данные записаны
            }
            
            size_t bytes_to_write = std::min(block_size, data.size() - offset);
            std::vector<uint8_t> block_data(block_size, 0);
            for (size_t j = 0; j < bytes_to_write; ++j) {
                block_data[j] = data[offset + j];
            }
            storage.writeBlock(allocated_blocks[i], block_data);
        }

        // Сохраняем только те блоки, которые реально использованы
        size_t actual_used_blocks = (data.size() + block_size - 1) / block_size;
        std::vector<size_t> used_blocks(allocated_blocks.begin(), 
                                        allocated_blocks.begin() + actual_used_blocks);
        file_blocks[filename] = used_blocks;
    }

    std::vector<uint8_t> readFile(const String* filename) {
        return readFile(cstring_bridge::toStdString(filename));
    }

    std::vector<uint8_t> readFile(const std::string& filename) {
        if (file_blocks.find(filename) == file_blocks.end()) {
            throw std::runtime_error("File not found: " + filename);
        }

        std::vector<uint8_t> result;
        size_t block_size = storage.getBlockSize();
        const auto& blocks = file_blocks[filename];

        for (size_t block_id : blocks) {
            std::vector<uint8_t> block_data = storage.readBlock(block_id);
            result.insert(result.end(), block_data.begin(), block_data.end());
        }

        return result;
    }

    void deleteFile(const String* filename) { deleteFile(cstring_bridge::toStdString(filename)); }
    void deleteFile(const std::string& filename) {
        file_blocks.erase(filename);
    }

    bool fileExists(const String* filename) const { return fileExists(cstring_bridge::toStdString(filename)); }
    bool fileExists(const std::string& filename) const {
        return file_blocks.find(filename) != file_blocks.end();
    }

    size_t getTotalBlocks() const { return storage.getTotalBlocks(); }
    size_t getBlockSize() const { return storage.getBlockSize(); }
};

#endif // HARD_DRIVE_HPP

