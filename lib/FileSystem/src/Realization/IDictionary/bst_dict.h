#pragma once

#include "Interface/idictionary.h"

#include <stdexcept>
#include <utility>
#include "../LibrarySequence/4/BinaryTree.h"

// Реализация IDictionary на основе уже готового сбалансированного дерева поиска BinaryTree<T>.
// Внутри дерева храним элементы типа Entry, в которых ключ и значение разделены.
template <typename K, typename V>
class BinarySearchTreeDictionary : public IDictionary<K, V> {
public:
    BinarySearchTreeDictionary() = default;
    ~BinarySearchTreeDictionary() override = default;

    std::size_t Count() const override { return tree_.Size(); }
    std::size_t Capacity() const override { return tree_.Size(); }

    bool ContainsKey(const K& key) const override {
        return find_entry(key) != nullptr;
    }

    V Get(const K& key) const override {
        Entry* e = find_entry(key);
        if (!e) {
            throw std::out_of_range("Key not found");
        }
        return e->value;
    }

    void Add(const K& key, const V& value) override {
        // BinaryTree::Insert выбросит исключение при попытке добавить дублирующий ключ
        tree_.Insert(Entry{key, value});
    }

    void AddOrUpdate(const K& key, const V& value) override {
        Entry* e = find_entry(key);
        if (e) {
            e->value = value;
        } else {
            tree_.Insert(Entry{key, value});
        }
    }

    void Remove(const K& key) override {
        // удаляем "по ключу": сравнение в Entry учитывает только key
        tree_.Remove(Entry{key, V{}});
    }

private:
    struct Entry {
        K key;
        V value;

        bool operator<(const Entry& other) const {
            return key < other.key;
        }
        bool operator==(const Entry& other) const {
            return key == other.key;
        }
    };

    BinaryTree<Entry> tree_;

    // Поиск элемента по ключу, используя доступ к корню BinaryTree и структуре TreeNode.
    Entry* find_entry(const K& key) const {
        Entry probe{key, V{}};
        TreeNode<Entry>* node = tree_.GetRoot();
        while (node) {
            if (probe < node->value) {
                node = node->left;
            } else if (node->value < probe) {
                node = node->right;
            } else {
                return &node->value;
            }
        }
        return nullptr;
    }
};



