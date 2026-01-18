#pragma once

#include "Realization/IDictionary/bst_dict.h"
#include "Realization/IDictionary/hash_table_dict.h"
#include "Realization/IDictionary/sorted_array_dict.h"
#include "histogram.h"
#include "../LibrarySequence/all_headers.h"

#include <exception>
#include <iostream>

namespace tests {

inline void Assert(bool cond, const std::string& message) {
    if (!cond) {
        throw std::runtime_error("Тест не пройден: " + message);
    }
}

template <typename D>
void TestDictionaryBasic(const std::string& name) {
    D dict;
    dict.AddOrUpdate(1, 10);
    dict.AddOrUpdate(2, 20);
    dict.AddOrUpdate(1, 15); // обновление

    Assert(dict.Count() == 2, name + ": Count должен быть 2");
    Assert(dict.ContainsKey(1), name + ": отсутствует ключ 1");
    Assert(dict.ContainsKey(2), name + ": отсутствует ключ 2");
    Assert(!dict.ContainsKey(3), name + ": неожиданный ключ 3");
    Assert(dict.Get(1) == 15, name + ": неверное значение для ключа 1");
    Assert(dict.Get(2) == 20, name + ": неверное значение для ключа 2");
}

inline void TestHistogramBasic() {
    MutableArraySequence<double> data;
    data.Resize(6);
    data.Set(0, 1.0);
    data.Set(1, 1.4);
    data.Set(2, 1.9);
    data.Set(3, 2.1);
    data.Set(4, 2.5);
    data.Set(5, 3.0);
    HistogramParams params{1.0, 4.0, 3}; // интервалы: [1,2), [2,3), [3,4)

    using Dict = SortedArrayDictionary<int, std::size_t>;
    Dict dict;
    auto bins = BuildHistogram<Dict>(data, params, nullptr, dict);

    tests::Assert(bins.GetLength() == 3, "Histogram: должно быть 3 интервала");
    // ожидаем: [1,2) -> 3, [2,3) -> 2, [3,4) -> 1
    tests::Assert(bins.Get(0).count == 3, "Histogram: неверное количество в первом интервале");
    tests::Assert(bins.Get(1).count == 2, "Histogram: неверное количество во втором интервале");
    tests::Assert(bins.Get(2).count == 1, "Histogram: неверное количество в третьем интервале");
}

inline void RunAll() {
    std::cout << "Запуск тестов...\n";

    TestDictionaryBasic<SortedArrayDictionary<int, std::size_t>>("SortedArrayDictionary");
    TestDictionaryBasic<BinarySearchTreeDictionary<int, std::size_t>>("BinarySearchTreeDictionary");
    TestDictionaryBasic<HashTableDictionary<int, std::size_t>>("HashTableDictionary");

    TestHistogramBasic();

    std::cout << "Все тесты успешно пройдены.\n";
}

} // namespace tests


