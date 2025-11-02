#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>

// Структура для передачи данных о диапазоне вычислений
struct FactorialArgs {
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
};

// Функция модульного умножения
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);

// Преобразование строки в uint64_t
bool ConvertStringToUI64(const char *str, uint64_t *val);

#endif