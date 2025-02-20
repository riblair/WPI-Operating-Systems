#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>

#define NUM_ITERATIONS 1000
#define MESSAGE_SIZE 1

double get_time_usec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000.0) + tv.tv_usec;
}

int main() {
    int pipefd[2];
    char message[MESSAGE_SIZE];
    pid_t pid;
    
    // Start timing
    double start_time = get_time_usec();
    
    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }
    if (pid == 0) {  
        close(pipefd[1]);  
        close(pipefd[0]);  
        
        for (int i = 0; i < NUM_ITERATIONS; i++) {
            // Read from parent
            if (read(pipefd[0], message, MESSAGE_SIZE) != MESSAGE_SIZE) {
                perror("read failed");
                exit(1);
            }
            
            // Write to parent
            if (write(pipefd[1], message, MESSAGE_SIZE) != MESSAGE_SIZE) {
                perror("write failed");
                exit(1);
            }
        }
        
        close(pipefd[0]);
        close(pipefd[1]);
        exit(0);
        
    } else {  // Parent process
        close(pipefd[0]);  
        close(pipefd[1]);  
        
        for (int i = 0; i < NUM_ITERATIONS; i++) {
            // Write to child
            if (write(pipefd[1], message, MESSAGE_SIZE) != MESSAGE_SIZE) {
                perror("write failed");
                exit(1);
            }
            
            // Read from child
            if (read(pipefd[0], message, MESSAGE_SIZE) != MESSAGE_SIZE) {
                perror("read failed");
                exit(1);
            }
        }
        
        wait(NULL);
        
        double end_time = get_time_usec();
        double total_time = end_time - start_time;
        // has to be *2 because each iteration causes 2 switches 
        double time = total_time / (NUM_ITERATIONS * 2); 
        
        printf("context switch: %f microseconds\n", time);
        
        close(pipefd[1]);
        close(pipefd[0]);
    }
    
    return 0;
}