#include "test_framework.hpp"
#include "../lib/CPU/StackMachine.hpp"
#include "../lib/CPU/Command.hpp"
#include "../lib/LazySequence/Sequence.h"
#include "../lib/LazySequence/LazySequence.h"
#include <vector>
#include <memory>

void test_cpu_push_pop() {
    std::vector<Command> commands = {Command(CommandType::HALT)};
    LazySequence<Command> program(commands.data(), (int)commands.size());
    StackMachine cpu(program);
    
    cpu.push(42);
    ASSERT_EQ(1, cpu.getStackSize());
    ASSERT_FALSE(cpu.isStackEmpty());
    
    int value = cpu.pop();
    ASSERT_EQ(42, value);
    ASSERT_EQ(0, cpu.getStackSize());
    ASSERT_TRUE(cpu.isStackEmpty());
}

void test_cpu_add() {
    std::vector<Command> commands = {
        Command(CommandType::PUSH, 5),
        Command(CommandType::PUSH, 3),
        Command(CommandType::ADD),
        Command(CommandType::HALT)
    };
    
    LazySequence<Command> program(commands.data(), (int)commands.size());
    StackMachine cpu(program);
    
    cpu.executeNext(); // PUSH 5
    cpu.executeNext(); // PUSH 3
    cpu.executeNext(); // ADD
    
    ASSERT_EQ(1, cpu.getStackSize());
    int result = cpu.pop();
    ASSERT_EQ(8, result);
}

void test_cpu_sub() {
    std::vector<Command> commands = {
        Command(CommandType::PUSH, 10),
        Command(CommandType::PUSH, 3),
        Command(CommandType::SUB),
        Command(CommandType::HALT)
    };
    
    LazySequence<Command> program(commands.data(), (int)commands.size());
    StackMachine cpu(program);
    
    cpu.executeNext(); // PUSH 10
    cpu.executeNext(); // PUSH 3
    cpu.executeNext(); // SUB
    
    ASSERT_EQ(1, cpu.getStackSize());
    int result = cpu.pop();
    ASSERT_EQ(7, result); // 10 - 3 = 7
}

void test_cpu_mul() {
    std::vector<Command> commands = {
        Command(CommandType::PUSH, 6),
        Command(CommandType::PUSH, 7),
        Command(CommandType::MUL),
        Command(CommandType::HALT)
    };
    
    LazySequence<Command> program(commands.data(), (int)commands.size());
    StackMachine cpu(program);
    
    cpu.executeNext(); // PUSH 6
    cpu.executeNext(); // PUSH 7
    cpu.executeNext(); // MUL
    
    ASSERT_EQ(1, cpu.getStackSize());
    int result = cpu.pop();
    ASSERT_EQ(42, result);
}

void test_cpu_div() {
    std::vector<Command> commands = {
        Command(CommandType::PUSH, 20),
        Command(CommandType::PUSH, 4),
        Command(CommandType::DIV),
        Command(CommandType::HALT)
    };
    
    LazySequence<Command> program(commands.data(), (int)commands.size());
    StackMachine cpu(program);
    
    cpu.executeNext(); // PUSH 20
    cpu.executeNext(); // PUSH 4
    cpu.executeNext(); // DIV
    
    ASSERT_EQ(1, cpu.getStackSize());
    int result = cpu.pop();
    ASSERT_EQ(5, result); // 20 / 4 = 5
}

void test_cpu_div_by_zero() {
    std::vector<Command> commands = {
        Command(CommandType::PUSH, 10),
        Command(CommandType::PUSH, 0),
        Command(CommandType::DIV),
        Command(CommandType::HALT)
    };
    
    LazySequence<Command> program(commands.data(), (int)commands.size());
    StackMachine cpu(program);
    
    cpu.executeNext(); // PUSH 10
    cpu.executeNext(); // PUSH 0
    ASSERT_THROWS(cpu.executeNext(), std::runtime_error); // DIV should throw
}

void test_cpu_dup() {
    std::vector<Command> commands = {
        Command(CommandType::PUSH, 5),
        Command(CommandType::DUP),
        Command(CommandType::HALT)
    };
    
    LazySequence<Command> program(commands.data(), (int)commands.size());
    StackMachine cpu(program);
    
    cpu.executeNext(); // PUSH 5
    cpu.executeNext(); // DUP
    
    ASSERT_EQ(2, cpu.getStackSize());
    int top = cpu.pop();
    ASSERT_EQ(5, top);
    int bottom = cpu.pop();
    ASSERT_EQ(5, bottom);
}

void test_cpu_swap() {
    std::vector<Command> commands = {
        Command(CommandType::PUSH, 1),
        Command(CommandType::PUSH, 2),
        Command(CommandType::SWAP),
        Command(CommandType::HALT)
    };
    
    LazySequence<Command> program(commands.data(), (int)commands.size());
    StackMachine cpu(program);
    
    cpu.executeNext(); // PUSH 1
    cpu.executeNext(); // PUSH 2
    cpu.executeNext(); // SWAP
    
    ASSERT_EQ(2, cpu.getStackSize());
    int top = cpu.pop();
    ASSERT_EQ(1, top); // После swap верхний элемент должен быть 1
    int bottom = cpu.pop();
    ASSERT_EQ(2, bottom);
}

void test_cpu_program_counter() {
    std::vector<Command> commands = {
        Command(CommandType::PUSH, 1),
        Command(CommandType::PUSH, 2),
        Command(CommandType::HALT)
    };
    
    LazySequence<Command> program(commands.data(), (int)commands.size());
    StackMachine cpu(program);
    
    ASSERT_EQ(0, cpu.getProgramCounter());
    cpu.executeNext();
    ASSERT_EQ(1, cpu.getProgramCounter());
    cpu.executeNext();
    ASSERT_EQ(2, cpu.getProgramCounter());
}

void test_cpu_pop_empty_stack() {
    std::vector<Command> commands = {Command(CommandType::HALT)};
    LazySequence<Command> program(commands.data(), (int)commands.size());
    StackMachine cpu(program);
    
    ASSERT_THROWS(cpu.pop(), std::runtime_error);
}

int main() {
    TestFramework framework;
    
    framework.addTest("CPU push/pop", test_cpu_push_pop);
    framework.addTest("CPU add", test_cpu_add);
    framework.addTest("CPU sub", test_cpu_sub);
    framework.addTest("CPU mul", test_cpu_mul);
    framework.addTest("CPU div", test_cpu_div);
    framework.addTest("CPU div by zero", test_cpu_div_by_zero);
    framework.addTest("CPU dup", test_cpu_dup);
    framework.addTest("CPU swap", test_cpu_swap);
    framework.addTest("CPU program counter", test_cpu_program_counter);
    framework.addTest("CPU pop empty stack", test_cpu_pop_empty_stack);
    
    framework.runAll();
    return framework.getFailedCount() > 0 ? 1 : 0;
}

