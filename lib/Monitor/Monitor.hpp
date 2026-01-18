#pragma once

class Computer;

// "Монитор" — интерактивная консоль/оболочка SimpleVM.
// Здесь только интерфейс, реализация в src/monitor.cpp.
namespace monitor {

class Monitor {
public:
    // Запускает цикл чтения команд из stdin.
    // Возвращает код завершения (0 — нормальный выход).
    static int Run(Computer& computer);
};

} // namespace monitor

