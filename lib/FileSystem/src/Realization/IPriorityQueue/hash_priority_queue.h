#pragma once

#include "../../Interface/ipriority_queue.h"
#include "../IDictionary/hash_table_dict.h"

#include <stdexcept>

// Очередь с приоритетом на основе хеш-таблицы.
// Храним элементы по индексам 0..size_-1, а максимум ищем линейным проходом.
template <typename TElement>
class HashTablePriorityQueue : public IPriorityQueue<TElement> {
public:
    using ValueType = typename IPriorityQueue<TElement>::ValueType;

    HashTablePriorityQueue() : size_(0), top_cache_() {}

    bool IsEmpty() const override {
        return size_ == 0;
    }

    int GetSize() const override {
        return static_cast<int>(size_);
    }

    const TElement& Top() const override {
        if (IsEmpty()) {
            throw std::out_of_range("HashTablePriorityQueue::Top: empty");
        }
        // пересчитываем максимум и кешируем его во внутреннем поле
        bool first = true;
        for (int i = 0; i < static_cast<int>(size_); ++i) {
            TElement cur = dict_.Get(i);
            if (first || top_cache_ < cur) {
                top_cache_ = cur;
                first = false;
            }
        }
        return top_cache_;
    }

    void Push(const TElement& value) override {
        int idx = static_cast<int>(size_);
        dict_.AddOrUpdate(idx, value);
        ++size_;
    }

    TElement Pop() override {
        if (IsEmpty()) {
            throw std::out_of_range("HashTablePriorityQueue::Pop: empty");
        }

        // находим индекс элемента с максимальным приоритетом
        int best_index = 0;
        TElement best = dict_.Get(0);
        for (int i = 1; i < static_cast<int>(size_); ++i) {
            TElement cur = dict_.Get(i);
            if (best < cur) {
                best = cur;
                best_index = i;
            }
        }

        // сдвигаем элементы после best_index влево
        for (int i = best_index; i < static_cast<int>(size_) - 1; ++i) {
            TElement next = dict_.Get(i + 1);
            dict_.AddOrUpdate(i, next);
        }
        // удаляем последний элемент
        dict_.Remove(static_cast<int>(size_ - 1));
        --size_;

        return best;
    }

private:
    mutable TElement top_cache_;
    HashTableDictionary<int, TElement> dict_;
    std::size_t size_;
};


