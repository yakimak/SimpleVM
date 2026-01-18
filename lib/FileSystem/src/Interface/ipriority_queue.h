#pragma once

#include <cstddef>

// Чисто виртуальный интерфейс очереди с приоритетом.
// Элементы типа TElement упорядочиваются по оператору < (максимум сверху).
template <typename TElement>
class IPriorityQueue {
public:
    using ValueType = TElement;

    virtual ~IPriorityQueue() = default;

    // Очередь пуста?
    virtual bool IsEmpty() const = 0;

    // Количество элементов в очереди.
    virtual int GetSize() const = 0;

    // Элемент с максимальным приоритетом (без удаления).
    // Должен выбросить std::out_of_range, если очередь пуста.
    virtual const TElement& Top() const = 0;

    // Вставка нового элемента.
    virtual void Push(const TElement& value) = 0;

    // Достать элемент с максимальным приоритетом и удалить его из очереди.
    // Должен выбросить std::out_of_range, если очередь пуста.
    virtual TElement Pop() = 0;
};


