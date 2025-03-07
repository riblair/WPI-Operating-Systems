#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sched.h>

#define NUM_ITERATIONS 10000
#define MESSAGE_SIZE 1

double get_time_usec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000.0) + tv.tv_usec;
}

int main() {
    int pipe_p_2_c[2];
    int pipe_c_2_p[2];
    char message[MESSAGE_SIZE];
    pid_t pid;

    cpu_set_t set;

    CPU_ZERO(&set);    
    // Start timing
    double start_time = get_time_usec();

    if (pipe(pipe_p_2_c) < 0 || pipe(pipe_c_2_p) < 0) {
        perror("pipe failed");
        return 1;
    }
    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }
    if (pid == 0) {  
        CPU_SET(0, &set);
        if(sched_setaffinity(pid, sizeof(set), &set) == -1) {
            perror("sched failed");
            exit(1);
        }

        close(pipe_p_2_c[1]);  
        close(pipe_c_2_p[0]);  
        
        for (int i = 0; i < NUM_ITERATIONS; i++) {
            // Read from parent
            if (read(pipe_p_2_c[0], message, MESSAGE_SIZE) != MESSAGE_SIZE) {
                perror("read failed");
                exit(1);
            }
            
            // Write to parent
            if (write(pipe_c_2_p[1], message, MESSAGE_SIZE) != MESSAGE_SIZE) {
                perror("write failed");
                exit(1);
            }
        }
        
        close(pipe_p_2_c[0]);
        close(pipe_c_2_p[1]);
        exit(0);
        
    } else {  
        CPU_SET(0, &set);
        if(sched_setaffinity(pid, sizeof(set), &set) == -1) {
            perror("sched failed");
            exit(1);
        }
        close(pipe_p_2_c[0]);  
        close(pipe_c_2_p[1]);  
        
        for (int i = 0; i < NUM_ITERATIONS; i++) {
            // Write to child
            if (write(pipe_p_2_c[1], message, MESSAGE_SIZE) != MESSAGE_SIZE) {
                perror("write failed");
                exit(1);
            }
            // Read from child
            if (read(pipe_c_2_p[0], message, MESSAGE_SIZE) != MESSAGE_SIZE) {
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
        
        close(pipe_p_2_c[1]);  
        close(pipe_c_2_p[0]);
    }
    
    return 0;
}