/**
 * @file    bignum_sub.h
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    28.11.2025
 *
 * @brief   Модуль для вычитания больших беззнаковых целых чисел.
 * @ingroup bignum_arithmetic
 *
 * @attention
 *   Проект предполагает, что пути к заголовочным файлам (таким как `bignum_t.h`)
 *   управляются через флаги компилятора (например, `-Iinclude`), поэтому
 *   используются прямые включения (`#include "bignum_t.h"`).
 *
 * @history
 *   - rev. 1 (05.08.2025): Первоначальное создание (v0.0.1).
 *   - rev. 2 (05.08.2025): Переход на API v0.0.2 с фиксированным буфером.
 *   - rev. 3 (05.08.2025): Первая попытка запрета перекрытия буферов (v0.0.3).
 *   - rev. 4 (05.08.2025): API переработан, добавлены именованные коды ошибок.
 *   - rev. 5 (05.08.2025): API финализирован, добавлены inline-функции и API отладки.
 *   - rev. 6 (05.08.2025): Доработка документации и контракта API.
 *   - rev. 7 (05.08.2025): Восстановлена полная история. Добавлен include <stddef.h>.
 *                         Финализирована документация Doxygen.
 *   - rev. 8 (06.08.2025): Переход к версии 0.0.4 с сильной декомпозиции эталонной функции
 *   - rev. 9 (07.08.2025): Переход к версии 0.0.5 с оптимизацией вычислительной функции
 *   - rev. 10(08.08.2025): Переход к версии 0.0.6 композитной ассемблерной с оптимизацией вычислительной функции
 *
 * @see     bignum.h
 * @since   1.0.0
 *
 */

#ifndef BIGNUM_SUB_H
#define BIGNUM_SUB_H

#include <bignum.h>
#include <stddef.h>
#include <stdint.h>

// Проверка на наличие определения BIGNUM_CAPACITY из общего заголовка
#ifndef BIGNUM_CAPACITY
#  error "bignum.h must define BIGNUM_CAPACITY"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Коды состояния для функции bignum_sub.
 */

typedef enum {
    BIGNUM_SUB_SUCCESS = 0,                  /** Успешное выполнение. **/
    BIGNUM_SUB_ERROR_NULL_PTR = -1,          /** Один из входных указателей `NULL`. **/ 
    BIGNUM_SUB_ERROR_NEGATIVE_RESULT = -2,   /** Результат отрицательный (`a < b`). **/ 
    BIGNUM_SUB_ERROR_CAPACITY_EXCEEDED = -3, /** Длина операнда (a или b) превышает `BIGNUM_CAPACITY`. **/
    BIGNUM_SUB_ERROR_BUFFER_OVERLAP = -4     /** Обнаружено перекрытие буферов. **/
} bignum_sub_status_t;


/**
 * @brief Выполняет вычитание двух больших беззнаковых целых чисел.
 *
 * @details
 *   ### Алгоритм
 *   1.  Проверяются входные указатели `result`, `a`, `b` на `NULL`.
 *   2.  Проверяется, что длины операндов `a->len` и `b->len` не превышают `BIGNUM_CAPACITY`.
 *   3.  С помощью `bignum_sub_ranges_overlap` проверяется, что диапазоны памяти `result`, `a` и `b` не пересекаются.
 *      Размер буфера для `bignum_t` должен вычисляться как `sizeof(uint64_t) * BIGNUM_CAPACITY`.
 *   4.  Проверяется, что `a->len >= b->len`. Если это не так, `bignum_cmp` не вызывается.
 *   5.  С помощью `bignum_cmp` проверяется, что `a >= b`.
 *   6.  Выполняется пословное вычитание `a - b` с распространением заимствования.
 *   7.  Длина результата нормализуется (удаляются ведущие нули).
 *
 *   ### Потокобезопасность
 *   Функция является потокобезопасной, так как не использует глобальное или
 *   статическое состояние. Всю ответственность за синхронизацию доступа к
 *   одним и тем же объектам `bignum_t` из разных потоков несет вызывающий код.
 *
 * @param[out] result Указатель на структуру `bignum_t` для записи результата.
 * @param[in]  a      Указатель на `bignum_t`, представляющую уменьшаемое.
 * @param[in]  b      Указатель на `bignum_t`, представляющую вычитаемое.
 *
 * @return bignum_sub_status_t Код состояния операции.
 * @retval BIGNUM_SUB_SUCCESS Успешное выполнение.
 * @retval BIGNUM_SUB_ERROR_NULL_PTR Один из входных указателей `NULL`.
 * @retval BIGNUM_SUB_ERROR_NEGATIVE_RESULT Результат отрицательный (`a < b`).
 * @retval BIGNUM_SUB_ERROR_CAPACITY_EXCEEDED Длина операнда (a или b) превышает `BIGNUM_CAPACITY`.
 * @retval BIGNUM_SUB_ERROR_BUFFER_OVERLAP Обнаружено перекрытие буферов.
 */
bignum_sub_status_t bignum_sub(bignum_t *result, const bignum_t *a, const bignum_t *b);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_SUB_H */
