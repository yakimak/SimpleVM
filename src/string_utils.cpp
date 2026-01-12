#include "string_utils.hpp"
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <cstring>

String **globalString = nullptr;
size_t globalStringCount = 0;
size_t globalStringCapacity = 0;
const size_t MaxStringLen = 10000;

void handleError(ErrorCode code, const char *context, short critical) {
    const char *msg = context ? context : "Нет доп информации";
    if (critical) {
        std::fprintf(stderr, "Критическая ошибка: %s\n", msg);
    } else if (critical == 0) {
        std::fprintf(stderr, "Ошибка: %s\n", msg);
    } else if (critical == -1) {
        std::fprintf(stderr, "Замечание: %s\n", msg);
    }
    
    switch(code) {
        case ErrorCode::MEMORY_ALLOCATION_FAILED:
            std::fprintf(stderr, "Не удалось выделить память (%s)\n", msg);
            break;
        case ErrorCode::NULL_POINTER:
            std::fprintf(stderr, "Передан нулевой указатель (%s)\n", msg);
            break;
        case ErrorCode::STRING_TOO_LONG:
            std::fprintf(stderr, "Строка слишком большая (%s)\n", msg);
            break;
        case ErrorCode::ERROR_INDEX:
            std::fprintf(stderr, "Индексы некорректны (%s)\n", msg);
            break;
        case ErrorCode::UTF_ERROR:
            std::fprintf(stderr, "Ошибка преобразования символа в UTF (%s)\n", msg);
            break;
        case ErrorCode::POINTER_CONVERSION_ERROR:
            std::fprintf(stderr, "Ошибка преобразования указателя (%s)\n", msg);
            break;
        case ErrorCode::ERROR_UNKNOW:
        default:
            std::fprintf(stderr, "Неизвестная ошибка (%s)\n", msg);
            break;
    }
    if (critical) {
        FreeAllString();
        std::exit(1);
    }
}

size_t cstrlen(const String *s) {
    if (s == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции cstrlen(). String *s пуст ", 0);
        return static_cast<size_t>(-1);
    }
    if (s->data == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции cstrlen(). void *data пуст ", 0);
        return static_cast<size_t>(-1);
    }
    size_t len = 0;
    my_utf *utf = static_cast<my_utf *>(s->data);
    while (utf[len].bytes[0]) {
        len++;
        if (len > MaxStringLen) {
            handleError(ErrorCode::STRING_TOO_LONG, "Возникла в функции cstrlen(). Возможно строка не содержит '\\0'", 0);
            return static_cast<size_t>(-1);
        }
    }
    return len;
}

size_t charlen(const unsigned char *ch) {
    if (ch == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции charlen(). char *ch пуст ", 0);
        return static_cast<size_t>(-1);
    }
    size_t len = 0;
    while (ch[len]) {
        len++;
        if (len > MaxStringLen) {
            handleError(ErrorCode::STRING_TOO_LONG, "Строка (char*) слишком большая или не содержит '\\0'", 0);
        }
    }
    return len;
}

void printstr(const String *string) {
    if (string == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции printstr(). String *string пуст ", 0);
        return;
    }
    my_utf *utf = static_cast<my_utf *>(string->data);
    size_t ln = string->length;
    for (size_t i = 0; i < ln; i++) {
        for (size_t j = 0; j < utf[i].size; j++) {
            std::printf("%c", utf[i].bytes[j]);
        }
    }
    std::printf("\n");
}

void registerString(String *s) {
    if (s == nullptr || s->data == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции registerString(). Указатель на строку пуст и строка не может быть зарегестрирована", 0);
        return;
    }
    String **newArray = nullptr;
    if (globalStringCount + 1 > globalStringCapacity) {
        bool isPlusCapacity;
        if (globalStringCapacity == 0) {
            globalStringCapacity++;
            isPlusCapacity = true;
            newArray = static_cast<String **>(std::malloc(globalStringCapacity * sizeof(String *)));
        } else {
            globalStringCapacity *= 2;
            isPlusCapacity = false;
            newArray = static_cast<String **>(std::realloc(globalString, globalStringCapacity * sizeof(String *)));
        }
        if (newArray == nullptr) {
            handleError(ErrorCode::MEMORY_ALLOCATION_FAILED, "Возникла в функции registerString(). Массив ссылок всех строк не может быть представлен в памяти", 0);
            if (isPlusCapacity) {
                globalStringCapacity--;
            } else {
                globalStringCapacity /= 2;
            }
            std::fprintf(stderr, "Строка: ");
            printstr(s);
            std::fprintf(stderr, " будет удалена, так как не может быть представлена сохранена в globalString\n");
            freeString(s);
            return;
        }
        globalString = newArray;
    }
    globalString[globalStringCount++] = s;
}

static void unregisterString(String *s) {
    for (size_t i = 0; i < globalStringCount; i++) {
        if (globalString[i] == s) {
            // сдвигаем всё после i влево
            for (size_t j = i; j + 1 < globalStringCount; j++) {
                globalString[j] = globalString[j + 1];
            }
            globalStringCount--;
            globalString[globalStringCount] = nullptr;
            break;
        }
    }
}

void freeString(String *s) {
    if (s == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции freeString(). Передана несуществующая строка", 0);
        return;
    }
    if (s->data == nullptr) {
        s->length = 0;
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции freeString(). Строка уже очищена", -1);
        return;
    }
    std::free(s->data);
    s->data = nullptr;
    s->length = 0;
    unregisterString(s);
}

void FreeAllString() {
    while (globalStringCount > 0) {
        freeString(globalString[globalStringCount - 1]);
    }
    std::free(globalString);
    globalString = nullptr;
    globalStringCapacity = 0;
    globalStringCount = 0;
}

void cstrcpy(String *s, const String *scopy) {
    if (s == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции cstrcpy(). Передан пустой указатель на String *s", 0);
        return;
    }
    if (scopy == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции cstrcpy(). Передан пустой указатель на String *scopy", 0);
        return;
    }
    size_t lens = s->length;
    size_t lenc = scopy->length;
    my_utf *utf = static_cast<my_utf *>(s->data);
    if (utf == nullptr) {
        handleError(ErrorCode::POINTER_CONVERSION_ERROR, "Возникла в функции cstrcpy(). Преобразовать указатель void на данные типа my_utf * не получилось", 0);
        return;
    }
    my_utf *utf_copy = static_cast<my_utf *>(scopy->data);
    if (utf_copy == nullptr) {
        handleError(ErrorCode::POINTER_CONVERSION_ERROR, "Возникла в функции cstrcpy(). Преобразовать указатель void на данные типа my_utf * не получилось", 0);
        return;
    }
    if (lenc > lens) {
        // выделяем место для lenc элементов + 1 терминатор
        my_utf *newUtf = static_cast<my_utf *>(std::realloc(utf, (lenc + 1) * sizeof(my_utf)));
        if (newUtf == nullptr) {
            handleError(ErrorCode::MEMORY_ALLOCATION_FAILED,
                "Возникла в функции cstrcpy(). Строка не может быть увеличена при копировании с заменой",
                0);
            return;
        }
        // обновляем и локальную переменную, и поле структуры
        utf = newUtf;
        s->data = newUtf;
    }
    // копируем ровно lenc элементов
    for (size_t i = 0; i < lenc; i++) {
        utf[i] = utf_copy[i];
    }
    // пишем терминатор
    utf[lenc].size = 1;
    utf[lenc].bytes[0] = '\0';
    // обновляем длину
    s->length = lenc;
}

void cstrcat(String *s, const String *scat) {
    if (s == nullptr || scat == nullptr) {
        handleError(ErrorCode::NULL_POINTER,
            "Возникла в функции cstrcat(). Передан NULL", 0);
        return;
    }

    size_t lens = s->length;
    size_t lenc = scat->length;

    // Исходный буфер
    my_utf *utf = static_cast<my_utf *>(s->data);
    // Буфер добавляемой строки
    my_utf *utf_copy = static_cast<my_utf *>(scat->data);

    if (utf == nullptr || utf_copy == nullptr) {
        handleError(ErrorCode::POINTER_CONVERSION_ERROR,
            "Возникла в функции cstrcat(). Некорректный указатель data", 0);
        return;
    }

    // Расширяем под суммарную длину + 1 терминатор, в байтах
    my_utf *newutf = static_cast<my_utf *>(std::realloc(utf, (lens + lenc + 1) * sizeof(my_utf)));
    if (newutf == nullptr) {
        handleError(ErrorCode::MEMORY_ALLOCATION_FAILED,
            "Возникла в функции cstrcat(). Не удалось увеличить буфер", 0);
        return;
    }

    // Обновляем указатели
    utf = newutf;
    s->data = newutf;

    // Копируем все символы из добавляемой строки
    for (size_t i = 0; i < lenc; i++) {
        utf[lens + i] = utf_copy[i];
    }

    // Пишем терминатор (my_utf с size=1 и байтом '\0')
    utf[lens + lenc].size = 1;
    utf[lens + lenc].bytes[0] = '\0';

    // Обновляем длину
    s->length = lens + lenc;
}

unsigned char* utf8_to_myutf(unsigned char *ch, my_utf* utf) {
    if (ch == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции utf8_to_myutf(). Переданный указатель c пуст", 0);
        return nullptr;
    }
    if (utf == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции utf8_to_myutf(). Переданный указатель utf пуст", 0);
        return nullptr;
    }
    //новая правка
    unsigned char *c = ch;
    //1 байт
    if (*c < 0x80) {
        utf->size = 1;
        utf->bytes[0] = (*c);
        utf->bytes[1] = 0x00;
        utf->bytes[2] = 0x00;
        utf->bytes[3] = 0x00;
    }
    //2 байт
    else if (*c < 0xE0) {
        utf->size = 2;
        utf->bytes[0] = (*c);
        c++;
        //первые символы обязательно 10
        utf->bytes[1] = (0xBF & (0x80 | *c));
        utf->bytes[2] = 0x00;
        utf->bytes[3] = 0x00;
    }
    //3 байт
    else if (*c < 0xF0) {
        utf->size = 3;
        utf->bytes[0] = (*c);
        c++;
        utf->bytes[1] = (0xBF & (0x80 | *c));
        c++;
        utf->bytes[2] = (0xBF & (0x80 | *c));
        utf->bytes[3] = 0x00;
    }
    //4 байт
    else if (*c < 0xF8) {
        utf->size = 4;
        utf->bytes[0] = (*c);
        c++;
        utf->bytes[1] = (0xBF & (0x80 | *c));
        c++;
        utf->bytes[2] = (0xBF & (0x80 | *c));
        c++;
        utf->bytes[3] = (0xBF & (0x80 | *c));
    }
    else {
        handleError(ErrorCode::UTF_ERROR, "Возникла в функции utf8_to_myutf()", 0);
    }
    c++;
    return c;
}

void my_utf_to_lower(my_utf *utf) {
    if (utf->bytes[0] < 0x80) {
        //1 байт
        //английские
        if (utf->bytes[0] >= 'A' && utf->bytes[0] <= 'Z') {
            utf->bytes[0] += 32;
        }
    }
    else if (utf->bytes[0] < 0xE0) {
        //2 байта
        //русские
        if (utf->bytes[0] == 0xD0 && utf->bytes[1] >= 0x90 && utf->bytes[1] <= 0x9F) {
            //а-п
            utf->bytes[1] += 32;
        }
        else if (utf->bytes[0] == 0xD0 && utf->bytes[1] >= 0xA0 && utf->bytes[1] <= 0xAF) {
            //р-я
            utf->bytes[0]++;
            utf->bytes[1] += utf->bytes[1] - 0xA0 + 0x80;
        }
        else if (utf->bytes[0] == 0xD0 && utf->bytes[1] == 0x81) {
            utf->bytes[0]++;
            utf->bytes[1] = 0x91;
        }
    }
}

void initString(String *s, const char *initValue) {
    if (initValue == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции initString()", 0);
        return;
    }
    if (s == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции initString()", 0);
        return;
    }
    size_t len = charlen(reinterpret_cast<const unsigned char *>(initValue));
    s->length = len;
    //my_utf или *my_utf
    s->data = std::malloc((len + 1) * sizeof(my_utf));
    if (s->data == nullptr) {
        handleError(ErrorCode::MEMORY_ALLOCATION_FAILED, "Возникла в функции initString()", 0);
        freeString(s);
        return;
    }
    my_utf *utf_data = static_cast<my_utf *>(s->data);
    unsigned char *chardata = reinterpret_cast<unsigned char *>(const_cast<char *>(initValue));
    size_t i = 0;
    // пока не нулевой байт
    while (*chardata) {
        // если превысили максимально допустимое число символов
        if (i >= MaxStringLen) {
            freeString(s);
            handleError(ErrorCode::STRING_TOO_LONG, "Возникла в функции initString(). Строка удалена", 0);
            return;
        }
        // разбираем следующий UTF-8 символ, возвращая указатель на следующий байт
        chardata = utf8_to_myutf(chardata, utf_data + i);
        i++;
    }
    utf_data[len].size = 1;
    utf_data[len].bytes[0] = '\0';
    utf_data[len].bytes[1] = 0;
    utf_data[len].bytes[2] = 0;
    utf_data[len].bytes[3] = 0;
    registerString(s);
}

void assignString(String *s, const unsigned char *assignValue) {
    if (assignValue == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции assignString(). const char *assignValue пустой", 0);
        return;
    }
    if (s == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции assignString(). String *s пустой", 0);
        return;
    }
    size_t len = charlen(assignValue);
    if (len > MaxStringLen) {
        handleError(ErrorCode::STRING_TOO_LONG, "Возникла в функции assignString(). const char* слишком большой", 0);
        return;
    }
    my_utf *newString = static_cast<my_utf *>(s->data);
    newString = static_cast<my_utf *>(std::realloc(s->data, (len + 1) * sizeof(my_utf)));
    if (newString == nullptr) {
        handleError(ErrorCode::MEMORY_ALLOCATION_FAILED, "Возникла в функции assignString(). Не хватает памяти", 0);
        return;
    }
    s->length = len;
    unsigned char *assignValuePtr = const_cast<unsigned char *>(assignValue);
    for (size_t i = 0; i <= len; i++) {
        assignValuePtr = utf8_to_myutf(assignValuePtr, newString + i);
    }
    s->data = newString;
}

String* concatStrings(String *s1, String *s2) {
    if (s1 == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции concatStrings(). String *s1 пустой", 0);
        return nullptr;
    }
    if (s2 == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции concatStrings(). String *s2 пустой", 0);
        return nullptr;
    }
    if (MaxStringLen - s1->length < s2->length) {
        handleError(ErrorCode::STRING_TOO_LONG, "Воникла в функции concatStrings. Длина получившейся строки слишком большая", 0);
        return nullptr;
    }
    size_t len = s1->length + s2->length;
    
    String *s = static_cast<String *>(std::malloc(sizeof(String)));
    if (s == nullptr) {
        handleError(ErrorCode::MEMORY_ALLOCATION_FAILED, "Возникла в функции concatStrings(). Не хватает памяти для String *", 0);
        return nullptr;
    }
    s->data = std::malloc((len + 1) * sizeof(my_utf));
    if (s->data == nullptr) {
        handleError(ErrorCode::MEMORY_ALLOCATION_FAILED, "Возникла в функции concatStrings(). Не хватает памяти для char *", 0);
        std::free(s);
        return nullptr;
    }
    s->length = len;
    cstrcpy(s, s1);
    cstrcat(s, s2);
    registerString(s);
    return s;
}

String* subString(String *s, size_t start, size_t end) {
    if (s == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции subString(). String *s пустой", 0);
        return nullptr;
    }
    if (start >= end || end > s->length) {
        handleError(ErrorCode::ERROR_INDEX, "Возникла в функции subString(). start и end не выполняют условия", 0);
        return nullptr;
    }
    size_t len = end - start;
    String *result = static_cast<String *>(std::malloc(sizeof(String)));
    if (result == nullptr) {
        handleError(ErrorCode::MEMORY_ALLOCATION_FAILED, "Возникла в функции subString(). String * не представлен в памяти", 0);
        return nullptr;
    }
    result->data = std::malloc((len + 1) * sizeof(my_utf));
    if (result->data == nullptr) {
        handleError(ErrorCode::MEMORY_ALLOCATION_FAILED, "Возникла в функции subString(). my_utf * не представлен в памяти", 0);
        std::free(result);
        return nullptr;
    }
    my_utf *news = static_cast<my_utf *>(result->data);
    my_utf *olds = static_cast<my_utf *>(s->data);
    for (size_t i = start; i < end; i++) {
        news[i - start] = olds[i];
    }
    // Устанавливаем длину
    result->length = len;
    // И терминатор в my_utf-формате
    news[len].size = 1;
    news[len].bytes[0] = '\0';
    return result;
}

size_t findSubstring(const String* string, const String *substring, bool ignoreCase) {
    if (string == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции findSubstring(). const String *string пустой", 0);
        return static_cast<size_t>(-1);
    }
    if (substring == nullptr) {
        handleError(ErrorCode::NULL_POINTER, "Возникла в функции findSubstring(). const char *substring пустой", 0);
        return static_cast<size_t>(-1);
    }
    size_t lensub = substring->length;
    size_t len = string->length;
    if (lensub > len) {
        return static_cast<size_t>(-1);
    }
    bool flag;
    for (size_t i = 0; i < len - lensub + 1; i++) {
        flag = true;
        for (size_t j = i; j < i + lensub; j++) {
            my_utf *strutf = static_cast<my_utf *>(string->data) + j;
            my_utf *substrutf = static_cast<my_utf *>(substring->data) + (j - i);
            my_utf temp_strutf = *strutf;
            my_utf temp_substrutf = *substrutf;
            if (!ignoreCase) {
                my_utf_to_lower(&temp_strutf);
                my_utf_to_lower(&temp_substrutf);
            }
            for (size_t k = 0; k < 4; k++) {
                if (temp_strutf.bytes[k] != temp_substrutf.bytes[k]) {
                    flag = false;
                    break;
                }
            }
        }
        if (flag) {
            return i;
        }
    }
    return static_cast<size_t>(-1);
}

