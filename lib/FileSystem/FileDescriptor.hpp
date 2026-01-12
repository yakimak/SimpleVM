#ifndef FILE_DESCRIPTOR_HPP
#define FILE_DESCRIPTOR_HPP

#include <string>
#include <ctime>
#include <cstddef>

/**
 * Дескриптор файла
 */
struct FileDescriptor {
    std::string name;
    size_t size;
    size_t first_block;
    size_t last_block;
    time_t created_time;
    time_t modified_time;
    bool is_directory;

    FileDescriptor()
        : size(0), first_block(0), last_block(0),
          created_time(0), modified_time(0), is_directory(false) {}

    FileDescriptor(const std::string& n, size_t sz, size_t first, size_t last, bool is_dir = false)
        : name(n), size(sz), first_block(first), last_block(last),
          created_time(std::time(nullptr)), modified_time(std::time(nullptr)),
          is_directory(is_dir) {}
};

#endif // FILE_DESCRIPTOR_HPP

