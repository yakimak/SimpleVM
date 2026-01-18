#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================
// Виртуальная файловая система в оперативной памяти
// ============================================
//
// Идея:
//  - описываем дерево директорий и файлов, существующее только в памяти;
//  - каждый виртуальный файл "ссылается" на существующий физический путь;
//  - операции во ВФС НЕ изменяют реальную ФС:
//      * "создание" файла = прикрепление существующего физического пути;
//      * удаление/перемещение/переименование затрагивает только виртуальное дерево.

namespace vfs {

// Простая модель прав доступа: битовая маска флагов.
enum class Permission : std::uint8_t {
    None    = 0,
    Read    = 1 << 0,
    Write   = 1 << 1,
    Execute = 1 << 2
};

inline Permission operator|(Permission a, Permission b) {
    return static_cast<Permission>(static_cast<std::uint8_t>(a) |
                                   static_cast<std::uint8_t>(b));
}

inline Permission operator&(Permission a, Permission b) {
    return static_cast<Permission>(static_cast<std::uint8_t>(a) &
                                   static_cast<std::uint8_t>(b));
}

inline bool Any(Permission p) {
    return static_cast<std::uint8_t>(p) != 0;
}

struct AccessControl {
    Permission mask = Permission::Read | Permission::Write;

    bool CanRead() const    { return Any(mask & Permission::Read); }
    bool CanWrite() const   { return Any(mask & Permission::Write); }
    bool CanExecute() const { return Any(mask & Permission::Execute); }
};

enum class NodeType { Directory, File };

// Базовый узел дерева ВФС.
class Node {
public:
    Node(std::string name, NodeType type, AccessControl ac, Node* parent = nullptr)
        : name_(std::move(name)), type_(type), access_(ac), parent_(parent) {}

    virtual ~Node() = default;

    const std::string& GetName() const { return name_; }
    void Rename(const std::string& new_name) { name_ = new_name; }

    NodeType GetType() const { return type_; }

    const AccessControl& GetAccess() const { return access_; }
    void SetAccess(const AccessControl& ac) { access_ = ac; }

    Node* GetParent() const { return parent_; }
    void SetParent(Node* p) { parent_ = p; }

    // Виртуальный путь от корня, вида /dir1/dir2/file
    std::string GetVirtualPath() const {
        if (!parent_) return "/"; // корень
        std::vector<std::string> comps;
        const Node* cur = this;
        while (cur && cur->parent_) {
            comps.push_back(cur->name_);
            cur = cur->parent_;
        }
        std::string result = "/";
        for (auto it = comps.rbegin(); it != comps.rend(); ++it) {
            if (it != comps.rbegin()) result += "/";
            result += *it;
        }
        return result;
    }

private:
    std::string name_;
    NodeType type_;
    AccessControl access_;
    Node* parent_;
};

// Виртуальный файл: хранит путь до реального файла.
class FileNode : public Node {
public:
    FileNode(std::string name,
             std::string physical_path,
             AccessControl ac,
             Node* parent = nullptr)
        : Node(std::move(name), NodeType::File, ac, parent),
          physical_path_(std::move(physical_path)) {}

    const std::string& GetPhysicalPath() const { return physical_path_; }

    void SetPhysicalPath(const std::string& new_path) {
        // Меняем только строку, реальный файл на диске не трогаем.
        physical_path_ = new_path;
    }

private:
    std::string physical_path_;
};

// Директория ВФС.
class DirectoryNode : public Node {
public:
    explicit DirectoryNode(std::string name,
                           AccessControl ac,
                           Node* parent = nullptr)
        : Node(std::move(name), NodeType::Directory, ac, parent) {}

    const std::vector<std::unique_ptr<Node>>& GetChildren() const {
        return children_;
    }

    Node* FindChild(const std::string& name) const {
        for (const auto& ch : children_) {
            if (ch->GetName() == name) return ch.get();
        }
        return nullptr;
    }

    DirectoryNode* AddDirectory(const std::string& name, AccessControl ac) {
        if (FindChild(name)) {
            throw std::invalid_argument("DirectoryNode::AddDirectory: name already exists");
        }
        children_.push_back(std::make_unique<DirectoryNode>(name, ac, this));
        return static_cast<DirectoryNode*>(children_.back().get());
    }

    FileNode* AddFile(const std::string& name,
                      const std::string& physical_path,
                      AccessControl ac) {
        if (FindChild(name)) {
            throw std::invalid_argument("DirectoryNode::AddFile: name already exists");
        }
        children_.push_back(
            std::make_unique<FileNode>(name, physical_path, ac, this));
        return static_cast<FileNode*>(children_.back().get());
    }

    // Удалить ребёнка по указателю (из ВФС), не трогая физический файл.
    void RemoveChild(Node* child) {
        for (auto it = children_.begin(); it != children_.end(); ++it) {
            if (it->get() == child) {
                children_.erase(it);
                return;
            }
        }
        throw std::invalid_argument("DirectoryNode::RemoveChild: child not found");
    }

private:
    std::vector<std::unique_ptr<Node>> children_;
};

// ==========================================
// VirtualFileSystem: корень + индексы поиска
// ==========================================

class VirtualFileSystem {
public:
    VirtualFileSystem() {
        AccessControl ac;
        ac.mask = Permission::Read | Permission::Write | Permission::Execute;
        root_ = std::make_unique<DirectoryNode>("", ac, nullptr);
    }

    DirectoryNode* GetRoot() const { return root_.get(); }

    // Прикрепить физический файл по виртуальному пути, вида "/dir1/dir2/file.txt".
    // При необходимости создаёт недостающие директории.
    FileNode* AttachFile(const std::string& virtual_path,
                         const std::string& physical_path,
                         AccessControl ac = AccessControl{}) {
        auto [parentDir, name] = ensure_parent_directory(virtual_path);
        FileNode* file = parentDir->AddFile(name, physical_path, ac);
        index_insert(name, file);
        return file;
    }

    // Создать директорию по виртуальному пути (без привязки к физическому каталогу).
    DirectoryNode* MakeDirectory(const std::string& virtual_path,
                                 AccessControl ac = AccessControl{}) {
        auto [parentDir, name] = ensure_parent_directory(virtual_path);
        return parentDir->AddDirectory(name, ac);
    }

    // Удалить узел из ВФС по виртуальному пути (физический файл НЕ удаляется).
    void Remove(const std::string& virtual_path) {
        Node* node = Resolve(virtual_path);
        if (!node || node == root_.get()) {
            throw std::invalid_argument("VirtualFileSystem::Remove: invalid path");
        }
        if (node->GetType() == NodeType::File) {
            index_erase(node->GetName(), node);
        }
        DirectoryNode* parent =
            dynamic_cast<DirectoryNode*>(node->GetParent());
        if (!parent) {
            throw std::runtime_error("VirtualFileSystem::Remove: parent is not directory");
        }
        parent->RemoveChild(node);
    }

    // Перемещение/переименование внутри ВФС (физический путь не меняется).
    void Move(const std::string& from_virtual_path,
              const std::string& to_virtual_path) {
        Node* node = Resolve(from_virtual_path);
        if (!node || node == root_.get()) {
            throw std::invalid_argument("VirtualFileSystem::Move: invalid source path");
        }

        auto [newParent, newName] = ensure_parent_directory(to_virtual_path);

        // Запрещаем ситуацию, когда в одной директории одновременно
        // есть и файл, и директория с одинаковым именем. Если в целевой
        // директории уже есть узел с таким именем — бросаем исключение.
        if (newParent->FindChild(newName) != nullptr) {
            throw std::invalid_argument("VirtualFileSystem::Move: target name already exists in destination directory");
        }

        DirectoryNode* oldParent =
            dynamic_cast<DirectoryNode*>(node->GetParent());
        if (!oldParent) {
            throw std::runtime_error("VirtualFileSystem::Move: parent is not directory");
        }
        // временно вынимаем узел из старого родителя
        std::unique_ptr<Node> owned;
        {
            auto& children = const_cast<std::vector<std::unique_ptr<Node>>&>(
                oldParent->GetChildren());
            for (auto it = children.begin(); it != children.end(); ++it) {
                if (it->get() == node) {
                    owned = std::move(*it);
                    children.erase(it);
                    break;
                }
            }
        }
        if (!owned) {
            throw std::runtime_error("VirtualFileSystem::Move: node extraction failed");
        }

        // если это файл — обновляем индекс по имени
        if (owned->GetType() == NodeType::File) {
            index_erase(owned->GetName(), owned.get());
        }

        owned->SetParent(newParent);
        owned->Rename(newName);
        if (owned->GetType() == NodeType::File) {
            index_insert(newName, owned.get());
        }

        auto& dstChildren = const_cast<std::vector<std::unique_ptr<Node>>&>(
            newParent->GetChildren());
        dstChildren.push_back(std::move(owned));
    }

    // Поиск всех файлов по имени (без учёта пути).
    std::vector<FileNode*> FindFilesByName(const std::string& name) const {
        std::vector<FileNode*> result;
        auto it = index_.find(name);
        if (it != index_.end()) {
            for (Node* node : it->second) {
                if (node->GetType() == NodeType::File) {
                    result.push_back(static_cast<FileNode*>(node));
                }
            }
        }
        return result;
    }

    // Разрешение произвольного виртуального пути в узел (или nullptr).
    Node* Resolve(const std::string& virtual_path) const {
        if (virtual_path.empty() || virtual_path[0] != '/') return nullptr;
        if (virtual_path == "/") return root_.get();

        std::vector<std::string> comps = split_path(virtual_path);
        Node* cur = root_.get();
        for (const std::string& name : comps) {
            DirectoryNode* dir = dynamic_cast<DirectoryNode*>(cur);
            if (!dir) return nullptr;
            cur = dir->FindChild(name);
            if (!cur) return nullptr;
        }
        return cur;
    }

private:
    std::unique_ptr<DirectoryNode> root_;
    // Индекс: имя файла -> список узлов с таким именем.
    std::unordered_map<std::string, std::vector<Node*>> index_;

    static std::vector<std::string> split_path(const std::string& path) {
        std::vector<std::string> res;
        std::string cur;
        for (char ch : path) {
            if (ch == '/') {
                if (!cur.empty()) {
                    res.push_back(cur);
                    cur.clear();
                }
            } else {
                cur.push_back(ch);
            }
        }
        if (!cur.empty()) res.push_back(cur);
        return res;
    }

    // Убедиться, что директория-родитель существует; при необходимости создать.
    // Возвращает пару (указатель на родительскую директорию, имя последнего компонента).
    std::pair<DirectoryNode*, std::string>
    ensure_parent_directory(const std::string& virtual_path) {
        if (virtual_path.empty() || virtual_path[0] != '/') {
            throw std::invalid_argument("VirtualFileSystem: path must start with '/'");
        }
        std::vector<std::string> comps = split_path(virtual_path);
        if (comps.empty()) {
            throw std::invalid_argument("VirtualFileSystem: empty path");
        }
        std::string leaf_name = comps.back();
        comps.pop_back();

        DirectoryNode* dir = root_.get();
        AccessControl default_ac;
        default_ac.mask = Permission::Read | Permission::Write | Permission::Execute;

        for (const std::string& name : comps) {
            Node* child = dir->FindChild(name);
            if (!child) {
                dir = dir->AddDirectory(name, default_ac);
            } else {
                dir = dynamic_cast<DirectoryNode*>(child);
                if (!dir) {
                    throw std::invalid_argument(
                        "VirtualFileSystem: intermediate path component is not a directory");
                }
            }
        }
        return {dir, leaf_name};
    }

    void index_insert(const std::string& name, Node* node) {
        index_[name].push_back(node);
    }

    void index_erase(const std::string& name, Node* node) {
        auto it = index_.find(name);
        if (it == index_.end()) return;
        auto& vec = it->second;
        for (auto vit = vec.begin(); vit != vec.end(); ++vit) {
            if (*vit == node) {
                vec.erase(vit);
                break;
            }
        }
        if (vec.empty()) {
            index_.erase(it);
        }
    }
};

} // namespace vfs


