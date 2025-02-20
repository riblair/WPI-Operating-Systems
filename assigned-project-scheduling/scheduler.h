#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>


#define MIN(a,b) (a < b ? a : b)


struct job {
    int id;
    int arrival_time;
    int length;
    int scheduled;
    // other such neccesities?
    // def need some for metrics...
    int original_length;  
    int start_time;      
    int wait_time;       
    int last_run;
    struct job *next;
};

struct metrics {
    float num_jobs;
    float sum_turnaround_time;
    float sum_response_time;
    float sum_wait_time; 
    struct job_stats* stats_head;
};

struct job_stats {
    int id;
    int response_time;
    int turnaround_time;
    int wait_time;
    struct job_stats* next;
};

struct args {
    int analysis_mode;
    char policy[5];
    char file_path[100];
    int timeslice;
};

void copy_job(struct job* dst, struct job* src) {
    dst->id             = src->id;
    dst->arrival_time   = src->arrival_time;
    dst->length         = src->length;
    dst->scheduled      = src->scheduled;
    dst->next           = src->next;
    dst->original_length= src->length;  
    dst->start_time     = -1;    
    dst->wait_time      = 0;      
    dst->last_run       = -1;
}
/* Environment Setup*/
struct args* arg_parse(char**);
struct job* create_job_queue(char*);
int count_jobs(struct job*);

/* Policy Handlers*/
struct job* handler_FIFO(struct job**);
struct job* handler_SJF(struct job**);
struct job* handler_RR(struct job**);

/* Run handlers*/
int remove_finished_jobs(struct job**);
void shift_job_queue(struct job**);
void add_new_jobs(struct job**, struct job**, int);
void handle_run(struct job*, int, struct metrics*, struct job*(struct job**));
void analyze_run(struct metrics*);