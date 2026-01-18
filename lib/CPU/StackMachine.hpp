#ifndef STACK_MACHINE_HPP
#define STACK_MACHINE_HPP

#include "LazySequence/Sequence.h"
#include "LazySequence/LazySequence.h"
#include "CPU/Command.hpp"
#include <stack>
#include <stdexcept>
#include <cstddef>

/**
 * Стековый процессор
 * (S, PC) -> execute(cmd) -> (S', PC')
 * где S — состояние стека, PC — счётчик команд
 */
class StackMachine {
private:
    std::stack<int> data_stack;
    size_t program_counter;
    LazySequence<Command>& program_stream;

public:
    StackMachine(LazySequence<Command>& program)
        : program_stream(program), program_counter(0) {}

    void executeNext() {
        // Новый LazySequence: команда берётся по индексу program_counter.
        // Для конечной программы — останавливаемся по длине.
        if (!program_stream.IsInfinite()) {
            Cardinal len = program_stream.GetLength();
            if (len.IsFinite() && program_counter >= len.GetFiniteValue()) return;
        }

        Command cmd = program_stream.Get((int)program_counter);
        switch(cmd.type) {
            case CommandType::PUSH:
                data_stack.push(cmd.operand);
                break;
            case CommandType::POP:
                if (!data_stack.empty()) data_stack.pop();
                break;
            case CommandType::ADD:
                if (data_stack.size() >= 2) {
                    int a = data_stack.top(); data_stack.pop();
                    int b = data_stack.top(); data_stack.pop();
                    data_stack.push(a + b);
                }
                break;
            case CommandType::SUB:
                if (data_stack.size() >= 2) {
                    int a = data_stack.top(); data_stack.pop();
                    int b = data_stack.top(); data_stack.pop();
                    data_stack.push(b - a);
                }
                break;
            case CommandType::MUL:
                if (data_stack.size() >= 2) {
                    int a = data_stack.top(); data_stack.pop();
                    int b = data_stack.top(); data_stack.pop();
                    data_stack.push(a * b);
                }
                break;
            case CommandType::DIV:
                if (data_stack.size() >= 2) {
                    int a = data_stack.top(); data_stack.pop();
                    int b = data_stack.top(); data_stack.pop();
                    if (a == 0) throw std::runtime_error("Division by zero");
                    data_stack.push(b / a);
                }
                break;
            case CommandType::DUP:
                if (!data_stack.empty()) {
                    data_stack.push(data_stack.top());
                }
                break;
            case CommandType::SWAP:
                if (data_stack.size() >= 2) {
                    int a = data_stack.top(); data_stack.pop();
                    int b = data_stack.top(); data_stack.pop();
                    data_stack.push(a);
                    data_stack.push(b);
                }
                break;
            case CommandType::HALT:
                // Остановка - не обрабатываем следующую команду
                return;
        }
        program_counter++;
    }

    void push(int value) {
        data_stack.push(value);
    }

    int pop() {
        if (data_stack.empty()) {
            throw std::runtime_error("Stack underflow");
        }
        int value = data_stack.top();
        data_stack.pop();
        return value;
    }

    size_t getProgramCounter() const { return program_counter; }
    size_t getStackSize() const { return data_stack.size(); }
    bool isStackEmpty() const { return data_stack.empty(); }
};

#endif // STACK_MACHINE_HPP

