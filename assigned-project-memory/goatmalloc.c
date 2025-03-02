#include <stddef.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

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

    // if next chunk is free group together 
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

