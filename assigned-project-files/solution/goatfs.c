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
void update_dbitmap(int d_pointer) {
    int total_blocks = _disk->Blocks;
    int inode_blocks = ceil(total_blocks * 0.1);
    int index = d_pointer - inode_blocks - 1;
    
    int outer_index = index / 8;
    int inner_index = index % 8; 
    d_bitmap[outer_index] |= 0x1 << inner_index;
}

void populate_d_bitmap(struct _Inode* inode, int inode_blocks) {
    //iterate though direct & indirect blocks
    // for each block pointer, mark it as taken in the d_bitmap 
    int block_count = 0;
    while(block_count < 5) { // populate direct blocks
        unsigned int block_id = inode->Direct[block_count];
        if(!block_id) return;
        update_dbitmap(block_id); 
        block_count++;
    } 
    if (block_count == 5) {  // populate indirect blocks 
        if(!inode->Indirect) return;
        int block_iter = 0;
        char indirect_buffer[BLOCK_SIZE];
        wread(inode->Indirect, indirect_buffer);
        unsigned int* data = (unsigned int*)malloc(sizeof(unsigned int));
        memcpy(data, indirect_buffer, 4);
        while(*data){
            update_dbitmap(*data);
            memcpy(data, indirect_buffer+(++block_iter)*4, 4);
        }
    }
}

void create_bitmaps(int total_blocks, int inode_blocks) {
    i_bitmap = (uint8_t*)malloc(inode_blocks * 16 * sizeof(uint8_t));
    int data_blocks = total_blocks-inode_blocks-1;
    int num = ceil(data_blocks/8.0);
    d_bitmap = (uint8_t*)malloc(num*sizeof(uint8_t));
    char inode_block[BLOCK_SIZE];
    for(int i = 0; i < inode_blocks; i++) {
        wread(i+1, inode_block);
        //update bitmap
        struct _Inode* inode = (struct _Inode*)malloc(sizeof(struct _Inode));
        for(int j = 0; j < 128; j++) {
            memcpy(inode, inode_block+j*sizeof(struct _Inode), sizeof(struct _Inode));
            if(inode->Valid) {
                i_bitmap[i*16+(int)(j/8)] |= (1 << (j % 8));
                populate_d_bitmap(inode, inode_blocks);
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

    create_bitmaps(sb->Blocks, sb->InodeBlocks);
    _disk->Mounts++;
    return SUCCESS_GOOD_MOUNT;
}

int find_free_inode(int block_index) {
    for(int i = 0; i < 16; i++) {
        if(i_bitmap[block_index*16+i] == 0xff) continue;
        for(int j = 0; j < 8; j++) {
            int a = i_bitmap[block_index*16 + i] >> j & 0x01;
            if(!a) return i*8+j;
        }
    }
    return -1;
}

int find_free_dblock() {
    int total_blocks = _disk->Blocks;
    int inode_blocks = ceil(total_blocks * 0.1);

    for(int i = 0; i < total_blocks-inode_blocks-1; i++) {
        uint8_t bit = d_bitmap[(i/8)] & 0x1 << (i%8);
        if (!bit) {
            return i+inode_blocks+1;
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
        i_bitmap[i*16+(int)(free_index/8)] |= (1 << (free_index % 8));
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

    // TODO: exit early via checking inode table instead of reading chunk 
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

    i_bitmap[((block_index - 1) * (BLOCK_SIZE / sizeof(struct _Inode)) + inode_offset) / 8] &= ~(1 << (inode_offset % 8));
    free(inode);
    free(sb);

    return true;
}

ssize_t stat(size_t inumber) {

    if(!_disk->Mounts) {
        return -1;
    }

    int block_index = (inumber / 128) + 1; // give us the block number...
    int inode_offset = inumber % 128;


    // char buffer[BLOCK_SIZE];

    // wread(0, buffer);

    // struct _SuperBlock *sb = (struct _SuperBlock *)malloc((sizeof(struct _SuperBlock)));
    // memcpy(sb, buffer, sizeof(struct _SuperBlock));

    char inode_block[BLOCK_SIZE];
    wread(block_index, inode_block);

    // Get the inode
    struct _Inode *inode = (struct _Inode *)malloc(sizeof(struct _Inode));
    memcpy(inode, inode_block + (inode_offset * sizeof(struct _Inode)), sizeof(struct _Inode));

    uint8_t bit_block = i_bitmap[(block_index-1)*16 + (int)(inode_offset / 8)];
    bit_block &= 0x1 << (inode_offset % 8); 

    if(!bit_block) {
        return -1;
    }

    return inode->Size;
}

ssize_t wfsread(size_t inumber, char* data, size_t length, size_t offset) {

    if(!_disk->Mounts) {
        return -1;
    }

    int block_index = (inumber / 128) + 1; // give us the block number...
    int inode_offset = inumber % 128;
    
    char inode_block[BLOCK_SIZE];
    wread(block_index, inode_block);
    struct _Inode *inode = (struct _Inode *)malloc(sizeof(struct _Inode));
    memcpy(inode, inode_block + (inode_offset * sizeof(struct _Inode)), sizeof(struct _Inode));

    uint8_t bit_block = i_bitmap[(block_index-1)*16 + (int)(inode_offset / 8)];
    bit_block &= 0x1 << (inode_offset % 8); 

    if(!bit_block) {
        return -1;
    }

    // get the data_block from direct

    // length -> ceil(length / 4096) = blocks needed to read
    // int num_blocks_needed = ceil(length / 4096); 
    
    size_t bytes_left = min(length, inode->Size - offset);
    char data_buffer[BLOCK_SIZE];
    char indirect_buffer[BLOCK_SIZE] = {0};
    int direct_iter = 0;
    int indirect_iter = 0; 
    int bytes_to_read;
    size_t bytes_read = 0;
    unsigned int* indirect_pointer = (unsigned int*)malloc(sizeof(unsigned int));
    // Must first SEEK to correct block if offset > blocksize
    int blocks_to_skip = offset / BLOCK_SIZE;
    size_t cur_offset = offset % BLOCK_SIZE;
    if(blocks_to_skip > 0 && blocks_to_skip <= 5) {
        direct_iter = blocks_to_skip;
    }
    else if (blocks_to_skip > 5) {
        direct_iter = 5;
        blocks_to_skip-=5;
        indirect_iter = blocks_to_skip;
    }

    while (bytes_left) {
        if(direct_iter > 4) {
            // if we have not read an indirect block, read indirect block...
            if(indirect_buffer[0] == 0) wread(inode->Indirect, indirect_buffer); 

            // iterate through indirect block pointers until we reach the end...
            memcpy(indirect_pointer, indirect_buffer+(indirect_iter++)*4, 4);
            wread(*indirect_pointer, data_buffer);
        }
        else {
            wread(inode->Direct[direct_iter++], data_buffer);
        }
        // we read length if length < 4096 - offset, otherwise
        if((bytes_left+cur_offset) > 4096) {
            bytes_to_read = 4096 - cur_offset;
        }
        else {
            bytes_to_read = bytes_left;
        }
        memcpy(data+bytes_read, data_buffer+cur_offset, bytes_to_read);
        bytes_read+= bytes_to_read;
        bytes_left -= bytes_to_read;
        cur_offset = 0;
    }

    return bytes_read;
}

ssize_t wfswrite(size_t inumber, char* data, size_t length, size_t offset){
    if(!_disk->Mounts) {
        return -1;
    }

    int block_index = (inumber / 128) + 1; // give us the block number...
    int inode_offset = inumber % 128;
    
    char inode_block[BLOCK_SIZE];
    wread(block_index, inode_block);
    struct _Inode *inode = (struct _Inode *)malloc(sizeof(struct _Inode));
    memcpy(inode, inode_block + (inode_offset * sizeof(struct _Inode)), sizeof(struct _Inode));

    uint8_t bit_block = i_bitmap[(block_index-1)*16 + (int)(inode_offset / 8)];
    bit_block &= 0x1 << (inode_offset % 8); 

    if(!bit_block) {
        return -1;
    }
    
    size_t bytes_left = length;
    char data_buffer[BLOCK_SIZE] = {0};
    char indirect_buffer[BLOCK_SIZE] = {0};
    int direct_iter = 0;
    int indirect_iter = 0; 
    int bytes_to_write;
    size_t bytes_written = 0;
    // unsigned int new_block = 0;
    unsigned int* indirect_pointer = (unsigned int*)malloc(sizeof(unsigned int));
    // Must first SEEK to correct block if offset > blocksize
    int blocks_to_skip = offset / BLOCK_SIZE;
    size_t cur_offset = offset % BLOCK_SIZE;
    if(blocks_to_skip > 0 && blocks_to_skip <= 5) {
        direct_iter = blocks_to_skip;
    }
    else if (blocks_to_skip > 5) {
        direct_iter = 5;
        blocks_to_skip-=5;
        indirect_iter = blocks_to_skip;
    }

    while (bytes_left) {
        if ((bytes_left + cur_offset) > BLOCK_SIZE) {
            bytes_to_write = BLOCK_SIZE - cur_offset;
        } else {
            bytes_to_write = bytes_left;
        }
        if(direct_iter > 4) {
            // if we have not read an indirect block, read indirect block...
            if (indirect_buffer[0] == 0&& inode->Indirect != 0) {
                 wread(inode->Indirect, indirect_buffer); 
            } 

            // iterate through indirect block pointers until we reach the end...
            memcpy(indirect_pointer, indirect_buffer+(indirect_iter)*4, 4);
            memcpy(data_buffer + cur_offset, data + bytes_written, bytes_to_write);
            wwrite(*indirect_pointer, data_buffer);
            indirect_iter++;
        }
        else {
            int direct_pointer = inode->Direct[direct_iter];
            if(!direct_pointer) {
                // find new inode block for system
                int d_block_pointer = find_free_dblock();
                if (d_block_pointer == -1) {
                    return -1;
                }
                update_dbitmap(d_block_pointer);
                //update inode
                inode->Direct[direct_iter] = d_block_pointer;
            }
            wread(inode->Direct[direct_iter], data_buffer);
            memcpy(data_buffer + cur_offset, data + bytes_written, bytes_to_write);
            wwrite(inode->Direct[direct_iter], data_buffer);
            direct_iter++;        
        }
        // we read length if length < 4096 - offset, otherwise
        if((bytes_left+cur_offset) > 4096) {
            bytes_to_write = 4096 - cur_offset;
        }
        else {
            bytes_to_write = bytes_left;
        }
        bytes_written+= bytes_to_write;
        bytes_left -= bytes_to_write;
        cur_offset = 0;
    }
    inode->Size += bytes_written;
    memcpy(inode_block + (inode_offset * sizeof(struct _Inode)), inode, sizeof(struct _Inode));
    wwrite(block_index, inode_block);
    
    free(inode);
    free(indirect_pointer);

    return bytes_written;
}
