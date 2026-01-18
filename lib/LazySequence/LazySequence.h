#ifndef LAZY_SEQUENCE_H
#define LAZY_SEQUENCE_H

#include <functional>
#include <vector>
#include <memory>
#include <stdexcept>
#include "Cardinal.h"
#include "Sequence.h"

template <class T> class Generator;

template <class T>
class LazySequence {
    template <class U> friend class Generator;
private:
    // Мемоизированные элементы (используем указатели: nullptr = не вычислено)
    mutable std::vector<T*> memoized_elements_;
    
    // Генератор на основе предыдущих элементов (рекуррентное правило)
    std::function<T(const Sequence<T>&)> generator_;
    
    // Генератор на основе индекса (простая функция от индекса)
    std::function<T(size_t)> index_generator_;
    
    // Начальная последовательность для рекуррентных правил
    std::shared_ptr<Sequence<T>> initial_sequence_;
    
    // Флаги состояния
    bool is_infinite_;
    size_t generation_count_;  // Для конечных последовательностей - известное количество элементов
    
    // Ссылка на базовую последовательность для ленивых операций (Where)
    std::shared_ptr<LazySequence<T>> base_sequence_;
    std::function<bool(T)> where_predicate_;

public:
    LazySequence();
    LazySequence(T* items, int count);
    LazySequence(const Sequence<T>* seq);
    LazySequence(std::function<T(const Sequence<T>&)> generator, const Sequence<T>& initial);
    LazySequence(std::function<T(size_t)> index_gen, bool infinite = true);
    LazySequence(const LazySequence<T>& other);
    ~LazySequence();

    T GetFirst();
    T GetLast();
    T Get(int index);
    std::shared_ptr<LazySequence<T>> GetSubsequence(int startIndex, int endIndex);
    Cardinal GetLength() const;
    size_t GetMaterializedCount() const;
    bool IsInfinite() const;
    void SetGenerationCount(size_t count);

    Sequence<T>* Append(T item);
    Sequence<T>* Prepend(T item);
    std::shared_ptr<LazySequence<T>> InsertAt(T item, int index);
    LazySequence<T>* Concat(LazySequence<T>* list);
    
    template <class T2>
    std::shared_ptr<LazySequence<T2>> Map(T2(*mapper)(T));
    
    template <class T2>
    std::shared_ptr<LazySequence<T2>> Map(std::function<T2(T)> mapper);
    
    template <class T2>
    T2 Reduce(std::function<T2(T2, T)> reducer, T2 initial, size_t max_elements = SIZE_MAX) const;
    
    T Reduce(std::function<T(T, T)> reducer, T initial, size_t max_elements = SIZE_MAX) const;
    
    std::shared_ptr<LazySequence<T>> Where(bool (*predicate)(T));
    std::shared_ptr<LazySequence<T>> Where(std::function<bool(T)> predicate);
    
    template <class T2>
    std::shared_ptr<LazySequence<std::pair<T, T2>>> Zip(Sequence<T2>* seq);
    
    template <class T2>
    std::shared_ptr<LazySequence<std::pair<T, T2>>> Zip(std::shared_ptr<Sequence<T2>> seq);
    
    template <class T2>
    std::shared_ptr<LazySequence<std::pair<T, T2>>> Zip(std::shared_ptr<LazySequence<T2>> seq);

private:
    void EnsureMaterialized(int index) const;
    T GenerateNext() const;
};

//////////////////////////////////////////////////////////////////////

template <class T> LazySequence<T>::LazySequence() : is_infinite_(false), generation_count_(0) {}

template <class T> LazySequence<T>::LazySequence(T* items, int count)
    : is_infinite_(false), generation_count_(count) {
    memoized_elements_.resize(count);
    for (int i = 0; i < count; ++i) {
        memoized_elements_[i] = new T(items[i]);
    }
}

template <class T> LazySequence<T>::LazySequence(const Sequence<T>* seq)
    : is_infinite_(false), generation_count_(seq->GetLength()) {
    size_t len = seq->GetLength();
    memoized_elements_.resize(len);
    for (size_t i = 0; i < len; ++i) {
        memoized_elements_[i] = new T(seq->Get((int)i));
    }
}

template <class T> LazySequence<T>::LazySequence(std::function<T(const Sequence<T>&)> generator, const Sequence<T>& initial)
    : generator_(generator), is_infinite_(true), generation_count_(0) {
    initial_sequence_ = std::make_shared<Sequence<T>>(initial);
    size_t len = initial.GetLength();
    memoized_elements_.resize(len);
    for (size_t i = 0; i < len; ++i) {
        memoized_elements_[i] = new T(initial.Get((int)i));
    }
    generation_count_ = len;
}

template <class T> LazySequence<T>::LazySequence(std::function<T(size_t)> index_gen, bool infinite)
    : index_generator_(index_gen), is_infinite_(infinite), generation_count_(0) {}

template <class T> LazySequence<T>::LazySequence(const LazySequence<T>& other)
    : generator_(other.generator_),
    index_generator_(other.index_generator_),
    initial_sequence_(other.initial_sequence_),
    is_infinite_(other.is_infinite_),
    generation_count_(other.generation_count_),
    base_sequence_(other.base_sequence_),
    where_predicate_(other.where_predicate_) {
    memoized_elements_.resize(other.memoized_elements_.size());
    for (size_t i = 0; i < other.memoized_elements_.size(); ++i) {
        if (other.memoized_elements_[i] != nullptr) {
            memoized_elements_[i] = new T(*other.memoized_elements_[i]);
        } else {
            memoized_elements_[i] = nullptr;
        }
    }
}

template <class T> LazySequence<T>::~LazySequence() {
    for (auto* ptr : memoized_elements_) {
        if (ptr != nullptr) {
            delete ptr;
        }
    }
}

//////////////////////////////////////////////////////////////////////

template <class T> T LazySequence<T>::GetFirst() {
    return Get(0);
}

template <class T> T LazySequence<T>::GetLast() {
    if (is_infinite_) {
        throw std::logic_error("Cannot get last element of infinite sequence");
    }
    if (generation_count_ == 0) {
        throw std::out_of_range("Sequence is empty");
    }
    EnsureMaterialized((int)generation_count_ - 1);
    return *memoized_elements_[generation_count_ - 1];
}

template <class T> T LazySequence<T>::Get(int index) {
    if (index < 0) {
        throw std::out_of_range("Negative index");
    }
    
    // Если это ленивая операция (Where), делегируем базовой последовательности
    if (base_sequence_ && where_predicate_) {
        // Для Where нужно найти k-й элемент, удовлетворяющий предикату
        size_t found = 0;
        size_t current = 0;
        while (true) {
            if (!base_sequence_->IsInfinite()) {
                Cardinal len = base_sequence_->GetLength();
                if (len.IsFinite() && current >= len.GetFiniteValue()) {
                    throw std::out_of_range("Not enough elements matching predicate");
                }
            }
            T val = base_sequence_->Get((int)current);
            if (where_predicate_(val)) {
                if (found == (size_t)index) {
                    return val;
                }
                ++found;
            }
            ++current;
        }
    }
    
    if (!is_infinite_ && index >= (int)generation_count_) {
        throw std::out_of_range("Index out of range");
    }

    EnsureMaterialized(index);
    return *memoized_elements_[index];
}

template <class T> std::shared_ptr<LazySequence<T>> LazySequence<T>::GetSubsequence(int startIndex, int endIndex) {
    if (startIndex < 0 || startIndex > endIndex) {
        throw std::out_of_range("Invalid index range");
    }

    if (!is_infinite_ && endIndex >= (int)generation_count_) {
        throw std::out_of_range("Index out of range");
    }

    EnsureMaterialized(endIndex);
    int subSize = endIndex - startIndex + 1;
    T* subItems = new T[subSize];
    for (int i = 0; i < subSize; ++i) {
        EnsureMaterialized(startIndex + i);
        subItems[i] = *memoized_elements_[startIndex + i];
    }
    auto result = std::make_shared<LazySequence<T>>(subItems, subSize);
    delete[] subItems;
    return result;
}

template <class T> Cardinal LazySequence<T>::GetLength() const {
    if (is_infinite_) {
        return Cardinal::AlephNull();
    }
    return Cardinal(generation_count_);
}

template <class T> size_t LazySequence<T>::GetMaterializedCount() const { 
    size_t count = 0;
    for (const auto& elem : memoized_elements_) {
        if (elem != nullptr) ++count;
    }
    return count;
}

template <class T> bool LazySequence<T>::IsInfinite() const {
    return is_infinite_;
}

template <class T> void LazySequence<T>::SetGenerationCount(size_t count) {
    generation_count_ = count;
}

//////////////////////////////////////////////////////////////////////

template <class T> Sequence<T>* LazySequence<T>::Append(T item) {
    if (is_infinite_) {
        throw std::logic_error("Cannot append to infinite sequence");
    }
    
    // Материализуем все элементы и создаем новую Sequence
    int newSize = (int)generation_count_ + 1;
    T* elements = new T[newSize];
    for (size_t i = 0; i < generation_count_; ++i) {
        EnsureMaterialized((int)i);
        elements[i] = *memoized_elements_[i];
    }
    elements[generation_count_] = item;
    Sequence<T>* result = new Sequence<T>(elements, newSize);
    delete[] elements;
    return result;
}

template <class T> Sequence<T>* LazySequence<T>::Prepend(T item) {
    if (is_infinite_) {
        throw std::logic_error("Cannot prepend to infinite sequence");
    }
    
    int newSize = (int)generation_count_ + 1;
    T* elements = new T[newSize];
    elements[0] = item;
    for (size_t i = 0; i < generation_count_; ++i) {
        EnsureMaterialized((int)i);
        elements[i + 1] = *memoized_elements_[i];
    }
    Sequence<T>* result = new Sequence<T>(elements, newSize);
    delete[] elements;
    return result;
}

template <class T> std::shared_ptr<LazySequence<T>> LazySequence<T>::InsertAt(T item, int index) {
    if (index < 0 || (!is_infinite_ && index > (int)generation_count_)) {
        throw std::out_of_range("Index out of range");
    }
    
    if (is_infinite_) {
        return std::make_shared<LazySequence<T>>(*this);
    }

    int newSize = (int)generation_count_ + 1;
    T* elements = new T[newSize];
    for (int i = 0; i < index; ++i) {
        EnsureMaterialized(i);
        elements[i] = *memoized_elements_[i];
    }
    elements[index] = item;
    for (size_t i = index; i < generation_count_; ++i) {
        EnsureMaterialized((int)i);
        elements[i + 1] = *memoized_elements_[i];
    }
    auto result = std::make_shared<LazySequence<T>>(elements, newSize);
    delete[] elements;
    return result;
}

template <class T> LazySequence<T>* LazySequence<T>::Concat(LazySequence<T>* list) {
    if (!list) {
        throw std::invalid_argument("List cannot be null");
    }
    
    if (is_infinite_) {
        // Для бесконечных последовательностей возвращаем копию текущей
        LazySequence<T>* result = new LazySequence<T>(*this);
        return result;
    }
    
    // Добавляем элементы из текущей последовательности
    size_t other_count = list->IsInfinite() ? SIZE_MAX : list->generation_count_;
    size_t limit = std::min(other_count, (size_t)10000); // Ограничение для безопасности
    int totalSize = (int)generation_count_ + (int)limit;
    T* elements = new T[totalSize];
    
    for (size_t i = 0; i < generation_count_; ++i) {
        EnsureMaterialized((int)i);
        elements[i] = *memoized_elements_[i];
    }
    // Добавляем элементы из второй последовательности
    for (size_t i = 0; i < limit; ++i) {
        elements[generation_count_ + i] = list->Get((int)i);
    }
    
    LazySequence<T>* result = new LazySequence<T>(elements, totalSize);
    delete[] elements;
    if (list->is_infinite_) {
        result->is_infinite_ = true;
    }
    return result;
}

template <class T>
template <class T2>
std::shared_ptr<LazySequence<T2>> LazySequence<T>::Map(T2(*mapper)(T)) {
    // Создаем ленивую последовательность с генератором на основе текущей
    auto base_copy = std::make_shared<LazySequence<T>>(*this);
    std::function<T2(size_t)> mapped_gen = [base_copy, mapper](size_t index) -> T2 {
        return mapper(base_copy->Get((int)index));
    };
    auto new_seq = std::make_shared<LazySequence<T2>>(mapped_gen, is_infinite_);
    new_seq->SetGenerationCount(generation_count_);
    return new_seq;
}

template <class T>
template <class T2>
std::shared_ptr<LazySequence<T2>> LazySequence<T>::Map(std::function<T2(T)> mapper) {
    auto base_copy = std::make_shared<LazySequence<T>>(*this);
    std::function<T2(size_t)> mapped_gen = [base_copy, mapper](size_t index) -> T2 {
        return mapper(base_copy->Get((int)index));
    };
    auto new_seq = std::make_shared<LazySequence<T2>>(mapped_gen, is_infinite_);
    new_seq->SetGenerationCount(generation_count_);
    return new_seq;
}

template <class T>
template <class T2>
T2 LazySequence<T>::Reduce(std::function<T2(T2, T)> reducer, T2 initial, size_t max_elements) const {
    T2 result = initial;
    size_t limit = is_infinite_ ? max_elements : std::min(max_elements, generation_count_);
    
    for (size_t i = 0; i < limit; ++i) {
        EnsureMaterialized((int)i);
        result = reducer(result, *memoized_elements_[i]);
    }
    return result;
}

template <class T> T LazySequence<T>::Reduce(std::function<T(T, T)> reducer, T initial, size_t max_elements) const {
    return Reduce<T>(reducer, initial, max_elements);
}

template <class T> std::shared_ptr<LazySequence<T>> LazySequence<T>::Where(bool (*predicate)(T)) {
    auto new_seq = std::make_shared<LazySequence<T>>();
    new_seq->is_infinite_ = is_infinite_;
    new_seq->base_sequence_ = std::make_shared<LazySequence<T>>(*this);
    new_seq->where_predicate_ = predicate;
    new_seq->SetGenerationCount(generation_count_);  // Может быть меньше после фильтрации
    return new_seq;
}

template <class T> std::shared_ptr<LazySequence<T>> LazySequence<T>::Where(std::function<bool(T)> predicate) {
    auto new_seq = std::make_shared<LazySequence<T>>();
    new_seq->is_infinite_ = is_infinite_;
    new_seq->base_sequence_ = std::make_shared<LazySequence<T>>(*this);
    new_seq->where_predicate_ = predicate;
    new_seq->SetGenerationCount(generation_count_);
    return new_seq;
}

template <class T>
template <class T2>
std::shared_ptr<LazySequence<std::pair<T, T2>>> LazySequence<T>::Zip(Sequence<T2>* seq) {
    if (!seq) {
        throw std::invalid_argument("Sequence cannot be null");
    }
    auto base_copy = std::make_shared<LazySequence<T>>(*this);
    size_t seq_length = seq->GetLength();
    
    std::function<std::pair<T, T2>(size_t)> zip_gen = [base_copy, seq](size_t index) -> std::pair<T, T2> {
        T first = base_copy->Get((int)index);
        if (index >= seq->GetLength()) {
            throw std::out_of_range("Second sequence is shorter than first");
        }
        T2 second = seq->Get((int)index);
        return std::make_pair(first, second);
    };
    
    // Результирующая последовательность конечна, если обе конечны
    bool result_infinite = is_infinite_;
    size_t result_count = std::min(generation_count_, seq_length);
    
    auto new_seq = std::make_shared<LazySequence<std::pair<T, T2>>>(zip_gen, result_infinite);
    new_seq->SetGenerationCount(result_count);
    return new_seq;
}

template <class T>
template <class T2>
std::shared_ptr<LazySequence<std::pair<T, T2>>> LazySequence<T>::Zip(std::shared_ptr<Sequence<T2>> seq) {
    return Zip(seq.get());
}

template <class T>
template <class T2>
std::shared_ptr<LazySequence<std::pair<T, T2>>> LazySequence<T>::Zip(std::shared_ptr<LazySequence<T2>> seq) {
    auto base_copy = std::make_shared<LazySequence<T>>(*this);
    auto seq_copy = std::make_shared<LazySequence<T2>>(*seq);
    
    std::function<std::pair<T, T2>(size_t)> zip_gen = [base_copy, seq_copy](size_t index) -> std::pair<T, T2> {
        T first = base_copy->Get((int)index);
        T2 second = seq_copy->Get((int)index);
        return std::make_pair(first, second);
    };
    
    // Результирующая последовательность бесконечна, если хотя бы одна бесконечна
    bool result_infinite = is_infinite_ || seq->IsInfinite();
    size_t result_count = 0;
    if (!is_infinite_ && !seq->IsInfinite()) {
        Cardinal len1 = GetLength();
        Cardinal len2 = seq->GetLength();
        if (len1.IsFinite() && len2.IsFinite()) {
            result_count = std::min(len1.GetFiniteValue(), len2.GetFiniteValue());
        }
    }
    
    auto new_seq = std::make_shared<LazySequence<std::pair<T, T2>>>(zip_gen, result_infinite);
    new_seq->SetGenerationCount(result_count);
    return new_seq;
}

//////////////////////////////////////////////////////////////////////

template <class T> void LazySequence<T>::EnsureMaterialized(int index) const {
    while ((int)memoized_elements_.size() <= index &&
        (is_infinite_ || (generation_count_ > 0 && memoized_elements_.size() < generation_count_))) {
        GenerateNext();
    }
    // Убеждаемся что элемент вычислен
    if (index >= 0 && index < (int)memoized_elements_.size() && memoized_elements_[index] == nullptr) {
        GenerateNext(); // Генерируем если еще не вычислен
    }
}

template <class T> T LazySequence<T>::GenerateNext() const {
    size_t current_size = memoized_elements_.size();
    
    // Если уже есть значение для этого индекса, возвращаем его
    if (current_size > 0 && memoized_elements_[current_size - 1] != nullptr) {
        // Проверяем, нужно ли генерировать следующий
        if (current_size <= (size_t)generation_count_ || is_infinite_) {
            // Продолжаем генерацию
        } else {
            throw std::logic_error("Cannot generate more elements");
        }
    }
    
    T next_element;
    
    // Используем index_generator_ если доступен (проще и эффективнее)
    if (index_generator_) {
        next_element = index_generator_(current_size);
    }
    // Иначе используем рекуррентное правило
    else if (generator_) {
        // Создаем Sequence из уже материализованных элементов
        int materializedCount = 0;
        for (const auto& ptr : memoized_elements_) {
            if (ptr != nullptr) {
                materializedCount++;
            }
        }
        T* current_elements = new T[materializedCount];
        int idx = 0;
        for (const auto& ptr : memoized_elements_) {
            if (ptr != nullptr) {
                current_elements[idx++] = *ptr;
            }
        }
        Sequence<T> current_seq(current_elements, materializedCount);
        next_element = generator_(current_seq);
        delete[] current_elements;
    }
    else {
        if (current_size < generation_count_) {
            // Для конечных последовательностей без генератора это не должно происходить
            throw std::logic_error("No generator and cannot generate more elements");
        }
        throw std::logic_error("Cannot generate more elements: no generator available");
    }
    
    memoized_elements_.push_back(new T(next_element));
    return next_element;
}

#endif // LAZY_SEQUENCE_H
