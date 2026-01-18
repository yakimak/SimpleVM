
#include "string_utils.h"
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

String **globalString = NULL;
size_t globalStringCount = 0;
size_t globalStringCapacity = 0;
size_t MaxStringLen = 1e4;
//bool Safety = true;


void handleError(ErrorCode code, char *context, short critical){
    context = context ? context : "Нет доп информации";
    if (critical) fprintf(stderr, "Критическая ошибка: %s\n", context);
    else if (critical == 0) fprintf(stderr, "Ошибка: %s\n", context);
    else if (critical == -1) fprintf(stderr, "Замечание: %s\n", context);
    switch(code){
        case MEMORY_ALLOCATION_FAILED:
            fprintf(stderr, "Не удалось выделить память (%s)\n", context);
            break;
        case NULL_POINTER:
            fprintf(stderr, "Передан нулевой указатель (%s)\n", context);
            break;
        case STRING_TOO_LONG:
            fprintf(stderr, "Строка слишком большая (%s)\n", context);
            break;
        case ERROR_INDEX:
            fprintf(stderr, "Индексы некорректны (%s)\n", context);
            break;
        case UTF_ERROR:
            fprintf(stderr, "Ошибка преобразования символа в UTF (%s)\n", context);
            break;
        case POINTER_CONVERSION_ERROR:
            fprintf(stderr, "Ошибка преобразования указателя (%s)\n", context);
            break;
        case ERROR_UNKNOW:
        default:
            fprintf(stderr, "Неизвестная ошибка (%s)\n", context);
            break;
    }
    if (critical){
        FreeAllString();
        exit(1);
    }
}

//void updateGlobal

size_t cstrlen(const String *s){
    if (s == NULL){
        handleError(NULL_POINTER, "Возникла в функции cstrlen(). String *s пуст ", 0);
        return -1;
    }
    if (s->data == NULL){
        handleError(NULL_POINTER, "Возникла в функции cstrlen(). void *data пуст ", 0);
        return -1;
    }
    size_t len = 0;
    my_utf *utf = (my_utf *)s->data;
    while (utf[len].bytes[0]){
        len++;
        if (len > MaxStringLen){
            handleError(STRING_TOO_LONG, "Возникла в функции cstrlen(). Возможно строка не содержит '\\0'", 0);
            return -1;
        }
    }
    return len;
}
size_t charlen(const unsigned char *ch){   
    if (ch == NULL){
        handleError(NULL_POINTER, "Возникла в функции charlen(). char *ch пуст ", 0);
        return -1;
    } 
    size_t len = 0;
    while (ch[len]){
        len++;
        if (len > MaxStringLen){
            handleError(STRING_TOO_LONG, "Строка (char*) слишком большая или не содержит '\\0'", 0);
        }
    }
    return len;
}
void printstr(const String *string){
    if (string == NULL){
        handleError(NULL_POINTER, "Возникла в функции printstr(). String *string пуст ", 0);
        return;
    }
    //size_t ln = cstrlen(string);
    my_utf *utf = (my_utf *)string->data;
    size_t ln = string->length;
    for (size_t i = 0; i < ln; i++){
        for (size_t j = 0; j < utf[i].size; j++){
            printf("%c", utf[i].bytes[j]);
        }
    }
    printf("\n");
}

void registerString(String *s){
    if (s == NULL || s->data == NULL){
        handleError(NULL_POINTER, "Возникла в функции registerString(). Указатель на строку пуст и строка не может быть зарегестрирована", 0);
        return;
    }
    // size_t len = cstrlen(s);
    String **newArray = NULL;
    if (globalStringCount+1 > globalStringCapacity){
        short isPlusCapacity;
        if (globalStringCapacity == 0) {
            globalStringCapacity++;
            isPlusCapacity = 1;
            newArray = malloc(globalStringCapacity*sizeof(String *));
        }
        else {
            globalStringCapacity*=2;
            isPlusCapacity = 0;
            newArray = realloc(globalString, globalStringCapacity*sizeof(String *));
        }
        //вопрос
        if (newArray == NULL){
            handleError(MEMORY_ALLOCATION_FAILED, "Возникла в функции registerString(). Массив ссылок всех строк не может быть представлен в памяти", 0);
            if (isPlusCapacity){
                globalStringCapacity--;
            }
            else{
                globalStringCapacity/=2;
            }
            //не знаю, правильно ли (неа :))
            fprintf(stderr, "Строка: ");
            printstr(s);
            fprintf(stderr, " будет удалена, так как не может быть представлена сохранена в globalString\n");
            freeString(s);
            return;
        }
        globalString = newArray;
    }
    //под вопросом!
    //String *newString = malloc(1 * sizeof(String *));
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
            globalString[globalStringCount] = NULL;
            break;
        }
    }
}

void freeString(String *s){
    if (s == NULL){
        handleError(NULL_POINTER, "Возникла в функции freeString(). Передана несуществующая строка", 0);
        return;
    }
    if (s->data == NULL){
        s->length = 0;
        handleError(NULL_POINTER, "Возникла в функции freeString(). Строка уже очищена", -1);
        return;
    }
    free(s->data);
    s->data = NULL;
    s->length = 0;
    unregisterString(s);
}
void FreeAllString(){
    while (globalStringCount > 0) {
        freeString(globalString[globalStringCount - 1]);
    }
    free(globalString);
    globalString = NULL;
    globalStringCapacity = 0;
    globalStringCount = 0;
}


void cstrcpy(String *s, const String *scopy){
    if (s == NULL){
        handleError(NULL_POINTER, "Возникла в функции cstrcpy(). Передан пустой указатель на String *s", 0);
    }
    if (scopy == NULL){
        handleError(NULL_POINTER, "Возникла в функции cstrcpy(). Передан пустой указатель на String *scopy", 0);
    }
    // size_t lens = cstrlen(s);
    //size_t lenc = cstrlen(scopy);
    size_t lens = s->length;
    size_t lenc = scopy->length;
    my_utf *utf = (my_utf *) s->data;
    if (utf == NULL){
        handleError(POINTER_CONVERSION_ERROR, "Возникла в функции cstrcpy(). Преобразовать указатель void на данные типа my_utf * не получилось", 0);
        return;
    }
    my_utf *utf_copy = (my_utf *) scopy->data;
    if (utf_copy == NULL){
        handleError(POINTER_CONVERSION_ERROR, "Возникла в функции cstrcpy(). Преобразовать указатель void на данные типа my_utf * не получилось", 0);
        return;
    }
    if (lenc > lens) {
        // выделяем место для lenc элементов + 1 терминатор
        my_utf *newUtf = realloc(utf, (lenc + 1) * sizeof(my_utf));
        if (newUtf == NULL) {
            handleError(MEMORY_ALLOCATION_FAILED,
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
    utf[lenc].size       = 1;
    utf[lenc].bytes[0]   = '\0';
    // обновляем длину
    s->length = lenc;
}

void cstrcat(String *s, const String *scat) {
    if (s == NULL || scat == NULL) {
        handleError(NULL_POINTER,
            "Возникла в функции cstrcat(). Передан NULL", 0);
        return;
    }

    size_t lens = s->length;
    size_t lenc = scat->length;

    // Исходный буфер
    my_utf *utf      = (my_utf *)s->data;
    // Буфер добавляемой строки
    my_utf *utf_copy = (my_utf *)scat->data;

    if (utf == NULL || utf_copy == NULL) {
        handleError(POINTER_CONVERSION_ERROR,
            "Возникла в функции cstrcat(). Некорректный указатель data", 0);
        return;
    }

    // Расширяем под суммарную длину + 1 терминатор, в байтах
    my_utf *newutf = realloc(utf, (lens + lenc + 1) * sizeof(my_utf));
    if (newutf == NULL) {
        handleError(MEMORY_ALLOCATION_FAILED,
            "Возникла в функции cstrcat(). Не удалось увеличить буфер", 0);
        return;
    }

    // Обновляем указатели
    utf     = newutf;
    s->data = newutf;

    // Копируем все символы из добавляемой строки
    for (size_t i = 0; i < lenc; i++) {
        utf[lens + i] = utf_copy[i];
    }

    // Пишем терминатор (my_utf с size=1 и байтом '\0')
    utf[lens + lenc].size     = 1;
    utf[lens + lenc].bytes[0] = '\0';

    // Обновляем длину
    s->length = lens + lenc;
}

unsigned char* utf8_to_myutf(unsigned char *ch, my_utf* utf){
    if (ch == NULL){
        handleError(NULL_POINTER, "Возникла в функции utf8_to_myutf(). Переданный указатель c пуст", 0);
        return NULL;
    }
    if (utf == NULL){
        handleError(NULL_POINTER, "Возникла в функции utf8_to_myutf(). Переданный указатель utf пуст", 0);
        return NULL;
    }
    //новая правка
    unsigned char *c = ch;
    //1 байтч
    if (*c < 0x80){
        utf->size = 1;
        utf->bytes[0] = (*c);
        utf->bytes[1] = (0x00);
        utf->bytes[2] = (0x00);
        utf->bytes[3] = (0x00);
    }
    //2 байт
    else if (*c < 0xE0){
        utf->size = 2;
        utf->bytes[0] = (*c);
        c++;
        //первые символы обязательно 10
        utf->bytes[1] = (0xBF&(0x80 | *c));
        utf->bytes[2] = (0x00);
        utf->bytes[3] = (0x00);
    }
    //3 байт
    else if (*c < 0xF0){
        utf->size = 3;
        utf->bytes[0] = (*c);
        c++;
        utf->bytes[1] = (0xBF&(0x80 | *c));
        c++;
        utf->bytes[2] = (0xBF&(0x80 | *c));
        utf->bytes[3] = (0x00);
    }
    //4 байт
    else if (*c < 0xF8){
        utf->size = 4;
        utf->bytes[0] = (*c);
        c++;
        utf->bytes[1] = (0xBF&(0x80 | *c));
        c++;
        utf->bytes[2] = (0xBF&(0x80 | *c));
        c++;
        utf->bytes[3] = (0xBF&(0x80 | *c));
    }
    else{
        handleError(UTF_ERROR, "Возникла в функции utf8_to_myutf()", 0);
    }
    c++;
    return c;
}
void my_utf_to_lower(my_utf *utf){
    if (utf->bytes[0]<0x80){
        //1 байт
        //английские
        if (utf->bytes[0]>='A' && utf->bytes[0] <='Z'){
            utf->bytes[0]+=32;
        }
    }
    else if (utf->bytes[0]<0xE0){
        //2 байта
        //русские
        if (utf->bytes[0]==0xD0 && utf->bytes[1] >= 0x90 && utf->bytes[1]<=0x9F){
            //а-п
            utf->bytes[1]+=32;
        }
        else if (utf->bytes[0]==0xD0 && utf->bytes[1] >= 0xA0 && utf->bytes[1]<=0xAF){
            //р-я
            utf->bytes[0]++;
            utf->bytes[1]+=utf->bytes[1]-0xA0+0x80;
        }
        else if (utf->bytes[0]==0xD0 && utf->bytes[1]==0x81){
            utf->bytes[0]++;
            utf->bytes[1] = 0x91;
        }
    }
    return;
}

void initString(String *s, const char *initValue){
    if (initValue == NULL){
        handleError(NULL_POINTER, "Возникла в функции initString()", 0);
        return;
    }
    if (s == NULL){
        handleError(NULL_POINTER, "Возникла в функции initString()", 0);
        return;
    }
    size_t len = 0;
    len = charlen((const unsigned char *)initValue);
    s->length = len;
    //my_utf или *my_utf
    s->data = (void *)malloc((len+1)*sizeof(my_utf));
    if (s->data == NULL){
        handleError(MEMORY_ALLOCATION_FAILED, "Возникла в функции initString()", 0);
        freeString(s);
        return;
    }
    my_utf *utf_data = (my_utf *) s->data;
    unsigned char *chardata = (unsigned char*)initValue;
    size_t i = 0;
    // пока не нулевой байт
    while (*chardata) {
        // если превысили максимально допустимое число символов
        if (i >= MaxStringLen) {
            freeString(s);
            handleError(STRING_TOO_LONG, "Возникла в функции initString(). Строка удалена", 0);
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

//проверил до сюда

void assignString(String *s, const unsigned char *assignValue){
    if (assignValue == NULL){
        handleError(NULL_POINTER, "Возникла в функции assignString(). const char *assignValue пустой", 0);
        return;
    }
    if (s == NULL){
        handleError(NULL_POINTER, "Возникла в функции assignString(). String *s пустой", 0);
        return;
    }
    size_t len = 0;
    len = charlen(assignValue);
    if (len>MaxStringLen){
        handleError(STRING_TOO_LONG, "Возникла в функции assignString(). const char* слишком большой", 0);
        return;
    }
    my_utf *newString = (my_utf *)s->data;
    //или newString = (my_utf *)realloc((my_utf *)s->data, (len+1)*sizeof(my_utf));
    newString = (my_utf *)realloc(s->data, (len+1)*sizeof(my_utf));
    if (newString == NULL){
        handleError(MEMORY_ALLOCATION_FAILED, "Возникла в функции assignString(). Не хватает памяти", 0);
        return;
    }
    s->length = len;
    for (size_t i = 0; i <= len; i++){
        assignValue = utf8_to_myutf((unsigned char *)assignValue, newString+i);
    }
    s->data = newString;
}

String* concatStrings(String *s1, String *s2){
    if (s1 == NULL){
        handleError(NULL_POINTER, "Возникла в функции concatStrings(). String *s1 пустой", 0);
        return NULL;
    }
    if (s2 == NULL){
        handleError(NULL_POINTER, "Возникла в функции concatStrings(). String *s2 пустой", 0);
        return NULL;
    }
    if (MaxStringLen- s1->length < s2->length){
        handleError(STRING_TOO_LONG, "Воникла в функции concatStrings. Длина получившейся строки слишком большая", 0);
        return NULL;
    }
    size_t len = s1->length + s2->length;
    
    String *s = (String *) malloc(sizeof(String));
    if (s == NULL){
        handleError(MEMORY_ALLOCATION_FAILED, "Возникла в функции concatStrings(). Не хватает памяти для String *", 0);
        return NULL;
    }
    s->data = (my_utf *) malloc((len+1)*sizeof(my_utf));
    if (s->data == NULL){
        handleError(MEMORY_ALLOCATION_FAILED, "Возникла в функции concatStrings(). Не хватает памяти для char *", 0);
        free(s);
        return NULL;
    }
    s->length = len;
    cstrcpy(s, s1);
    cstrcat(s, s2);
    registerString(s);
    return s;
}

String* subString(String *s, size_t start, size_t end){
    if (s == NULL){
        handleError(NULL_POINTER, "Возникла в функции subString(). String *s пустой", 0);
        return NULL;
    }
    if (start>=end || end > s->length){
        handleError(ERROR_INDEX, "Возникла в функции subString(). start и end не выполняют условия", 0);
        return NULL;
    }
    size_t len = end-start;
    String *result = (String *)malloc(sizeof(String));
    if (result == NULL){
        handleError(MEMORY_ALLOCATION_FAILED, "Возникла в функции subString(). String * не представлен в памяти", 0);
        return NULL;
    }
    result->data = (my_utf *) malloc((len+1)*sizeof(my_utf));
    if (result->data == NULL){
        handleError(MEMORY_ALLOCATION_FAILED, "Возникла в функции subString(). my_utf * не представлен в памяти", 0);
        free(result);
        return NULL;
    }
    my_utf *news = (my_utf *)result->data;
    my_utf *olds = (my_utf *)s->data;
    for (size_t i = start; i < end; i++) {
        news[i - start] = olds[i];
    }
    // Устанавливаем длину
    result->length = len;
    // И терминатор в my_utf-формате
    news[len].size     = 1;
    news[len].bytes[0] = '\0';
    return result;
}

size_t findSubstring(const String* string, const String *substring, bool ignoreCase){
    if (string == NULL){
        handleError(NULL_POINTER, "Возникла в функции findSubstring(). const String *string пустой", 0);
        return -1;
    }
    if (substring == NULL){
        handleError(NULL_POINTER, "Возникла в функции findSubstring(). const char *substring пустой", 0);
        return -1;
    }
    size_t lensub = substring->length;
    //size_t lensub = cstrlen(substring);
    //size_t len = cstrlen(string);
    size_t len = string->length;
    if (lensub>len){
        return -1;
    }
    bool flag;
    for (size_t i = 0; i < len-lensub+1; i++){
        flag = true;
        for (size_t j = i; j < i+lensub; j++){
            my_utf *strutf = ((my_utf *)string->data)+j;
            my_utf *substrutf = ((my_utf *)substring->data)+(j-i);
            my_utf temp_strutf =  *strutf;
            my_utf temp_substrutf = *substrutf;
            if (!ignoreCase){
                my_utf_to_lower(&temp_strutf);
                my_utf_to_lower(&temp_substrutf);
            }
            for (size_t k = 0; k < 4; k++){
                if (temp_strutf.bytes[k] != temp_substrutf.bytes[k]){
                    flag = false;
                    break;
                }
            }
        }
        if (flag){
            size_t start = i;
            return start;
        }
    }
    return -1;
}