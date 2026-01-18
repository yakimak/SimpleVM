#pragma once

#include "Interface/idictionary.h"

#include <algorithm>
#include <utility>
#include "../LibrarySequence/all_headers.h"

// Реализация IDictionary на основе отсортированного динамического массива.
// Поиск осуществляется бинарным поиском.
// Это именно ассоциативная память (словарь), а не «сортированная последовательность» из спецификации.
template <typename K, typename V>
class SortedArrayDictionary : public IDictionary<K, V> {
public:
    using Pair = std::pair<K, V>;

    std::size_t Count() const override { return static_cast<std::size_t>(data_.GetLength()); }

    std::size_t Capacity() const override { return static_cast<std::size_t>(data_.GetLength()); }

    bool ContainsKey(const K& key) const override {
        return find_index(key) != -1;
    }

    V Get(const K& key) const override {
        int idx = find_index(key);
        if (idx < 0) {
            throw std::out_of_range("Key not found");
        }
        return data_.Get(idx).second;
    }

    void Add(const K& key, const V& value) override {
        if (ContainsKey(key)) {
            throw std::invalid_argument("Key already exists");
        }
        insert_in_order(key, value);
    }

    void AddOrUpdate(const K& key, const V& value) override {
        int idx = find_index(key);
        if (idx >= 0) {
            Pair p = data_.Get(idx);
            p.second = value;
            data_.Set(idx, p);
            return;
        }
        insert_in_order(key, value);
    }

    void Remove(const K& key) override {
        int idx = find_index(key);
        if (idx < 0) {
            throw std::out_of_range("Key not found");
        }
        // сдвиг элементов влево и уменьшение длины
        int n = data_.GetLength();
        for (int i = idx; i + 1 < n; ++i) {
            data_.Set(i, data_.Get(i + 1));
        }
        data_.Resize(n - 1);
    }

private:
    MutableArraySequence<Pair> data_;

    int find_index(const K& key) const {
        int left = 0;
        int right = data_.GetLength() - 1;
        while (left <= right) {
            int mid = left + (right - left) / 2;
            K mid_key = data_.Get(mid).first;
            if (mid_key == key) {
                return mid;
            }
            if (mid_key < key) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
        return -1;
    }

    void insert_in_order(const K& key, const V& value) {
        int n = data_.GetLength();
        int pos = 0;
        while (pos < n && data_.Get(pos).first < key) {
            ++pos;
        }
        data_.Resize(n + 1);
        for (int i = n; i > pos; --i) {
            data_.Set(i, data_.Get(i - 1));
        }
        data_.Set(pos, Pair(key, value));
    }
};



