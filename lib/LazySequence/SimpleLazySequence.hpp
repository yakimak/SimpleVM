#ifndef SIMPLE_LAZY_SEQUENCE_HPP
#define SIMPLE_LAZY_SEQUENCE_HPP

#include <functional>

//Ленивая последовательность согласно ТЗ
//Используется для потоков команд и данных

template<typename T>
class LazySequence {
private:
    std::function<T()> generator;
    std::function<bool()> has_next;

public:
    LazySequence(std::function<T()> gen, std::function<bool()> has_nxt)
        : generator(gen), has_next(has_nxt) {}

    T next() {
        return generator();
    }

    bool hasNext() const {
        return has_next();
    }

    void reset() {
        // В простой версии генератор управляется внешне
    }
};

#endif // SIMPLE_LAZY_SEQUENCE_HPP

