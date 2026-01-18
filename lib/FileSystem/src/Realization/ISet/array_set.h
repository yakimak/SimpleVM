#pragma once

#include "../../Interface/iset.h"
#include "../../../LibrarySequence/all_headers.h"

#include <stdexcept>

// Множество на основе отсортированного динамического массива.
// Хранит элементы без повторений.
template <typename TElement>
class ArraySet : public ISet<TElement> {
public:
    using ValueType = typename ISet<TElement>::ValueType;

    std::size_t Count() const override {
        return static_cast<std::size_t>(data_.GetLength());
    }

    bool IsEmpty() const override {
        return data_.GetLength() == 0;
    }

    bool Contains(const TElement& value) const override {
        return index_of(value) >= 0;
    }

    void Add(const TElement& value) override {
        // не добавляем дубликаты
        if (Contains(value)) return;

        int n = data_.GetLength();
        int pos = 0;
        // ищем позицию для вставки (поддерживаем упорядоченность)
        while (pos < n && data_.Get(pos) < value) {
            ++pos;
        }
        data_.Resize(n + 1);
        for (int i = n; i > pos; --i) {
            data_.Set(i, data_.Get(i - 1));
        }
        data_.Set(pos, value);
    }

    void Remove(const TElement& value) override {
        int idx = index_of(value);
        if (idx < 0) {
            throw std::out_of_range("ArraySet::Remove: value not found");
        }
        int n = data_.GetLength();
        for (int i = idx; i + 1 < n; ++i) {
            data_.Set(i, data_.Get(i + 1));
        }
        data_.Resize(n - 1);
    }

private:
    MutableArraySequence<TElement> data_;

    int index_of(const TElement& value) const {
        // бинарный поиск по отсортированному массиву
        int left = 0;
        int right = data_.GetLength() - 1;
        while (left <= right) {
            int mid = left + (right - left) / 2;
            TElement midVal = data_.Get(mid);
            if (midVal == value) return mid;
            if (midVal < value)
                left = mid + 1;
            else
                right = mid - 1;
        }
        return -1;
    }
};


