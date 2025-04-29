#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

/*

The main idea is that we have to break up the file into set size chunks and have each thread run 
on a different chunk. We need a struct for thread data and a struct for total results. We also 
have to figure out how to look to the adjacent chunk if lets say 1 chunk ends AAA and the next chunk
begins AAB we will not add up to 5 A's, which I guess is not totally wrong but we should account for it. 

*/

// struct for individual thread data 
typedef struct {
    long start_offset; // where for the thread to start 
    long end_offset; 
    char* filename; 
    char first_char; // these 4 account for going across chunk boundaries
    uint32_t first_count; 
    char last_char; 
    uint32_t last_count; 
    int thread_id; 
    int boundary_merged; // whetheer or not we have checked if letter go across borders 
} ThreadData; 

// struct for overall data 
typedef struct {
    uint32_t* counts;   // array of number of occurences of each letter 
    char* chars;    // array of letters entered 
    int entries; 
    int capacity; 
    int size; 
} Result; 

Result* thread_results; 
pthread_mutex_t merge_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_result (Result* result, int initial_capacity) {
    // initialize in memory 
    result->counts = (uint32_t*)malloc(initial_capacity * sizeof(uint32_t));
    result->chars = (char*)malloc(initial_capacity * sizeof(char)); 
    result->entries = 0; 
    result->capacity = initial_capacity; 
}

void add_entry(Result* result, uint32_t count, char c) {
    if (result->entries >= result->capacity){
        // if there is not enough storage we have to reallocate
        result->capacity *= 2; 
        result->counts = (uint32_t*)realloc(result->counts, result->capacity * sizeof(uint32_t)); 
        result-> chars = (char*)realloc(result->chars, result->capacity * sizeof(char)); 
    }
    // otherwise 
    result->counts[result->entries] = count; 
    result->chars[result->entries] = c; 
    result->entries ++; 
}

void free_result(Result* result) {
    free(result->counts);
    free(result->chars);
    result->size = 0;
    result->capacity = 0;
}

// Get file size to divy up between threads later 
long get_file_size(const char* filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

// Thread function to perform RLE on a chunk of a file
void* thread_rle(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    char* fname = data->filename;
    long start = data->start_offset;
    long end = data->end_offset;
    
    // enter CR
    pthread_mutex_lock(&file_mutex);
    FILE* f1 = fopen(fname, "r");
    if (f1 == NULL) {
        printf("wpzip: cannot open file %s\n", fname);
        pthread_mutex_unlock(&file_mutex);
        return NULL;
    }
    
    // finds the start offset of the file 
    fseek(f1, start, SEEK_SET);
    
    // Initialize result
    Result* result = &thread_results[data->thread_id];
    init_result(result, 1024);
    
    uint32_t count = 0;
    char current;
    char c;
    
    // Read the first character
    if (start < end) {
        c = fgetc(f1);
        current = c;
        count = 1;
        start++;
    }
    
    // Process the rest of the chunk
    while (start < end) {
        c = fgetc(f1);
        start++;
        
        // if we encounter a new character restart the count 
        if (c != current) {
            add_entry(result, count, current);
            current = c;
            count = 1;
        } else {
            // otherwise increment 
            count++;
        }
    }
    
    // Store the last character and count
    if (count > 0) {
        data->last_char = current;
        data->last_count = count;
        // Only add the last entry if it's not the last chunk or it's the only chunk
        if (end != get_file_size(fname) || start == end) {
            add_entry(result, count, current);
        }
    }
    
    // Store the first character and count
    if (result->size > 0) {
        data->first_char = result->chars[0];
        data->first_count = result->counts[0];
    }
    
    fclose(f1);
    pthread_mutex_unlock(&file_mutex);
    
    return NULL;
}

// Function to merge results where character runs cross chunk boundaries
void merge_boundary_results(ThreadData* thread_data, int num_threads) {
    for (int i = 1; i < num_threads; i++) {
        ThreadData* prev = &thread_data[i-1];
        ThreadData* curr = &thread_data[i];
        
        // Skip if from different files 
        if (strcmp(prev->filename, curr->filename) != 0 || curr->boundary_merged) {
            continue;
        }
        
        // Check if the last character of the previous chunk is the same as the first character of the current chunk
        if (prev->last_char == curr->first_char) {
            // Merge the counts
            Result* prev_result = &thread_results[prev->thread_id];
            Result* curr_result = &thread_results[curr->thread_id];
            
            // If the previous result has entries, update the last entry
            if (prev_result->size > 0) {
                // Remove the first entry from the current result
                prev_result->counts[prev_result->size - 1] += curr_result->counts[0];
                
                // Shift all entries in the current result
                for (int j = 0; j < curr_result->size - 1; j++) {
                    curr_result->counts[j] = curr_result->counts[j + 1];
                    curr_result->chars[j] = curr_result->chars[j + 1];
                }
                curr_result->size--;
            }
            
            curr->boundary_merged = 1;
        }
    }
}

// Function to write all results to stdout
void write_results(Result* results, int num_threads) {
    for (int i = 0; i < num_threads; i++) {
        Result* result = &results[i];
        for (int j = 0; j < result->size; j++) {
            fwrite(&result->counts[j], 4, 1, stdout);
            fwrite(&result->chars[j], 1, 1, stdout);
        }
    }
}

