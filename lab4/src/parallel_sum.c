#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

#include <pthread.h>

struct SumArgs {
  int *array;
  int begin;
  int end;
};

void GenerateArray(int *array, unsigned int array_size, unsigned int seed) {
    srand(seed);
    for (unsigned int i = 0; i < array_size; i++) {
        array[i] = rand();
    }
}

int Sum(const struct SumArgs *args) {
  int sum = 0;
  for (int i = args->begin; i < args->end; i++) {
    sum += args->array[i];
  }
  return sum;
}

void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(size_t)Sum(sum_args);
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;
  
  static struct option options[] = {
      {"threads_num", required_argument, 0, 't'},
      {"array_size", required_argument, 0, 'a'},
      {"seed", required_argument, 0, 's'},
      {0, 0, 0, 0}
  };

  int option_index = 0;
  int c;
  while ((c = getopt_long(argc, argv, "t:a:s:", options, &option_index)) != -1) {
      switch (c) {
          case 't':
              threads_num = atoi(optarg);
              if (threads_num <= 0) {
                  printf("threads_num must be a positive number\n");
                  return 1;
              }
              break;
          case 'a':
              array_size = atoi(optarg);
              if (array_size <= 0) {
                  printf("array_size must be a positive number\n");
                  return 1;
              }
              break;
          case 's':
              seed = atoi(optarg);
              if (seed <= 0) {
                  printf("seed must be a positive number\n");
                  return 1;
              }
              break;
          case '?':
              printf("Usage: %s --threads_num <num> --array_size <num> --seed <num>\n", argv[0]);
              return 1;
      }
  }

  if (threads_num == 0 || array_size == 0 || seed == 0) {
      printf("Usage: %s --threads_num <num> --array_size <num> --seed <num>\n", argv[0]);
      return 1;
  }

  printf("Threads: %u, Array Size: %u, Seed: %u\n", threads_num, array_size, seed);


  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  pthread_t threads[threads_num];
  struct SumArgs args[threads_num];

  int segment_size = array_size / threads_num;
  for (uint32_t i = 0; i < threads_num; i++) {
      args[i].array = array;
      args[i].begin = i * segment_size;
      args[i].end = (i == threads_num - 1) ? array_size : (i + 1) * segment_size;
      printf("Thread %d: range [%d, %d)\n", i, args[i].begin, args[i].end);
  }

  struct timespec start_time, end_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);

  for (uint32_t i = 0; i < threads_num; i++) {
      if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
          printf("Error: pthread_create failed!\n");
          free(array);
          return 1;
      }
  }

  int total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
      int sum = 0;
      pthread_join(threads[i], (void **)&sum);
      total_sum += sum;
  }

  clock_gettime(CLOCK_MONOTONIC, &end_time);

  double elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;

  free(array);
  
  printf("Total sum: %d\n", total_sum);
  printf("Elapsed time: %.2f ms\n", elapsed_time);
  
  return 0;
}