#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

/*

The main idea is that we have to break up the file into set size chunks and have each thread run 
on a different chunk. We need a struct for thread data and a struct for total results. We also 
have to figure out how to look to the adjacent chunk if lets say 1 chunk ends AAA and the next chunk
begins AAB we will not add up to 5 A's, which I guess is not totally wrong but we should account for it. 

*/

#define SMALL_FILE_THRESHOLD 1024

typedef struct {
    int* counts;   // array of number of occurences of each letter 
    char* chars;    // array of letters entered 
    int entries;  
    int capacity;
    Result* next;  
} Result; 

typedef struct {
    long start_offset; // starting byte for T_RLE
    long end_offset;  // end byte for T_RLE
    char* filename; 
    char* file_data;
    Result* r;
    FILE* fp;
} ThreadData; 

Result* init_result (int initial_capacity) {
    // initialize in memory 
    Result* result = (Result*)malloc(sizeof(Result));
    result->counts = (int*)malloc(initial_capacity * sizeof(int));
    result->chars = (char*)malloc(initial_capacity * sizeof(char)); 
    result->entries = 0; 
    result->capacity = initial_capacity; 
    return result;
}

// TODO: need a method to dynamically make results larger...

void free_result(Result* result) {
    free(result->counts);
    free(result->chars);
    result->capacity = 0;
}

long get_file_size(const char* filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

void add_entry(Result* result, int count, char c) {
    if (result->entries == result->capacity) {
        result->capacity *= 2;
        result->counts = realloc(result->counts, result->capacity * sizeof(int));
        result->chars = realloc(result->chars, result->capacity * sizeof(char));
    }
    result->counts[result->entries] = count; 
    result->chars[result->entries] = c; 
    result->entries++; 
}

void* thread_rle(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    long start = data->start_offset;
    long end = data->end_offset;
    Result* result = data->r;
    char* file_data = data->file_data;
    
    int count = 1;
    char current;
    char c;
    // // enter CR
    // pthread_mutex_lock(&mutex);
    c = file_data[start];
    current = c;
    count = 1;
    start++;
    
    // Process the rest 
    while (start < end) {
        c = file_data[start];
        // if we encounter a new character restart the count 
        if (c != current) {
            add_entry(result, count, current);
            current = c;
            count = 1;
        } else {
            count++;
        }
        start++;
    }

    if(count > 0 && start == end) add_entry(result, count, current);
    //update that this thread has finished. Exit condition for main thread 
    pthread_mutex_lock(&thread_count_m);
    threads_done++;
    pthread_mutex_unlock(&thread_count_m);
    // notify that this thread is done working such that other threads can spawn.
    sem_post(&thread_s);
    return NULL;
}

Result* merge(Result* result_LL, long total_threads) {
    Result* merged = init_result(total_threads * SMALL_FILE_THRESHOLD);

    char last_char = 0;
    int last_count = 0;
    int first = 1; 

    // TODO: change this to work with a LL instead of an array.
    // perhaps even better, we dont need to create a new results object all together, just amend the LL
    for (int i = 0; i < num_results; i++) {
        Result* r = results[i];
        for (int j = 0; j < r->entries; j++) {
            char c = r->chars[j];
            int count = r->counts[j];

            if (first) {
                last_char = c;
                last_count = count;
                first = 0;
            } else if (c == last_char) {
                // Merge with previous
                last_count += count;
            } else {
                // Push previous and start new
                add_entry(merged, last_count, last_char);
                last_char = c;
                last_count = count;
            }
        }
    }

    // Push the final accumulated entry
    if (!first) {
        add_entry(merged, last_count, last_char);
    }

    return merged;
}

void write_results(Result* big_result) {
    // this could very easily be adapted to work with LL, might want to do this
    for (int j = 0; j < big_result->entries; j++) {
        fwrite(&big_result->counts[j], 4, 1, stdout);
        fwrite(&big_result->chars[j], 1, 1, stdout);
    }
}

ThreadData* init_threadData_array(Result* head_iter, char* filename, long file_size, int threads_per_file) {
    // initialize threaddata objects for use in threads
    int fd = open(filename, O_RDONLY); 
    int use_mmap = file_size > SMALL_FILE_THRESHOLD;
    int chunk = file_size / threads_per_file; 
    // threads first 
    // pthread_t threads[num_threads];
    ThreadData* threads_d[threads_per_file]; 
    char* data = NULL;

    Result* curr_iter = head_iter;

    if (use_mmap) {
        // mmap the entire file 
        data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    }
    else {
        data = malloc(file_size); 
        ssize_t read_bytes = read(fd, data, file_size);
        read_bytes = read_bytes; 
    }
    for(int i = 0; i < threads_per_file; i++) {
        threads_d[i] = malloc(sizeof(ThreadData));
        threads_d[i]->filename = filename; 
        threads_d[i]->file_data = data;
        threads_d[i]->start_offset = (chunk * i);
        threads_d[i]->r = curr_iter;

        curr_iter->next = init_result(SMALL_FILE_THRESHOLD / 4); 
        curr_iter = curr_iter->next;

        if (i != threads_per_file - 1) {
            threads_d[i]->end_offset = (chunk * (i+1));  
        } else {
            threads_d[i]->end_offset = file_size;
        }
    }
    head_iter = curr_iter;
    return threads_d;

}

void per_file(Result* results_array[], int file_num, char* filename, int num_threads){
    int fd = open(filename, O_RDONLY); 

    long file_size = get_file_size(filename); 
    int use_mmap = file_size > SMALL_FILE_THRESHOLD;
    int chunk = file_size / num_threads; 
    // threads first 
    pthread_t threads[num_threads];
    ThreadData* threads_d[num_threads]; 
    char* data = NULL;

    if (use_mmap) {
        // mmap the entire file 
        data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    }
    else {
        data = malloc(file_size); 
        ssize_t read_bytes = read(fd, data, file_size);
        read_bytes = read_bytes; 
    }

    for(int i = 0; i < num_threads; i++) {
        threads_d[i] = malloc(sizeof(ThreadData));
        threads_d[i]->filename = filename; 
        threads_d[i]->file_data = data;
        threads_d[i]->start_offset = (chunk * i);
        threads_d[i]->r = results_array[i+file_num*num_threads];
        if (i != num_threads - 1) {
            threads_d[i]->end_offset = (chunk * (i+1));  
        } else {
            threads_d[i]->end_offset = file_size;
        }
        pthread_create(&threads[i], NULL, thread_rle, threads_d[i]); 
    }
    
    // wait for all threads to finish...
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL); 
    }

    if (use_mmap) {
        munmap(data, file_size); 
    }
    else {
        free(data); 
    }
    close(fd); 
    
}

Result* head;
Result * head_iter;
pthread_mutex_t* thread_count_m;
sem_t* thread_s;
long file_size;
int threads_per_file;
int threads_done = 0;

/* 
        El Plan
        One file and ALL threads
        partition sections based on file_size and num_threads
        Each thread runs RLE on its section and returns a Results struct
        We merge connected results
        We then move on to next file
        repeat until out of files...
        We write to disk.
    */

int main(int argc, char **argv) {
    // If there aren't enough arguments, exit with error.
    if (argc < 2) {
        printf("wpzip: numOfThreads file1 [file2 ...]\n");
        exit(1);
    }

    int num_files = (argc-2);
    int num_threads = atoi(argv[1]);
    if (num_threads == 0) {
        num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    }

    sem_init(thread_s, 0, num_threads);
    pthread_mutex_init(thread_count_m, NULL);

    head = init_result(SMALL_FILE_THRESHOLD / 4);
    head_iter = head;
    pthread_t* thread_ptr; // used to point at the last thread for this 
    int total_threads = 0;
    // Go through each file.
    for (int i = 2; i < argc; i++) {
        char* filename = argv[i]; 
        file_size = get_file_size(filename);
        printf("File_size: %ld\n", file_size);
        // dynamically allocate threads based on size
        threads_per_file = file_size / SMALL_FILE_THRESHOLD;
        total_threads+=threads_per_file;
        ThreadData** threads_d = init_threadData_array(head_iter, filename, file_size, threads_per_file);

        for(int j = 0; j < threads_per_file; j++) {
            sem_wait(&thread_s);
            pthread_create(malloc(sizeof(pthread_t)), NULL, thread_rle, threads_d[j]);
        }
    }
    // wait for all threads to finish.
    while(threads_done != total_threads) {}
    
    Result* final_result = merge(head, total_threads);
    write_results(final_result);   
    // Exit with success.
    exit(0);
}

/* summary of changes I wanna make

    results_array should be a linked_list 
        additions need to happen in order, and should be gaurded by a primative
    we should dynamically allocate a set of threads per file based on size
        We should use a semaphore to gate creation of new threads
*/ 