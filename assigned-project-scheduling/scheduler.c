#include "scheduler.h"

struct args* arg_parse(char* argv[]) {
    struct args* args = malloc(sizeof(struct args));
    for(int i = 1; i < 5; i++) {
        switch (i)
        {
        case 1: // analysis mode
            args->analysis_mode = atoi(argv[i]);
            break;
        case 2: // policy type
            strncpy(args->policy, argv[i], 5);
            break;
        case 3: // input file_path
            strncpy(args->file_path, argv[i], 100);
            break;
        case 4: // analysis mode
            args->timeslice = atoi(argv[i]);
            break;
        }
    }
    return args;
}

struct job* create_job_queue(char* file_path) {
    
    FILE* in_file = fopen(file_path, "r");
    if(in_file == NULL) return NULL;
    char* line = malloc(100* sizeof(char));
    size_t len = 0;
    __ssize_t nread;
    int ids = 0;


    struct job* job_head = malloc(sizeof(struct job));
    struct job* job_iter = job_head;
    struct job* job_iter2;

    while((nread = getline(&line, &len, in_file)) != -1) {
        job_iter2 = job_iter;
        job_iter->id = ids++;
        job_iter->arrival_time = atoi(strtok(line, ","));
        job_iter->length = atoi(strtok(NULL, ","));

        job_iter->next = malloc(sizeof(struct job));
        job_iter = job_iter->next;
    }
    free(job_iter);
    job_iter2->next = NULL;
    return job_head;
}

int count_jobs(struct job* job_head) {
    int count = 0;
    struct job* job_iter = job_head;

    while(job_iter != NULL) {
        count++;
        job_iter = job_iter->next;
    }

    return count;
}

/* 
    FIFO Selects first job that has a non-zero length 
    Assuptions: 
        Jobs will have non-zero lengths
        Finished jobs will be removed from the queue
*/
struct job* handler_FIFO(struct job** job_queue) {
    return *job_queue;
}

struct job* handler_SJF(struct job** job_queue) {
    // loop through jobs 
    // check sim time 
    // check for shortest job length 
    int shortest_length = INT_MAX; 
    struct job* chosen_job = NULL; 
    struct job* job_iter = *job_queue; 
    while(job_iter != NULL) {
        // check length and compare to shortest length 
        // if arrival is not sim time break 
        if (job_iter->length < shortest_length){
            shortest_length = job_iter->length;
            chosen_job = job_iter; 
        }
        job_iter = job_iter->next; 
    }
    return chosen_job; 
}

/* 
    RR Selects jobs on a rotating basis. 
    Assuptions: 
        The first element is always the one that is next to be queued
        Queued jobs that do not finish will be pushed to the back
        Finished jobs will be removed from the queue
*/
struct job* handler_RR(struct job** job_queue) {
    return *job_queue;
}


int remove_finished_jobs(struct job** job_queue) {
    if(*job_queue == NULL) {
        return 0;
    }
     /* get rid of elements that have length 0 */
    // there should only be ONE element max that needs to get removed.
    int removed = 0;
    struct job* job_q_iter = *job_queue;
    job_q_iter = job_q_iter->next;
    struct job* job_q_iter2 = *job_queue;

    if(job_q_iter2->length == 0) { // head needs to be removed
        struct job* tmp = *job_queue;
        if(job_q_iter != NULL) {
            *job_queue = job_q_iter;
        }
        else {
            *job_queue = NULL;
        }
        free(tmp);
        removed = 1;
    }

    while(job_q_iter != NULL && !removed) {
        if(job_q_iter->length == 0) {
            struct job* tmp = job_q_iter;
            job_q_iter2->next = job_q_iter->next;
            free(tmp);
            removed = 1;
        }
        else {
            job_q_iter2 = job_q_iter;
            job_q_iter = job_q_iter->next;
        }
    }
    return removed;
}

void shift_job_queue(struct job** job_queue) {
    if(*job_queue == NULL) {
        return;
    }
    struct job* job_q_iter = *job_queue;
    // make job_q_iter_end point to last element
    while(job_q_iter->next != NULL) {
        job_q_iter = job_q_iter->next;
    }
    /* Shift first element to end of queue*/
    // Not single element queue
    if(job_q_iter != *job_queue) {
        // shift first element of queue to the end
        struct job* tmp = *job_queue;
        job_q_iter->next = tmp;
        *job_queue = tmp->next;
        tmp->next = NULL;
    }
}
/* Add new jobs to end of queue*/
void add_new_jobs(struct job** job_queue, struct job** job_head, int sim_time) {
    struct job* job_head_iter = *job_head;
    struct job* job_q_iter = *job_queue;
    // make job_q_iter_end point to last element
    if(job_q_iter != NULL) {
        while(job_q_iter->next != NULL) job_q_iter = job_q_iter->next;
    }


    /* scan through job_head for new unadded elements*/
    while(job_head_iter != NULL) {
        if(!job_head_iter->scheduled && job_head_iter->arrival_time <= sim_time) {
                job_head_iter->scheduled = 1;
                // make copy and append to end of queue
                struct job* tmp = malloc(sizeof(struct job));
                copy_job(tmp, job_head_iter);
                tmp->next = NULL; //dont want to copy the next val
                if(*job_queue == NULL) { // empty queue
                    *job_queue = tmp;
                    job_q_iter = *job_queue;
                }
                else { // append to next and iterate 
                    job_q_iter->next = tmp;
                    job_q_iter = tmp;
                }
            }
        job_head_iter = job_head_iter->next;
    }
}

void print_job_queue(struct job** job_queue) {
    struct job* job_iter = *job_queue;
    printf("Start of Job Q\n");
    while(job_iter != NULL) {
        printf("Job [%d, %p], at: %d, l: %d, next: %p\n", 
            job_iter->id, &job_iter, job_iter->arrival_time, 
            job_iter->length, job_iter->next);
        job_iter = job_iter->next;
    }
    printf("End of Job Q\n");
}

void insert_stats_sorted(struct job_stats** head, struct job_stats* new_stats) {
    struct job_stats* current;
    
    // check if empty 
    if (*head == NULL || (*head)->id > new_stats->id) {
        new_stats->next = *head;
        *head = new_stats;
        return;
    }
    current = *head;
    while (current->next != NULL && current->next->id < new_stats->id) {
        current = current->next;
    }
    // Insert the new node
    new_stats->next = current->next;
    current->next = new_stats;
}

void handle_run(struct job** job_head, int timeslice, struct metrics* run_metrics, struct job* policy_func(struct job**)) {
    int sim_time = 0, runtime;
    struct job* job_curr;

    int jobs_run = 0;
    int jobs_total = count_jobs(*job_head);

    struct job** job_queue = malloc(sizeof(struct job*));
    add_new_jobs(job_queue, job_head, sim_time); 
    while(jobs_run < jobs_total) {
        // print_job_queue(job_queue);
        job_curr = policy_func(job_queue);
        if(job_curr == NULL) {
            sim_time++;
            add_new_jobs(job_queue, job_head, sim_time);
        }
        else {  
            // setting start time 
            if (job_curr->start_time == -1) {
                job_curr->start_time = sim_time;
                job_curr->wait_time = sim_time - job_curr->arrival_time;
            }

            if (job_curr->last_run != -1) {
                job_curr->wait_time += sim_time - job_curr->last_run;  
            }
            job_curr->last_run = sim_time;
            runtime = timeslice ? MIN(timeslice, job_curr->length) : job_curr->length;
            job_curr->length -= runtime;

            printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", 
                sim_time, job_curr->id, job_curr->arrival_time, runtime);
            fflush(stdout);
            job_curr->last_run = sim_time + runtime; 
            sim_time += runtime;
            add_new_jobs(job_queue, job_head, sim_time);
            if(job_curr->length == 0) {
                // job is done configure the stats 
                struct job_stats* stats = malloc(sizeof(struct job_stats)); 
                stats->id = job_curr->id;
                stats->response_time = job_curr->start_time - job_curr->arrival_time;
                stats->turnaround_time = sim_time - job_curr->arrival_time;
                stats->wait_time = job_curr->wait_time;
                stats->next = NULL;
                // Insert into stats list in order of job ID
                insert_stats_sorted(run_metrics->stats_head, stats);
                run_metrics->sum_response_time += stats->response_time;
                run_metrics->sum_turnaround_time += stats->turnaround_time;
                run_metrics->sum_wait_time += stats->wait_time;
                remove_finished_jobs(job_queue);   
                jobs_run++;
            }
            else {
                shift_job_queue(job_queue);
            }
        }
    }
}

void analyze_run(struct metrics* run_metrics, char* policy) {
    printf("Begin analyzing %s:\n", policy); 
    struct job_stats* stats = *run_metrics->stats_head;
    while (stats != NULL) {
        printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n",
               stats->id,
               stats->response_time,
               stats->turnaround_time,
               stats->wait_time);
        stats = stats->next;
    }
     
    // get the averages 
    float avg_response = run_metrics->sum_response_time / run_metrics->num_jobs;
    float avg_turnaround = run_metrics->sum_turnaround_time / run_metrics->num_jobs;
    float avg_wait = run_metrics->sum_wait_time / run_metrics->num_jobs;
    printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n", avg_response, avg_turnaround, avg_wait);
    printf("End analyzing %s.\n", policy);
    return;
}

int main(int argc, char* argv[]) {
    // four args on input
    /*
    Four inputs on cmd line
    1. analysis mode -> 0/1 disable/enable
    2. name of scheduler {"FIFO", "SJF", "RR"}
    3. name of workload file ".in files"
    4. timeslice duration...
    
    jobs are listed in order of arrival,
    ordered as pairs (arrival-time, run-time)
    all times are integers

    jobs should have ids based on arrival, and be stored in a linked-list.

    Should simulate execution and print out arrival times, and run times
    "t=0: [Job 0] arrived at [0], ran for: [3]"
    */

    if(argc != 5) {
        printf("Incorrect amount of args inputs. Expected ./scheduler [0/1] [policy {'FIFO', 'SJF', 'RR'}] ['input_file_path.in'] [timeslice duration]");
        exit(1);
    }
    struct args* args = arg_parse(argv);
    struct job* job_head = create_job_queue(args->file_path);
    struct metrics* run_metrics = malloc(sizeof(struct metrics));
    run_metrics->num_jobs = count_jobs(job_head);
    
    struct job* (*policy_func)(struct job**);
    run_metrics->stats_head = malloc(sizeof(struct job_stats*));
    *(run_metrics->stats_head) = NULL;
    
    if(!strncmp("FIFO", args->policy, 4)) policy_func = handler_FIFO;
    else if(!strncmp("SJF", args->policy, 3)) policy_func = handler_SJF;
    else if(!strncmp("RR", args->policy, 2)) policy_func = handler_RR;
    else {
        printf("UNEXPECTED POLICY: %s, \n", args->policy);
        exit(1);
    }
    printf("Execution trace with %s:\n", args->policy);
    handle_run(&job_head, args->timeslice, run_metrics, policy_func);
    printf("End of execution with %s.\n", args->policy);
    if(args->analysis_mode) analyze_run(run_metrics, args->policy);
    return 0;
}