#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "common.h"

struct Server {
    char ip[255];
    int port;
};

struct ThreadData {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
    int success;
    pthread_t thread_id;
};

void* ProcessServer(void* arg) {
    struct ThreadData* data = (struct ThreadData*)arg;
    
    printf("Thread for server %s:%d started (range %lu-%lu)\n", 
           data->server.ip, data->server.port, data->begin, data->end);
    
    struct hostent *hostname = gethostbyname(data->server.ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed with %s\n", data->server.ip);
        data->success = 0;
        return NULL;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->server.port);
    
    // Копируем адрес из h_addr_list[0]
    memcpy(&server_addr.sin_addr, hostname->h_addr_list[0], hostname->h_length);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed for %s:%d!\n", 
                data->server.ip, data->server.port);
        data->success = 0;
        return NULL;
    }

    printf("Connecting to %s:%d...\n", data->server.ip, data->server.port);
    
    if (connect(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Connection to %s:%d failed\n", data->server.ip, data->server.port);
        close(sck);
        data->success = 0;
        return NULL;
    }

    printf("Connected to %s:%d, sending task...\n", data->server.ip, data->server.port);

    char task[sizeof(uint64_t) * 3];
    memcpy(task, &data->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &data->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &data->mod, sizeof(uint64_t));

    if (send(sck, task, sizeof(task), 0) < 0) {
        fprintf(stderr, "Send failed to %s:%d\n", data->server.ip, data->server.port);
        close(sck);
        data->success = 0;
        return NULL;
    }

    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
        fprintf(stderr, "Receive failed from %s:%d\n", data->server.ip, data->server.port);
        close(sck);
        data->success = 0;
        return NULL;
    }

    memcpy(&data->result, response, sizeof(uint64_t));
    data->success = 1;
    
    printf("Server %s:%d completed with result: %lu\n", 
           data->server.ip, data->server.port, data->result);
    
    close(sck);
    return NULL;
}

int main(int argc, char **argv) {
    uint64_t k = 0;
    uint64_t mod = 0;
    char servers_file[255] = {'\0'};

    while (true) {
        static struct option options[] = {{"k", required_argument, 0, 0},
                                          {"mod", required_argument, 0, 0},
                                          {"servers", required_argument, 0, 0},
                                          {0, 0, 0, 0}};

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0: {
            switch (option_index) {
            case 0:
                if (!ConvertStringToUI64(optarg, &k)) {
                    fprintf(stderr, "Invalid k value: %s\n", optarg);
                    return 1;
                }
                break;
            case 1:
                if (!ConvertStringToUI64(optarg, &mod)) {
                    fprintf(stderr, "Invalid mod value: %s\n", optarg);
                    return 1;
                }
                break;
            case 2:
                strncpy(servers_file, optarg, sizeof(servers_file) - 1);
                servers_file[sizeof(servers_file) - 1] = '\0';
                break;
            default:
                printf("Index %d is out of options\n", option_index);
            }
        } break;

        case '?':
            printf("Arguments error\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (k == 0 || mod == 0 || !strlen(servers_file)) {
        fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n", argv[0]);
        return 1;
    }

    // Чтение серверов из файла
    FILE* file = fopen(servers_file, "r");
    if (file == NULL) {
        fprintf(stderr, "Cannot open servers file: %s\n", servers_file);
        return 1;
    }

    struct Server* servers = NULL;
    int servers_num = 0;
    char line[255];
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;
        
        char* colon = strchr(line, ':');
        if (colon == NULL) {
            fprintf(stderr, "Invalid server format: %s (expected ip:port)\n", line);
            continue;
        }
        
        *colon = '\0';
        char* ip = line;
        int port = atoi(colon + 1);
        
        if (port <= 0) {
            fprintf(stderr, "Invalid port in: %s\n", line);
            continue;
        }
        
        servers = realloc(servers, (servers_num + 1) * sizeof(struct Server));
        strncpy(servers[servers_num].ip, ip, sizeof(servers[servers_num].ip) - 1);
        servers[servers_num].ip[sizeof(servers[servers_num].ip) - 1] = '\0';
        servers[servers_num].port = port;
        servers_num++;
    }
    fclose(file);

    if (servers_num == 0) {
        fprintf(stderr, "No valid servers found in file: %s\n", servers_file);
        free(servers);
        return 1;
    }

    printf("Found %d servers\n", servers_num);

    // Создаем и инициализируем данные для потоков
    struct ThreadData* thread_data = malloc(servers_num * sizeof(struct ThreadData));
    if (!thread_data) {
        fprintf(stderr, "Memory allocation failed\n");
        free(servers);
        return 1;
    }

    // Распределение работы между серверами
    uint64_t numbers_per_server = k / servers_num;
    uint64_t remainder = k % servers_num;
    uint64_t current = 1;

    printf("\n=== Distributing work ===\n");
    for (int i = 0; i < servers_num; i++) {
        thread_data[i].server = servers[i];
        thread_data[i].begin = current;
        thread_data[i].end = current + numbers_per_server - 1;
        
        if ((uint64_t)i < remainder) {
            thread_data[i].end++;
        }
        
        thread_data[i].mod = mod;
        thread_data[i].success = 0;
        current = thread_data[i].end + 1;

        printf("Server %d (%s:%d): numbers %lu to %lu\n", 
               i, servers[i].ip, servers[i].port, 
               thread_data[i].begin, thread_data[i].end);
    }

    // ЗАПУСК ВСЕХ ПОТОКОВ ПАРАЛЛЕЛЬНО
    printf("\n=== Starting parallel execution ===\n");
    for (int i = 0; i < servers_num; i++) {
        if (pthread_create(&thread_data[i].thread_id, NULL, ProcessServer, &thread_data[i])) {
            fprintf(stderr, "Error creating thread for server %d\n", i);
            thread_data[i].success = 0;
        } else {
            printf("Started thread for server %s:%d\n", 
                   thread_data[i].server.ip, thread_data[i].server.port);
        }
    }

    // ОЖИДАНИЕ ЗАВЕРШЕНИЯ ВСЕХ ПОТОКОВ ПАРАЛЛЕЛЬНО
    printf("\n=== Waiting for all servers to complete ===\n");
    for (int i = 0; i < servers_num; i++) {
        pthread_join(thread_data[i].thread_id, NULL);
    }

    // СБОР РЕЗУЛЬТАТОВ ПОСЛЕ ЗАВЕРШЕНИЯ ВСЕХ ПОТОКОВ
    printf("\n=== Collecting results ===\n");
    uint64_t total_result = 1;
    int all_success = 1;
    int successful_servers = 0;

    for (int i = 0; i < servers_num; i++) {
        if (thread_data[i].success) {
            total_result = MultModulo(total_result, thread_data[i].result, mod);
            printf("Server %s:%d: result = %lu\n", 
                   thread_data[i].server.ip, thread_data[i].server.port, thread_data[i].result);
            successful_servers++;
        } else {
            printf("Server %s:%d: FAILED\n", 
                   thread_data[i].server.ip, thread_data[i].server.port);
            all_success = 0;
        }
    }

    // Вывод итогов
    printf("\n=== Final Results ===\n");
    if (all_success) {
        printf("All %d servers completed successfully\n", servers_num);
        printf("Final result: %lu! mod %lu = %lu\n", k, mod, total_result);
        
        // Верификация
        uint64_t sequential_result = 1;
        for (uint64_t i = 1; i <= k; i++) {
            sequential_result = MultModulo(sequential_result, i, mod);
        }
        printf("Verification (sequential): %lu\n", sequential_result);
        printf("Results match: %s\n", total_result == sequential_result ? "YES" : "NO");
    } else if (successful_servers > 0) {
        printf("%d of %d servers completed successfully\n", successful_servers, servers_num);
        printf("Partial result: %lu! mod %lu = %lu\n", k, mod, total_result);
        printf("WARNING: Result may be incorrect due to server failures\n");
    } else {
        printf("All servers failed. Cannot compute result.\n");
    }

    // Освобождение ресурсов
    free(thread_data);
    free(servers);
    
    return all_success ? 0 : 1;
}