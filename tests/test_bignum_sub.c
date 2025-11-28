/**
 * @file    test_bignum_sub.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    28.11.2025
 *
 * @brief   Детерминированные тесты для модуля bignum_sub.
 *
 * @details
 *   Этот файл содержит исчерпывающий набор детерминированных тестов для
 *   функции `bignum_sub`. Тесты покрывают все "счастливые пути",
 *   граничные случаи, а также все возможные коды ошибок, определенные
 *   в `bignum_sub_status_t`.
 *
 * @history
 *   - rev. 1 (05.08.2025): Первоначальное создание для API v0.0.4.
 *   - rev. 2 (05.08.2025): По результатам ревью добавлены тесты на сложные
 *                         граничные случаи и сценарии для полного покрытия.
 *   - rev. 3 (05.08.2025): По результатам ревью улучшена читаемость, исправлены
 *                         типы итераторов и добавлены явные проверки длин.
 */

#include "bignum_sub.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>


// Вспомогательная функция для определения нуля для bignum_t
static int is_zero(const bignum_t *x) {
    return (x->len == 1 && x->words[0] == 0);
}


// Вспомогательная функция для сравнения bignum_t
static int bignum_equals(const bignum_t* a, const bignum_t* b) {
    if (is_zero(a) && is_zero(b)) return 1;
    if (a->len != b->len) return 0;
    if (a->len == 0) return 1;
    return memcmp(a->words, b->words, a->len * sizeof(uint64_t)) == 0;
}


// Вспомогательная функция для инициализации bignum_t из массива uint64_t
void bignum_from_array(bignum_t *num, const uint64_t *arr, size_t len) {
    memset(num, 0, sizeof(bignum_t));
    for (size_t i = 0; i < len; ++i) {
        num->words[i] = arr[i];
    }
    num->len = len;
    // Нормализация: удаление ведущих нулей
    while (num->len > 0 && num->words[num->len - 1] == 0) {
        num->len--;
    }
    if (num->len == 0) {
        num->words[0] = 0;
        num->len = 1;
    }
}

 

// Макрос для запуска тестов и подсчета результатов
#define RUN_TEST(test_func) \
    do { \
        printf("Running %s...\n", #test_func); \
        if (test_func()) { \
            printf("  %s: PASSED\n", #test_func); \
            tests_passed++; \
        } else { \
            printf("  %s: FAILED\n", #test_func); \
            tests_failed++; \
        } \
    } while (0)

static int tests_passed = 0;
static int tests_failed = 0;

// --- Тесты на "счастливые пути" ---

int test_simple_sub() {
    bignum_t a, b, result, expected;
    bignum_from_array(&a, (uint64_t[]){10}, 1);
    bignum_from_array(&b, (uint64_t[]){5}, 1);
    bignum_from_array(&expected, (uint64_t[]){5}, 1);
    bignum_sub_status_t status = bignum_sub(&result, &a, &b);
    return status == BIGNUM_SUB_SUCCESS && bignum_equals(&result, &expected) && result.len == 1;
}

int test_sub_with_borrow() {
    bignum_t a, b, result, expected;
    bignum_from_array(&a, (uint64_t[]){0, 1}, 2); // 2^64
    bignum_from_array(&b, (uint64_t[]){1}, 1);
    bignum_from_array(&expected, (uint64_t[]){0xFFFFFFFFFFFFFFFFULL}, 1);
    bignum_sub_status_t status = bignum_sub(&result, &a, &b);
    return status == BIGNUM_SUB_SUCCESS && bignum_equals(&result, &expected) && result.len == 1;
}

int test_sub_a_longer_no_borrow() {
    bignum_t a, b, result, expected;
    bignum_from_array(&a, (uint64_t[]){10, 20}, 2);
    bignum_from_array(&b, (uint64_t[]){5}, 1);
    bignum_from_array(&expected, (uint64_t[]){5, 20}, 2);
    bignum_sub_status_t status = bignum_sub(&result, &a, &b);
    return status == BIGNUM_SUB_SUCCESS && bignum_equals(&result, &expected) && result.len == 2;
}

int test_multi_word_borrow_chain() {
    bignum_t a, b, result, expected;
    bignum_from_array(&a, (uint64_t[]){0, 0, 1}, 3); // 2^128
    bignum_from_array(&b, (uint64_t[]){1}, 1);
    bignum_from_array(&expected, (uint64_t[]){0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, 2);
    bignum_sub_status_t status = bignum_sub(&result, &a, &b);
    return status == BIGNUM_SUB_SUCCESS && bignum_equals(&result, &expected) && result.len == 2;
}

// --- Тесты на граничные случаи и нормализацию ---

int test_sub_to_zero_and_normalize() {
    bignum_t a, b, result, expected;
    bignum_from_array(&a, (uint64_t[]){100, 200}, 2);
    bignum_from_array(&b, (uint64_t[]){100, 200}, 2);
    bignum_from_array(&expected, (uint64_t[]){0}, 1);
    bignum_sub_status_t status = bignum_sub(&result, &a, &b);
    return status == BIGNUM_SUB_SUCCESS && bignum_equals(&result, &expected) && result.len == 1;
}

int test_multi_word_equality_to_zero() {
    bignum_t a, b, result, expected;
    uint64_t arr[] = {1, 2, 3, 4};
    bignum_from_array(&a, arr, 4);
    bignum_from_array(&b, arr, 4);
    bignum_from_array(&expected, (uint64_t[]){0}, 1);
    bignum_sub_status_t status = bignum_sub(&result, &a, &b);
    return status == BIGNUM_SUB_SUCCESS && bignum_equals(&result, &expected) && result.len == 1;
}

int test_sub_zero_operand() {
    bignum_t a, b, result, expected;
    uint64_t arr_a[] = {123, 456};
    bignum_from_array(&a, arr_a, 2);
    bignum_from_array(&b, (uint64_t[]){0}, 1);
    bignum_from_array(&expected, arr_a, 2);
    bignum_sub_status_t status = bignum_sub(&result, &a, &b);
    return status == BIGNUM_SUB_SUCCESS && bignum_equals(&result, &expected) && result.len == 2;
}

int test_sub_from_max_capacity() {
    bignum_t a, b, result, expected;
    uint64_t arr_a[BIGNUM_CAPACITY];
    uint64_t arr_b[BIGNUM_CAPACITY];
    uint64_t arr_exp[BIGNUM_CAPACITY];

    for(size_t i=0; i<BIGNUM_CAPACITY; ++i) arr_a[i] = 0xFFFFFFFFFFFFFFFFULL;
    arr_b[0] = 1; // b = 1 (len=1), остальные слова implicit zero
    for(size_t i=1; i<BIGNUM_CAPACITY; ++i) arr_b[i] = 0;

    bignum_from_array(&a, arr_a, BIGNUM_CAPACITY);
    bignum_from_array(&b, arr_b, 1);
    
    arr_exp[0] = 0xFFFFFFFFFFFFFFFEULL;
    for(size_t i=1; i<BIGNUM_CAPACITY; ++i) arr_exp[i] = 0xFFFFFFFFFFFFFFFFULL;
    bignum_from_array(&expected, arr_exp, BIGNUM_CAPACITY);

    bignum_sub_status_t status = bignum_sub(&result, &a, &b);
    return status == BIGNUM_SUB_SUCCESS && bignum_equals(&result, &expected) && result.len == BIGNUM_CAPACITY;
}

int test_full_capacity_operands() {
    bignum_t a, b, result, expected;
    uint64_t arr_a[BIGNUM_CAPACITY];
    uint64_t arr_b[BIGNUM_CAPACITY];
    uint64_t arr_exp[BIGNUM_CAPACITY];

    for(size_t i=0; i<BIGNUM_CAPACITY; ++i) {
        arr_a[i] = i + 10;
        arr_b[i] = i + 5;
        arr_exp[i] = 5;
    }
    bignum_from_array(&a, arr_a, BIGNUM_CAPACITY);
    bignum_from_array(&b, arr_b, BIGNUM_CAPACITY);
    bignum_from_array(&expected, arr_exp, BIGNUM_CAPACITY);

    bignum_sub_status_t status = bignum_sub(&result, &a, &b);
    return status == BIGNUM_SUB_SUCCESS && bignum_equals(&result, &expected) && result.len == BIGNUM_CAPACITY;
}

// --- Тесты на обработку ошибок ---

int test_err_null_pointer() {
    bignum_t a, b, result;
    bignum_from_array(&a, (uint64_t[]){1}, 1);
    bignum_from_array(&b, (uint64_t[]){1}, 1);
    bool r1 = (bignum_sub(NULL, &a, &b) == BIGNUM_SUB_ERROR_NULL_PTR);
    bool r2 = (bignum_sub(&result, NULL, &b) == BIGNUM_SUB_ERROR_NULL_PTR);
    bool r3 = (bignum_sub(&result, &a, NULL) == BIGNUM_SUB_ERROR_NULL_PTR);
    return r1 && r2 && r3;
}

int test_err_negative_result() {
    bignum_t a, b, result;
    bignum_from_array(&a, (uint64_t[]){5}, 1);
    bignum_from_array(&b, (uint64_t[]){10}, 1);
    bignum_sub_status_t status = bignum_sub(&result, &a, &b);
    return status == BIGNUM_SUB_ERROR_NEGATIVE_RESULT;
}

int test_err_capacity_exceeded() {
    bignum_t a, b, result;
    bignum_from_array(&a, (uint64_t[]){1}, 1);
    bignum_from_array(&b, (uint64_t[]){1}, 1);
    a.len = BIGNUM_CAPACITY + 1; // Искусственно портим длину
    bignum_sub_status_t status = bignum_sub(&result, &a, &b);
    a.len = 1; // Восстанавливаем
    return status == BIGNUM_SUB_ERROR_CAPACITY_EXCEEDED;
}

int test_err_buffer_overlap() {
    bignum_t a, b;
    bignum_from_array(&a, (uint64_t[]){10}, 1);
    bignum_from_array(&b, (uint64_t[]){5}, 1);
    bool r1 = (bignum_sub(&a, &a, &b) == BIGNUM_SUB_ERROR_BUFFER_OVERLAP);
    bool r2 = (bignum_sub(&b, &a, &b) == BIGNUM_SUB_ERROR_BUFFER_OVERLAP);
    return r1 && r2;
}


int main() {
    printf("\n--- Launching Deterministic Tests for bignum_sub ---\n");

    printf("\n--- Running Happy Path Tests ---\n");
    RUN_TEST(test_simple_sub);
    RUN_TEST(test_sub_with_borrow);
    RUN_TEST(test_sub_a_longer_no_borrow);
    RUN_TEST(test_multi_word_borrow_chain);

    printf("\n--- Running Boundary and Normalization Tests ---\n");
    RUN_TEST(test_sub_to_zero_and_normalize);
    RUN_TEST(test_multi_word_equality_to_zero);
    RUN_TEST(test_sub_zero_operand);
    RUN_TEST(test_sub_from_max_capacity);
    RUN_TEST(test_full_capacity_operands);

    printf("\n--- Running Error Handling Tests ---\n");
    RUN_TEST(test_err_null_pointer);
    RUN_TEST(test_err_negative_result);
    RUN_TEST(test_err_capacity_exceeded);
    RUN_TEST(test_err_buffer_overlap);

    printf("\n--- Test Summary ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n----------------------\n");

    return tests_failed > 0 ? 1 : 0;
}