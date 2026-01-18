#pragma once

#include "../../Interface/iset.h"
#include "../../../LibrarySequence/4/BinaryTree.h"

#include <stdexcept>

// Множество на основе сбалансированного бинарного дерева поиска BinaryTree<T>.
template <typename TElement>
class TreeSet : public ISet<TElement> {
public:
    using ValueType = typename ISet<TElement>::ValueType;

    std::size_t Count() const override {
        return tree_.Size();
    }

    bool IsEmpty() const override {
        return tree_.Empty();
    }

    bool Contains(const TElement& value) const override {
        return tree_.Search(value);
    }

    void Add(const TElement& value) override {
        // не допускаем дубликатов
        if (Contains(value)) return;
        tree_.Insert(value);
    }

    void Remove(const TElement& value) override {
        tree_.Remove(value); // BinaryTree сам бросит исключение, если элемента нет
    }

private:
    BinaryTree<TElement> tree_;
};


