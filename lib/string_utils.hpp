#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <cstddef>
#include <cstdint>

//структура my_utf 5 полей 
//1 поле - число от 1 до 4 кол-во задействованных байт

//МАССИВ UNSIGNED CHAR

//void pointer
struct String
{
    size_t length;
    void *data;
};

enum class ErrorCode {
    MEMORY_ALLOCATION_FAILED,
    NULL_POINTER,
    STRING_TOO_LONG,
    ERROR_UNKNOW,
    ERROR_INDEX,
    UTF_ERROR,
    POINTER_CONVERSION_ERROR
};

struct my_utf {
    uint8_t bytes[4]; // 0010 1100 0000 0000
    uint8_t size;
};

void handleError(ErrorCode code, const char *context, short critical);
size_t cstrlen(const String *s);
size_t charlen(const unsigned char *ch);
void printstr(const String *string);
void registerString(String *s);
void freeString(String *s);
void FreeAllString();
void cstrcpy(String *s, const String *scopy);
void cstrcat(String *s, const String *scat);
unsigned char* utf8_to_myutf(unsigned char *c, my_utf* utf);
void my_utf_to_lower(my_utf *utf);
void initString(String *s, const char *initValue);
void assignString(String *s, const unsigned char *assignValue);
String *concatStrings(String *s1, String *s2);
String *subString(String *s, size_t start, size_t end);
size_t findSubstring(const String *string, const String *substring, bool ignoreCase);

#endif // STRING_UTILS_HPP

