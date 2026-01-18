#pragma once

#include "Interface/idictionary.h"
#include "../LibrarySequence/all_headers.h"

#include <cmath>
#include <functional>
#include <string>
#include <utility>

// Структура для представления одного интервала (бин) гистограммы.
struct HistogramBin {
    // левая граница интервала (бина) гистограммы
    double from;
    // правая граница интервала
    double to;
    // количество элементов в интервале
    std::size_t count = 0;

    // ===== Продвинутый уровень: статистика по классу =====
    // сумма значений в интервале
    double sum = 0.0;
    // сумма квадратов значений (для дисперсии и т.п.)
    double sum_squares = 0.0;
    // минимум и максимум в интервале
    double min_value = 0.0;
    double max_value = 0.0;
    // признак, что в интервал вообще что‑то попало
    bool has_values = false;

    // Для максимально полной реализации — имя класса (необязательно)
    std::string class_name;
};

// Параметры гистограммы (равномерное разбиение).
struct HistogramParams {
    double min_value;
    double max_value;
    int bins_count; // количество интервалов
};

// Описание класса (интервала) для конструктора гистограмм (неравномерное разбиение).
struct HistogramClass {
    double from;
    double to;
    std::string name; // человекочитаемое имя / описание класса
};

// Критерий (функция), по которому строится гистограмма.
// Например, f(x) = x или f(x) = floor(x).
using HistogramKeyFunc = std::function<double(double)>;

// ======================
// Равномерная гистограмма
// ======================
//
// Функция построения гистограммы поверх абстрактного словаря.
// D — конкретная реализация IDictionary<int, std::size_t>.
// data_seq — любая реализация Sequence<double> (например, MutableArraySequence<double>).
template <typename D>
MutableArraySequence<HistogramBin> BuildHistogram(
    const Sequence<double>& data_seq,
    const HistogramParams& params,
    HistogramKeyFunc key_func,
    D& dict_out) {
    // ВАЖНО: не переинициализируем dict_out через оператор присваивания из временного объекта.
    // Для реализаций словаря на базе MutableArraySequence (SortedArrayDictionary, HashTableDictionary)
    // оператор присваивания по умолчанию будет некорректно копировать внутренние «сырые» указатели,
    // что приводит к use-after-free и сегфолтам. Предполагаем, что вызывающий код передаёт сюда
    // «чистый» словарь, и просто используем его как есть.

    double range = params.max_value - params.min_value;
    MutableArraySequence<HistogramBin> empty_result;
    if (params.bins_count <= 0 || range <= 0.0) {
        return empty_result;
    }
    double bin_width = range / params.bins_count;

    auto compute_bin_index = [&](double value) -> int {
        double mapped = key_func ? key_func(value) : value;
        if (mapped < params.min_value) return 0;
        if (mapped >= params.max_value) return params.bins_count - 1;
        int idx = static_cast<int>((mapped - params.min_value) / bin_width);
        if (idx < 0) idx = 0;
        if (idx >= params.bins_count) idx = params.bins_count - 1;
        return idx;
    };

    // Внутренний массив статистик по бинам
    struct InternalStats {
        std::size_t count = 0;
        double sum = 0.0;
        double sum_squares = 0.0;
        double min_value = 0.0;
        double max_value = 0.0;
        bool has_values = false;
    };

    MutableArraySequence<InternalStats> stats;
    stats.Resize(params.bins_count);

    // Проход по данным: считаем статистику по каждому бину
    for (int i = 0; i < data_seq.GetLength(); ++i) {
        double v = data_seq.Get(i);
        double mapped = key_func ? key_func(v) : v;
        int idx = compute_bin_index(v);

        InternalStats s = stats.Get(idx);
        if (!s.has_values) {
            s.has_values = true;
            s.min_value = mapped;
            s.max_value = mapped;
        } else {
            if (mapped < s.min_value) s.min_value = mapped;
            if (mapped > s.max_value) s.max_value = mapped;
        }
        s.count += 1;
        s.sum += mapped;
        s.sum_squares += mapped * mapped;
        stats.Set(idx, s);
    }

    // Заполняем ассоциативный массив количеством (базовый уровень)
    for (int i = 0; i < params.bins_count; ++i) {
        InternalStats s = stats.Get(i);
        if (s.count > 0) {
            dict_out.AddOrUpdate(i, s.count);
        }
    }

    // Формируем итоговую последовательность бинов с полной статистикой
    MutableArraySequence<HistogramBin> result;
    result.Resize(params.bins_count);
    for (int i = 0; i < params.bins_count; ++i) {
        HistogramBin bin;
        bin.from = params.min_value + i * bin_width;
        bin.to = bin.from + bin_width;

        InternalStats s = stats.Get(i);
        bin.count = s.count;
        bin.sum = s.sum;
        bin.sum_squares = s.sum_squares;
        bin.min_value = s.min_value;
        bin.max_value = s.max_value;
        bin.has_values = s.has_values;
        // class_name здесь можно не задавать — для равномерного случая он не обязателен

        result.Set(i, bin);
    }
    return result;
}

// ===============================
// Гистограмма по заданным классам
// ===============================
//
// Построение гистограммы по явному набору классов (неравномерное разбиение).
// classes: последовательность интервалов [from, to) с осмысленными именами.
// D — любая реализация IDictionary<int, std::size_t>.
template <typename D>
MutableArraySequence<HistogramBin> BuildHistogramWithClasses(
    const Sequence<double>& data_seq,
    const Sequence<HistogramClass>& classes,
    HistogramKeyFunc key_func,
    D& dict_out) {
    // Аналогично BuildHistogram: не выполняем присваивание dict_out = D{} по причинам,
    // описанным выше (во избежание некорректного копирования внутренних указателей).

    int bins_count = classes.GetLength();
    MutableArraySequence<HistogramBin> empty_result;
    if (bins_count <= 0) {
        return empty_result;
    }

    struct InternalStats {
        std::size_t count = 0;
        double sum = 0.0;
        double sum_squares = 0.0;
        double min_value = 0.0;
        double max_value = 0.0;
        bool has_values = false;
    };

    MutableArraySequence<InternalStats> stats;
    stats.Resize(bins_count);

    auto find_class_index = [&](double value) -> int {
        double mapped = key_func ? key_func(value) : value;
        // Простой линейный поиск по классам
        for (int i = 0; i < bins_count; ++i) {
            HistogramClass c = classes.Get(i);
            if (mapped >= c.from && mapped < c.to) {
                return i;
            }
        }
        // Если не попало ни в один интервал — прижимаем к краям
        if (mapped < classes.Get(0).from) return 0;
        return bins_count - 1;
    };

    // Считаем статистику по каждому явно заданному классу
    for (int i = 0; i < data_seq.GetLength(); ++i) {
        double v = data_seq.Get(i);
        double mapped = key_func ? key_func(v) : v;
        int idx = find_class_index(v);

        InternalStats s = stats.Get(idx);
        if (!s.has_values) {
            s.has_values = true;
            s.min_value = mapped;
            s.max_value = mapped;
        } else {
            if (mapped < s.min_value) s.min_value = mapped;
            if (mapped > s.max_value) s.max_value = mapped;
        }
        s.count += 1;
        s.sum += mapped;
        s.sum_squares += mapped * mapped;
        stats.Set(idx, s);
    }

    // Заполняем словарь количеством
    for (int i = 0; i < bins_count; ++i) {
        InternalStats s = stats.Get(i);
        if (s.count > 0) {
            dict_out.AddOrUpdate(i, s.count);
        }
    }

    // Формируем итоговую гистограмму, используя описание классов
    MutableArraySequence<HistogramBin> result;
    result.Resize(bins_count);
    for (int i = 0; i < bins_count; ++i) {
        HistogramClass c = classes.Get(i);
        InternalStats s = stats.Get(i);

        HistogramBin bin;
        bin.from = c.from;
        bin.to = c.to;
        bin.count = s.count;
        bin.sum = s.sum;
        bin.sum_squares = s.sum_squares;
        bin.min_value = s.min_value;
        bin.max_value = s.max_value;
        bin.has_values = s.has_values;
        bin.class_name = c.name;

        result.Set(i, bin);
    }

    return result;
}


