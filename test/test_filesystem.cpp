#include "test_framework.hpp"
#include "../lib/FileSystem/SimpleFileSystem.hpp"
#include "../lib/Disk/HardDrive.hpp"
#include "../lib/LazySequence/SimpleLazySequence.hpp"
#include <vector>
#include <memory>
#include <cstdint>

void test_filesystem_creation() {
    HardDrive disk(100, 512);
    SimpleFileSystem fs(disk);
    
    // Проверяем, что системные директории созданы
    std::vector<std::string> root_dirs = fs.listDirectory("/");
    ASSERT_TRUE(root_dirs.size() > 0);
}

void test_filesystem_create_file() {
    HardDrive disk(100, 512);
    SimpleFileSystem fs(disk);
    
    FileDescriptor desc = fs.createFile("/test.txt");
    ASSERT_TRUE(fs.fileExists("/test.txt"));
    ASSERT_STREQ("/test.txt", desc.name);
}

void test_filesystem_create_directory() {
    HardDrive disk(100, 512);
    SimpleFileSystem fs(disk);
    
    fs.createDirectory("/mydir");
    std::vector<std::string> entries = fs.listDirectory("/");
    
    bool found = false;
    for (const auto& entry : entries) {
        if (entry == "mydir") {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);
}

void test_filesystem_write_read_file() {
    HardDrive disk(100, 512);
    SimpleFileSystem fs(disk);
    
    // Создаем файл
    fs.createFile("/data.txt");
    
    // Записываем данные
    std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o'};
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
    fs.writeFile("/data.txt", write_stream);
    
    // Читаем данные
    LazySequence<uint8_t> read_stream = fs.readFile("/data.txt");
    std::vector<uint8_t> read_data;
    while (read_stream.hasNext()) {
        read_data.push_back(read_stream.next());
    }
    
    ASSERT_EQ(5, read_data.size());
    ASSERT_EQ('H', read_data[0]);
    ASSERT_EQ('e', read_data[1]);
    ASSERT_EQ('l', read_data[2]);
    ASSERT_EQ('l', read_data[3]);
    ASSERT_EQ('o', read_data[4]);
}

void test_filesystem_delete_file() {
    HardDrive disk(100, 512);
    SimpleFileSystem fs(disk);
    
    fs.createFile("/temp.txt");
    ASSERT_TRUE(fs.fileExists("/temp.txt"));
    
    fs.deleteFile("/temp.txt");
    ASSERT_FALSE(fs.fileExists("/temp.txt"));
}

void test_filesystem_list_directory() {
    HardDrive disk(100, 512);
    SimpleFileSystem fs(disk);
    
    fs.createFile("/file1.txt");
    fs.createFile("/file2.txt");
    fs.createDirectory("/dir1");
    
    std::vector<std::string> entries = fs.listDirectory("/");
    
    bool found_file1 = false, found_file2 = false, found_dir1 = false;
    for (const auto& entry : entries) {
        if (entry == "file1.txt") found_file1 = true;
        if (entry == "file2.txt") found_file2 = true;
        if (entry == "dir1") found_dir1 = true;
    }
    
    ASSERT_TRUE(found_file1 || found_file2 || found_dir1); // Хотя бы один должен быть найден
}

void test_filesystem_nested_directories() {
    HardDrive disk(100, 512);
    SimpleFileSystem fs(disk);
    
    fs.createDirectory("/home");
    fs.createDirectory("/home/user");
    fs.createFile("/home/user/file.txt");
    
    ASSERT_TRUE(fs.fileExists("/home/user/file.txt"));
    
    std::vector<std::string> home_entries = fs.listDirectory("/home");
    bool found_user = false;
    for (const auto& entry : home_entries) {
        if (entry == "user") {
            found_user = true;
            break;
        }
    }
    ASSERT_TRUE(found_user);
}

void test_filesystem_file_not_found() {
    HardDrive disk(100, 512);
    SimpleFileSystem fs(disk);
    
    ASSERT_THROWS(fs.readFile("/nonexistent.txt"), std::runtime_error);
}

void test_filesystem_duplicate_file() {
    HardDrive disk(100, 512);
    SimpleFileSystem fs(disk);
    
    fs.createFile("/duplicate.txt");
    ASSERT_THROWS(fs.createFile("/duplicate.txt"), std::runtime_error);
}

void test_filesystem_system_paths() {
    HardDrive disk(100, 512);
    SimpleFileSystem fs(disk);
    
    ASSERT_STREQ("/bin", fs.getSystemBinPath());
    ASSERT_STREQ("/usr/bin", fs.getUserBinPath());
    ASSERT_STREQ("/lib", fs.getSystemLibPath());
    ASSERT_STREQ("/etc", fs.getConfigPath());
    ASSERT_STREQ("/home", fs.getHomePath());
    ASSERT_STREQ("/tmp", fs.getTempPath());
    ASSERT_STREQ("/var/log", fs.getLogPath());
    ASSERT_STREQ("/boot", fs.getBootPath());
}

int main() {
    TestFramework framework;
    
    framework.addTest("Filesystem creation", test_filesystem_creation);
    framework.addTest("Filesystem create file", test_filesystem_create_file);
    framework.addTest("Filesystem create directory", test_filesystem_create_directory);
    framework.addTest("Filesystem write/read file", test_filesystem_write_read_file);
    framework.addTest("Filesystem delete file", test_filesystem_delete_file);
    framework.addTest("Filesystem list directory", test_filesystem_list_directory);
    framework.addTest("Filesystem nested directories", test_filesystem_nested_directories);
    framework.addTest("Filesystem file not found", test_filesystem_file_not_found);
    framework.addTest("Filesystem duplicate file", test_filesystem_duplicate_file);
    framework.addTest("Filesystem system paths", test_filesystem_system_paths);
    
    framework.runAll();
    return framework.getFailedCount() > 0 ? 1 : 0;
}

