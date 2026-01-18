#pragma once

#include "../../Interface/ipriority_queue.h"
#include "../../../LibrarySequence/4/BinaryTree.h"
#include "../../../LibrarySequence/4/TreeNode.h"

#include <stdexcept>

// Очередь с приоритетом на основе сбалансированного бинарного дерева поиска.
// Максимальный элемент определяется по оператору < для TElement.
template <typename TElement>
class TreePriorityQueue : public IPriorityQueue<TElement> {
public:
    using ValueType = typename IPriorityQueue<TElement>::ValueType;

    TreePriorityQueue() : next_id_(0) {}

    bool IsEmpty() const override {
        return tree_.Empty();
    }

    int GetSize() const override {
        return static_cast<int>(tree_.Size());
    }

    const TElement& Top() const override {
        if (IsEmpty()) {
            throw std::out_of_range("TreePriorityQueue::Top: empty");
        }
        Entry* max_entry = find_max_entry();
        if (!max_entry) {
            throw std::out_of_range("TreePriorityQueue::Top: empty");
        }
        return max_entry->value;
    }

    void Push(const TElement& value) override {
        Entry e;
        e.value = value;
        e.id = next_id_++;
        tree_.Insert(e);
    }

    TElement Pop() override {
        if (IsEmpty()) {
            throw std::out_of_range("TreePriorityQueue::Pop: empty");
        }
        Entry* max_entry = find_max_entry();
        if (!max_entry) {
            throw std::out_of_range("TreePriorityQueue::Pop: empty");
        }
        Entry to_remove = *max_entry;
        TElement result = to_remove.value;
        tree_.Remove(to_remove);
        return result;
    }

private:
    struct Entry {
        TElement value;
        int id;

        bool operator<(const Entry& other) const {
            if (value < other.value) return true;
            if (other.value < value) return false;
            return id < other.id;
        }
        bool operator==(const Entry& other) const {
            return value == other.value && id == other.id;
        }
    };

    BinaryTree<Entry> tree_;
    int next_id_;

    Entry* find_max_entry() const {
        TreeNode<Entry>* node = tree_.GetRoot();
        if (!node) return nullptr;
        while (node->right) {
            node = node->right;
        }
        return &node->value;
    }
};


