#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <cstdint>

/**
 * Типы команд для стекового процессора
 */
enum class CommandType {
    PUSH,   // Поместить значение в стек
    POP,    // Извлечь значение из стека
    ADD,    // Сложение
    SUB,    // Вычитание
    MUL,    // Умножение
    DIV,    // Деление
    DUP,    // Дублировать верхний элемент стека
    SWAP,   // Поменять местами два верхних элемента
    HALT    // Остановка выполнения
};

/**
 * Структура команды
 */
struct Command {
    CommandType type;
    int operand;  // Операнд (используется для PUSH)

    Command(CommandType t, int op = 0) : type(t), operand(op) {}
};

#endif // COMMAND_HPP

