#ifndef SIMPLE_FILE_SYSTEM_HPP
#define SIMPLE_FILE_SYSTEM_HPP

#include "Disk/HardDrive.hpp"
#include "FileSystem/FileDescriptor.hpp"
#include "LazySequence/SimpleLazySequence.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <cstdint>
#include <memory>
#include <ctime>

// Простая файловая система: FS = (B, F, φ)
class SimpleFileSystem {
private:
    std::vector<bool> free_blocks;
    std::unordered_map<std::string, FileDescriptor> file_table;
    HardDrive& drive;
    
    std::unordered_map<std::string, std::set<std::string>> directory_structure;
    std::string getDirectory(const std::string& path) {
        std::string normalized = path;
        // Убираем завершающий слэш, если он есть (кроме корня)
        if (normalized.size() > 1 && normalized.back() == '/') {
            normalized.pop_back();
        }
        size_t last_slash = normalized.find_last_of('/');
        if (last_slash == std::string::npos) {
            return "/";  // Корневая директория
        }
        if (last_slash == 0) {
            return "/";  // Родитель корня - это корень
        }
        return normalized.substr(0, last_slash);
    }

    std::string getFilename(const std::string& path) {
        std::string normalized = path;
        // Убираем завершающий слэш, если он есть (кроме корня)
        if (normalized.size() > 1 && normalized.back() == '/') {
            normalized.pop_back();
        }
        size_t last_slash = normalized.find_last_of('/');
        if (last_slash == std::string::npos) {
            return normalized;
        }
        return normalized.substr(last_slash + 1);
    }

    std::vector<size_t> findFreeBlocks(size_t count) {
        std::vector<size_t> result;
        for (size_t i = 0; i < free_blocks.size() && result.size() < count; ++i) {
            if (free_blocks[i]) {
                result.push_back(i);
            }
        }
        return result;
    }

    void markBlocksAsUsed(const std::vector<size_t>& blocks) {
        for (size_t block : blocks) {
            if (block < free_blocks.size()) {
                free_blocks[block] = false;
            }
        }
    }

    void markBlocksAsFree(const std::vector<size_t>& blocks) {
        for (size_t block : blocks) {
            if (block < free_blocks.size()) {
                free_blocks[block] = true;
            }
        }
    }

    void ensureDirectoryExists(const std::string& dir_path) {
        if (directory_structure.find(dir_path) == directory_structure.end()) {
            // Создаем директорию
            directory_structure[dir_path] = std::set<std::string>();
            
            // Если это не корневая директория, убеждаемся что родитель существует
            if (dir_path != "/") {
                std::string parent = getDirectory(dir_path);
                ensureDirectoryExists(parent);
                directory_structure[parent].insert(getFilename(dir_path));
            }
        }
    }

    void initializeSystemDirectories() {
        std::vector<std::string> system_dirs = {
            "/bin", "/usr", "/usr/bin", "/usr/lib", "/lib", "/etc", "/home", "/tmp", "/var", "/var/log", "/boot"
        };
        for (const auto& dir : system_dirs) {
            createDirectory(dir);
        }
    }

public:
    SimpleFileSystem(HardDrive& hdd) : drive(hdd) {
        free_blocks.resize(drive.getTotalBlocks(), true);
        directory_structure["/"] = std::set<std::string>();
        initializeSystemDirectories();
    }

    FileDescriptor createFile(const std::string& name) {
        std::string normalized_name = name;
        if (normalized_name[0] != '/') normalized_name = "/" + normalized_name;
        if (file_table.find(normalized_name) != file_table.end()) {
            throw std::runtime_error("File already exists: " + normalized_name);
        }
        std::string dir = getDirectory(normalized_name);
        std::string dir_key = dir;
        if (dir_key != "/" && dir_key.back() != '/') dir_key += "/";
        ensureDirectoryExists(dir_key);
        std::vector<size_t> blocks = findFreeBlocks(1);
        if (blocks.empty()) throw std::runtime_error("No free blocks available");
        FileDescriptor desc(normalized_name, 0, blocks[0], blocks[0], false);
        file_table[normalized_name] = desc;
        markBlocksAsUsed(blocks);
        directory_structure[dir_key].insert(getFilename(normalized_name));
        return desc;
    }

    void createDirectory(const std::string& path) {
        std::string normalized_path = path;
        if (normalized_path[0] != '/') normalized_path = "/" + normalized_path;
        if (normalized_path.back() != '/') normalized_path += "/";
        if (directory_structure.find(normalized_path) != directory_structure.end()) return;
        std::string parent = getDirectory(normalized_path);
        std::string parent_key = parent;
        if (parent_key != "/" && parent_key.back() != '/') parent_key += "/";
        ensureDirectoryExists(parent_key);
        directory_structure[normalized_path] = std::set<std::string>();
        directory_structure[parent_key].insert(getFilename(normalized_path));
        std::vector<size_t> blocks = findFreeBlocks(1);
        if (!blocks.empty()) {
            file_table[normalized_path] = FileDescriptor(normalized_path, 0, blocks[0], blocks[0], true);
            markBlocksAsUsed(blocks);
        }
    }

    void deleteFile(const std::string& name) {
        std::string normalized_name = name;
        if (normalized_name[0] != '/') normalized_name = "/" + normalized_name;
        auto it = file_table.find(normalized_name);
        if (it != file_table.end()) {
            FileDescriptor& desc = it->second;
            std::vector<size_t> blocks;
            for (size_t i = desc.first_block; i <= desc.last_block; ++i) blocks.push_back(i);
            markBlocksAsFree(blocks);
            if (drive.fileExists(normalized_name)) drive.deleteFile(normalized_name);
            std::string dir = getDirectory(normalized_name);
            std::string dir_key = dir;
            if (dir_key != "/" && dir_key.back() != '/') dir_key += "/";
            if (directory_structure.find(dir_key) != directory_structure.end()) {
                directory_structure[dir_key].erase(getFilename(normalized_name));
            }
            file_table.erase(it);
        }
    }

    LazySequence<uint8_t> readFile(const std::string& name) {
        std::string normalized_name = name;
        if (normalized_name[0] != '/') normalized_name = "/" + normalized_name;
        if (file_table.find(normalized_name) == file_table.end()) {
            throw std::runtime_error("File not found: " + normalized_name);
        }
        std::vector<uint8_t> data = drive.readFile(normalized_name);
        
        // Обрезаем до реального размера файла
        const FileDescriptor& desc = file_table.at(normalized_name);
        if (desc.size < data.size()) {
            data.resize(desc.size);
        }
        
        auto data_ptr = std::make_shared<std::vector<uint8_t>>(data);
        auto index_ptr = std::make_shared<size_t>(0);
        auto generator = [data_ptr, index_ptr]() -> uint8_t {
            return (*index_ptr < data_ptr->size()) ? (*data_ptr)[(*index_ptr)++] : 0;
        };
        auto has_next = [data_ptr, index_ptr]() -> bool { return *index_ptr < data_ptr->size(); };
        return LazySequence<uint8_t>(generator, has_next);
    }

    void writeFile(const std::string& name, LazySequence<uint8_t> data) {
        std::string normalized_name = name;
        if (normalized_name[0] != '/') normalized_name = "/" + normalized_name;
        std::vector<uint8_t> file_data;
        while (data.hasNext()) file_data.push_back(data.next());
        size_t block_size = drive.getBlockSize();
        size_t needed_blocks = (file_data.size() + block_size - 1) / block_size;
        if (file_table.find(normalized_name) != file_table.end()) {
            FileDescriptor& old_desc = file_table[normalized_name];
            std::vector<size_t> old_blocks;
            for (size_t i = old_desc.first_block; i <= old_desc.last_block; ++i) old_blocks.push_back(i);
            markBlocksAsFree(old_blocks);
        } else {
            createFile(normalized_name);
        }
        std::vector<size_t> allocated_blocks = findFreeBlocks(needed_blocks);
        if (allocated_blocks.size() < needed_blocks) {
            throw std::runtime_error("Not enough free blocks available");
        }
        drive.writeFile(normalized_name, file_data, allocated_blocks);
        FileDescriptor& desc = file_table[normalized_name];
        desc.size = file_data.size();
        desc.first_block = allocated_blocks.empty() ? 0 : allocated_blocks[0];
        desc.last_block = allocated_blocks.empty() ? 0 : allocated_blocks[needed_blocks - 1];
        desc.modified_time = std::time(nullptr);
        markBlocksAsUsed(allocated_blocks);
    }

    std::vector<std::string> listDirectory(const std::string& path) {
        std::string normalized_path = path;
        if (normalized_path[0] != '/') normalized_path = "/" + normalized_path;
        if (normalized_path.back() != '/') normalized_path += "/";
        std::vector<std::string> result;
        if (directory_structure.find(normalized_path) != directory_structure.end()) {
            for (const auto& entry : directory_structure[normalized_path]) result.push_back(entry);
        }
        return result;
    }

    bool fileExists(const std::string& name) const {
        std::string normalized_name = name;
        if (normalized_name[0] != '/') normalized_name = "/" + normalized_name;
        return file_table.find(normalized_name) != file_table.end();
    }
    std::string getSystemBinPath() const { return "/bin"; }
    std::string getUserBinPath() const { return "/usr/bin"; }
    std::string getSystemLibPath() const { return "/lib"; }
    std::string getConfigPath() const { return "/etc"; }
    std::string getHomePath() const { return "/home"; }
    std::string getTempPath() const { return "/tmp"; }
    std::string getLogPath() const { return "/var/log"; }
    std::string getBootPath() const { return "/boot"; }
};

#endif // SIMPLE_FILE_SYSTEM_HPP

