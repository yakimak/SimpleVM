#include <stdio.h>
#include <stdint.h>
#include "string_utils.h"
void TestAllStringFuncs(void) {
    String s1, s2, dest;
    String *tmp;
    size_t pos;

    // Тест cstrcpy
    printf("=== Тест cstrcpy ===\n");
    printf("Expected:\nHello\n");
    initString(&s1, "Hello");
    initString(&dest, "");
    cstrcpy(&dest, &s1);
    printf("Actual:\n"); printstr(&dest);
    freeString(&s1); freeString(&dest);

    // Тест cstrcat
    printf("=== Тест cstrcat ===\n");
    printf("Expected:\nHello World!\n");
    initString(&s1, "Hello");
    initString(&s2, " World!");
    initString(&dest, "");
    cstrcpy(&dest, &s1);
    cstrcat(&dest, &s2);
    printf("Actual:\n"); printstr(&dest);
    freeString(&s1); freeString(&s2); freeString(&dest);

    // Тест assignString и concatStrings
    printf("=== Тест assignString + concatStrings ===\n");
    printf("Expected:\nFooBar\n");
    initString(&dest, "");
    assignString(&dest, (const unsigned char*)"Foo");
    initString(&s2, "Bar");
    tmp = concatStrings(&dest, &s2);
    printf("Actual:\n"); printstr(tmp);
    freeString(tmp); freeString(&dest); freeString(&s2);

    // Тест subString
    printf("=== Тест subString ===\n");
    printf("Expected:\ntest\n");
    initString(&s1, "Substring test example");
    tmp = subString(&s1, 10, 14);
    printf("Actual:\n"); printstr(tmp);
    freeString(tmp); freeString(&s1);

    // Тест findSubstring
    printf("=== Тест findSubstring ===\n");
    printf("Expected:\n10\n");
    initString(&s1, "Substring test example");
    initString(&s2, "test");
    pos = findSubstring(&s1, &s2, false);
    printf("Actual:\n%zu\n", pos);
    freeString(&s1); freeString(&s2);

    // Тест cstrlen
    printf("=== Тест cstrlen ===\n");
    initString(&s1, "UTF8: 😊");
    printf("Expected:\n10\nActual:\n%zu\n", cstrlen(&s1));
    freeString(&s1);

    // Очистка глобального реестра
    FreeAllString();
}
int main(){
    //Присвоение или выхов в функцию
    // String strabc, str123, str;

    // initString(&strabc, "ABCФ");
    // initString(&str123, "123");
    // initString(&str, "890");
    //assignString(&str, "LOL");
    //printstr(&strabc);
    //prinе");
    TestAllStringFuncs();
    FreeAllString();
    return 0;
}