/**
 * @file    test_bignum_sub_mt.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    28.11.2025
 *
 * @brief   Динамический тест на потокобезопасность для bignum_sub.
 *
 * @details
 *   Этот тест создает несколько потоков, каждый из которых многократно
 *   вызывает функцию `bignum_sub` со своим, уникальным набором данных.
 *   Поскольку функция не использует глобальное состояние и оперирует только
 *   данными, переданными через аргументы, вызовы из разных потоков не должны
 *   влиять друг на друга. Успешное прохождение теста доказывает это.
 *
 * @note    Для сборки требуется флаг `-pthread`.
 *
 * @history
 *   - rev. 1 (01.08.2025): Некорректная версия с заглушкой.
 *   - rev. 2 (01.08.2025): Реализован полноценный динамический тест с pthreads.
 */

#include "bignum_sub.h"
#include "bignum.h"
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

#define NUM_THREADS 10
#define NUM_ITERATIONS 100000

typedef struct {
    int thread_id;
    bignum_t a;
    bignum_t b;
    bignum_t expected;
    int ok; /* 1 = success, 0 = failure */
} thread_data_t;

static int is_zero(const bignum_t *x) {
    return (x->len == 1 && x->words[0] == 0);
}

static int bignum_are_equal(const bignum_t* x, const bignum_t* y) {
    if (is_zero(x) && is_zero(y)) return 1;
    if (x->len != y->len) return 0;
    return memcmp(x->words, y->words, x->len * sizeof(uint64_t)) == 0;
}

void* thread_func(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    data->ok = 1;

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        bignum_t res;
        /* Гарантируем детерминированную инициализацию */
        memset(&res, 0, sizeof(res));

        bignum_sub_status_t status = bignum_sub(&res, &data->a, &data->b);

        if (status != BIGNUM_SUB_SUCCESS) {
            data->ok = 0;
            break;
        }
        if (!bignum_are_equal(&res, &data->expected)) {
            data->ok = 0;
            break;
        }
    }
    return NULL;
}

int main(void) {
    printf("\n--- Starting MT test for bignum_sub ---\n");
    pthread_t threads[NUM_THREADS];
    thread_data_t data[NUM_THREADS];
    bignum_t b = {.words = {1}, .len = 1};

    for (int i = 0; i < NUM_THREADS; ++i) {
        data[i].thread_id = i;
        memset(&data[i].a, 0, sizeof(data[i].a));
        data[i].a.len = 1;
        data[i].a.words[0] = 100 + (uint64_t)i;
        data[i].b = b;
        memset(&data[i].expected, 0, sizeof(data[i].expected));

        bignum_sub_status_t st = bignum_sub(&data[i].expected, &data[i].a, &data[i].b);
        if (st != BIGNUM_SUB_SUCCESS) {
            fprintf(stderr, "Failed to compute expected for thread %d: status=%d\n", i, (int)st);
            return 1;
        }

        data[i].ok = 1;

        int rc = pthread_create(&threads[i], NULL, thread_func, &data[i]);
        if (rc != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    int all_ok = 1;
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
        if (!data[i].ok) {
            printf("Thread %d failed!\n", i);
            all_ok = 0;
        }
    }

    if (!all_ok) {
        fprintf(stderr, "--- MT test for bignum_sub FAILED ---\n");
        return 1;
    }

    printf("\n--- MT test for bignum_sub passed ---\n");
    return 0;
}
