#pragma once

#include "../../Interface/iset.h"
#include "../IDictionary/hash_table_dict.h"

// Множество на основе уже реализованной HashTableDictionary.
// В качестве значения используем фиктивный тип (char), нас интересуют только ключи.
template <typename TElement>
class HashSet : public ISet<TElement> {
public:
    using ValueType = typename ISet<TElement>::ValueType;

    std::size_t Count() const override {
        return dict_.Count();
    }

    bool IsEmpty() const override {
        return dict_.Count() == 0;
    }

    bool Contains(const TElement& value) const override {
        return dict_.ContainsKey(value);
    }

    void Add(const TElement& value) override {
        if (dict_.ContainsKey(value)) return;
        dict_.Add(value, 1);
    }

    void Remove(const TElement& value) override {
        dict_.Remove(value);
    }

private:
    HashTableDictionary<TElement, char> dict_;
};


