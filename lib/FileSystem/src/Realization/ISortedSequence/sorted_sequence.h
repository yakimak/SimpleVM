#pragma once

#include "../../Interface/isorted_sequence.h"
#include "../../../LibrarySequence/all_headers.h"

#include <stdexcept>

// Конкретная реализация ISortedSequence на основе динамического массива.
template <typename TElement>
class SortedSequence : public ISortedSequence<TElement> {
public:
    using ValueType = typename ISortedSequence<TElement>::ValueType;

    // Атрибуты
    int GetLength() const override { return data_.GetLength(); }

    bool IsEmpty() const override { return data_.GetLength() == 0; }

    // Методы
    TElement Get(int index) const override {
        if (index < 0 || index >= data_.GetLength()) {
            throw std::out_of_range("SortedSequence::Get: index out of range");
        }
        return data_.Get(index);
    }

    TElement GetFirst() const override {
        if (data_.GetLength() == 0) {
            throw std::out_of_range("SortedSequence::GetFirst: empty");
        }
        return data_.Get(0);
    }

    TElement GetLast() const override {
        if (data_.GetLength() == 0) {
            throw std::out_of_range("SortedSequence::GetLast: empty");
        }
        return data_.Get(data_.GetLength() - 1);
    }

    int IndexOf(const TElement& element) const override {
        // бинарный поиск
        int left = 0;
        int right = data_.GetLength() - 1;
        while (left <= right) {
            int mid = left + (right - left) / 2;
            TElement midVal = data_.Get(mid);
            if (midVal == element) return mid;
            if (midVal < element)
                left = mid + 1;
            else
                right = mid - 1;
        }
        return -1;
    }

    // Добавление с сохранением упорядоченности
    void Add(const TElement& element) override {
        int n = data_.GetLength();
        // ищем позицию вставки
        int pos = 0;
        while (pos < n && data_.Get(pos) < element) {
            ++pos;
        }
        data_.Resize(n + 1);
        for (int i = n; i > pos; --i) {
            data_.Set(i, data_.Get(i - 1));
        }
        data_.Set(pos, element);
    }

    // Дополнительные удобные методы (не из интерфейса)
    SortedSequence<TElement> GetSubsequence(int startIndex, int endIndex) const {
        if (startIndex < 0 || endIndex < startIndex || endIndex >= data_.GetLength()) {
            throw std::out_of_range("SortedSequence::GetSubsequence: invalid indices");
        }
        SortedSequence<TElement> result;
        int newLen = endIndex - startIndex + 1;
        result.data_.Resize(newLen);
        for (int i = 0; i < newLen; ++i) {
            result.data_.Set(i, data_.Get(startIndex + i));
        }
        return result;
    }

    const MutableArraySequence<TElement>& RawData() const { return data_; }

private:
    MutableArraySequence<TElement> data_;
};


