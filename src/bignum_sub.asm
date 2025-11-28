; -----------------------------------------------------------------------------
; @file    bignum_sub.asm
; @author  git@bayborodov.com
; @version 1.0.0
; @date    28.11.2025
;
; @brief   Реализация функции вычитания больших целых чисел (bignum) на YASM x86_64.
;
; @details
;   Эталонная ассемблерная реализация для Yasm x86-64 (System V ABI).
;   Модуль предоставляет функцию bignum_sub для вычитания двух больших чисел произвольной длины,
;   определённых типом bignum_t. Реализованы проверки корректности входных указателей, 
;   диапазонов длин операндов, отсутствие перекрытия буферов, а также нормализация результата.   
;
; @history
;   - rev. 1 (08.08.2025): Первоначальная реализация на ассемблере.
;   - rev. 2 (07.11.2025): Removed version control functions and .data section
; -----------------------------------------------------------------------------

section .text

; =============================================================================
; @brief      Выполняет логический сдвиг большого числа влево.
;
; @details
;   **Алгоритм:**
;   1.  Проверка указателя на NULL.
;   2.  Проверка на переполнение:
;       a. Если `shift_amount` >= 2048, вернуть OVERFLOW.
;       b. Если `len` == 32, старший бит старшего слова установлен и
;          `shift_amount` > 0, вернуть OVERFLOW.
;   3.  Проверка тривиальных случаев (нулевой сдвиг, нулевая длина).
;   4.  Расчет сдвига в словах (`word_shift`) и битах (`bit_shift`).
;   5.  Расчет новой длины с усечением до `BIGNUM_CAPACITY`.
;   6.  Сдвиг по словам: копирование данных в старшие позиции.
;   7.  Сдвиг по битам: сдвиг внутри слов с переносом битов.
;   8.  Обновление `len` и нормализация (удаление ведущих нулей).
;
; @abi        System V AMD64 ABI
; @param[in]  rdi: bignum_t* restrict num (указатель на структуру)
; @param[in]  rsi: size_t shift_amount (величина сдвига)
;
; @return     rax: bignum_shift_status_t (0, -1 или -2)
; @retval 0 – success
; @retval -1 – null pointer
; @retval -2 – overflow
; @clobbers   rbx, r8–r15, rcx, rdx
; =============================================================================
; --- Константы ---
BIGNUM_CAPACITY         equ 32
BIGNUM_WORD_SIZE        equ 8
BIGNUM_BITS             equ BIGNUM_CAPACITY * 64
BIGNUM_OFFSET_WORDS     equ 0
BIGNUM_OFFSET_LEN       equ BIGNUM_CAPACITY * BIGNUM_WORD_SIZE
SUCCESS                 equ 0
ERROR_NULL_ARG          equ -1
ERROR_OVERFLOW          equ -2

BUF_QWORDS              equ BIGNUM_CAPACITY      ; 32 qword = 256 bytes
BUF_SIZE                equ 256

; bignum_sub_status_t статус и коды ошибок из bignum_sub.h 
BIGNUM_SUB_SUCCESS                 equ  0
BIGNUM_SUB_ERROR_NULL_PTR          equ -1
BIGNUM_SUB_ERROR_NEGATIVE_RESULT   equ -2
BIGNUM_SUB_ERROR_CAPACITY_EXCEEDED equ -3
BIGNUM_SUB_ERROR_BUFFER_OVERLAP    equ -4


global bignum_sub
extern bignum_cmp

;/**
; * @brief   Вычитание двух больших чисел (a - b) с проверкой входных данных и нормализацией результата.
; *
; * @param   rdi Указатель на структуру bignum_t для записи результата.
; * @param   rsi Указатель на структуру bignum_t — первый операнд (minuend, a).
; * @param   rdx Указатель на структуру bignum_t — второй операнд (subtrahend, b).
; * 
; * @return  Статус выполнения (bignum_sub_status_t):
; *          - BIGNUM_SUB_OK                  — вычитание успешно.
; *          - BIGNUM_SUB_ERR_NULL_PTR        — один из указателей NULL.
; *          - BIGNUM_SUB_ERR_CAPACITY_EXCEEDED— длина операнда некорректна или превышает ёмкость.
; *          - BIGNUM_SUB_ERR_BUFFER_OVERLAP   — перекрытие буферов res, a или b.
; *          - BIGNUM_SUB_ERR_NEGATIVE_RESULT  — a < b, результат отрицателен.
; *
; * @note    Структура bignum_t:
; *            uint64_t words[BIGNUM_CAPACITY];  // little-endian qword array
; *            int32_t  len;                     // текущее число слов
; *            // 4 байта паддинга
; *
; * @details
; * Основные этапы алгоритма:
; *   1) Проверка указателей на NULL.
; *   2) Загрузка и валидация полей len операндов (должны быть от 1 до BIGNUM_CAPACITY).
; *   3) Проверка перекрытия буферов результата и операндов (каждый по BUF_SIZE байт).
; *   4) Сравнение a и b через внешнюю функцию bignum_cmp.
; *   5) Если a < b — возврат ошибки BIGNUM_SUB_ERR_NEGATIVE_RESULT.
; *   6) Вычитание с учётом заимствований, развёртка цикла по 4 слова, хвостовые итерации.
; *   7) Нормализация длины результата — удаление старших нулевых слов, len ≥ 1.
; */
;**
; @brief   Вычитать большие числа: result = a − b.
; @param   rdi Указатель на bignum_t result.
; @param   rsi Указатель на bignum_t a (уменьшаемое).
; @param   rdx Указатель на bignum_t b (вычитаемое).
; @return  eax = код статуса (bignum_sub_status_t).
;
; @note    Структура bignum_t:
;          uint64_t words[BIGNUM_CAPACITY];
;          size_t_t  len;     // [1..BIGNUM_CAPACITY]
;          // 4 байта паддинга
;**
bignum_sub:
    ; Сохраним указатели на стек
    push    rbp
    mov     rbp, rsp
    sub     rsp, 32
    mov     [rbp-8],  rdi    ; result*
    mov     [rbp-16], rsi    ; a*
    mov     [rbp-24], rdx    ; b*

    ; 1) validate_inputs(result, a, b)

    ; 1. Проверка NULL-поинтеров
    test    rdi, rdi
    je      .err_null
    test    rsi, rsi
    je      .err_null
    test    rdx, rdx
    je      .err_null

    ; 2. Загрузка a->len и b->len (смещение 256)
    ; поле len — 4-байта signed int, за ним 4-байта паддинга
    movsxd  r8, dword [rsi + 256]    ; r8 := (int64_t)a->len
    movsxd  r9, dword [rdx + 256]    ; r9 := (int64_t)b->len

    ; 3. Проверка a->len > 0
    cmp     r8, 0
    jle     .err_cap

    ; 4. Проверка b->len > 0
    cmp     r9, 0
    jle     .err_cap

    ; 5. Проверка a->len <= BIGNUM_CAPACITY
    cmp     r8, BIGNUM_CAPACITY
    jg      .err_cap

    ; 6. Проверка b->len <= BIGNUM_CAPACITY
    cmp     r9, BIGNUM_CAPACITY
    jg      .err_cap
    
    ; Всё ок validate_inputs(result, a, b) успешно завершена

    ; 2) check_buffer_overlap(result, a, b)

    ; сохранить a и b
    mov     r8, rsi        ; r8 = a
    mov     r9, rdx        ; r9 = b

    ; 1. bignum_sub_ranges_overlap(res, BUF_SIZE, a, BUF_SIZE)
    mov     rsi, BUF_SIZE  ; n1 = BUF_SIZE
    mov     rdx, r8        ; p2 = a
    mov     rcx, BUF_SIZE  ; n2 = BUF_SIZE

    ; compute end1 = p1 + n1
    mov     r8, rdi
    add     r8, rsi

    ; compute end2 = p2 + n2
    mov     r9, rdx
    add     r9, rcx

    ; if (p1 >= end2) → no overlap
    cmp     rdi, r9
    jae     .no_overlap1

    ; if (p2 >= end1) → no overlap
    cmp     rdx, r8
    jae     .no_overlap1

    ; overlap → return true
    ; mov     al, 1
    jmp     .err_overlap

.no_overlap1:

    ; 2. bignum_sub_ranges_overlap(res, BUF_SIZE, b, BUF_SIZE)
    mov     rsi, BUF_SIZE  ; n1 = BUF_SIZE
    mov     rdx, r9        ; p2 = b
    mov     rcx, BUF_SIZE  ; n2 = BUF_SIZE

    ; compute end1 = p1 + n1
    mov     r8, rdi
    add     r8, rsi

    ; compute end2 = p2 + n2
    mov     r9, rdx
    add     r9, rcx

    ; if (p1 >= end2) → no overlap
    cmp     rdi, r9
    jae     .no_overlap2

    ; if (p2 >= end1) → no overlap
    cmp     rdx, r8
    jae     .no_overlap2

    ; overlap → return true
    ; mov     al, 1
    jmp     .err_overlap

.no_overlap2:
    ; no overlap - check_buffer_overlap(result, a, b) успешно завершена

    ; 3) compare_operands(a, b)
    mov     rdi, [rbp-16]    ; a*
    mov     rsi, [rbp-24]    ; b*

    call    bignum_cmp         ; возвращает int в EAX

    ; Сравниваем только 32-битный EAX с нулём
    cmp     eax, 0
    jl      .err_negative


    ; 4) do_subtraction(res, a, lena, b, lenb)
    mov     rdi, [rbp-8]               ; rdi = res*
    mov     rsi, [rbp-16]              ; rsi = a*
    mov     edx, [rsi + BIGNUM_OFFSET_LEN]    ; edx = a->len
    mov     rcx, [rbp-24]              ; rcx = b*
    mov     r8d, [rcx  + BIGNUM_OFFSET_LEN]   ; r8d = b->len

    mov     r14, rdi           ; R14 = result ptr

    ; --- 1. Нулевание буфера результата rdi = res* ---
    xor     rax, rax          ; паттерн заполнения бууфера - нуль
    mov     rcx, BUF_QWORDS   ; количество обнуляемых значений
    rep     stosq             ; 32 qword = 256 байт

    mov     rcx, [rbp-24]      ; rcx = b* восстановить RCX = b ptr после нулевания

    xor     r9, r9            ; r9 = borrow (0 или 1)
    xor     r10, r10          ; r10 = i = 0

.loop_unroll_4:
    ; Если осталось меньше 4 элементов b — выйти в эпилог по b
    mov     r11, r8           ; r11 = lenb
    sub     r11, r10          ; r11 = lenb - i
    cmp     r11, 4
    jb      .tail_b

    ; ---- Развёртка 4 итераций ----
    ; Итерация 0
    mov     r11, [rsi + r10*8]   ; a0
    mov     r12, r9              ; перенос
    sub     r11, r12             ; tmp = a0 - borrow
    sbb     r11, [rcx + r10*8]   ; tmp -= b0, и CF старого вычета
    mov     [r14 + r10*8], r11   ; r14 = result ptr 
    sbb     r9, r9               ; r9 = 0 или -1 (из CF)
    and     r9, 1                ; r9 = borrow_next

    ; Итерация 1
    mov     r11, [rsi + r10*8 + 8]
    mov     r12, r9
    sub     r11, r12
    sbb     r11, [rcx + r10*8 + 8]
    mov     [r14 + r10*8 + 8], r11
    sbb     r9, r9
    and     r9, 1

    ; Итерация 2
    mov     r11, [rsi + r10*8 + 16]
    mov     r12, r9
    sub     r11, r12
    sbb     r11, [rcx + r10*8 + 16]
    mov     [r14 + r10*8 + 16], r11
    sbb     r9, r9
    and     r9, 1

    ; Итерация 3
    mov     r11, [rsi + r10*8 + 24]
    mov     r12, r9
    sub     r11, r12
    sbb     r11, [rcx + r10*8 + 24]
    mov     [r14 + r10*8 + 24], r11
    sbb     r9, r9
    and     r9, 1

    add     r10, 4
    jmp     .loop_unroll_4

.tail_b:
    cmp     r10d, r8d         ; i < lenb ?
    jge     .tail_a

    ; ---- Хвост по b ----
.tail_b_loop:
    mov     r11, [rsi + r10*8]
    mov     r12, r9
    sub     r11, r12           ; tmp = a[i] - borrow
    sbb     r11, [rcx + r10*8] ; tmp -= b[i]
    mov     [r14 + r10*8], r11
    sbb     r9, r9
    and     r9, 1

    inc     r10
    cmp     r10d, r8d
    jl      .tail_b_loop

.tail_a:
    cmp     r10d, edx         ; i < lena ?
    jge     .sub_done

    ; ---- Хвост по a ----
.tail_a_loop:
    mov     r11, [rsi + r10*8]
    mov     r12, r9
    sub     r11, r12           ; tmp = a[i] - borrow
    mov     [r14 + r10*8], r11
    sbb     r9, r9
    and     r9, 1

    inc     r10
    cmp     r10d, edx
    jl      .tail_a_loop

.sub_done:                             ; рассчет завершен

    ; 5) normalize_result(result, a->len)
    ; Вход:
    ;   rdi = ptr to result
    ;   edx = исходная длина a->len (после вычитания в поле BIGNUM_OFFSET_LEN)
    ; Цель: пройти от edx-1 вниз и найти первое ненулевое слово,
    ;       минимальный результат len = 1.

    mov     rdi, [rbp-8]     ; result*
    mov     ecx, edx            ; ecx = текущая длина
    dec     ecx                 ; ecx = index = len - 1

.norm_unroll2:
    cmp     ecx, 1
    jl      .norm_scalar2

    mov     rax, [rdi + rcx*8]
    or      rax, [rdi + rcx*8 - 8]
    test    rax, rax
    jne     .norm_scalar2

    sub     ecx, 2
    jmp     .norm_unroll2

.norm_scalar2:
    cmp     ecx, 0
    jl      .norm_all_zero

    mov     rax, [rdi + rcx*8]
    test    rax, rax
    jne     .norm_found

    dec     ecx
    jmp     .norm_scalar2

.norm_found:
    inc     ecx
    mov     [rdi + BIGNUM_OFFSET_LEN], ecx
    jmp     .norm_done

.norm_all_zero:
    mov     DWORD [rdi + BIGNUM_OFFSET_LEN], 1

.norm_done:

    ; Успех
    mov     eax, BIGNUM_SUB_SUCCESS

.done:
    leave
    ret

.err_null:
    mov     rax, BIGNUM_SUB_ERROR_NULL_PTR
    leave
    ret

.err_negative:
    mov     eax, BIGNUM_SUB_ERROR_NEGATIVE_RESULT
    leave
    ret

.err_cap:
    mov     rax, BIGNUM_SUB_ERROR_CAPACITY_EXCEEDED
    leave
    ret

.err_overlap:
    mov     rax, BIGNUM_SUB_ERROR_BUFFER_OVERLAP
    leave
    ret
