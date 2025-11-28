/**
 * @file    test_bignum_sub_extra.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    28.11.2025
 *
 * @brief   Экстра-тесты (робастность и фаззинг) для модуля bignum_sub
 *
 * @details
 *   Этот файл содержит набор экстра-тестов для функции `bignum_sub`,
 *   фокусирующихся на робастности, обработке некорректных данных и
 *   стабильности при случайных входных данных (фаззинг).
 *
 * @history
 *   - rev. 1 (05.08.2025): Первоначальное создание для API v0.0.4.
 *   - rev. 2 (05.08.2025): Добавлены тесты на перекрытие, нулевую длину и фаззинг.
 *   - rev. 3 (05.08.2025): Улучшен фаззинг-тест.
 *   - rev. 4 (05.08.2025): Неудачная попытка исправить ошибки компиляции.
 *   - rev. 5 (05.08.2025): По результатам RCA удален нереализуемый тест
 *                         `test_overlap_inputs_a_b`. Причина: структура bignum_t
 *                         содержит статический массив, а не указатель, что делает
 *                         невозможным симуляцию перекрытия входных буферов
 *                         в рамках простого юнит-теста без изменения архитектуры.
 */

#include "bignum_sub.h"
#include <bignum_cmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/*
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
*/


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

#define FUZZ_ITERATIONS 10000

static int tests_passed = 0;
static int tests_failed = 0;

// --- Тесты на робастность ---

int test_robustness_a_len_exceeds_capacity() {
    bignum_t a, b, result;
    bignum_from_array(&a, (uint64_t[]){1}, 1);
    bignum_from_array(&b, (uint64_t[]){1}, 1);
    a.len = BIGNUM_CAPACITY + 1;
    bignum_sub_status_t status = bignum_sub(&result, &a, &b);
    a.len = 1;
    return status == BIGNUM_SUB_ERROR_CAPACITY_EXCEEDED;
}

int test_robustness_b_len_exceeds_capacity() {
    bignum_t a, b, result;
    bignum_from_array(&a, (uint64_t[]){1}, 1);
    bignum_from_array(&b, (uint64_t[]){1}, 1);
    b.len = BIGNUM_CAPACITY + 1;
    bignum_sub_status_t status = bignum_sub(&result, &a, &b);
    b.len = 1;
    return status == BIGNUM_SUB_ERROR_CAPACITY_EXCEEDED;
}

int test_robustness_zero_len() {
    bignum_t a, b, result;
    bignum_from_array(&a, (uint64_t[]){1}, 1);
    bignum_from_array(&b, (uint64_t[]){1}, 1);
    a.len = 0;
    bignum_sub_status_t status = bignum_sub(&result, &a, &b);
    return status < 0;
}

// --- Тесты на перекрытие буферов ---

int test_overlap_result_a() {
    bignum_t a, b;
    bignum_from_array(&a, (uint64_t[]){10}, 1);
    bignum_from_array(&b, (uint64_t[]){5}, 1);
    return bignum_sub(&a, &a, &b) == BIGNUM_SUB_ERROR_BUFFER_OVERLAP;
}

int test_overlap_result_b() {
    bignum_t a, b;
    bignum_from_array(&a, (uint64_t[]){10}, 1);
    bignum_from_array(&b, (uint64_t[]){5}, 1);
    return bignum_sub(&b, &a, &b) == BIGNUM_SUB_ERROR_BUFFER_OVERLAP;
}

// --- Фаззинг-тест ---

int test_fuzzing_robustness() {
    unsigned int seed = time(NULL) ^ getpid();
    srand(seed);
    printf("Fuzzing with seed: %u\n", seed);

    for (int i = 0; i < FUZZ_ITERATIONS; ++i) {
        bignum_t a, b, result;

        a.len = rand() % (BIGNUM_CAPACITY + 5);
        b.len = rand() % (BIGNUM_CAPACITY + 5);

        for (int j = 0; j < BIGNUM_CAPACITY; ++j) {
            a.words[j] = ((uint64_t)rand() << 32) | rand();
            b.words[j] = ((uint64_t)rand() << 32) | rand();
        }

        bignum_sub_status_t status = bignum_sub(&result, &a, &b);
        
        if (status == BIGNUM_SUB_SUCCESS) {
            if (result.len > BIGNUM_CAPACITY || result.len <= 0) {
                fprintf(stderr, "Fuzzing test failed: invalid result.len %ld on OK status\n", result.len);
                return 0;
            }
            if (bignum_cmp(&result, &a) > 0) {
                fprintf(stderr, "Fuzzing test failed: result > a on OK status\n");
                return 0;
            }
        }
    }
    return 1;
}


int main() {
    printf("\n--- Launching Extra Tests for bignum_sub  ---\n");

    printf("\n--- Running Robustness Tests ---\n");
    RUN_TEST(test_robustness_a_len_exceeds_capacity);
    RUN_TEST(test_robustness_b_len_exceeds_capacity);
    RUN_TEST(test_robustness_zero_len);
    
    printf("\n--- Running Buffer Overlap Tests ---\n");
    RUN_TEST(test_overlap_result_a);
    RUN_TEST(test_overlap_result_b);

    printf("\n--- Running Fuzzing Test ---\n");
    RUN_TEST(test_fuzzing_robustness);

    printf("\n--- Test Summary ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n----------------------\n");

    return tests_failed > 0 ? 1 : 0;
}