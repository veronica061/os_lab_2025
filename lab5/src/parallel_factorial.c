#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

// Структура для передачи данных в потоки
typedef struct {
    long long start;
    long long end;
    long long mod;
    long long partial_result;
} thread_data_t;

// Глобальные переменные
long long global_result = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Функция для вычисления произведения в диапазоне по модулю
void* compute_partial_factorial(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    data->partial_result = 1;
    
    for (long long i = data->start; i <= data->end; i++) {
        data->partial_result = (data->partial_result * (i % data->mod)) % data->mod;
    }
    
    // Синхронизированное обновление глобального результата
    pthread_mutex_lock(&mutex);
    global_result = (global_result * data->partial_result) % data->mod;
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

// Функция для разбора аргументов командной строки
int parse_arguments(int argc, char* argv[], long long* k, int* pnum, long long* mod) {
    // Значения по умолчанию
    *k = 0;
    *pnum = 1;
    *mod = 1000000007; // Большой простой модуль по умолчанию
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            *k = atoll(argv[++i]);
        }
        else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            *pnum = atoi(argv[i] + 7);
        }
        else if (strncmp(argv[i], "--mod=", 6) == 0) {
            *mod = atoll(argv[i] + 6);
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s -k <number> [--pnum=<threads>] [--mod=<modulus>]\n", argv[0]);
            printf("Example: %s -k 10 --pnum=4 --mod=1000000007\n", argv[0]);
            return 0;
        }
    }
    
    if (*k <= 0) {
        printf("Error: k must be a positive number\n");
        printf("Use -h for help\n");
        return 0;
    }
    
    if (*pnum <= 0) {
        printf("Error: pnum must be a positive number\n");
        *pnum = 1;
    }
    
    if (*mod <= 0) {
        printf("Error: mod must be a positive number\n");
        *mod = 1000000007;
    }
    
    return 1;
}

int main(int argc, char* argv[]) {
    long long k, mod;
    int pnum;
    
    // Разбор аргументов командной строки
    if (!parse_arguments(argc, argv, &k, &pnum, &mod)) {
        return 1;
    }
    
    printf("Computing %lld! mod %lld using %d threads\n", k, mod, pnum);
    
    // Если количество потоков больше k, ограничиваем его
    if (pnum > k) {
        pnum = k;
        printf("Adjusted number of threads to %d (cannot exceed k)\n", pnum);
    }
    
    // Создание потоков и данных для них
    pthread_t* threads = malloc(pnum * sizeof(pthread_t));
    thread_data_t* thread_data = malloc(pnum * sizeof(thread_data_t));
    
    if (!threads || !thread_data) {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    // Распределение работы между потоками
    long long numbers_per_thread = k / pnum;
    long long remainder = k % pnum;
    long long current_start = 1;
    
    for (int i = 0; i < pnum; i++) {
        thread_data[i].start = current_start;
        thread_data[i].end = current_start + numbers_per_thread - 1;
        
        // Распределяем остаток по первым потокам
        if (i < remainder) {
            thread_data[i].end++;
        }
        
        thread_data[i].mod = mod;
        current_start = thread_data[i].end + 1;
        
        printf("Thread %d: numbers from %lld to %lld\n", 
               i, thread_data[i].start, thread_data[i].end);
    }
    
    // Запуск потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_create(&threads[i], NULL, compute_partial_factorial, &thread_data[i]) != 0) {
            printf("Error creating thread %d\n", i);
            return 1;
        }
    }
    
    // Ожидание завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Вывод результата
    printf("\nResult: %lld! mod %lld = %lld\n", k, mod, global_result);
    
    // Проверка (последовательное вычисление для верификации)
    long long sequential_result = 1;
    for (long long i = 1; i <= k; i++) {
        sequential_result = (sequential_result * (i % mod)) % mod;
    }
    
    printf("Verification (sequential): %lld\n", sequential_result);
    printf("Results match: %s\n", global_result == sequential_result ? "YES" : "NO");
    
    // Освобождение ресурсов
    free(threads);
    free(thread_data);
    pthread_mutex_destroy(&mutex);
    
    return 0;
}