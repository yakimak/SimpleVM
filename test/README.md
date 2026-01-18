# Модульные тесты SimpleVM

Этот каталог содержит модульные тесты для всех компонентов виртуальной машины.

## Структура тестов

- `test_framework.hpp` - Простой тестовый фреймворк с макросами для проверок
- `test_memory.cpp` - Тесты для класса MemoryBlock
- `test_cpu.cpp` - Тесты для стекового процессора (StackMachine)
- `test_disk.cpp` - Тесты для жесткого диска (HardDrive)
- `test_filesystem.cpp` - Тесты для файловой системы (vfs::VirtualFileSystem)
- `test_computer.cpp` - Тесты для главного класса Computer

## Сборка тестов

Тесты автоматически собираются при сборке проекта через CMake:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Запуск тестов

### Вариант 1: Через CTest (рекомендуется)

**Для Visual Studio (multi-config):**
```bash
cd build
ctest -C Release
```

**Для Unix Makefiles (single-config):**
```bash
cd build
ctest
```

Или с подробным выводом:

```bash
ctest -C Release --verbose
```

### Вариант 2: Запуск отдельных тестов

**Для Visual Studio:**
```bash
cd build
Release\test_memory.exe
Release\test_cpu.exe
Release\test_disk.exe
Release\test_filesystem.exe
Release\test_computer.exe
```

**Для Unix:**
```bash
cd build
./test_memory
./test_cpu
./test_disk
./test_filesystem
./test_computer
```

## Покрытие тестами

### MemoryBlock (test_memory.cpp)
- ✅ Создание памяти с заданными параметрами
- ✅ Чтение и запись блоков
- ✅ Работа с несколькими блоками
- ✅ Обработка ошибок выхода за границы
- ✅ Проверка размера данных
- ✅ Инициализация нулями

### StackMachine (test_cpu.cpp)
- ✅ Операции PUSH и POP
- ✅ Арифметические операции (ADD, SUB, MUL, DIV)
- ✅ Деление на ноль
- ✅ Операции DUP и SWAP
- ✅ Счетчик команд (Program Counter)
- ✅ Обработка пустого стека

### HardDrive (test_disk.cpp)
- ✅ Создание диска
- ✅ Запись и чтение файлов
- ✅ Работа с несколькими блоками
- ✅ Удаление файлов
- ✅ Перезапись файлов
- ✅ Обработка ошибок

### VirtualFileSystem (test_filesystem.cpp)
- ✅ Создание файлов и директорий (AttachFile/MakeDirectory)
- ✅ Удаление файлов/узлов (Remove)
- ✅ Листинг директорий (через дерево)
- ✅ Вложенные директории
- ✅ Поиск по имени (FindFilesByName)
- ✅ Перемещение/переименование (Move)

### Computer (test_computer.cpp)
- ✅ Создание компьютера
- ✅ Включение/выключение
- ✅ Доступ к компонентам (RAM, HDD, CPU, FS)
- ✅ Выполнение команд процессора
- ✅ Работа с файловой системой
- ✅ Проверка состояния питания

## Тестовый фреймворк

Используется простой собственный тестовый фреймворк с макросами:

- `ASSERT_TRUE(condition)` - проверка истинности
- `ASSERT_FALSE(condition)` - проверка ложности
- `ASSERT_EQ(expected, actual)` - проверка равенства чисел
- `ASSERT_STREQ(expected, actual)` - проверка равенства строк
- `ASSERT_THROWS(statement, exception_type)` - проверка выбрасывания исключения

## Добавление новых тестов

1. Создайте новый файл `test_<component>.cpp`
2. Включите `test_framework.hpp`
3. Напишите функции-тесты
4. В `main()` добавьте тесты через `framework.addTest()`
5. Обновите `CMakeLists.txt` для добавления нового исполняемого файла

Пример:

```cpp
#include "test_framework.hpp"
#include "../lib/YourComponent.hpp"

void test_your_component() {
    YourComponent comp;
    ASSERT_TRUE(comp.isValid());
}

int main() {
    TestFramework framework;
    framework.addTest("Your component test", test_your_component);
    framework.runAll();
    return framework.getFailedCount() > 0 ? 1 : 0;
}
```

## Примечания

- Все тесты изолированы друг от друга
- Каждый тест создает свои собственные экземпляры компонентов
- Тесты не требуют внешних зависимостей
- Используется стандарт C++17

