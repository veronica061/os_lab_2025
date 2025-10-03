#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

void print_process_info(const char* stage, pid_t parent_pid, pid_t child_pid) {
    printf("\n=== %s ===\n", stage);
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), 
             "echo 'Processes:' && "
             "ps -o pid,ppid,state,comm -p %d,%d 2>/dev/null && "
             "echo '' && "
             "echo 'All zombie processes:' && "
             "ps -eo pid,ppid,state,comm | awk '$3 == \"Z\"' 2>/dev/null || echo 'No zombies found'", 
             parent_pid, child_pid);
    
    system(cmd);
    printf("========================\n");
}

int main() {
    printf("ZOMBIE PROCESS DEMONSTRATION\n");
    printf("===================================================\n");
    
    pid_t parent_pid = getpid();
    printf("Parent Process PID: %d\n", parent_pid);
    
    pid_t child_pid = fork();
    
    if (child_pid < 0) {
        perror("Fork failed");
        return 1;
    }
    
    if (child_pid == 0) {
        printf("Child Process: PID %d (Parent: %d)\n", getpid(), getppid());
        printf("Child: I'm exiting now and will become a ZOMBIE! 🧟\n");
        
        exit(42);
    } else {
        
        printf("Parent: Created child process with PID: %d\n", child_pid);
        
        print_process_info("INITIAL STATE", parent_pid, child_pid);
        
        printf("\nParent: Sleeping 3 seconds to let child exit...\n");
        sleep(3);
        
        print_process_info("AFTER CHILD EXIT (ZOMBIE SHOULD APPEAR)", parent_pid, child_pid);
        
        printf("\nParent: Sleeping 5 more seconds (zombie remains)...\n");
        sleep(5);
        
        print_process_info("ZOMBIE STILL PRESENT", parent_pid, child_pid);
        
        printf("\nParent: Now calling wait() to clean up the zombie...\n");
        int status;
        pid_t waited_pid = waitpid(child_pid, &status, 0);
        
        if (waited_pid > 0) {
            if (WIFEXITED(status)) {
                printf("Successfully cleaned up zombie! Child exit code: %d\n", WEXITSTATUS(status));
            }
        }
        
        print_process_info("AFTER CLEANUP (ZOMBIE SHOULD BE GONE)", parent_pid, child_pid);
        
        printf("\nDEMONSTRATION COMPLETED SUCCESSFULLY!\n");
    }
    
    return 0;
}
