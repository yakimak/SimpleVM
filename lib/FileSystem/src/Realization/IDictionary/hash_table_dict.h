#pragma once

#include "Interface/idictionary.h"

#include <cmath>
#include <stdexcept>
#include <utility>
#include <type_traits>
#include "../LibrarySequence/all_headers.h"

// Простая реализация IDictionary на основе хеш-таблицы
// с открытой адресацией и линейным пробированием.
template <typename K, typename V>
class HashTableDictionary : public IDictionary<K, V> {
public:
    explicit HashTableDictionary(std::size_t initial_capacity = 16)
        : keys_(), count_(0), capacity_(0) {
        if (initial_capacity < 4) initial_capacity = 4;
        capacity_ = next_power_of_two(initial_capacity);
        keys_.Resize(static_cast<int>(capacity_));
        for (int i = 0; i < static_cast<int>(capacity_); ++i) {
            keys_.Set(i, KeyState{});
        }
    }

    std::size_t Count() const override { return count_; }
    std::size_t Capacity() const override { return capacity_; }

    bool ContainsKey(const K& key) const override {
        return find_index(key) >= 0;
    }

    V Get(const K& key) const override {
        int idx = find_index(key);
        if (idx < 0) throw std::out_of_range("Key not found");
        return keys_.Get(idx).value;
    }

    void Add(const K& key, const V& value) override {
        if (ContainsKey(key)) {
            throw std::invalid_argument("Key already exists");
        }
        if ((count_ + 1) * 100 / capacity_ > 70) {
            rehash(capacity_ * 2);
        }
        insert_or_update(key, value);
    }

    void AddOrUpdate(const K& key, const V& value) override {
        if ((count_ + 1) * 100 / capacity_ > 70) {
            rehash(capacity_ * 2);
        }
        insert_or_update(key, value);
    }

    void Remove(const K& key) override {
        int idx = find_index(key);
        if (idx < 0) {
            throw std::out_of_range("Key not found");
        }
        KeyState slot = keys_.Get(idx);
        slot.state = SlotState::Deleted;
        slot.key = K{};
        slot.value = V{};
        keys_.Set(idx, slot);
        if (count_ > 0) {
            --count_;
        }
    }

private:
    enum class SlotState { Empty, Occupied, Deleted };

    struct KeyState {
        K key{};
        V value{};
        SlotState state = SlotState::Empty;
    };

    MutableArraySequence<KeyState> keys_;
    std::size_t count_;
    std::size_t capacity_;

    static std::size_t next_power_of_two(std::size_t n) {
        std::size_t p = 1;
        while (p < n) p <<= 1;
        return p;
    }

    std::size_t hash_key(const K& key) const {
        // Для целых типов используем прямое преобразование,
        // для остальных — std::hash.
        if constexpr (std::is_integral<K>::value) {
            // MurmurHash
            std::size_t x = static_cast<std::size_t>(key);
            x ^= x >> 33;
            x *= 0xff51afd7ed558ccdULL;
            x ^= x >> 33;
            x *= 0xc4ceb9fe1a85ec53ULL;
            x ^= x >> 33;
            return x;
        } else {
            return std::hash<K>{}(key);
        }
    }

    int find_index(const K& key) const {
        if (capacity_ == 0) return -1;
        std::size_t mask = capacity_ - 1;
        std::size_t h = hash_key(key) & mask;
        for (std::size_t i = 0; i < capacity_; ++i) {
            std::size_t idx = (h + i) & mask;
            KeyState slot = keys_.Get(static_cast<int>(idx));
            if (slot.state == SlotState::Empty) {
                return -1;
            }
            if (slot.state == SlotState::Occupied && slot.key == key) {
                return static_cast<int>(idx);
            }
        }
        return -1;
    }

    void insert_or_update(const K& key, const V& value) {
        std::size_t mask = capacity_ - 1;
        std::size_t h = hash_key(key) & mask;
        int first_deleted = -1;

        for (std::size_t i = 0; i < capacity_; ++i) {
            std::size_t idx = (h + i) & mask;
            KeyState slot = keys_.Get(static_cast<int>(idx));

            if (slot.state == SlotState::Occupied && slot.key == key) {
                slot.value = value;
                keys_.Set(static_cast<int>(idx), slot);
                return;
            }

            if (slot.state == SlotState::Deleted && first_deleted < 0) {
                first_deleted = static_cast<int>(idx);
            }

            if (slot.state == SlotState::Empty) {
                std::size_t target_idx = (first_deleted >= 0)
                                             ? static_cast<std::size_t>(first_deleted)
                                             : idx;
                KeyState target = keys_.Get(static_cast<int>(target_idx));
                target.key = key;
                target.value = value;
                target.state = SlotState::Occupied;
                keys_.Set(static_cast<int>(target_idx), target);
                ++count_;
                return;
            }
        }
    }

    void rehash(std::size_t new_capacity) {
        new_capacity = next_power_of_two(new_capacity);
        MutableArraySequence<KeyState> old = keys_; // глубокая копия через конструктор

        // Просто изменяем размер существующего массива ключей.
        // DynamicArray внутри MutableArraySequence сам корректно перераспределит память.
        keys_.Resize(static_cast<int>(new_capacity));
        capacity_ = new_capacity;
        count_ = 0;

        for (int i = 0; i < old.GetLength(); ++i) {
            KeyState slot = old.Get(i);
            if (slot.state == SlotState::Occupied) {
                insert_or_update(slot.key, slot.value);
            }
        }
    }
};



