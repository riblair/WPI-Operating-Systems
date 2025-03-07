#include <stddef.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "goatmalloc.h" 

// pointer to arena 
void *_arena_start = NULL; 
size_t _arena_size = 0;    
int statusno = 0; 

int init(size_t size) {
    // if the size does not fit in our constraints error out 
    if (size <= 0 || size > MAX_ARENA_SIZE) {
        fprintf(stderr, "Initializing arena:\n... requested size %zu bytes\n", size);
        statusno = ERR_BAD_ARGUMENTS;
        return statusno;
    }

    // page size 
    int page_size = getpagesize();
    printf("... pagesize is %d bytes\n", page_size);

    // adjust size based on page size 
    size_t adjusted_size = ((size + page_size - 1) / page_size) * page_size;
    printf("... adjusting size with page boundaries\n");
    printf("... adjusted size is %zu bytes\n", adjusted_size);

    // given code in project 
    int fd = open("/dev/zero", O_RDWR);
    if (fd == -1) {
        perror("open");
        statusno = ERR_SYSCALL_FAILED;
        return statusno;
    }

    _arena_start = mmap(NULL, adjusted_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);

    // failed to create arena 
    if (_arena_start == MAP_FAILED) {
        perror("mmap");
        statusno = ERR_SYSCALL_FAILED;
        return statusno;
    }

    // create start and end for the arena 
    _arena_size = adjusted_size;
    printf("... mapping arena with mmap()\n");
    printf("... arena starts at %p\n", _arena_start);
    printf("... arena ends at %p\n", (void *)((char *)_arena_start + _arena_size));

    // Initialize the chunk list with one large free node
    node_t *initial_node = (node_t *)_arena_start;
    initial_node->size = _arena_size - sizeof(node_t);
    initial_node->is_free = 1;
    initial_node->fwd = NULL;
    initial_node->bwd = NULL;

    printf("... initializing header for initial free chunk\n");
    printf("... header size is %zu bytes\n", sizeof(node_t));

    statusno = (int)_arena_size;
    return statusno;
}

int destroy() {
    if (_arena_start == NULL) {
        statusno = ERR_UNINITIALIZED;
        return statusno;
    }

    printf("Destroying Arena:\n");
    printf("... unmapping arena with munmap()\n");

    if (munmap(_arena_start, _arena_size) == -1) {
        perror("munmap");
        statusno = ERR_SYSCALL_FAILED;
        return statusno;
    }

    _arena_start = NULL;
    _arena_size = 0;
    statusno = 0;

    return statusno;
}

void* walloc(size_t size) {
    if (_arena_start == NULL) {
        statusno = ERR_UNINITIALIZED;
        return NULL;
    }

    printf("Allocating memory:\n");
    printf("... looking for free chunk of >= %zu bytes\n", size);

    node_t *current = (node_t *)_arena_start;
    while (current) {
        // checking if there a free chunk that fits in this size 
        if (current->is_free && current->size >= size) {
            printf("... found free chunk of %zu bytes with header at %p\n", current->size, (void *)current);
            printf("... free chunk -> fwd currently points to %p\n", (void *)current->fwd);
            printf("... free chunk -> bwd currently points to %p\n", (void *)current->bwd);

            // if the size is big enough to fit more than just this memory split it 
            if (current->size >= size + sizeof(node_t) + 1) {
                printf("... splitting required\n");
                // go to end of this chunk's use 
                node_t *new_chunk = (node_t *)((char *)current + sizeof(node_t) + size);
                new_chunk->size = current->size - size - sizeof(node_t);
                new_chunk->is_free = 1;
                new_chunk->fwd = current->fwd;
                new_chunk->bwd = current;
                // set the back for new chunk 
                if (current->fwd) {
                    current->fwd->bwd = new_chunk;
                }
                // update chunk we just allocated 
                current->fwd = new_chunk;
                current->size = size;
            } else {
                printf("... splitting not required\n");
            }

            current->is_free = 0;
            printf("... updating chunk header at %p\n", (void *)current);
            printf("... being careful with my pointer arithmetic and void pointer casting\n");

            return (void *)((char *)current + sizeof(node_t));
        }
        current = current->fwd;
    }

    statusno = ERR_OUT_OF_MEMORY;
    return NULL;
}

void wfree(void *ptr) {
    if (ptr == NULL || ptr < _arena_start || ptr >= (void *)((char *)_arena_start + _arena_size)) {
        fprintf(stderr, "Invalid pointer supplied\n");
        return;
    }

    printf("Freeing allocated memory:\n");
    printf("... supplied pointer %p:\n", ptr);
    printf("... being careful with my pointer arithmetic and void pointer casting\n");

    // get header of chunk 
    node_t *chunk = (node_t *)((char *)ptr - sizeof(node_t));
    printf("... accessing chunk header at %p\n", (void *)chunk);
    printf("... chunk of size %zu\n", chunk->size);

    chunk->is_free = 1;
    ptr = chunk; 

    // if next chunk is free, group together 
    printf("... checking if coalescing is needed\n");
    if (chunk->fwd && chunk->fwd->is_free) {
        printf("... coalescing with next chunk\n");
        chunk->size += sizeof(node_t) + chunk->fwd->size;
        chunk->fwd = chunk->fwd->fwd;
        if (chunk->fwd) {
            chunk->fwd->bwd = chunk;
        }
    }

    // if chunk behind is free 
    if (chunk->bwd && chunk->bwd->is_free) {
        printf("... coalescing with previous chunk\n");
        chunk->bwd->size += sizeof(node_t) + chunk->size;
        chunk->bwd->fwd = chunk->fwd;
        if (chunk->fwd) {
            chunk->fwd->bwd = chunk->bwd;
        }
    }
    
}

void* wrealloc(void* ptr, size_t size){
    if(ptr == NULL){
        return walloc(size); 

    }else if (size == 0){
        wfree(ptr); 
        return walloc(0); // ptr
    }

    /* 
        If fwd chunk is free and size of current chunk + fwd chunk > size, 
            split fwd into two, combine current and fwd_1, adjust headers, adjust free_list, and return ptr.      
        else
            find a chunk that is large enough to fit size. 
                If no chunks are available, return NULL
            else
                Copy data/headers, adjust freelist, free old chunk, return ptr. 
    */

    // get pointer to header
    node_t *current = (node_t *)((char *)ptr - sizeof(node_t));

    // Check if next chunk is free and if combined they're big enough
    node_t *next_chunk = current->fwd;
    if (next_chunk && next_chunk->is_free) {
        size_t free_forward_space = current->size + sizeof(node_t) + next_chunk->size;
        
        if (free_forward_space >= size) {
            // If there's enough space to split 
            if (free_forward_space - size >= sizeof(node_t) + 1) {
                // Create a new free chunk 
                node_t *new_free = (node_t *)((char *)current + sizeof(node_t) + size);
                // outside of the combined space 
                // the last section of the original 2 chunks that is not apart of the realloc 
                new_free->size = free_forward_space - size - sizeof(node_t);
                new_free->is_free = 1;
                new_free->fwd = next_chunk->fwd;
                new_free->bwd = current;
                
                // Update the forward 
                current->fwd = new_free;
                
                // Update the backward 
                if (new_free->fwd) {
                    new_free->fwd->bwd = new_free;
                }
                
                current->size = size;
            } else {
                // Take the entire combined space
                current->size = free_forward_space;
                current->fwd = next_chunk->fwd;
                
                // Update backward pointer 
                if (next_chunk->fwd) {
                    next_chunk->fwd->bwd = current;
                }
            }
            return ptr;
        }
    }

    // COPY SECTION
    // not sure if it works yet 
    // If the fwd is not free we have to find a new space 
    void *new_ptr = walloc(size);
    if (!new_ptr) {
        // out of arena 
        return NULL; 
    }
    
    // Copy data case 
    size_t copy_size;
    if (current->size < size) {
        copy_size = current->size;
    } else {
        copy_size = size;
    }
    memcpy(new_ptr, ptr, copy_size);
    
    // Free the old chunk
    wfree(ptr);
    
    return new_ptr;
}
