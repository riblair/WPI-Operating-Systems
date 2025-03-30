// Goat File System Implementation
// if you would like to start from scratch, start here! Just note that "make" won't work until you have completed some basic skeleton code. 


// if you are looking for a skeleton code, look at at ".goatfs.c" and copy the headers and empty functions here; you should be able to run "make" in the top-level directory without any errors.

// here is the skeleton code;
// to use, copy everything to the "goatfs.c"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "disk.h"
#include "goatfs.h"


void debug(){
    // Buffer to hold block data
    int num_reads = 0; 
    char buffer[BLOCK_SIZE];
    
    // Read superblock
    wread(0, buffer);
    num_reads++; 
    
    char super_buffer[sizeof(struct _SuperBlock)];
    strncpy(super_buffer, buffer, 16);
    // Cast the buffer to a superblock structure
    // must only cast first 16 bytes into a struct
    struct _SuperBlock *sb = (struct _SuperBlock *)super_buffer;
    
    // Verify magic number and display superblock info
    printf("SuperBlock:\n");
    if (sb->MagicNumber == MAGIC_NUMBER) {
        printf("magic number is valid\n");
    } else {
        printf("magic number is invalid\n");
        return;
    }
    
    printf("\t%u blocks\n", sb->Blocks);
    printf("\t%u inode blocks\n", sb->InodeBlocks);
    printf("\t%u inodes\n", sb->Inodes);

    for(int i = 0; i < sb->InodeBlocks; i++) {

        char inode_block[BLOCK_SIZE];
        char inode_buffer[sizeof(struct _Inode)];

        wread(i+1, inode_buffer);
        num_reads++; 
        strncpy(inode_buffer, inode_block, 32);
        
        struct _Inode *inode = (struct _Inode*)inode_buffer;

        if(!inode->Valid) {
            continue;
        }
        printf("Inode %d:\n", i);
        printf("\tsize: %u\n", inode->Size);
        // need to print each block
        printf("\tdirect blocks:");
        int block_count = 0;
        
        while(block_count < 5) { // printing direct blocks
            unsigned int block_id = inode->Direct[block_count];
            if (block_id) {
                printf(" %u", block_id);
                block_count++;
            }
            else {
                break;
            }
        } 
        printf("\n");
        if (block_count == 5) { // print indirect blocks
            printf("\tindirect block: %u\n", inode->Indirect);
        }


        printf("\t%u inodes\n", sb->Inodes);
        printf("\t%u inodes\n", sb->Inodes);

        // 
        // int num_per_block = 128; 
        
        // for (int j = 0; j < num_per_block; j++){

        // }
    }
}


bool format(){

    return true;
}

int mount(){
    return -1;
}

ssize_t create(){
    return 0;
}
bool wremove(size_t inumber){
    return true;
}
ssize_t stat(size_t inumber) {
    return 0;
}
ssize_t wfsread(size_t inumber, char* data, size_t length, size_t offset){
    return 0;
}
ssize_t wfswrite(size_t inumber, char* data, size_t length, size_t offset){
    return 0;
}
