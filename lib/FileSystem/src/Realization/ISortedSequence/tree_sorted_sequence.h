#pragma once

#include "../../Interface/isorted_sequence.h"
#include "../../../LibrarySequence/4/BinaryTree.h"

#include <stdexcept>

// Реализация ISortedSequence на основе сбалансированного бинарного дерева поиска.
// Для поддержки дубликатов оборачиваем значение в Entry с уникальным идентификатором.
template <typename TElement>
class TreeSortedSequence : public ISortedSequence<TElement> {
public:
    using ValueType = typename ISortedSequence<TElement>::ValueType;

    TreeSortedSequence() : next_id_(0) {}

    int GetLength() const override {
        return static_cast<int>(tree_.Size());
    }

    bool IsEmpty() const override {
        return tree_.Empty();
    }

    TElement Get(int index) const override {
        auto arr = to_array();
        if (index < 0 || index >= arr.GetLength()) {
            throw std::out_of_range("TreeSortedSequence::Get: index out of range");
        }
        return arr.Get(index);
    }

    TElement GetFirst() const override {
        if (IsEmpty()) {
            throw std::out_of_range("TreeSortedSequence::GetFirst: empty");
        }
        auto arr = to_array();
        return arr.Get(0);
    }

    TElement GetLast() const override {
        if (IsEmpty()) {
            throw std::out_of_range("TreeSortedSequence::GetLast: empty");
        }
        auto arr = to_array();
        return arr.Get(arr.GetLength() - 1);
    }

    int IndexOf(const TElement& element) const override {
        auto arr = to_array();
        for (int i = 0; i < arr.GetLength(); ++i) {
            if (arr.Get(i) == element) return i;
        }
        return -1;
    }

    void Add(const TElement& element) override {
        Entry e;
        e.value = element;
        e.id = next_id_++;
        tree_.Insert(e);
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

    // Строим отсортированный по значению массив элементов (LKP‑обход).
    MutableArraySequence<TElement> to_array() const {
        MutableArraySequence<TElement> result;
        int n = static_cast<int>(tree_.Size());
        result.Resize(n);
        int idx = 0;
        tree_.LKP([&](const Entry& e) {
            result.Set(idx++, e.value);
        });
        return result;
    }
};


