#include "Realization/IDictionary/bst_dict.h"
#include "Realization/IDictionary/hash_table_dict.h"
#include "Realization/IDictionary/sorted_array_dict.h"
#include "Realization/IPriorityQueue/hash_priority_queue.h"
#include "Realization/IPriorityQueue/tree_priority_queue.h"
#include "Realization/ISet/array_set.h"
#include "Realization/ISet/hash_set.h"
#include "Realization/ISet/tree_set.h"
#include "Realization/ISortedSequence/sorted_sequence.h"
#include "Realization/ISortedSequence/tree_sorted_sequence.h"
#include "VirtualFS/virtual_file_system.h"
#include "histogram.h"
#include "tests.h"
#include "../LibrarySequence/all_headers.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

using Clock = std::chrono::high_resolution_clock;

struct DataSet {
    MutableArraySequence<double> values;
    bool generated = false;
};

// Объявление, чтобы использовать в VirtualFsPlayground
void ClearInput();

// ==========================
// Простейшие проверки Realization
// ==========================

void TestAllDictionaries() {
    std::cout << "Проверка реализаций IDictionary...\n";

    auto run = [](auto dict, const std::string& name) {
        using D = decltype(dict);
        D d;
        d.AddOrUpdate(1, 10);
        d.AddOrUpdate(2, 20);
        d.AddOrUpdate(1, 15);
        if (d.Count() != 2 || !d.ContainsKey(1) || !d.ContainsKey(2) || d.ContainsKey(3)) {
            std::cout << "  [" << name << "] базовый тест не пройден\n";
            return;
        }
        if (d.Get(1) != 15 || d.Get(2) != 20) {
            std::cout << "  [" << name << "] неверные значения\n";
            return;
        }
        std::cout << "  [" << name << "] OK\n";
    };

    run(SortedArrayDictionary<int, int>{}, "SortedArrayDictionary");
    run(BinarySearchTreeDictionary<int, int>{}, "BinarySearchTreeDictionary");
    run(HashTableDictionary<int, int>{}, "HashTableDictionary");
}

void TestAllPriorityQueues() {
    std::cout << "Проверка реализаций IPriorityQueue...\n";

    auto run = [](auto pq, const std::string& name) {
        using Q = decltype(pq);
        Q q;
        q.Push(3);
        q.Push(1);
        q.Push(5);
        q.Push(4);

        if (q.IsEmpty() || q.GetSize() != 4) {
            std::cout << "  [" << name << "] некорректный размер/пустота\n";
            return;
        }
        if (q.Top() != 5) {
            std::cout << "  [" << name << "] ожидали Top() == 5\n";
            return;
        }
        int first = q.Pop();
        int second = q.Pop();
        if (first != 5 || second != 4) {
            std::cout << "  [" << name << "] Pop() вернул неверную последовательность\n";
            return;
        }
        std::cout << "  [" << name << "] OK\n";
    };

    run(HashTablePriorityQueue<int>{}, "HashTablePriorityQueue");
    run(TreePriorityQueue<int>{}, "TreePriorityQueue");
}

void TestAllSets() {
    std::cout << "Проверка реализаций ISet...\n";

    auto run = [](auto set, const std::string& name) {
        using S = decltype(set);
        S s;
        s.Add(1);
        s.Add(2);
        s.Add(2);
        s.Add(3);
        if (s.IsEmpty() || s.Count() != 3) {
            std::cout << "  [" << name << "] ожидаем 3 разных элемента\n";
            return;
        }
        if (!s.Contains(1) || !s.Contains(2) || !s.Contains(3) || s.Contains(4)) {
            std::cout << "  [" << name << "] Contains работает неверно\n";
            return;
        }
        s.Remove(2);
        if (s.Contains(2)) {
            std::cout << "  [" << name << "] Remove не удалил элемент\n";
            return;
        }
        std::cout << "  [" << name << "] OK\n";
    };

    run(ArraySet<int>{}, "ArraySet");
    run(HashSet<int>{}, "HashSet");
    run(TreeSet<int>{}, "TreeSet");
}

void TestAllSortedSequences() {
    std::cout << "Проверка реализаций ISortedSequence...\n";

    auto run = [](auto seq, const std::string& name) {
        using S = decltype(seq);
        S s;
        s.Add(5);
        s.Add(1);
        s.Add(3);
        s.Add(3);
        if (s.IsEmpty() || s.GetLength() != 4) {
            std::cout << "  [" << name << "] неверная длина/пустота\n";
            return;
        }
        // элементы должны быть отсортированы по возрастанию
        if (s.GetFirst() != 1 || s.GetLast() != 5) {
            std::cout << "  [" << name << "] GetFirst/GetLast неверны\n";
            return;
        }
        int idx3 = s.IndexOf(3);
        if (idx3 < 0) {
            std::cout << "  [" << name << "] IndexOf(3) не найден\n";
            return;
        }
        std::cout << "  [" << name << "] OK\n";
    };

    run(SortedSequence<int>{}, "SortedSequence");
    run(TreeSortedSequence<int>{}, "TreeSortedSequence");
}

void TestVirtualFileSystem() {
    std::cout << "Проверка VirtualFileSystem...\n";

    vfs::VirtualFileSystem fs;
    auto* root = fs.GetRoot();
    if (!root) {
        std::cout << "  [VFS] корень не создан\n";
        return;
    }

    // создаём пару директорий и файлов (пути к реальным файлам фиктивные)
    fs.MakeDirectory("/dir1");
    fs.MakeDirectory("/dir1/sub");
    auto* f1 = fs.AttachFile("/dir1/file1.txt", "/tmp/file1.txt");
    auto* f2 = fs.AttachFile("/dir1/sub/file2.txt", "/tmp/file2.txt");

    if (!f1 || !f2) {
        std::cout << "  [VFS] не удалось создать файлы\n";
        return;
    }

    // проверка поиска по имени
    auto found = fs.FindFilesByName("file1.txt");
    if (found.empty() || found[0]->GetPhysicalPath() != "/tmp/file1.txt") {
        std::cout << "  [VFS] FindFilesByName работает неверно\n";
        return;
    }

    // проверка Resolve/Move/Remove
    auto* node_before = fs.Resolve("/dir1/sub/file2.txt");
    if (!node_before || node_before->GetType() != vfs::NodeType::File) {
        std::cout << "  [VFS] Resolve до перемещения не сработал\n";
        return;
    }

    fs.Move("/dir1/sub/file2.txt", "/dir1/file2_renamed.txt");
    auto* node_after = fs.Resolve("/dir1/file2_renamed.txt");
    if (!node_after || node_after->GetType() != vfs::NodeType::File) {
        std::cout << "  [VFS] Move/Resolve после перемещения не сработали\n";
        return;
    }

    fs.Remove("/dir1/file1.txt");
    auto* node_removed = fs.Resolve("/dir1/file1.txt");
    if (node_removed) {
        std::cout << "  [VFS] Remove не удалил узел\n";
        return;
    }

    std::cout << "  [VFS] OK\n";
}

// ==============================
// Интерактивная работа с VirtualFS
// ==============================

void PrintVfsTree(const vfs::DirectoryNode* dir, int indent = 0) {
    if (!dir) return;
    const auto& children = dir->GetChildren();
    for (const auto& ch : children) {
        const vfs::Node* node = ch.get();
        for (int i = 0; i < indent; ++i) {
            std::cout << "  ";
        }
        if (node->GetType() == vfs::NodeType::Directory) {
            std::cout << "[D] " << node->GetName() << "\n";
            auto* subdir = static_cast<const vfs::DirectoryNode*>(node);
            PrintVfsTree(subdir, indent + 1);
        } else {
            auto* f = static_cast<const vfs::FileNode*>(node);
            std::cout << "[F] " << node->GetName()
                      << " -> " << f->GetPhysicalPath() << "\n";
        }
    }
}

void VirtualFsPlayground() {
    std::cout << "Интерактивная виртуальная файловая система (VirtualFS).\n";
    vfs::VirtualFileSystem fs;

    while (true) {
        std::cout << "\n=== VirtualFS ===\n";
        std::cout << "1) Показать дерево от корня\n";
        std::cout << "2) Создать директорию (MakeDirectory)\n";
        std::cout << "3) Прикрепить файл (AttachFile)\n";
        std::cout << "4) Переместить/переименовать (Move)\n";
        std::cout << "5) Удалить узел (Remove)\n";
        std::cout << "6) Найти файлы по имени (FindFilesByName)\n";
        std::cout << "0) Назад в главное меню\n";

        int cmd;
        std::cout << "Ваш выбор: ";
        if (!(std::cin >> cmd)) {
            ClearInput();
            std::cout << "Некорректный ввод.\n";
            continue;
        }

        try {
            if (cmd == 0) {
                break;
            } else if (cmd == 1) {
                std::cout << "Корень: /\n";
                PrintVfsTree(fs.GetRoot(), 1);
            } else if (cmd == 2) {
                ClearInput();
                std::string path;
                std::cout << "Введите виртуальный путь директории (например, /docs/projects): ";
                std::getline(std::cin, path);
                fs.MakeDirectory(path);
                std::cout << "Директория создана.\n";
            } else if (cmd == 3) {
                ClearInput();
                std::string vpath, ppath;
                std::cout << "Введите виртуальный путь файла (например, /docs/file.txt): ";
                std::getline(std::cin, vpath);
                std::cout << "Введите физический путь (строка-метка, реальный файл не трогаем): ";
                std::getline(std::cin, ppath);
                fs.AttachFile(vpath, ppath);
                std::cout << "Файл прикреплён.\n";
            } else if (cmd == 4) {
                ClearInput();
                std::string from, to;
                std::cout << "Откуда перемещать (виртуальный путь): ";
                std::getline(std::cin, from);
                std::cout << "Куда (новый виртуальный путь): ";
                std::getline(std::cin, to);
                fs.Move(from, to);
                std::cout << "Узел перемещён/переименован.\n";
            } else if (cmd == 5) {
                ClearInput();
                std::string path;
                std::cout << "Какой узел удалить (виртуальный путь): ";
                std::getline(std::cin, path);
                fs.Remove(path);
                std::cout << "Узел удалён.\n";
            } else if (cmd == 6) {
                ClearInput();
                std::string name;
                std::cout << "Имя файла для поиска (без пути): ";
                std::getline(std::cin, name);
                auto files = fs.FindFilesByName(name);
                if (files.empty()) {
                    std::cout << "Файлы с именем \"" << name << "\" не найдены.\n";
                } else {
                    std::cout << "Найдено " << files.size() << " файл(ов):\n";
                    for (auto* f : files) {
                        std::cout << "  " << f->GetVirtualPath()
                                  << " -> " << f->GetPhysicalPath() << "\n";
                    }
                }
            } else {
                std::cout << "Неизвестная команда.\n";
            }
        } catch (const std::exception& ex) {
            std::cout << "Ошибка VirtualFS: " << ex.what() << "\n";
        }
    }
}

void ClearInput() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int ReadInt(const std::string& prompt, int min_val, int max_val) {
    while (true) {
        std::cout << prompt;
        int x;
        if (std::cin >> x && x >= min_val && x <= max_val) {
            return x;
        }
        std::cout << "Некорректный ввод. Повторите.\n";
        ClearInput();
    }
}

double ReadDouble(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        double x;
        if (std::cin >> x) {
            return x;
        }
        std::cout << "Некорректный ввод. Повторите.\n";
        ClearInput();
    }
}

void InputDataManual(DataSet& ds) {
    // Сбрасываем текущие данные, не создавая новый объект (чтобы не ломать внутренние указатели)
    ds.values.Resize(0);
    int n = ReadInt("Введите количество элементов: ", 0, 1'000'000);
    ds.values.Resize(n);
    std::cout << "Введите " << n << " чисел:\n";
    for (int i = 0; i < n; ++i) {
        double v = ReadDouble("  x[" + std::to_string(i) + "] = ");
        ds.values.Set(i, v);
    }
    ds.generated = true;
}

void GenerateRandomData(DataSet& ds) {
    // Аналогично, очищаем существующую последовательность
    ds.values.Resize(0);
    int n = ReadInt("Введите количество элементов: ", 0, 5'000'000);
    double minv = ReadDouble("Минимальное значение: ");
    double maxv = ReadDouble("Максимальное значение: ");
    if (maxv < minv) std::swap(minv, maxv);

    std::mt19937_64 rng(static_cast<std::uint64_t>(std::random_device{}()));
    std::uniform_real_distribution<double> dist(minv, maxv);

    for (int i = 0; i < n; ++i) {
        ds.values.Resize(i + 1);
        ds.values.Set(i, dist(rng));
    }
    ds.generated = true;
    std::cout << "Сгенерировано " << n << " элементов.\n";
}

HistogramParams ReadHistogramParams(const DataSet& ds) {
    HistogramParams params{};
    if (ds.values.GetLength() == 0) {
        params.min_value = 0.0;
        params.max_value = 1.0;
    } else {
        double mn = ds.values.Get(0);
        double mx = ds.values.Get(0);
        for (int i = 0; i < ds.values.GetLength(); ++i) {
            double v = ds.values.Get(i);
            if (v < mn) mn = v;
            if (v > mx) mx = v;
        }
        params.min_value = mn;
        params.max_value = mx;
    }

    std::cout << "По умолчанию диапазон данных: [" << params.min_value << ", " << params.max_value << "].\n";
    std::cout << "Хотите задать диапазон вручную? (1 - да, 0 - нет): ";
    int choice = 0;
    if (std::cin >> choice && choice == 1) {
        params.min_value = ReadDouble("Минимум: ");
        params.max_value = ReadDouble("Максимум: ");
        if (params.max_value < params.min_value) std::swap(params.min_value, params.max_value);
    } else {
        ClearInput();
    }

    params.bins_count = ReadInt("Количество интервалов гистограммы: ", 1, 10'000);
    return params;
}

HistogramKeyFunc ChooseKeyFunc() {
    std::cout << "Выберите критерий для гистограммы:\n";
    std::cout << "1) По самому значению x\n";
    std::cout << "2) По функции floor(x)\n";
    int c = ReadInt("Ваш выбор: ", 1, 2);
    if (c == 2) {
        return [](double x) { return std::floor(x); };
    }
    return nullptr; // будет интерпретировано как f(x) = x
}

template <typename Dict>
std::pair<MutableArraySequence<HistogramBin>, double> RunHistogramWithDict(
    const Sequence<double>& data,
    const HistogramParams& params,
    const HistogramKeyFunc& key_func) {
    Dict dict;
    auto start = Clock::now();
    auto bins = BuildHistogram<Dict>(data, params, key_func, dict);
    auto end = Clock::now();
    std::chrono::duration<double, std::milli> dur = end - start;
    return {bins, dur.count()};
}

void PrintHistogram(const Sequence<HistogramBin>& bins) {
    std::cout << "Гистограмма:\n";
    for (int i = 0; i < bins.GetLength(); ++i) {
        auto b = bins.Get(i);
        std::cout << "[" << std::setw(8) << b.from << ", " << std::setw(8) << b.to << ") : "
                  << b.count << "\n";
    }
}

void SaveHistogramToCsv(const std::string& filename,
                        const Sequence<HistogramBin>& bins,
                        const std::string& dict_name,
                        double elapsed_ms,
                        const std::vector<std::vector<double>>& bin_values) {
    std::ofstream out(filename);
    if (!out) {
        std::cout << "Не удалось открыть файл для записи.\n";
        return;
    }
    out << "dict_name,bin_from,bin_to,count,elapsed_ms,values\n";
    for (int i = 0; i < bins.GetLength(); ++i) {
        auto b = bins.Get(i);
        out << dict_name << "," << b.from << "," << b.to << "," << b.count << "," << elapsed_ms << ",";

        if (i >= 0 && static_cast<std::size_t>(i) < bin_values.size()) {
            const auto& vals = bin_values[static_cast<std::size_t>(i)];
            for (std::size_t j = 0; j < vals.size(); ++j) {
                if (j > 0) {
                    out << ' ';
                }
                out << vals[j];
            }
        }
        out << "\n";
    }
    std::cout << "Результат сохранён в файл " << filename << "\n";
}

// ==============================
// Векторная графика (SVG) для гистограммы
// ==============================

// Вспомогательная функция: вычисление индекса бина для значения с учётом
// параметров гистограммы и выбранной key-функции.
int ComputeBinIndexForValue(double value,
                            const HistogramParams& params,
                            const HistogramKeyFunc& key_func) {
    if (params.bins_count <= 0) return 0;
    double range = params.max_value - params.min_value;
    if (range <= 0.0) return 0;

    double bin_width = range / params.bins_count;
    double mapped = key_func ? key_func(value) : value;
    if (mapped < params.min_value) return 0;
    if (mapped >= params.max_value) return params.bins_count - 1;

    int idx = static_cast<int>((mapped - params.min_value) / bin_width);
    if (idx < 0) idx = 0;
    if (idx >= params.bins_count) idx = params.bins_count - 1;
    return idx;
}

// Формирование набора значений для каждого бина гистограммы.
// Для каждого исходного числа x сохраняем (после применения key_func)
// его векторно в соответствующий интервал.
std::vector<std::vector<double>> BuildBinValues(const Sequence<double>& data,
                                                const HistogramParams& params,
                                                const HistogramKeyFunc& key_func) {
    std::vector<std::vector<double>> result(
        static_cast<std::size_t>(std::max(params.bins_count, 0)));

    for (int i = 0; i < data.GetLength(); ++i) {
        double v_raw = data.Get(i);
        int idx = ComputeBinIndexForValue(v_raw, params, key_func);
        if (idx < 0 || idx >= params.bins_count) continue;

        double v = key_func ? key_func(v_raw) : v_raw;
        result[static_cast<std::size_t>(idx)].push_back(v);
    }
    return result;
}

// Построение SVG‑файла с числовой прямой и точками по бинам гистограммы.
// bins здесь — обычный std::vector, чтобы можно было загружать гистограмму
// из CSV независимо от внутренних Sequence/Dictionary.
// bin_values содержит реальные числа, попавшие в каждый из отрезков.
void SaveHistogramToSvg(const std::string& filename,
                        const std::vector<HistogramBin>& bins,
                        const std::vector<std::vector<double>>& bin_values) {
    if (bins.empty()) {
        std::cout << "SVG: гистограмма пуста, нечего визуализировать.\n";
        return;
    }

    const int width = 800;
    const int height = 200;
    const int margin_left = 60;
    const int margin_right = 40;
    const int axis_y = height / 2;

    // Диапазон по оси X
    double min_x = bins.front().from;
    double max_x = bins.front().to;
    for (const auto& b : bins) {
        if (b.from < min_x) min_x = b.from;
        if (b.to > max_x)   max_x = b.to;
    }
    double range = max_x - min_x;
    if (range <= 0.0) {
        range = 1.0;
        max_x = min_x + range;
    }

    // Внутренний диапазон для делений и точек — заметно короче оси,
    // чтобы хвост и стрелка значительно выходили за крайние отрезки.
    const int inner_left = margin_left + 80;
    const int inner_right = width - margin_right - 80;

    auto to_screen_x = [&](double x) -> double {
        double t = (x - min_x) / range; // [0,1]
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;
        return inner_left + t * (inner_right - inner_left);
    };

    std::ofstream out(filename);
    if (!out) {
        std::cout << "Не удалось открыть файл " << filename << " для записи SVG.\n";
        return;
    }

    out << R"(<?xml version="1.0" encoding="UTF-8"?>)"
        << "\n";
    out << R"(<svg xmlns="http://www.w3.org/2000/svg" width=")" << width
        << R"(" height=")" << height << R"(" viewBox="0 0 )" << width << " " << height << R"(">)"
        << "\n";

    // Фон
    out << R"(<rect x="0" y="0" width="100%" height="100%" fill="white"/>)"
        << "\n";

    // Ось (числовая прямая) со стрелкой вправо
    double x0 = margin_left;
    double x1 = width - margin_right;
    out << R"(<line x1=")" << x0 << R"(" y1=")" << axis_y
        << R"(" x2=")" << x1 << R"(" y2=")" << axis_y
        << R"(" stroke="black" stroke-width="2"/>)"
        << "\n";
    // Стрелка направления
    out << R"(<polygon points=")"
        << x1 << "," << axis_y
        << " " << (x1 - 10) << "," << (axis_y - 5)
        << " " << (x1 - 10) << "," << (axis_y + 5)
        << R"(" fill="black"/>)"
        << "\n";

    // Деления по границам бинов + подписи границ отрезков (сверху оси)
    out << std::fixed << std::setprecision(2);
    for (const auto& b : bins) {
        double sx = to_screen_x(b.from);
        out << R"(<line x1=")" << sx << R"(" y1=")" << (axis_y - 8)
            << R"(" x2=")" << sx << R"(" y2=")" << (axis_y + 8)
            << R"(" stroke="black" stroke-width="1"/>)"
            << "\n";
        out << R"(<text x=")" << sx << R"(" y=")" << (axis_y - 12)
            << R"(" text-anchor="middle" font-size="10" fill="black">)"
            << b.from << R"(</text>)"
            << "\n";
    }
    double sx_last = to_screen_x(bins.back().to);
    out << R"(<line x1=")" << sx_last << R"(" y1=")" << (axis_y - 8)
        << R"(" x2=")" << sx_last << R"(" y2=")" << (axis_y + 8)
        << R"(" stroke="black" stroke-width="1"/>)"
        << "\n";
    out << R"(<text x=")" << sx_last << R"(" y=")" << (axis_y - 12)
        << R"(" text-anchor="middle" font-size="10" fill="black">)"
        << bins.back().to << R"(</text>)"
        << "\n";

    // Отдельные числа внутри каждого интервала: каждое значение,
    // попавшее в бин, отображается точкой на оси и числом под осью.
    // Если точки близко, подписи "лесенкой" уходят ниже.
    struct ValuePoint {
        double x;
        double v;
    };
    std::vector<ValuePoint> all_points;
    for (std::size_t i = 0; i < bins.size() && i < bin_values.size(); ++i) {
        const auto& vals = bin_values[i];
        for (double v : vals) {
            all_points.push_back({to_screen_x(v), v});
        }
    }
    std::sort(all_points.begin(), all_points.end(),
              [](const ValuePoint& a, const ValuePoint& b) {
                  return a.x < b.x;
              });

    const double close_threshold = 12.0; // пиксели по X, считаем "близко"
    const int value_point_radius = 3;
    const int base_offset_y = 22;
    const int step_offset_y = 16;
    int current_level = 0;

    for (std::size_t i = 0; i < all_points.size(); ++i) {
        if (i > 0 && std::abs(all_points[i].x - all_points[i - 1].x) < close_threshold) {
            ++current_level; // ближе — уходим ниже предыдущего
        } else {
            current_level = 0; // новый кластер — снова ближе к оси
        }

        double sx = all_points[i].x;
        double v = all_points[i].v;

        // Точка всегда лежит на оси, подпись — ниже, "лесенкой" при скученности.
        double py = axis_y;
        double text_y = axis_y + base_offset_y + current_level * step_offset_y;

        out << R"(<circle cx=")" << sx << R"(" cy=")" << py
            << R"(" r=")" << value_point_radius
            << R"(" fill="blue" stroke="black" stroke-width="1"/>)"
            << "\n";

        out << R"(<text x=")" << sx << R"(" y=")" << text_y
            << R"(" text-anchor="middle" font-size="10" fill="black">)"
            << v << R"(</text>)"
            << "\n";
    }

    out << R"(</svg>)" << "\n";

    std::cout << "SVG‑визуализация гистограммы сохранена в файл " << filename << "\n";
}

// Загрузка гистограммы из CSV (формат, который создаёт SaveHistogramToCsv)
// и генерация SVG + открытие системным просмотрщиком.
void VisualizeHistogramFromCsv() {
    std::cout << "Введите имя CSV‑файла с гистограммой (по умолчанию histogram_single.csv): ";
    std::string fname;
    ClearInput();
    std::getline(std::cin, fname);
    if (fname.empty()) fname = "histogram_single.csv";

    std::ifstream in(fname);
    if (!in) {
        std::cout << "Не удалось открыть файл " << fname << " для чтения.\n";
        return;
    }

    std::string line;
    // Пропускаем заголовок
    if (!std::getline(in, line)) {
        std::cout << "Файл пуст.\n";
        return;
    }

    std::vector<HistogramBin> bins;
    std::vector<std::vector<double>> bin_values;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string dict_name;
        std::string s_from, s_to, s_count, s_elapsed, s_values;

        if (!std::getline(ss, dict_name, ',')) continue;
        if (!std::getline(ss, s_from, ',')) continue;
        if (!std::getline(ss, s_to, ',')) continue;
        if (!std::getline(ss, s_count, ',')) continue;
        if (!std::getline(ss, s_elapsed, ',')) continue;
        std::getline(ss, s_values); // список значений (может быть пустым)

        HistogramBin b{};
        try {
            b.from = std::stod(s_from);
            b.to = std::stod(s_to);
            b.count = static_cast<std::size_t>(std::stoll(s_count));
        } catch (...) {
            continue;
        }
        bins.push_back(b);

        std::vector<double> vals;
        if (!s_values.empty()) {
            std::stringstream ssv(s_values);
            double vv;
            while (ssv >> vv) {
                vals.push_back(vv);
            }
        }
        bin_values.push_back(std::move(vals));
    }

    if (bins.empty()) {
        std::cout << "В файле " << fname << " не удалось прочитать ни одного интервала гистограммы.\n";
        return;
    }

    std::string svg_name = "histogram.svg";
    SaveHistogramToSvg(svg_name, bins, bin_values);

    // Открытие SVG системным просмотрщиком (на macOS — команда open).
    // Если вы используете Linux или Windows, замените команду при необходимости.
    std::string cmd = "open \"" + svg_name + "\"";
    int res = std::system(cmd.c_str());
    if (res != 0) {
        std::cout << "Не удалось автоматически открыть SVG. Откройте файл "
                  << svg_name << " вручную.\n";
    }
}

void CompareAlgorithms(const DataSet& ds) {
    if (!ds.generated) {
        std::cout << "Сначала введите или сгенерируйте данные.\n";
        return;
    }
    auto params = ReadHistogramParams(ds);
    auto key_func = ChooseKeyFunc();
    auto bin_values = BuildBinValues(ds.values, params, key_func);

    std::cout << "Выполняется построение гистограммы для трёх реализаций словаря...\n";

    auto r1 = RunHistogramWithDict<SortedArrayDictionary<int, std::size_t>>(ds.values, params, key_func);
    auto r2 = RunHistogramWithDict<BinarySearchTreeDictionary<int, std::size_t>>(ds.values, params, key_func);
    auto r3 = RunHistogramWithDict<HashTableDictionary<int, std::size_t>>(ds.values, params, key_func);

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "\nСравнение времени построения (мс):\n";
    std::cout << "  SortedArrayDictionary   : " << r1.second << "\n";
    std::cout << "  BinarySearchTreeDictionary: " << r2.second << "\n";
    std::cout << "  HashTableDictionary     : " << r3.second << "\n";

    // Дополнительно можно выгрузить результаты сравнения во внешний CSV
    std::cout << "Сохранить таблицу сравнения во внешний CSV-файл? (1 - да, 0 - нет): ";
    int cmp_save = 0;
    if (std::cin >> cmp_save && cmp_save == 1) {
        ClearInput();
        std::string fname;
        std::cout << "Имя файла (по умолчанию compare.csv): ";
        std::getline(std::cin, fname);
        if (fname.empty()) fname = "compare.csv";
        std::ofstream out(fname);
        if (!out) {
            std::cout << "Не удалось открыть файл для записи.\n";
        } else {
            out << "dict_name,elapsed_ms\n";
            out << "SortedArray," << r1.second << "\n";
            out << "BST," << r2.second << "\n";
            out << "HashTable," << r3.second << "\n";
            std::cout << "Таблица сравнения сохранена в " << fname << "\n";
        }
    } else {
        ClearInput();
    }

    std::cout << "\nКакую гистограмму вывести на экран?\n";
    std::cout << "1) SortedArrayDictionary\n";
    std::cout << "2) BinarySearchTreeDictionary\n";
    std::cout << "3) HashTableDictionary\n";
    int choice = ReadInt("Ваш выбор: ", 1, 3);
    const MutableArraySequence<HistogramBin>* bins = nullptr;
    double t_ms = 0.0;
    std::string name;
    if (choice == 1) {
        bins = &r1.first;
        t_ms = r1.second;
        name = "SortedArray";
    } else if (choice == 2) {
        bins = &r2.first;
        t_ms = r2.second;
        name = "BST";
    } else {
        bins = &r3.first;
        t_ms = r3.second;
        name = "HashTable";
    }
    PrintHistogram(*bins);
    std::cout << "Время построения: " << t_ms << " мс\n";

    std::cout << "Сохранить эту гистограмму в CSV? (1 - да, 0 - нет): ";
    int save_choice = 0;
    if (std::cin >> save_choice && save_choice == 1) {
        ClearInput();
        std::cout << "Введите имя файла (по умолчанию histogram.csv): ";
        std::string fname;
        std::getline(std::cin, fname);
        if (fname.empty()) fname = "histogram.csv";
        SaveHistogramToCsv(fname, *bins, name, t_ms, bin_values);
    } else {
        ClearInput();
    }
}

void PrintDataPreview(const DataSet& ds) {
    if (!ds.generated || ds.values.GetLength() == 0) {
        std::cout << "Данные отсутствуют.\n";
        return;
    }
    std::cout << "Всего элементов: " << ds.values.GetLength() << "\n";
    int show = std::min(ds.values.GetLength(), 20);
    std::cout << "Первые " << show << " элементов:\n";
    for (int i = 0; i < show; ++i) {
        std::cout << ds.values.Get(i) << " ";
    }
    std::cout << "\n";
}

void BuildSingleAlgorithm(const DataSet& ds) {
    if (!ds.generated) {
        std::cout << "Сначала введите или сгенерируйте данные.\n";
        return;
    }
    auto params = ReadHistogramParams(ds);
    auto key_func = ChooseKeyFunc();
    auto bin_values = BuildBinValues(ds.values, params, key_func);

    std::cout << "Выберите алгоритм (структуру словаря):\n";
    std::cout << "1) SortedArrayDictionary\n";
    std::cout << "2) BinarySearchTreeDictionary\n";
    std::cout << "3) HashTableDictionary\n";
    int choice = ReadInt("Ваш выбор: ", 1, 3);

    if (choice == 1) {
        auto r = RunHistogramWithDict<SortedArrayDictionary<int, std::size_t>>(ds.values, params, key_func);
        MutableArraySequence<HistogramBin> bins = r.first; // копирующий конструктор, без присваивания
        double t_ms = r.second;
        std::string name = "SortedArray";

        PrintHistogram(bins);
        std::cout << "Время построения: " << t_ms << " мс\n";

        std::cout << "Сохранить эту гистограмму в CSV? (1 - да, 0 - нет): ";
        int save_choice = 0;
        if (std::cin >> save_choice && save_choice == 1) {
            ClearInput();
            std::cout << "Введите имя файла (по умолчанию histogram_single.csv): ";
            std::string fname;
            std::getline(std::cin, fname);
            if (fname.empty()) fname = "histogram_single.csv";
            SaveHistogramToCsv(fname, bins, name, t_ms, bin_values);
        } else {
            ClearInput();
        }
    } else if (choice == 2) {
        auto r = RunHistogramWithDict<BinarySearchTreeDictionary<int, std::size_t>>(ds.values, params, key_func);
        MutableArraySequence<HistogramBin> bins = r.first;
        double t_ms = r.second;
        std::string name = "BST";

        PrintHistogram(bins);
        std::cout << "Время построения: " << t_ms << " мс\n";

        std::cout << "Сохранить эту гистограмму в CSV? (1 - да, 0 - нет): ";
        int save_choice = 0;
        if (std::cin >> save_choice && save_choice == 1) {
            ClearInput();
            std::cout << "Введите имя файла (по умолчанию histogram_single.csv): ";
            std::string fname;
            std::getline(std::cin, fname);
            if (fname.empty()) fname = "histogram_single.csv";
            SaveHistogramToCsv(fname, bins, name, t_ms, bin_values);
        } else {
            ClearInput();
        }
    } else {
        auto r = RunHistogramWithDict<HashTableDictionary<int, std::size_t>>(ds.values, params, key_func);
        MutableArraySequence<HistogramBin> bins = r.first;
        double t_ms = r.second;
        std::string name = "HashTable";

        PrintHistogram(bins);
        std::cout << "Время построения: " << t_ms << " мс\n";

        std::cout << "Сохранить эту гистограмму в CSV? (1 - да, 0 - нет): ";
        int save_choice = 0;
        if (std::cin >> save_choice && save_choice == 1) {
            ClearInput();
            std::cout << "Введите имя файла (по умолчанию histogram_single.csv): ";
            std::string fname;
            std::getline(std::cin, fname);
            if (fname.empty()) fname = "histogram_single.csv";
            SaveHistogramToCsv(fname, bins, name, t_ms, bin_values);
        } else {
            ClearInput();
        }
    }
}

void MainMenu() {
    DataSet data;
    while (true) {
        std::cout << "\n=== Лабораторная работа №2: сравнение алгоритмов поиска ===\n";
        std::cout << "1) Ручной ввод данных\n";
        std::cout << "2) Генерация случайных данных\n";
        std::cout << "3) Просмотр текущих данных\n";
        std::cout << "4) Построение гистограммы (один алгоритм)\n";
        std::cout << "5) Сравнение алгоритмов (построение гистограммы)\n";
        std::cout << "6) Запустить тесты\n";
        std::cout << "7) тест VirtualFS\n";
        std::cout << "8) Визуализировать гистограмму из CSV (SVG‑числовая прямая)\n";
        std::cout << "0) Выход\n";

        int cmd = ReadInt("Ваш выбор: ", 0, 8);
        try {
            if (cmd == 0) {
                break;
            } else if (cmd == 1) {
                InputDataManual(data);
            } else if (cmd == 2) {
                GenerateRandomData(data);
            } else if (cmd == 3) {
                PrintDataPreview(data);
            } else if (cmd == 4) {
                BuildSingleAlgorithm(data);
            } else if (cmd == 5) {
                CompareAlgorithms(data);
            } else if (cmd == 6) {
                // Тесты из модуля histogram/словарей
                tests::RunAll();
                // Дополнительно тестируем все реализации из Realization и виртуальную ФС
                TestAllDictionaries();
                TestAllPriorityQueues();
                TestAllSets();
                TestAllSortedSequences();
                TestVirtualFileSystem();
            } else if (cmd == 7) {
                VirtualFsPlayground();
            } else if (cmd == 8) {
                VisualizeHistogramFromCsv();
            }
        } catch (const std::exception& ex) {
            std::cout << "Ошибка: " << ex.what() << "\n";
        }
    }
}

int main() {
    std::setlocale(LC_ALL, "ru_RU.UTF-8");
    MainMenu();
    return 0;
}


