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

int display_super_block(struct _SuperBlock* sb) {
    printf("SuperBlock:\n");
    if (sb->MagicNumber == MAGIC_NUMBER) {
        printf("    magic number is valid\n");
    } else {
        printf("    magic number is invalid\n");
        return 1;
    }
    
    printf("    %u blocks\n", sb->Blocks);
    printf("    %u inode blocks\n", sb->InodeBlocks);
    printf("    %u inodes\n", sb->Inodes);
    return 0;
}

void display_inode(struct _Inode* inode, int position) {
    printf("Inode %d:\n", position);
    printf("    size: %u bytes\n", inode->Size);
    // need to print each block
    printf("    direct blocks:");
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
        printf("    indirect block: %u\n", inode->Indirect);
        printf("    indirect data blocks:");
        char indirect_buffer[BLOCK_SIZE];
        wread(inode->Indirect, indirect_buffer);
        int block_iter = 0;
        unsigned int* data = (unsigned int*)malloc(sizeof(unsigned int));
        memcpy(data, indirect_buffer, 4);
        while(*data){
            printf(" %u", *data);
            memcpy(data, indirect_buffer+(++block_iter)*4, 4);
        }
        printf("\n");
    }
}

void debug(){
    // Buffer to hold block data
    char buffer[BLOCK_SIZE];
    
    // Read superblock
    wread(0, buffer);
    
    struct _SuperBlock *sb = (struct _SuperBlock*)malloc(sizeof(struct _SuperBlock));
    memcpy(sb, buffer, sizeof(struct _SuperBlock));
    
    // Verify magic number and display superblock info
    if (display_super_block(sb)) return;
    
    struct _Inode* inode = (struct _Inode*)malloc(sizeof(struct _Inode));
    for(int i = 0; i < sb->InodeBlocks; i++) {
        char inode_block[BLOCK_SIZE];
        wread(i+1, inode_block);
        for(int j = 0; j < 128; j++) {
            memcpy(inode, inode_block+j*sizeof(struct _Inode), sizeof(struct _Inode));
            if(!inode->Valid) continue;
            display_inode(inode, j);
        }
    }
}

bool format(){
    if(_disk->Mounts) return false;

    int blocks  = _disk->Blocks;
    int inode_blocks = (int)ceil(blocks * .1);
    struct _SuperBlock *sb = (struct _SuperBlock*)malloc(sizeof(struct _SuperBlock));

    sb->MagicNumber = MAGIC_NUMBER;
    sb->Blocks = blocks;
    sb->InodeBlocks = inode_blocks;
    sb->Inodes = 128*inode_blocks; 
    
    char empty_buffer[BLOCK_SIZE] = {0}; 
    memcpy(empty_buffer, sb, sizeof(struct _SuperBlock));
    wwrite(0, empty_buffer);
    char empty_buffer2[BLOCK_SIZE] = {0}; 
    for(int i = 1; i < blocks; i++) {
        wwrite(i, empty_buffer2);
    }
    return true;
}

int mount(){
    if(_disk->Mounts) return ERR_BAD_MAGIC_NUMBER;
    char buffer[BLOCK_SIZE];
    // Read superblock
    wread(0, buffer);
    struct _SuperBlock *sb = (struct _SuperBlock*)malloc(sizeof(struct _SuperBlock));
    memcpy(sb, buffer, sizeof(struct _SuperBlock));
    int inode_blocks = (int)ceil(_disk->Blocks * .1);

    if(sb->MagicNumber != MAGIC_NUMBER) return ERR_BAD_MAGIC_NUMBER; 
    if(sb->Blocks != _disk->Blocks) return ERR_NOT_ENOUGH_BLOCKS;
    if(sb->InodeBlocks != inode_blocks) return ERR_NOT_ENOUGH_INODES;
    if(sb->Inodes != inode_blocks*128) return ERR_CREATE_INODE; 

    for(int i = 0; i < inode_blocks; i++) {
        wread(i+1, buffer);
        // do something...
    }
    _disk->Mounts++;
    return SUCCESS_GOOD_MOUNT;
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
