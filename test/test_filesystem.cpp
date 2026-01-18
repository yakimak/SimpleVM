#include "test_framework.hpp"
#include "VirtualFS/virtual_file_system.h"
#include <vector>

static std::vector<std::string> listDir(vfs::VirtualFileSystem& fs, const std::string& path) {
    vfs::Node* n = fs.Resolve(path);
    if (!n || n->GetType() != vfs::NodeType::Directory) return {};
    auto* dir = static_cast<vfs::DirectoryNode*>(n);
    std::vector<std::string> out;
    for (const auto& ch : dir->GetChildren()) out.push_back(ch->GetName());
    return out;
}

void test_filesystem_creation() {
    vfs::VirtualFileSystem fs;
    ASSERT_TRUE(fs.GetRoot() != nullptr);
    ASSERT_TRUE(fs.Resolve("/") != nullptr);
}

void test_filesystem_create_file() {
    vfs::VirtualFileSystem fs;
    fs.AttachFile("/test.txt", "/tmp/test.txt");
    vfs::Node* n = fs.Resolve("/test.txt");
    ASSERT_TRUE(n != nullptr);
    ASSERT_TRUE(n->GetType() == vfs::NodeType::File);
}

void test_filesystem_create_directory() {
    vfs::VirtualFileSystem fs;
    fs.MakeDirectory("/mydir");
    std::vector<std::string> entries = listDir(fs, "/");
    bool found = false;
    for (const auto& entry : entries) if (entry == "mydir") found = true;
    ASSERT_TRUE(found);
}

void test_filesystem_delete_file() {
    vfs::VirtualFileSystem fs;
    fs.AttachFile("/temp.txt", "/tmp/temp.txt");
    ASSERT_TRUE(fs.Resolve("/temp.txt") != nullptr);
    fs.Remove("/temp.txt");
    ASSERT_TRUE(fs.Resolve("/temp.txt") == nullptr);
}

void test_filesystem_list_directory() {
    vfs::VirtualFileSystem fs;
    fs.AttachFile("/file1.txt", "/tmp/file1.txt");
    fs.AttachFile("/file2.txt", "/tmp/file2.txt");
    fs.MakeDirectory("/dir1");

    std::vector<std::string> entries = listDir(fs, "/");
    bool found_file1 = false, found_file2 = false, found_dir1 = false;
    for (const auto& entry : entries) {
        if (entry == "file1.txt") found_file1 = true;
        if (entry == "file2.txt") found_file2 = true;
        if (entry == "dir1") found_dir1 = true;
    }
    ASSERT_TRUE(found_file1);
    ASSERT_TRUE(found_file2);
    ASSERT_TRUE(found_dir1);
}

void test_filesystem_nested_directories() {
    vfs::VirtualFileSystem fs;
    fs.MakeDirectory("/home");
    fs.MakeDirectory("/home/user");
    fs.AttachFile("/home/user/file.txt", "/tmp/file.txt");

    vfs::Node* n = fs.Resolve("/home/user/file.txt");
    ASSERT_TRUE(n != nullptr);
    ASSERT_TRUE(n->GetType() == vfs::NodeType::File);

    std::vector<std::string> home_entries = listDir(fs, "/home");
    bool found_user = false;
    for (const auto& entry : home_entries) if (entry == "user") found_user = true;
    ASSERT_TRUE(found_user);
}

void test_filesystem_file_not_found() {
    vfs::VirtualFileSystem fs;
    ASSERT_TRUE(fs.Resolve("/nonexistent.txt") == nullptr);
}

void test_filesystem_duplicate_file() {
    vfs::VirtualFileSystem fs;
    fs.AttachFile("/duplicate.txt", "/tmp/dup.txt");
    ASSERT_THROWS(fs.AttachFile("/duplicate.txt", "/tmp/dup2.txt"), std::invalid_argument);
}

void test_filesystem_move_and_find() {
    vfs::VirtualFileSystem fs;
    fs.MakeDirectory("/dir");
    fs.AttachFile("/dir/a.txt", "/tmp/a.txt");
    auto found = fs.FindFilesByName("a.txt");
    ASSERT_TRUE(!found.empty());

    fs.Move("/dir/a.txt", "/dir/b.txt");
    ASSERT_TRUE(fs.Resolve("/dir/a.txt") == nullptr);
    ASSERT_TRUE(fs.Resolve("/dir/b.txt") != nullptr);
}

int main() {
    TestFramework framework;
    
    framework.addTest("Filesystem creation", test_filesystem_creation);
    framework.addTest("Filesystem create file", test_filesystem_create_file);
    framework.addTest("Filesystem create directory", test_filesystem_create_directory);
    framework.addTest("Filesystem delete file", test_filesystem_delete_file);
    framework.addTest("Filesystem list directory", test_filesystem_list_directory);
    framework.addTest("Filesystem nested directories", test_filesystem_nested_directories);
    framework.addTest("Filesystem file not found", test_filesystem_file_not_found);
    framework.addTest("Filesystem duplicate file", test_filesystem_duplicate_file);
    framework.addTest("Filesystem move/find", test_filesystem_move_and_find);
    
    framework.runAll();
    return framework.getFailedCount() > 0 ? 1 : 0;
}

