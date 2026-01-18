#pragma once

#include "Interface/isorted_sequence.h"

#include <cstddef>
#include <cmath>

// ================================
// Варианты бинарного поиска (M-2.x)
// ================================
//
// Работают поверх любого ISortedSequence<TElement>, предполагая:
//  - строгий порядок по оператору <,
//  - отсутствие модификаций последовательности во время поиска.

// Базовый бинарный поиск (деление пополам) — соответствует M-2.
template <typename TElement>
int BinarySearchMid(const ISortedSequence<TElement>& seq, const TElement& value) {
    int left = 0;
    int right = seq.GetLength() - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        TElement midVal = seq.Get(mid);
        if (midVal == value) return mid;
        if (midVal < value)
            left = mid + 1;
        else
            right = mid - 1;
    }
    return -1;
}

// ======================
// M-2.1: выбор пропорции
// ======================

// Деление по золотому сечению: mid = left + phi * (right - left),
// где phi ≈ 0.618 (1/φ).
template <typename TElement>
int BinarySearchGolden(const ISortedSequence<TElement>& seq, const TElement& value) {
    const double alpha = (std::sqrt(5.0) - 1.0) / 2.0; // ≈ 0.618
    int left = 0;
    int right = seq.GetLength() - 1;
    while (left <= right) {
        int len = right - left;
        int mid = left + static_cast<int>(len * alpha);
        if (mid < left) mid = left;
        if (mid > right) mid = right;

        TElement midVal = seq.Get(mid);
        if (midVal == value) return mid;
        if (midVal < value)
            left = mid + 1;
        else
            right = mid - 1;
    }
    return -1;
}

// ===============================
// M-2.2: пропорции как p:q и Фибоначчи
// ===============================

// Деление в отношении p:q (p,q >= 1).
// Точка деления: mid = left + (p / (p+q)) * (right - left).
template <typename TElement>
int BinarySearchRatio(const ISortedSequence<TElement>& seq,
                      const TElement& value,
                      std::size_t p,
                      std::size_t q) {
    if (p == 0 || q == 0) {
        // деградируем в обычное деление пополам
        return BinarySearchMid(seq, value);
    }

    double alpha = static_cast<double>(p) / static_cast<double>(p + q);
    int left = 0;
    int right = seq.GetLength() - 1;
    while (left <= right) {
        int len = right - left;
        int mid = left + static_cast<int>(len * alpha);
        if (mid < left) mid = left;
        if (mid > right) mid = right;

        TElement midVal = seq.Get(mid);
        if (midVal == value) return mid;
        if (midVal < value)
            left = mid + 1;
        else
            right = mid - 1;
    }
    return -1;
}

// Вспомогательная функция: n‑е число Фибоначчи (начиная с F0 = 0, F1 = 1).
inline std::size_t FibonacciNumber(int n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    std::size_t a = 0;
    std::size_t b = 1;
    for (int i = 2; i <= n; ++i) {
        std::size_t c = a + b;
        a = b;
        b = c;
    }
    return b;
}

// Деление с использованием пропорции, заданной номером Фибоначчи.
// Для заданного k >= 2 берём отношение F_{k-1} : F_k.
template <typename TElement>
int BinarySearchFibonacci(const ISortedSequence<TElement>& seq,
                          const TElement& value,
                          int fibIndex) {
    if (fibIndex < 2) {
        return BinarySearchMid(seq, value);
    }
    std::size_t fk = FibonacciNumber(fibIndex);
    std::size_t fkPrev = FibonacciNumber(fibIndex - 1);
    if (fk == 0 || fkPrev == 0) {
        return BinarySearchMid(seq, value);
    }

    double alpha = static_cast<double>(fkPrev) / static_cast<double>(fk);
    int left = 0;
    int right = seq.GetLength() - 1;
    while (left <= right) {
        int len = right - left;
        int mid = left + static_cast<int>(len * alpha);
        if (mid < left) mid = left;
        if (mid > right) mid = right;

        TElement midVal = seq.Get(mid);
        if (midVal == value) return mid;
        if (midVal < value)
            left = mid + 1;
        else
            right = mid - 1;
    }
    return -1;
}


