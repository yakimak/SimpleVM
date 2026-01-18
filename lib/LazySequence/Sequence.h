#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <stdexcept>
#include <memory>
#include "DynamicArray.h"

template <class T>
class Sequence {
protected:
    DynamicArray<T>* elements_;

public:
    Sequence();
    Sequence(T* items, int count);
    Sequence(const DynamicArray<T>& items);
    Sequence(const Sequence<T>& other);
    template<typename Container>
    Sequence(const Container& container);

    virtual ~Sequence();

    T GetFirst() const;
    T GetLast() const;
    T Get(int index) const;
    size_t GetLength() const;

    std::shared_ptr<Sequence<T>> GetSubsequence(int startIndex, int endIndex) const;

    Sequence<T>* Append(T item);
    Sequence<T>* Prepend(T item);
};

//////////////////////////////////////////////////////////////////////

template <class T> Sequence<T>::Sequence() : elements_(new DynamicArray<T>(0)) {}

template <class T> Sequence<T>::Sequence(T* items, int count) : elements_(new DynamicArray<T>(items, count)) {}

template <class T> Sequence<T>::Sequence(const DynamicArray<T>& items) : elements_(new DynamicArray<T>(items)) {}

template <class T> Sequence<T>::Sequence(const Sequence<T>& other) : elements_(new DynamicArray<T>(*other.elements_)) {}

template <class T>
template<typename Container>
Sequence<T>::Sequence(const Container& container) {
    int count = (int)container.size();
    T* items = new T[count];
    int i = 0;
    for (const auto& item : container) {
        items[i++] = item;
    }
    elements_ = new DynamicArray<T>(items, count);
    delete[] items;
}

template <class T> Sequence<T>::~Sequence() {
    delete elements_;
}

//////////////////////////////////////////////////////////////////////

template <class T> T Sequence<T>::GetFirst() const {
    if (elements_->GetSize() == 0) throw std::out_of_range("Sequence is empty");
    return elements_->Get(0);
}

template <class T> T Sequence<T>::GetLast() const {
    if (elements_->GetSize() == 0) throw std::out_of_range("Sequence is empty");
    return elements_->Get(elements_->GetSize() - 1);
}

template <class T> T Sequence<T>::Get(int index) const {
    return elements_->Get(index);
}

template <class T> size_t Sequence<T>::GetLength() const {
    return elements_->GetSize();
}

//////////////////////////////////////////////////////////////////////

template <class T> std::shared_ptr<Sequence<T>> Sequence<T>::GetSubsequence(int startIndex, int endIndex) const {
    if (startIndex < 0 || endIndex >= elements_->GetSize() || startIndex > endIndex) {
        throw std::out_of_range("Invalid index range");
    }
    
    int subSize = endIndex - startIndex + 1;
    T* subItems = new T[subSize];
    for (int i = 0; i < subSize; ++i) {
        subItems[i] = elements_->Get(startIndex + i);
    }
    
    auto result = std::make_shared<Sequence<T>>(subItems, subSize);
    delete[] subItems;
    return result;
}

template <class T> Sequence<T>* Sequence<T>::Append(T item) {
    elements_->Resize(elements_->GetSize() + 1);
    elements_->Set(elements_->GetSize() - 1, item);
    return this;
}

template <class T> Sequence<T>* Sequence<T>::Prepend(T item) {
    int oldSize = elements_->GetSize();
    elements_->Resize(oldSize + 1);
    for (int i = oldSize; i > 0; --i) {
        elements_->Set(i, elements_->Get(i - 1));
    }
    elements_->Set(0, item);
    return this;
}

#endif // SEQUENCE_H
