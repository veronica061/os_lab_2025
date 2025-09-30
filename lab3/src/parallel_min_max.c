#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

pid_t *child_pids = NULL;
int child_count = 0;

void timeout_handler(int sig) {
    printf("Timeout reached! Sending SIGKILL to all child processes...\n");
    for (int i = 0; i < child_count; i++) {
        if (child_pids[i] > 0) {
            printf("Killing child process PID: %d\n", child_pids[i]);
            kill(child_pids[i], SIGKILL);
        }
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = 0; 
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {"timeout", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "ft:", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed must be a positive number\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be a positive number\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum must be a positive number\n");
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case 't':
        timeout = atoi(optarg);
        if (timeout <= 0) {
          printf("timeout must be a positive number\n");
          return 1;
        }
        printf("Timeout set to %d seconds\n", timeout);
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"] [--by_files]\n",
           argv[0]);
    return 1;
  }

  child_pids = malloc(pnum * sizeof(pid_t));
  if (child_pids == NULL) {
    printf("Memory allocation failed for child_pids\n");
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  
  int pipes[2 * pnum];
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes + i * 2) < 0) {
        printf("Pipe creation failed!\n");
        free(child_pids);
        free(array);
        return 1;
      }
    }
  }

  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  if (timeout > 0) {
    signal(SIGALRM, timeout_handler);
    alarm(timeout);
    printf("Timer set for %d seconds\n", timeout);
  }

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      child_pids[child_count++] = child_pid;
      
      if (child_pid == 0) {
        int segment_size = array_size / pnum;
        int begin = i * segment_size;
        int end = (i == pnum - 1) ? array_size : (i + 1) * segment_size;
        
        printf("Child process %d (PID: %d) processing range [%d, %d)\n", 
               i, getpid(), begin, end);
        
        struct MinMax local_min_max = GetMinMax(array, begin, end);
        
        if (with_files) {
          char filename_min[20], filename_max[20];
          sprintf(filename_min, "min_%d.txt", i);
          sprintf(filename_max, "max_%d.txt", i);
          
          FILE *file_min = fopen(filename_min, "w");
          FILE *file_max = fopen(filename_max, "w");
          if (file_min && file_max) {
            fprintf(file_min, "%d", local_min_max.min);
            fprintf(file_max, "%d", local_min_max.max);
            fclose(file_min);
            fclose(file_max);
            printf("Child %d wrote results to files\n", i);
          }
        } else {
          close(pipes[i * 2]);
          
          write(pipes[i * 2 + 1], &local_min_max.min, sizeof(int));
          write(pipes[i * 2 + 1], &local_min_max.max, sizeof(int));
          
          close(pipes[i * 2 + 1]);
          printf("Child %d sent results via pipe\n", i);
        }
        free(array);
        return 0;
      }

    } else {
      printf("Fork failed!\n");
      free(child_pids);
      free(array);
      return 1;
    }
  }
  int completed_children = 0;
  while (active_child_processes > 0) {
    int status;
    pid_t finished_pid = wait(&status);
    
    if (finished_pid > 0) {
      active_child_processes -= 1;
      completed_children++;
      
      if (WIFEXITED(status)) {
        printf("Child process %d exited normally with status %d\n", 
               finished_pid, WEXITSTATUS(status));
      } else if (WIFSIGNALED(status)) {
        printf("Child process %d was killed by signal %d\n", 
               finished_pid, WTERMSIG(status));
      }
    }
  }
  if (timeout > 0) {
    alarm(0); 
    printf("All child processes completed, timer disabled\n");
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;
  int successful_children = 0;
  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;
    int results_available = 0;

    if (with_files) {
      char filename_min[20], filename_max[20];
      sprintf(filename_min, "min_%d.txt", i);
      sprintf(filename_max, "max_%d.txt", i);
      
      FILE *file_min = fopen(filename_min, "r");
      FILE *file_max = fopen(filename_max, "r");
      
      if (file_min && file_max) {
        if (fscanf(file_min, "%d", &min) == 1 && fscanf(file_max, "%d", &max) == 1) {
          successful_children++;
          results_available = 1;
          printf("Successfully read results from child %d: min=%d, max=%d\n", i, min, max);
        }
        fclose(file_min);
        fclose(file_max);
      }
      remove(filename_min);
      remove(filename_max);
    } else {
      close(pipes[i * 2 + 1]);
      
      int min_read = read(pipes[i * 2], &min, sizeof(int));
      int max_read = read(pipes[i * 2], &max, sizeof(int));
      
      if (min_read == sizeof(int) && max_read == sizeof(int)) {
        successful_children++;
        results_available = 1;
        printf("Successfully read results from child %d via pipe: min=%d, max=%d\n", i, min, max);
      } else {
        printf("Failed to read results from child %d via pipe (min_read=%d, max_read=%d)\n", 
               i, min_read, max_read);
      }
      
      close(pipes[i * 2]);
    }

    if (results_available) {
      if (min < min_max.min) min_max.min = min;
      if (max > min_max.max) min_max.max = max;
    }
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(child_pids);
  free(array);

  printf("\nResults:\n");
  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Successful children: %d/%d\n", successful_children, pnum);
  printf("Elapsed time: %fms\n", elapsed_time);
  
  if (timeout > 0 && successful_children < pnum) {
    printf("Warning: Not all child processes completed successfully (timeout may have occurred)\n");
  } else if (successful_children == pnum) {
    printf("All child processes completed successfully\n");
  }
  
  fflush(NULL);
  return 0;
}