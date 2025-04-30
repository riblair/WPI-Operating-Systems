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
    char first_char; // these 4 account for going across chunk boundaries
    int first_count; 
    char last_char; 
    int last_count; 
    int thread_id; 
    int boundary_merged; // whetheer or not we have checked if letter go across borders 
    Result* r;
} ThreadData; 

Result* thread_results; 
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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
    char* fname = data->filename;
    long start = data->start_offset;
    long end = data->end_offset;
    Result* result = data->r;
    
    FILE* f1 = fopen(fname, "r");
    if (f1 == NULL) {
        printf("wpzip: cannot open file %s\n", fname);
        return NULL;
    }
    
    // finds the start offset of the file 
    fseek(f1, start, SEEK_SET);
    
    init_result(1024);
    
    int count = 0;
    char current;
    char c;
    // // enter CR
    // pthread_mutex_lock(&mutex);
    if (start < end) {
        c = fgetc(f1);
        current = c;
        count = 1;
        start++;
    }
    
    // Process the rest 
    while (start < end) {
        c = fgetc(f1);
        start++;
        
        // if we encounter a new character restart the count 
        if (c != current) {
            add_entry(result, count, current);
            current = c;
            count = 1;
        } else {
            count++;
        }
    }
    
    if (count > 0) {
        data->last_char = current;
        data->last_count = count;
        if (end != get_file_size(fname) || start == end) {
            add_entry(result, count, current);
        }
    }
    
    data->first_char = result->chars[0];
    data->first_count = result->counts[0];
    
    fclose(f1);
    // pthread_mutex_unlock(&mutex);
    
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
        
        // Check letter ran across 
        if (prev->last_char == curr->first_char) {
            Result* prev_result = &thread_results[prev->thread_id];
            Result* curr_result = &thread_results[curr->thread_id];

            // Remove the first entry from the current result
            prev_result->counts[prev_result->entries - 1] += curr_result->counts[0];
                
            // Shift 
            for (int j = 0; j < curr_result->entries - 1; j++) {
                curr_result->counts[j] = curr_result->counts[j + 1];
                curr_result->chars[j] = curr_result->chars[j + 1];
            }
            curr_result->entries--;
            curr->boundary_merged = 1;
        }
    }
}

Result* merge(Result* results[], int num_threads, int num_files) {
    //make big results object
    long entry_sum = 0;
    for(int i = 0; i < num_threads*num_files; i++) {
        entry_sum += results[i]->entries;
    }

    Result* big_results = init_result(entry_sum);

    //copy 0 into beginning of big_result 
    for(int i = 0; i < results[0]->entries; i++) {
        big_results->counts[i] = results[0]->counts[i];
        big_results->chars[i] = results[0]->chars[i];
    }
    big_results->entries = results[0]->entries;

    for(int i = 1; i < num_threads*num_files; i++) {
        // look at beginning of 1 and end of new big_results,
        // if same, merge,
        int start = 0;
        if(big_results->chars[big_results->entries-1] == results[i]->chars[0]) {
            big_results->counts[big_results->entries-1] += results[i]->counts[0];
            start++;
        }
        // copy rest of data into big results 
        for(int j = start; j < results[i]->entries; j++) {
            big_results->counts[big_results->entries] = results[i]->counts[j];
            big_results->chars[big_results->entries] = results[i]->chars[j];
            big_results->entries++;
        }
    }
    return big_results;
}

void write_results(Result* big_result) {
    for (int j = 0; j < big_result->entries; j++) {
        fwrite(&big_result->counts[j], 4, 1, stdout);
        fwrite(&big_result->chars[j], 1, 1, stdout);
    }
}

// void write_results(Result* results[], int num_threads) {
//     for (int i = 0; i < num_threads; i++) {
//         Result* result = &results[i];
//         for (int j = 0; j < result->entries; j++) {
//             fwrite(&result->counts[j], 4, 1, stdout);
//             fwrite(&result->chars[j], 1, 1, stdout);
//         }
//     }
// }

void per_file(Result* results_array[], int file_num, char* filename, int num_threads){
    long file_size = get_file_size(filename); 
    int chunk = file_size / num_threads; 

    // threads first 
    pthread_t threads[num_threads];
    ThreadData* threads_d[num_threads]; 
    for(int i = 0; i < num_threads; i++) {
        threads_d[i] = malloc(sizeof(ThreadData));
    }

    for(int i = 0; i < num_threads; i++) {

        threads_d[i]->thread_id = i;
        threads_d[i]->filename = filename; 
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

    for(int i=0; i < num_threads; i++) {
        // free(threads[i]); 
        free(threads_d[i]);
    }
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

    if(num_threads == 0) {
        // determing our own number...
        // num_threads = sysconf(_SC_NPROCESSORS_ONLN);
        num_threads = 4;
    }
    Result* results_array[num_threads*num_files];
    for(int i = 0; i < num_threads*num_files; i++) {
        results_array[i] = init_result(0xfffff);
    }

    // Go through each file.
    for (int i = 2; i < argc; i++) {
        char* filename = argv[i]; 
        per_file(results_array, i-2, filename, num_threads); 
    }
    
    Result* final_result = merge(results_array, num_threads, num_files);

    write_results(final_result);   
    
    

    // Exit with success.
    exit(0);
}

