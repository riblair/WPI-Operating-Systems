#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

/*

The main idea is that we have to break up the file into set size chunks and have each thread run 
on a different chunk. We need a struct for thread data and a struct for total results. We also 
have to figure out how to look to the adjacent chunk if lets say 1 chunk ends AAA and the next chunk
begins AAB we will not add up to 5 A's, which I guess is not totally wrong but we should account for it. 

*/

#define SMALL_FILE_THRESHOLD 512

typedef struct {
    int* counts;   // array of number of occurences of each letter 
    char* chars;    // array of letters entered 
    int entries;  
    int capacity;  
} Result; 

typedef struct {
    long start_offset; // where for the thread to start 
    long end_offset; 
    char* filename; 
    char* file_data;
    char first_char; // these 4 account for going across chunk boundaries
    int first_count; 
    char last_char; 
    int last_count; 
    int thread_id; 
    int boundary_merged; // whetheer or not we have checked if letter go across borders 
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
    
    if (count > 0) {
        data->last_char = current;
        data->last_count = count;
        if (start == end) {
            add_entry(result, count, current);
        }
    }
    
    data->first_char = result->chars[0];
    data->first_count = result->counts[0];

    // pthread_mutex_unlock(&mutex);
    
    return NULL;
}

Result* merge(Result* results[], int num_results) {
    long estimated_total = 0;
    for (int i = 0; i < num_results; i++) {
        estimated_total += results[i]->entries;
    }

    Result* merged = init_result(estimated_total);

    char last_char = 0;
    int last_count = 0;
    int first = 1; 

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
    for (int j = 0; j < big_result->entries; j++) {
        fwrite(&big_result->counts[j], 4, 1, stdout);
        fwrite(&big_result->chars[j], 1, 1, stdout);
    }
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
        threads_d[i]->thread_id = i;
        threads_d[i]->filename = filename; 
        threads_d[i]->file_data = data;
        threads_d[i]->boundary_merged = 0; 
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

int main(int argc, char **argv) {
    // If there aren't enough arguments, exit with error.
    if (argc < 2) {
        printf("wpzip: numOfThreads file1 [file2 ...]\n");
        exit(1);
    }
    /* El Plan
    One file and ALL threads
    partition sections based on file_size and num_threads
    Each thread runs RLE on its section and returns a Results struct
    We merge connected results
    We then move on to next file
    repeat until out of files...
    We write to disk.
    */

    int num_threads = atoi(argv[1]);
    int num_files = (argc-2);

    int min_chunk_size = 2048;
    long largest_file = 0;
    for (int i = 2; i < argc; i++) {
        long sz = get_file_size(argv[i]);
        if (sz > largest_file) {
            largest_file = sz; 
        }
    }

    if (num_threads == 0) {
        num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    }

    // Adjust threads based on largest file
    int lower_bound_threads = largest_file / min_chunk_size;
    if (lower_bound_threads < 1) lower_bound_threads = 1;
    if (num_threads > lower_bound_threads)
    num_threads = lower_bound_threads;

    Result* results_array[num_threads*num_files];
    for(int i = 0; i < num_threads*num_files; i++) {
        results_array[i] = init_result(largest_file / num_threads / 2);
    }

    // Go through each file.
    for (int i = 2; i < argc; i++) {
        char* filename = argv[i]; 
        per_file(results_array, i-2, filename, num_threads); 
    }
    
    int total_chunks = num_threads * num_files;
    Result* final_result = merge(results_array, total_chunks);

    write_results(final_result);   

    // Exit with success.
    exit(0);
}

