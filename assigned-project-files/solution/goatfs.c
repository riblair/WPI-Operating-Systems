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

void create_bitmap(int inode_blocks) {
    bitmap = (uint8_t*)malloc(inode_blocks * 16 * sizeof(uint8_t));
    char inode_block[BLOCK_SIZE];
    for(int i = 0; i < inode_blocks; i++) {
        wread(i+1, inode_block);
        //update bitmap
        struct _Inode* inode = (struct _Inode*)malloc(sizeof(struct _Inode));
        for(int j = 0; j < 128; j++) {
            memcpy(inode, inode_block+j*sizeof(struct _Inode), sizeof(struct _Inode));
            if(inode->Valid) {
                bitmap[i*16+(int)(j/8)] |= (1 << (j % 8));
            }
        }
    }
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

    create_bitmap(sb->InodeBlocks);
    _disk->Mounts++;
    return SUCCESS_GOOD_MOUNT;
}

int find_free_inode(int block_index) {
    for(int i = 0; i < 16; i++) {
        if(bitmap[block_index*16+i] == 0xff) continue;
        for(int j = 0; j < 8; j++) {
            int a = bitmap[block_index*16 + i] >> j & 0x01;
            if(!a) return i*8+j;
        }
    }
    return -1;
}

void write_Inode(int block_index, int inode_index) {
    struct _Inode* inode = (struct _Inode*)malloc(sizeof(struct _Inode));
    char inode_block[BLOCK_SIZE];
    wread(block_index, inode_block);
    memcpy(inode, inode_block+inode_index*sizeof(struct _Inode), sizeof(struct _Inode));
    inode->Valid = 1;
    // do we need to set inode->direct to all zeros?
    inode->Indirect = 0;
    inode->Size = 0;
    memcpy(inode_block+inode_index*sizeof(struct _Inode),inode, sizeof(struct _Inode));
    wwrite(block_index, inode_block);
}

ssize_t create(){
    char buffer[BLOCK_SIZE];
    // Read superblock
    wread(0, buffer);
    struct _SuperBlock *sb = (struct _SuperBlock*)malloc(sizeof(struct _SuperBlock));
    memcpy(sb, buffer, sizeof(struct _SuperBlock));

    for(int i = 0; i < sb->InodeBlocks; i++) {
        // check bitmap 
        int free_index = find_free_inode(i);
        if (free_index == -1) continue; 

        write_Inode(i+1, free_index);
        bitmap[i*16+(int)(free_index/8)] |= (1 << (free_index % 8));
        return free_index;
    }
    return ERR_CREATE_INODE;
}
bool wremove(size_t inumber){
    // if it has not been mounted yet, can't remove it 
    if (!_disk->Mounts) {
        return false;
    }

    char buffer[BLOCK_SIZE];
    wread(0, buffer);
    struct _SuperBlock *sb = (struct _SuperBlock *)malloc(sizeof(struct _SuperBlock));
    memcpy(sb, buffer, sizeof(struct _SuperBlock));

    int block_index = (inumber / 128) + 1;  // +1 because inode blocks start at block 1
    int inode_offset = inumber % 128;

    // Check if block_index is valid
    if (block_index > sb->InodeBlocks) {
        free(sb);
        return false;
    }

    char inode_block[BLOCK_SIZE];
    wread(block_index, inode_block);

    // Get the inode
    struct _Inode *inode = (struct _Inode *)malloc(sizeof(struct _Inode));
    memcpy(inode, inode_block + (inode_offset * sizeof(struct _Inode)), sizeof(struct _Inode));

    if (!inode->Valid) {
        free(inode);
        free(sb);
        return false;
    }

    // Free all direct blocks
    for (int i = 0; i < 5; i++) {
        if (inode->Direct[i] == 0) {
            break;
        }
        char empty_block[BLOCK_SIZE] = {0};
        wwrite(inode->Direct[i], empty_block);
        
        // Reset the direct block reference
        inode->Direct[i] = 0;
    }

    if (inode->Indirect != 0) {
        char indirect_buffer[BLOCK_SIZE];
        wread(inode->Indirect, indirect_buffer);
        unsigned int *indirect_blocks = (unsigned int *)indirect_buffer;
        for (int i = 0; i < BLOCK_SIZE / sizeof(unsigned int); i++) {
            if (indirect_blocks[i] == 0) {
                break;
            }
            char empty_block[BLOCK_SIZE] = {0};
            wwrite(indirect_blocks[i], empty_block);
        }
        char empty_block[BLOCK_SIZE] = {0};
        wwrite(inode->Indirect, empty_block);
        
        inode->Indirect = 0;
    }

    inode->Valid = 0;
    inode->Size = 0;
    memcpy(inode_block + (inode_offset * sizeof(struct _Inode)), inode, sizeof(struct _Inode));
    wwrite(block_index, inode_block);

    bitmap[((block_index - 1) * (BLOCK_SIZE / sizeof(struct _Inode)) + inode_offset) / 8] &= ~(1 << (inode_offset % 8));
    free(inode);
    free(sb);

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
