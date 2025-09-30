#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <seed> <array_size>\n", argv[0]);
        printf("Example: %s 42 100000\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        return 1;
    } else if (pid == 0) {
        printf("Child process (PID: %d) is running sequential_min_max...\n", getpid());
        
        execl("./sequential_min_max", "sequential_min_max", argv[1], argv[2], NULL);
        
        perror("execl failed");
        exit(1);
    } else {
        printf("Parent process (PID: %d) started child process (PID: %d)\n", getpid(), pid);
        
        int status;
        waitpid(pid, &status, 0); 
        
        if (WIFEXITED(status)) {
            printf("Child process exited with status: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child process terminated by signal: %d\n", WTERMSIG(status));
        }
        
        printf("Parent process finished.\n");
    }
    
    return 0;
}