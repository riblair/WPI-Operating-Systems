CS4513: Project 1 Goat File System
==================================

Note, this document includes a number of design questions that can help your implementation. We highly recommend that you answer each design question **before** attempting the corresponding implementation.
These questions will help you design and plan your implementation and guide you towards the resources you need.
Finally, if you are unsure how to start the project, we recommend you visit office hours for some guidance on these questions before attempting to implement this project.


Team members
-----------------

1. Jonathan (jrtinti@wpi.edu)
2. Riley (rpblair@wpi.edu)

Design Questions
------------------

1. When implementing the `debug()` function, you will need to load the file system via the emulated disk and retrieve the information for superblock and inodes.
1.1 How will you read the superblock?
1.2 How will you traverse all the inodes? 
1.3 How will you determine all the information related to an inode?
1.4 How will you determine all the blocks related to an inode?

Brief response please!
1.1 We will create an instance of the superblock struct and then look at the fields in the struct such as MagicNumber and InodeBlocks.
1.2 Traverse through the InodeBlocks referenced by the superblock by using a for loop.
1.3 We will create an instance of each Inode struct and then query the relevant fields.
1.4 We will create an instance of each data block and then query the relevant fields.
---

2. When implementing the `format()` function, you will need to write the superblock and clear the remaining blocks in the file system.

2.1 What should happen if the the file system is already mounted?
2.2 What information must be written into the superblock?
2.3 How would you clear all the remaining blocks?

Brief response please!

2.1 If the file system is already mounted, we will return false because we know it does not need to be formatted. 
2.2 The information that must be implemented into the superblock is the magic number, the number of blocks, number of inode blocks, and number of inodes. 
2.3 Create an array 128 long set all to 0 and then memcpy that into the buffer for all of the inodes. 
---

3. When implementing the `mount()` function, you will need to prepare a filesystem for use by reading the superblock and allocating the free block bitmap.

3.1 What should happen if the file system is already mounted?
3.2 What sanity checks must you perform before building up the free block bitmaps?
3.3 How will you determine which blocks are free?

Brief response please!

3.1 If the file is already mounted, we will return an error code so it is not mounted twice. 
3.2 We need to check that the disk and the superblock have the same number of blocks, inode blocks, inodes, and that the superblock has the correct magic number. 
3.3 We will create our own in memory bit map to store a 0 for a block that is free and a 1 for one that is taken. 
---

4. To implement `create()`, you will need to locate a free inode and save a new inode into the inode table.

4.1 How will you locate a free inode?
4.2 What information would you see in a new inode?
4.3 How will you record this new inode?

Brief response please!

4.1 Loop through the bit map and see which inodes have a entry of 0. 
4.2 We need to set the Valid field in the inode to 1, the size to 0, and the indirect to 0. 
4.3 We will write the new inode to that inode block. 
---

5. To implement `remove()`, you will need to locate the inode and then free its associated blocks.

5.1 How will you determine if the specified inode is valid?
5.2 How will you free the direct blocks?
5.3 How will you free the indirect blocks?
5.4 How will you update the inode table?

Brief response please!

5.1 We find the index in the inode table from the inumber and we use the superblock and find the inode block to find the inode and check the valid field
5.2 Loop through all of the direct blocks and set their value to 0. 
5.3 Read the indirect blocks if they are there and loop through and set them to 0 if they are not already. 
5.4 We use memcpy to update the table as well as updating the bitmap to show valid inodes. 
---

6. To implement `stat()`, you will need to locate the inode and return its size.

6.1 How will you determine if the specified inode is valid?
6.2 How will you determine the inode's size?

Brief response please!

6.1 We find the index in the inode table from the inumber and we use the superblock and find the inode block to find the inode and check the valid field
6.2 The inode has a field called size; we query that. 
---

7. To implement `read()`, you will need to locate the inode and copy data from appropriate blocks to the user-specified data buffer.

7.1  How will you determine if the specified inode is valid?
7.2  How will you determine which block to read from?
7.3  How will you handle the offset?
7.4  How will you copy from a block to the data buffer?

Brief response please!
7.1 We will check our bitmap for that value and see if there is a valid entry there. 
7.2 We are going to find the inode block by dividing the inumber by 128 and finding the index in the block by using modulus 128. 
7.3 For the offset, we first need to figure out the number of blocks we have to skip, if any, by dividing offset by BLOCK_SIZE and then we create a current offset by doing offset % BLOCK_SIZE. We will then update the current offset by how much we read. 
7.4 We will read from the data buffer into our direct blocks and then memcpy into our buffer. We will do this until we run out of direct blocks and then switch to indirect. 
---

8. To implement `write()`, you will need to locate the inode and copy data the user-specified data buffer to data blocks in the file system.

8.1  How will you determine if the specified inode is valid?
8.2  How will you determine which block to write to?
8.3  How will you handle the offset?
8.4  How will you know if you need a new block?
8.5  How will you manage allocating a new block if you need another one?
8.6  How will you copy to a block from the data buffer?
8.7  How will you update the inode?

Brief response please!
8.1 We will check our bitmap for that value and see if there is a valid entry there. 
8.2 We are going to find the inode block by dividing the inumber by 128 and finding the index in the block by using modulus 128. 
8.3 For the offset, we first need to figure out the number of blocks we have to skip, if any, by dividing offset by BLOCK_SIZE and then we create a current offset by doing offset % BLOCK_SIZE. We will then update the current offset by how much we write._disk
8.4 We created a bitmap for data blocks that stores when a data block is in existence and we have iterate through the bitmap to see if it is empty. 
8.5 We will search through the d_bitmap to find a free block to allocate. We then update the d_bitmap to mark it as taken, and then update the inode to reflect this change.
8.6 memcpy in a similar fashion to wfsread 
8.7 Set the inode size field equal to the bytes written and then memcpy the buffer to the inode.
---


Errata
------

Describe any known errors, bugs, or deviations from the requirements.

---

(Optional) Additional Test Cases
--------------------------------

Describe any new test cases that you developed.

