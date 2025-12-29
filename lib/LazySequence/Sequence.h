#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "numerable.h"
#include <stdexcept>

// Базовый интерфейс АТД Sequence (минимально скорректирован,
// чтобы не блокировать наследование LazySequence)
template<typename T>
class Sequence {
protected:
    using ValueType = T;

public:
    virtual ~Sequence() = default;

    // Базовые операции доступа/модификации
    virtual T GetFirst() const = 0;
    virtual T GetLast() const = 0;
    virtual T Get(int index) const = 0;
    virtual void Set(int index, T value) = 0;

    // Извлечение подпоследовательности
    virtual Sequence<T>* GetSubsequence(int startIndex, int endIndex) = 0;

    // Длина (для бесконечных реализаций допускается -1)
    virtual int GetLength() const = 0;

    // Модифицирующие операции (ковариантный возврат у наследников допустим)
    virtual Sequence<T>* Append(T item) = 0;
    virtual Sequence<T>* Prepend(T item) = 0;
    virtual Sequence<T>* InsertAt(T item, int index) = 0;
    virtual Sequence<T>* Concat(Sequence<T>* list) = 0;

    // Индексация
    virtual T operator[](int index) const = 0;

    virtual Sequence<T>* Map(T (*)(T&)) {
        throw std::logic_error("Sequence::Map(T(*)(T&)) is not implemented");
    }
    virtual Sequence<T>* Map(T (*)(T&, int)) {
        throw std::logic_error("Sequence::Map(T(*)(T&,int)) is not implemented");
    }
    virtual Sequence<T>* where(bool (*)(T&)) {
        throw std::logic_error("Sequence::where(bool(*)(T&)) is not implemented");
    }

    // Reduce — сигнатуры могли различаться в старых версиях,
    // поэтому предоставляем безопасную заглушку.
    virtual T ReduceStub() const {
        throw std::logic_error("Sequence::Reduce is not implemented in this interface");
    }
};

#endif // SEQUENCE_H
