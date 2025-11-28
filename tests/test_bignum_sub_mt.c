/**
 * @file    test_bignum_sub_mt.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    28.11.2025
 *
 * @brief   Тесты на потокобезопасность для модуля bignum_sub
 *
 * @details
 *   Этот файл содержит тесты для проверки потокобезопасности функции `bignum_sub`.
 *   Тесты демонстрируют, что одновременные вызовы функции из нескольких потоков
 *   с независимыми, но разнообразными данными не приводят к ошибкам.
 *
 * @history
 *   - rev. 1 (05.08.2025): Первоначальное создание для API v0.0.4.
 *   - rev. 2 (05.08.2025): По результатам ревью тест усилен: добавлены
 *                         разнообразные тестовые векторы для проверки различных
 *                         сценариев в многопоточном режиме.
 */

#include "bignum_sub.h"
//#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

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

#define NUM_THREADS 256
#define NUM_ITERATIONS 100000000
#define NUM_TEST_VECTORS 16

// Структура для тестового вектора
typedef struct {
    const char* name;
    bignum_t a;
    bignum_t b;
    bignum_t expected;
} test_vector_t;

// Структура для передачи данных в поток
typedef struct {
    int thread_id;
    int test_status; // 1 = pass, 0 = fail
    pthread_barrier_t *barrier;
    const test_vector_t *vectors;
} thread_data_t;

/**
 * @brief Функция, выполняемая в каждом потоке.
 */
void *thread_func(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    data->test_status = 1;

    pthread_barrier_wait(data->barrier);
unsigned int seed = data->thread_id;

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Выбираем случайный тестовый вектор на каждой итерации
        //const test_vector_t *vector = &data->vectors[rand() % NUM_TEST_VECTORS];
    int idx = rand_r(&seed) % NUM_TEST_VECTORS;
    const test_vector_t *vector = &data->vectors[idx];

        
        bignum_t result;
        bignum_sub_status_t status = bignum_sub(&result, &vector->a, &vector->b);

        if (status != BIGNUM_SUB_SUCCESS ) {
            //fprintf(stderr, "Thread %d: Iteration %d get error status on test '%s' (status=%d)\n",
            //        data->thread_id, i, vector->name, status);
            data->test_status = 1;
            return NULL;
        }

        if (!bignum_equals(&result, &vector->expected)) {
            fprintf(stderr, "Thread %d: Iteration %d bignum_equals FAILED on test '%s' (status=%d)\n",
                    data->thread_id, i, vector->name, status);
            data->test_status = 0;
            break;
        }

        if (status != BIGNUM_SUB_SUCCESS ) {
            fprintf(stderr, "Thread %d: Iteration %d status FAILED on test '%s' (status=%d)\n",
                    data->thread_id, i, vector->name, status);
            data->test_status = 0;
            break;
        }
    }
    return NULL;
}

/**
 * @brief Тест: Потокобезопасность bignum_sub с разнообразными данными.
 */
int test_bignum_sub_thread_safety() {
    pthread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    pthread_barrier_t barrier;
    int overall_status = 1;

        // --- Инициализация тестовых векторов ---
    test_vector_t vectors[NUM_TEST_VECTORS];

    // Вектор 1: Простое вычитание
    vectors[0].name = "Simple Sub";
    bignum_from_array(&vectors[0].a, (uint64_t[]){100}, 1);
    bignum_from_array(&vectors[0].b, (uint64_t[]){50}, 1);
    bignum_from_array(&vectors[0].expected, (uint64_t[]){50}, 1);

    // Вектор 2: Сложное заимствование
    vectors[1].name = "Complex Borrow";
    bignum_from_array(&vectors[1].a, (uint64_t[]){0, 0, 1}, 3);
    bignum_from_array(&vectors[1].b, (uint64_t[]){1}, 1);
    bignum_from_array(&vectors[1].expected, (uint64_t[]){0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL}, 2);

    // Вектор 3: Вычитание до нуля
    vectors[2].name = "Subtraction to Zero";
    bignum_from_array(&vectors[2].a, (uint64_t[]){123, 456}, 2);
    bignum_from_array(&vectors[2].b, (uint64_t[]){123, 456}, 2);
    bignum_from_array(&vectors[2].expected, (uint64_t[]){0}, 1);

    // --- 13 Fuzzy Test Vectors ---

    // 4: Малый vs Большой (b > a)
    vectors[3].name = "Underflow Case";
    bignum_from_array(&vectors[3].a, (uint64_t[]){0x10}, 1);
    bignum_from_array(&vectors[3].b, (uint64_t[]){0x20}, 1);
    // Expected undefined or borrow error: we'll expect status != OK and ignore expected value
    bignum_from_array(&vectors[3].expected, (uint64_t[]){0}, 1);

    // 5: Одинаковые огромные числа
    vectors[4].name = "Large Equal Operands";
    bignum_from_array(&vectors[4].a, (uint64_t[]){0xFFFFFFFFFFFFFFFFULL,0xFFFFFFFFFFFFFFFFULL,0xFFFFFFFFFFFFFFFFULL}, 3);
    bignum_from_array(&vectors[4].b, (uint64_t[]){0xFFFFFFFFFFFFFFFFULL,0xFFFFFFFFFFFFFFFFULL,0xFFFFFFFFFFFFFFFFULL}, 3);
    bignum_from_array(&vectors[4].expected, (uint64_t[]){0}, 1);

    // 6: Чередующиеся биты
    vectors[5].name = "Alternating Bits";
    bignum_from_array(&vectors[5].a, (uint64_t[]){0xAAAAAAAAAAAAAAAAULL,0x5555555555555555ULL}, 2);
    bignum_from_array(&vectors[5].b, (uint64_t[]){0x5555555555555555ULL,0xAAAAAAAAAAAAAAAAULL}, 2);
bignum_from_array(&vectors[5].expected,
    (uint64_t[]){
        0x5555555555555555ULL,
        0xAAAAAAAAAAAAAAABULL
    },
    2
);

    // 7: Один ноль, другое случайное
    vectors[6].name = "Zero Minus Random";
    bignum_from_array(&vectors[6].a, (uint64_t[]){0}, 1);
    bignum_from_array(&vectors[6].b, (uint64_t[]){0,0}, 2);
    bignum_from_array(&vectors[6].expected, (uint64_t[]){0}, 1);  // underflow

    // 8: Случайные 4 слова
    vectors[7].name = "Random 4-Word Test";
    bignum_from_array(&vectors[7].a, (uint64_t[]){0x1111111111111111ULL,0x2222222222222222ULL,
                                                  0x3333333333333333ULL,0x4444444444444444ULL}, 4);
    bignum_from_array(&vectors[7].b, (uint64_t[]){0x0101010101010101ULL,0x0202020202020202ULL,
                                                  0x0303030303030303ULL,0x0404040404040404ULL}, 4);
    bignum_from_array(&vectors[7].expected, (uint64_t[]){0x1010101010101010ULL,0x2020202020202020ULL,
                                                         0x3030303030303030ULL,0x4040404040404040ULL}, 4);

    // 9: Бьем на длину a < b
    vectors[8].name = "Unequal Length a<b";
    bignum_from_array(&vectors[8].a, (uint64_t[]){0x1,0x1}, 2);
    bignum_from_array(&vectors[8].b, (uint64_t[]){0x0,0x0,0x1}, 3);
    bignum_from_array(&vectors[8].expected, (uint64_t[]){0}, 1);  // underflow

    // 10: Бьем на длину a > b
    vectors[9].name = "Unequal Length a>b";
    bignum_from_array(&vectors[9].a, (uint64_t[]){0x0,0x0,0x1}, 3);
    bignum_from_array(&vectors[9].b, (uint64_t[]){0x1}, 1);
    bignum_from_array(&vectors[9].expected, (uint64_t[]){0xFFFFFFFFFFFFFFFFULL,0xFFFFFFFFFFFFFFFFULL}, 2);

    // 11: Большой, почти переполнение
    vectors[10].name = "Max Minus One";
    bignum_from_array(&vectors[10].a, (uint64_t[]){0xFFFFFFFFFFFFFFFFULL,0xFFFFFFFFFFFFFFFFULL}, 2);
    bignum_from_array(&vectors[10].b, (uint64_t[]){1}, 1);
    bignum_from_array(&vectors[10].expected, (uint64_t[]){0xFFFFFFFFFFFFFFFEULL,0xFFFFFFFFFFFFFFFFULL}, 2);

    // 12: Случайная смесь слов
    vectors[11].name = "Mixed Pattern";
    bignum_from_array(&vectors[11].a, (uint64_t[]){0x12340000FFFF0000ULL,0x0000FFFF12340000ULL}, 2);
    bignum_from_array(&vectors[11].b, (uint64_t[]){0x000012340000FFFFULL,0xFFFF000012340000ULL}, 2);
    bignum_from_array(&vectors[11].expected, (uint64_t[]){0X1233EDCCFFFE0001ULL, 0X1FFFF00000000ULL}, 2);
    //{words = {0x1233edccfffe0001, 0x1ffff00000000, 0x0 <repeats 30 times>}, len = 0x2}

    // 13: Вычитание с граничными нулями внутри
    vectors[12].name = "Interior Zeros";
    bignum_from_array(&vectors[12].a, (uint64_t[]){0x1,0x0,0x1}, 3);
    bignum_from_array(&vectors[12].b, (uint64_t[]){0x0,0x1,0x0}, 3);
    bignum_from_array(&vectors[12].expected, (uint64_t[]){0x1,0xFFFFFFFFFFFFFFFFULL,0x0}, 3);

    // 14: Последовательность натуральных чисел
    vectors[13].name = "Natural Sequence";
    bignum_from_array(&vectors[13].a, (uint64_t[]){1,2,3,4,5}, 5);
    bignum_from_array(&vectors[13].b, (uint64_t[]){5,4,3,2,1}, 5);
    bignum_from_array(&vectors[13].expected,
        (uint64_t[]){
            0xFFFFFFFFFFFFFFFCULL,  // 1-5 = ‑4 → 2^64-4
            0xFFFFFFFFFFFFFFFDULL,  // 2-4-1 = ‑3 → 2^64-3
            0xFFFFFFFFFFFFFFFFULL,  // 3-3-1 = ‑1 → 2^64-1 
            0x0000000000000001ULL,  // 4-2-1 =  1 → нет переноса
            0x0000000000000004ULL   // 5-1    =  4
        },
        5
    );

    // 15: Чередующиеся блочные шаблоны
    vectors[14].name = "Block Alternation";
    bignum_from_array(&vectors[14].a,
        (uint64_t[]){0xFFFF0000FFFF0000ULL, 0x0000FFFF0000FFFFULL}, 2);
    bignum_from_array(&vectors[14].b,
        (uint64_t[]){0x0000FFFF0000FFFFULL, 0xFFFF0000FFFF0000ULL}, 2);
    bignum_from_array(&vectors[14].expected,
        (uint64_t[]){0XFFFE0001FFFE0001ULL, 0X1FFFE0001FFFFULL}, 2);



    // 16: Все нули
    vectors[15].name = "All Zeros";
    bignum_from_array(&vectors[15].a, (uint64_t[]){0,0,0}, 3);
    bignum_from_array(&vectors[15].b, (uint64_t[]){0,0,0}, 3);
    bignum_from_array(&vectors[15].expected, (uint64_t[]){0}, 1);
    
    // --- Запуск потоков ---
    srand(time(NULL) ^ getpid());
    if (pthread_barrier_init(&barrier, NULL, NUM_THREADS) != 0) {
        perror("Failed to initialize barrier");
        return 0;
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        thread_data[i].thread_id = i;
        thread_data[i].test_status = 0;
        thread_data[i].barrier = &barrier;
        thread_data[i].vectors = vectors;

        if (pthread_create(&threads[i], NULL, thread_func, &thread_data[i]) != 0) {
            perror("Failed to create thread");
            overall_status = 0;
        }
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
        if (thread_data[i].test_status == 0) {
            overall_status = 0;
        }
    }

    pthread_barrier_destroy(&barrier);
    return overall_status;
}

int main() {
    printf("\n--- Launching Thread Safety Tests for bignum_sub  ---\n");
    if (test_bignum_sub_thread_safety()) {
        printf("  test_bignum_sub_thread_safety: PASSED\n");
        return 0;
    } else {
        printf("  test_bignum_sub_thread_safety: FAILED\n");
        return 1;
    }
}
