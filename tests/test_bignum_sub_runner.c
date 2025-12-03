/**
 * @file    test_bignum_sub_runner.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    03.12.2025
 *
 * @brief Интеграционный тест‑раннер для библиотеки libbignum_sub.a.
 * @details Применяется для проверки достаточности сигнатур 
 *          в файле заголовка (header) при сборке и линковке
 *          статической библиотеки libbignum_sub.a
 *
 * @history
 *   - rev. 1 (03.12.2025): Создание теста
 */  
#include "bignum_sub.h"
#include <assert.h>
#include <stdio.h>  
int main() {
 printf("Running test: test_bignum_sub_runner... "); 
 bignum_t res = {.words = {0}, .len = 0};  
 bignum_t a = {.words = {12345}, .len = 1};
 bignum_t b = {.words = {10000}, .len = 1};  	
 bignum_sub(&res, &a, &b);  
 assert(1);
 printf("PASSED\n");   
 return 0;  
}