#ifndef LAZYSEQUENCE_H
#define LAZYSEQUENCE_H

// #include <deque>
#include <algorithm>
#include "all_headers.h"

// Типы для работы с кардинальными числами
enum class Cardinal {
    FINITE,
    INFINITE
};

template<typename T>
class LazySequence : public Sequence<T>, public IEnumerable<T> {
private:
    // Внутренний генератор
    std::function<T(size_t)> generator_function;

    // Вспомогательный генератор элементов объявим ниже, после определения Operation

    // Рекуррентное правило (если задано)
    std::function<T(const std::vector<T>&, size_t)> recurrent_function;

    // Мемоизация вычисленных элементов (массив материализованных значений по индексу)
    // Используем optional, чтобы отличать «ещё не вычислено» от «вычислено и сохранено».
    mutable std::vector<std::optional<T>> memoized_elements;

    // Массив блоков, добавленных через операции concat (конечные куски последовательности)
    std::vector<std::shared_ptr<std::vector<T>>> concat_blocks;

    // Список операций (для append, prepend, insert, remove, concat)
    struct Operation {
        enum Type { APPEND, PREPEND, INSERT, REMOVE, CONCAT } type;
        size_t position;
        // Порядковый номер добавления операции в очередь (для стабильного порядка)
        size_t order;
        T value;
        std::shared_ptr<LazySequence<T>> sequence;

        Operation(Type t, size_t pos = 0, T val = T{})
            : type(t), position(pos), order(0), value(val) {}
        Operation(Type t, size_t pos, std::shared_ptr<LazySequence<T>> seq)
            : type(t), position(pos), order(0), sequence(std::move(seq)) {}
    };

    // Компаратор для очереди с приоритетом операций:
    // операции с меньшим индексом позиции имеют более высокий приоритет;
    // при равной позиции приоритетнее та, что была добавлена раньше (меньший order)
    struct OperationComparator {
        bool operator()(const Operation& a, const Operation& b) const {
            if (a.position != b.position)
                return a.position > b.position;
            return a.order > b.order;
        }
    };
    // исопльзуется только в operations_queue, как отдельный обьект он не нужен
    mutable std::vector<Operation> operations;
    // Очередь с приоритетом операций (append/prepend/insert/remove/concat)
    mutable std::priority_queue<Operation, std::vector<Operation>, OperationComparator> operations_queue;
    // Локальный счётчик для задания стабильного порядка операций
    mutable size_t next_operation_order = 0;

    // Начальные элементы для рекуррентного правила
    std::vector<T> initial_elements;

    // Флаги состояния
    // bool is_infinite;
    Cardinal cardinal;
    mutable size_t materialized_count;
    int known_length; // Для конечных последовательностей (если известно), иначе -1

    // Материализация элемента по индексу:
    // единственный допустимый путь вычисления элемента —
    // через generator_function с мемоизацией в memoized_elements.
    T materializeElement(size_t index) const;

    // Гарантировать, что memoized_elements имеет размер не меньше index+1
    void ensureMemoCapacity(size_t index) const {
        if (memoized_elements.size() <= index) {
            memoized_elements.resize(index + 1);
        }
    }

    // Вспомогательный генератор элементов по спецификации (внутренний объект)
    class Generator {
    private:
        LazySequence<T>* host;

        bool uses_external_rule = false;
        std::function<T(Sequence<T>*)> external_rule;

        size_t next_index = 0;

        bool has_operation = false;
        Operation operation;

        size_t op_index = 0;

        // когда встраиваем другую последовательность в середину (INSERT/CONCAT).
        bool injection_phase = false;
        Sequence<T>* injected_sequence = nullptr;
        size_t injected_seq_pos = 0;
        bool injected_single_used = false;
        // сколько элементов нужно пропустить при REMOVE.
        size_t removed_remaining = 0;

        bool append_phase = false;
        Sequence<T>* appended_sequence = nullptr;
        size_t appended_seq_pos = 0;
        bool appended_single_used = false;
        T appended_single_value{};

    public:
        explicit Generator(LazySequence<T>* owner,
                           std::function<T(Sequence<T>*)> rule)
            : host(owner),
              uses_external_rule(true),
              external_rule(std::move(rule)) {}

        Generator(LazySequence<T>* owner,
                  size_t index,
                  Operation op,
                  Sequence<T>* aux_sequence = nullptr)
            : host(owner),
              uses_external_rule(false),
              next_index(0),
              has_operation(true),
              operation(std::move(op)),
              op_index(index),
              injected_sequence(aux_sequence) {
            if (operation.type == Operation::REMOVE) {
                if (injected_sequence) {
                    removed_remaining = static_cast<size_t>(
                        injected_sequence->GetLength() < 0 ? 0 : injected_sequence->GetLength()
                    );
                } else {
                    removed_remaining = 1;
                }
            }
        }

        bool HasNext() const {
            if (uses_external_rule) {
                return true;
            }
            int base_len = host->GetLength();
            if (base_len < 0) {
                return true;
            }
            long long effective = base_len;
            if (has_operation) {
                switch (operation.type) {
                    case Operation::INSERT:
                    case Operation::APPEND:
                        effective += 1;
                        break;
                    case Operation::REMOVE:
                        effective -= static_cast<long long>(std::max<size_t>(1, removed_remaining));
                        break;
                    case Operation::PREPEND:
                    case Operation::CONCAT:
                        break;
                }
            }
            return next_index < static_cast<size_t>(std::max<long long>(0, effective));
        }

        std::unique_ptr<T> TryGetNext() {
            if (!HasNext()) return nullptr;
            return std::make_unique<T>(GetNext());
        }

        T GetNext() {
            if (uses_external_rule) {
                return external_rule(static_cast<Sequence<T>*>(host));
            }
            if (append_phase) {
                if (appended_sequence) {
                    T v = appended_sequence->Get(static_cast<int>(appended_seq_pos++));
                    return v;
                }
                if (!appended_single_used) {
                    appended_single_used = true;
                    return appended_single_value;
                }
                throw IndexOutOfRange("Generator reached end after append");
            }
            if (has_operation &&
                (operation.type == Operation::INSERT) &&
                injection_phase) {
                if (injected_sequence) {
                    T v = injected_sequence->Get(static_cast<int>(injected_seq_pos++));
                    if (injected_sequence->GetLength() >= 0 &&
                        injected_seq_pos >= static_cast<size_t>(injected_sequence->GetLength())) {
                        injection_phase = false;
                    }
                    return v;
                } else {
                    if (!injected_single_used) {
                        injected_single_used = true;
                        injection_phase = false;
                        return operation.value;
                    }
                    injection_phase = false;
                }
            }
            if (has_operation && next_index == op_index) {
                switch (operation.type) {
                    case Operation::INSERT: {
                        injection_phase = true;
                        if (injected_sequence) {
                            T v = injected_sequence->Get(static_cast<int>(injected_seq_pos++));
                            if (injected_sequence->GetLength() >= 0 &&
                                injected_seq_pos >= static_cast<size_t>(injected_sequence->GetLength())) {
                                injection_phase = false;
                            }
                            return v;
                        } else {
                            injected_single_used = true;
                            injection_phase = false;
                            return operation.value;
                        }
                    }
                    case Operation::REMOVE: {
                        size_t to_skip = std::max<size_t>(1, removed_remaining);
                        next_index += to_skip;
                        T v = host->materializeElement(next_index++);
                        return v;
                    }
                    case Operation::APPEND: {
                        int base_len = host->GetLength();
                        if (base_len >= 0) {
                            if (next_index < static_cast<size_t>(base_len)) {
                                T v = host->materializeElement(next_index++);
                                return v;
                            }
                            append_phase = true;
                            if (operation.sequence) {
                                appended_sequence = operation.sequence.get();
                            } else {
                                appended_single_value = operation.value;
                            }
                            if (appended_sequence) {
                                return appended_sequence->Get(static_cast<int>(appended_seq_pos++));
                            }
                            appended_single_used = true;
                            return appended_single_value;
                        } else {
                            T v = host->materializeElement(next_index++);
                            return v;
                        }
                    }
                    case Operation::PREPEND:
                    case Operation::CONCAT:
                        break;
                }
            }
            T v = host->materializeElement(next_index++);
            return v;
        }

        Generator* Append(T item) const {
            Operation op(Operation::APPEND, 0, item);
            return new Generator(host, static_cast<size_t>(host->GetLength() >= 0 ? host->GetLength() : next_index), op);
        }
        Generator* Append(Sequence<T>* items) const {
            Operation op(Operation::APPEND, 0, std::shared_ptr<LazySequence<T>>(nullptr));
            return new Generator(host,
                                 static_cast<size_t>(host->GetLength() >= 0 ? host->GetLength() : next_index),
                                 op,
                                 items);
        }
        Generator* Insert(T item) const {
            Operation op(Operation::INSERT, op_index, item);
            return new Generator(host, op_index, op);
        }
        Generator* Insert(Sequence<T>* items) const {
            Operation op(Operation::INSERT, op_index, std::shared_ptr<LazySequence<T>>(nullptr));
            return new Generator(host, op_index, op, items);
        }
        Generator* Remove(T /*item*/) const {
            Operation op(Operation::REMOVE, op_index, T{});
            return new Generator(host, op_index, op);
        }
        Generator* Remove(Sequence<T>* items) const {
            Operation op(Operation::REMOVE, op_index, std::shared_ptr<LazySequence<T>>(nullptr));
            return new Generator(host, op_index, op, items);
        }
    };

public:
    // Конструкторы
    LazySequence();
    LazySequence(T* items, int count);
    explicit LazySequence(Sequence<T>* seq);
    explicit LazySequence(std::function<T(size_t)> func, bool infinite = true);
    LazySequence(std::function<T(const std::vector<T>&, size_t)> recurrent_func,
                 const std::vector<T>& initial);
    LazySequence(const LazySequence<T>& other);

    // Деструктор
    ~LazySequence() override = default;

    // === Реализация интерфейса Sequence<T> ===
    T GetFirst() const override;
    T GetLast() const override;                  // для бесконечных — исключение
    T Get(int index) const override;             // адаптация size_t -> int
    void Set(int index, T value) override;       // запись в кэш/проверка границ
    Sequence<T>* GetSubsequence(int startIndex, int endIndex) override;
    int GetLength() const override;              // для бесконечных: -1
    Sequence<T>* Append(T item) override;        // ковариантный возврат LazySequence<T>*
    Sequence<T>* Prepend(T item) override;       // ковариантный возврат LazySequence<T>*
    Sequence<T>* InsertAt(T item, int index) override; // ковариантный возврат LazySequence<T>*
    Sequence<T>* Concat(Sequence<T>* list) override;   // ковариантный возврат LazySequence<T>*
    T operator[](int index) const override { return Get(index); }

    // === Указатель-ориентированные функции интерфейса Sequence ===
    // Оборачиваются во внутренний std::function
    Sequence<T>* Map(T (*func)(T&)) override;
    Sequence<T>* Map(T (*func)(T&, int)) override;
    Sequence<T>* where(bool (*predicate)(T&)) override;
    bool IsInfinite() const;
    size_t GetMaterializedCount() const;
    // Явная материализация первых n элементов (обеспечивает O(1) доступ к префиксу)
    void MaterializePrefix(size_t n);

    // ВАЖНО: Reduce в базовом классе теперь имеет безопасную дефолтную реализацию (throw),
    // а удобные варианты остаются как дополнительные (не интерфейс Sequence)

    // === Удобные дополнения, сохранены для совместимости с текущими тестами/демо ===
    template<typename T2>
    LazySequence<T2>* Map(std::function<T2(T)> func) const;

    LazySequence<T>* Where(std::function<bool(T)> predicate) const;

    template<typename T2>
    T2 Reduce(std::function<T2(T2, T)> func,
              T2 initial_value,
              size_t max_elements = SIZE_MAX) const;

    // Итератор/перечислитель
    class Enumerator : public IEnumerator<T> {
    private:
        const LazySequence<T>& sequence;
        size_t current_index;
        bool started;
        bool ended;

    public:
        explicit Enumerator(const LazySequence<T>& seq)
            : sequence(seq), current_index(0), started(false), ended(false) {}

        ~Enumerator() override = default;

        void Reset() override {
            current_index = 0;
            started = false;
            ended = false;
        }

        bool MoveNext() override {
            if (ended) {
                return false;
            }

            int length = sequence.GetLength();

            if (!started) {
                started = true;
                if (length == 0) {
                    ended = true;
                    return false;
                }
                current_index = 0;
                return true;
            }

            ++current_index;

            if (length >= 0 && current_index >= static_cast<size_t>(length)) {
                ended = true;
                return false;
            }

            return true;
        }

        T Current() override {
            if (!started || ended) {
                throw std::logic_error("LazySequence::Enumerator::Current called on invalid state");
            }
            return sequence.Get(static_cast<int>(current_index));
        }
    };

    IEnumerator<T>* GetEnumerator() override { return new Enumerator(*this); }
};

template<typename T>
LazySequence<T>::LazySequence()
    : cardinal(Cardinal::FINITE),
      materialized_count(0),
      known_length(0),
      next_operation_order(0) {
}

template<typename T>
LazySequence<T>::LazySequence(T* items, int count)
    : cardinal(Cardinal::FINITE),
      materialized_count(count),
      known_length(count),
      next_operation_order(0) {
    if (count > 0) {
        memoized_elements.resize(static_cast<size_t>(count));
        for (size_t i = 0; i < static_cast<size_t>(count); ++i) {
            memoized_elements[i] = items[i];
        }
    }
}

template<typename T>
LazySequence<T>::LazySequence(Sequence<T>* seq)
    : cardinal(Cardinal::FINITE),
      materialized_count(0),
      known_length(0),
      next_operation_order(0) {
    if (!seq) {
        known_length = 0;
        return;
    }
    int len = seq->GetLength();
    if (len < 0) {
        throw std::invalid_argument("LazySequence(Sequence<T>*): infinite base Sequence is not supported in this implementation");
    }
    known_length = len;
    if (len > 0) {
        memoized_elements.resize(static_cast<size_t>(len));
        for (int i = 0; i < len; ++i) {
            T v = seq->Get(i);
            memoized_elements[static_cast<size_t>(i)] = v;
            ++materialized_count;
        }
    }
}

template<typename T>
LazySequence<T>::LazySequence(std::function<T(size_t)> func, bool infinite)
    : generator_function(std::move(func)),
      cardinal(infinite ? Cardinal::INFINITE : Cardinal::FINITE),
      materialized_count(0),
      known_length(-1),
      next_operation_order(0) {
}

template<typename T>
LazySequence<T>::LazySequence(std::function<T(const std::vector<T>&, size_t)> recurrent_func,
                              const std::vector<T>& initial)
    : initial_elements(initial),
      cardinal(Cardinal::INFINITE),
      materialized_count(0),
      known_length(-1),
      next_operation_order(0) {
    // Мемоизируем начальные элементы
    if (!initial.empty()) {
        memoized_elements.resize(initial.size());
        for (size_t i = 0; i < initial.size(); ++i) {
            memoized_elements[i] = initial[i];
        }
        materialized_count = initial.size();
    }

    // Генератор, который предоставляет рекуррентной функции все предыдущие элементы [0..index-1]
    generator_function = [this, recurrent_func](size_t index) -> T {
        if (index < this->initial_elements.size()) {
            return this->initial_elements[index];
        }
        std::vector<T> previous_elements(index);
        for (size_t j = 0; j < index; ++j) {
            if (j < this->memoized_elements.size() && this->memoized_elements[j].has_value()) {
                previous_elements[j] = *(this->memoized_elements[j]);
            } else {
                previous_elements[j] = this->materializeElement(j);
            }
        }
        return recurrent_func(previous_elements, index);
    };
}

template<typename T>
LazySequence<T>::LazySequence(const LazySequence<T>& other)
    : generator_function(other.generator_function),
      recurrent_function(other.recurrent_function),
      memoized_elements(other.memoized_elements),
      concat_blocks(other.concat_blocks),
      operations(other.operations),
      operations_queue(other.operations_queue),
      initial_elements(other.initial_elements),
      cardinal(other.cardinal),
      materialized_count(other.materialized_count),
      known_length(other.known_length),
      next_operation_order(other.next_operation_order) {
}

template<typename T>
T LazySequence<T>::materializeElement(size_t index) const {
    // 1) сначала пробуем взять из массива мемоизации
    if (index < memoized_elements.size()) {
        const auto& slot = memoized_elements[index];
        if (slot.has_value()) {
            return *slot;
        }
    }

    // 2) вычисляем только через generator_function
    if (!generator_function) {
        throw IndexOutOfRange("materializeElement: generator is not defined for index " + std::to_string(index));
    }

    T result = generator_function(index);

    // 3) сохраняем в memoized_elements
    ensureMemoCapacity(index);
    if (!memoized_elements[index].has_value()) {
        materialized_count++;
    }
    memoized_elements[index] = result;
    return result;
}

// === Реализация интерфейса Sequence<T> ===
template<typename T>
T LazySequence<T>::GetFirst() const {
    return Get(0);
}

template<typename T>
T LazySequence<T>::GetLast() const {
    if (cardinal == Cardinal::INFINITE || known_length < 0) {
        throw std::runtime_error("Cannot get last element of infinite sequence");
    }
    if (known_length == 0) {
        throw IndexOutOfRange("Sequence is empty");
    }
    return Get(known_length - 1);
}

template<typename T>
T LazySequence<T>::Get(int index) const {
    if (index < 0) throw IndexOutOfRange("Negative index");
    if (cardinal == Cardinal::FINITE && known_length >= 0 && index >= known_length) {
        throw IndexOutOfRange("Index " + std::to_string(index) + " is out of range");
    }
    return materializeElement(static_cast<size_t>(index));
}

template<typename T>
void LazySequence<T>::Set(int index, T value) {
    if (index < 0) throw IndexOutOfRange("Negative index");
    if (cardinal == Cardinal::FINITE && known_length >= 0 && index >= known_length) {
        throw IndexOutOfRange("Set out of range");
    }
    size_t idx = static_cast<size_t>(index);
    ensureMemoCapacity(idx);
    bool was_present = memoized_elements[idx].has_value();
    memoized_elements[idx] = value;
    if (!was_present) {
        ++materialized_count;
    }
}

template<typename T>
Sequence<T>* LazySequence<T>::GetSubsequence(int startIndex, int endIndex) {
    if (startIndex < 0 || endIndex < 0 || startIndex > endIndex) {
        throw std::invalid_argument("Invalid subsequence bounds");
    }
    if (cardinal == Cardinal::FINITE && known_length >= 0 && endIndex >= known_length) {
        throw IndexOutOfRange("End index is out of range");
    }

    auto* result = new LazySequence<T>();
    result->known_length = endIndex - startIndex + 1;
    // Копируем (или материализуем) значения из родительской последовательности
    size_t len = static_cast<size_t>(result->known_length);
    if (len > 0) {
        result->memoized_elements.resize(len);
        for (size_t j = 0; j < len; ++j) {
            size_t srcIndex = static_cast<size_t>(startIndex) + j;
            T val = this->materializeElement(srcIndex);
            result->memoized_elements[j] = val;
            ++result->materialized_count;
        }
    }
    return static_cast<Sequence<T>*>(result);
}

template<typename T>
int LazySequence<T>::GetLength() const {
    return cardinal == Cardinal::INFINITE ? -1 : std::max(0, known_length);
}

template<typename T>
Sequence<T>* LazySequence<T>::Append(T item) {
    // Если последовательность бесконечная, по условию ничего не добавляем —
    // возвращаем копию текущего объекта без изменений.
    if (IsInfinite()) {
        return static_cast<Sequence<T>*>(new LazySequence<T>(*this));
    }

    int len = GetLength(); // для конечной последовательности len >= 0
    if (len < 0) {
        // На всякий случай, но до сюда доходить не должны.
        return static_cast<Sequence<T>*>(new LazySequence<T>(*this));
    }

    // Материализуем все элементы текущей конечной последовательности
    std::vector<T> buffer(static_cast<size_t>(len + 1));
    for (int i = 0; i < len; ++i) {
        buffer[static_cast<size_t>(i)] = Get(i);
    }
    buffer[static_cast<size_t>(len)] = item;

    // Строим новую LazySequence только из массива элементов
    auto* result = new LazySequence<T>(buffer.data(), len + 1);
    return static_cast<Sequence<T>*>(result);
}

template<typename T>
Sequence<T>* LazySequence<T>::Prepend(T item) {
    // Для бесконечной последовательности по условию не изменяем структуру —
    // возвращаем копию текущей последовательности.
    if (IsInfinite()) {
        return static_cast<Sequence<T>*>(new LazySequence<T>(*this));
    }

    int len = GetLength(); // для конечной последовательности len >= 0
    if (len < 0) {
        return static_cast<Sequence<T>*>(new LazySequence<T>(*this));
    }

    // Материализуем все элементы текущей конечной последовательности
    std::vector<T> buffer(static_cast<size_t>(len + 1));
    buffer[0] = item;
    for (int i = 0; i < len; ++i) {
        buffer[static_cast<size_t>(i + 1)] = Get(i);
    }

    // Строим новую LazySequence только из массива элементов
    auto* result = new LazySequence<T>(buffer.data(), len + 1);
    return static_cast<Sequence<T>*>(result);
}

template<typename T>
Sequence<T>* LazySequence<T>::InsertAt(T item, int index) {
    if (index < 0) throw IndexOutOfRange("Negative index");
    if (cardinal == Cardinal::FINITE && known_length >= 0 && index > known_length) {
        throw IndexOutOfRange("Insert out of range");
    }
    // Для бесконечной последовательности по условию не изменяем структуру —
    // возвращаем копию текущей последовательности.
    if (IsInfinite()) {
        return static_cast<Sequence<T>*>(new LazySequence<T>(*this));
    }

    int len = GetLength(); // для конечной последовательности len >= 0
    if (len < 0) {
        return static_cast<Sequence<T>*>(new LazySequence<T>(*this));
    }

    if (index > len) {
        throw IndexOutOfRange("Insert out of range");
    }

    // Материализуем все элементы текущей конечной последовательности
    std::vector<T> buffer(static_cast<size_t>(len + 1));

    // Элементы до позиции вставки
    for (int i = 0; i < index; ++i) {
        buffer[static_cast<size_t>(i)] = Get(i);
    }

    // Вставляем новый элемент
    buffer[static_cast<size_t>(index)] = item;

    // Элементы после позиции вставки сдвигаем на один вправо
    for (int i = index; i < len; ++i) {
        buffer[static_cast<size_t>(i + 1)] = Get(i);
    }

    // Строим новую LazySequence только из массива элементов
    auto* result = new LazySequence<T>(buffer.data(), len + 1);
    return static_cast<Sequence<T>*>(result);
}

template<typename T>
Sequence<T>* LazySequence<T>::Concat(Sequence<T>* list) {
    // Если текущая последовательность бесконечная, по требованиям
    // не пытаемся реально конкатенировать, а возвращаем её копию.
    auto* result = new LazySequence<T>(*this);
    if (IsInfinite()) {
        return static_cast<Sequence<T>*>(result);
    }

    // Поддерживаем конкат только с LazySequence для эффективности,
    // иначе — материализуем правую часть по мере запроса
    auto* otherLazy = dynamic_cast<LazySequence<T>*>(list);
    result->operations.emplace_back(
        Operation::CONCAT,
        0,
        std::shared_ptr<LazySequence<T>>(otherLazy ? new LazySequence<T>(*otherLazy) : nullptr)
    );
    {
        Operation& op = result->operations.back();
        op.order = result->next_operation_order++;
        result->operations_queue.push(op);
    }

    // Если правая последовательность конечна, материализуем её как отдельный блок
    int rightLen = otherLazy ? otherLazy->GetLength() : list->GetLength();
    if (rightLen >= 0 && rightLen > 0) {
        auto block = std::make_shared<std::vector<T>>(static_cast<size_t>(rightLen));
        for (int i = 0; i < rightLen; ++i) {
            (*block)[static_cast<size_t>(i)] = list->Get(i);
        }
        result->concat_blocks.push_back(block);
    }

    if (result->cardinal == Cardinal::FINITE && result->known_length >= 0) {
        if (rightLen < 0) result->known_length = -1;
        else result->known_length += rightLen;
    } else {
        result->known_length = -1;
        result->cardinal = Cardinal::INFINITE;
    }

    result->generator_function = [left = *result, right = list](size_t i) mutable -> T {
        int leftLen = left.GetLength();
        if (leftLen >= 0 && static_cast<int>(i) < leftLen) return left.materializeElement(i);
        size_t j = (leftLen < 0) ? 0 : (i - static_cast<size_t>(leftLen));
        return right->Get(static_cast<int>(j));
    };
    return static_cast<Sequence<T>*>(result);
}

template <typename T>
bool LazySequence<T>::IsInfinite() const {
    return cardinal == Cardinal::INFINITE;
}

template <typename T>
size_t LazySequence<T>::GetMaterializedCount() const {
    return materialized_count;
}

template <typename T>
void LazySequence<T>::MaterializePrefix(size_t n) {
    if (n == 0) return;

    // Ограничиваем n длиной конечной последовательности, если она известна
    if (cardinal == Cardinal::FINITE && known_length >= 0 &&
        static_cast<int>(n) > known_length) {
        n = static_cast<size_t>(known_length);
    }

    for (size_t i = 0; i < n; ++i) {
        (void)materializeElement(i);
    }
}


// === Map/where указатель-ориентированные (интерфейс Sequence) ===
template<typename T>
Sequence<T>* LazySequence<T>::Map(T (*func)(T&)) {
    // Нельзя захватывать `this`, т.к. вызывающий код может удалить
    // исходную последовательность сразу после Map (см. интерактивный режим).
    LazySequence<T> base_copy(*this);

    auto wrapper = [base_copy, func](size_t i) -> T {
        T value = base_copy.Get(static_cast<int>(i));
        return func(value);
    };

    // Длину результирующей последовательности в общем случае определить
    // заранее сложно (особенно при дальнейших ленивых операциях),
    // поэтому помечаем результат как «бесконечный» (GetLength() = -1),
    // а реальные границы контролируются через исключения в Get().
    return static_cast<Sequence<T>*>(
        new LazySequence<T>(std::move(wrapper), true)
    );
}

template<typename T>
Sequence<T>* LazySequence<T>::Map(T (*func)(T&, int)) {
    LazySequence<T> base_copy(*this);

    auto wrapper = [base_copy, func](size_t i) -> T {
        T value = base_copy.Get(static_cast<int>(i));
        return func(value, static_cast<int>(i));
    };

    return static_cast<Sequence<T>*>(
        new LazySequence<T>(std::move(wrapper), true)
    );
}

template<typename T>
Sequence<T>* LazySequence<T>::where(bool (*predicate)(T&)) {
    // Нельзя захватывать `this`, т.к. во внешнем коде базовая
    // последовательность может быть удалена после вызова where(...)
    LazySequence<T> base_copy(*this);

    // Если базовая последовательность конечна, безопаснее и проще
    // СРАЗУ материализовать отфильтрованный результат в отдельный массив.
    int base_len = base_copy.GetLength();
    if (!base_copy.IsInfinite() && base_len >= 0) {
        std::vector<T> filtered;
        filtered.reserve(static_cast<size_t>(base_len));

        for (int i = 0; i < base_len; ++i) {
            T v = base_copy.Get(i);
            if (predicate(v)) {
                filtered.push_back(v);
            }
        }

        // Строим новую конечную LazySequence только из отфильтрованных элементов
        if (filtered.empty()) {
            return static_cast<Sequence<T>*>(
                new LazySequence<T>(nullptr, 0)
            );
        }
        return static_cast<Sequence<T>*>(
            new LazySequence<T>(filtered.data(), static_cast<int>(filtered.size()))
        );
    }

    // Для бесконечной последовательности оставляем ленивый вариант:
    auto gen = [base_copy, predicate](size_t k) -> T {
        size_t current = 0, found = 0;
        while (true) {
            // Для бесконечной последовательности length < 0, условие не сработает
            int length = base_copy.GetLength();
            if (length >= 0 && static_cast<int>(current) >= length) {
                throw IndexOutOfRange("Not enough elements matching predicate");
            }

            T tmp = base_copy.Get(static_cast<int>(current));
            if (predicate(tmp)) {
                if (found == k) {
                    return tmp;
                }
                ++found;
            }
            ++current;
        }
    };

    return static_cast<Sequence<T>*>(
        new LazySequence<T>(std::move(gen), true /* бесконечная (длина заранее неизвестна) */)
    );
}

// === Удобные дополнения (как в исходной версии) ===
template<typename T>
template<typename T2>
LazySequence<T2>* LazySequence<T>::Map(std::function<T2(T)> func) const {
    auto wrapper = [this, func](size_t i) -> T2 { return func(this->Get(static_cast<int>(i))); };
    // Аналогично указательным Map — помечаем как «бесконечную».
    return new LazySequence<T2>(std::move(wrapper), true);
}

template<typename T>
LazySequence<T>* LazySequence<T>::Where(std::function<bool(T)> predicate) const {
    auto gen = [this, predicate](size_t k) -> T {
        size_t current = 0, found = 0;
        while (true) {
            if (cardinal == Cardinal::FINITE && known_length >= 0 &&
                static_cast<int>(current) >= known_length) {
                throw IndexOutOfRange("Not enough elements matching predicate");
            }
            T val = this->Get(static_cast<int>(current));
            if (predicate(val)) {
                if (found == k) return val;
                ++found;
            }
            ++current;
        }
    };
    return new LazySequence<T>(std::move(gen), true);
}

template<typename T>
template<typename T2>
T2 LazySequence<T>::Reduce(std::function<T2(T2, T)> func,
                           T2 initial_value,
                           size_t max_elements) const {
    T2 acc = initial_value;
    size_t limit = (cardinal == Cardinal::INFINITE || known_length < 0) ? max_elements
                                                     : std::min(max_elements, static_cast<size_t>(known_length));
    for (size_t i = 0; i < limit; ++i) acc = func(acc, Get(static_cast<int>(i)));
    return acc;
}


#endif // LAZYSEQUENCE_H
