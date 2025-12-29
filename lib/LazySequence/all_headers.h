#ifndef ALL_HEADERS_H
#define ALL_HEADERS_H

// #include "DynamicArray.h"
// #include "LinkedList.h"
// #include "ArraySequence.h"
// #include "ListSequence.h"
// #include "ImmutableListSequence.h"
// #include "MutableListSequence.h"
// #include "ImmutableArraySequence.h"
// #include "MutableArraySequence.h"
// #include "Deque.h"
// #include "SkipListSequence.h"

#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include <stdexcept>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>
#include <optional>
#include <queue>
#include <stack>
#include <string>
#include <fstream>
#include <sstream>

// Подключаем наши заголовочные файлы
// #include "numerable.h"
#include "Sequence.h"
// #include "TuplePrinter.h"
// #include "ZipOperationSequence.h"

// Forward declarations
template<typename T> class LazySequence;


// Исключения
class IndexOutOfRange : public std::out_of_range {
public:
    IndexOutOfRange(const std::string& msg) : std::out_of_range(msg) {}
};

#endif // ALL_HEADERS_H
