Project 06: Concurrency
==========================

Members
-------

1. Jonathan Tinti (jrtinti@wpi.edu)
2. Riley Blair (rpblair@wpi.edu)

Design: Correctness (40 points)
------

> 1. How do you parallelize the compression process to maximize the underlying physical resources (i.e., CPUs)? 
>    - How did you determine the number of threads to use for the wpzip?  (4 points)
>    - How did you break down the compression process into different tasks?  (4 points) 
>    - What are those tasks? (4 points)
>    - Which tasks were parallelized and which are serial? (4 points)

Response.

### Paralleization
    We break the file up into sections that the threads do the RLE algortihm on, then we merge the boundaries, and finally write the full buffer to a file.
    We broke down the compression process into tasks that needed to be run in serial, and the ones that can be parallelized.
    The three main tasks we identified were runnning the RLE algorithm on a section of a file, writing the encoded data to disk, and merging encoded sections.
    Running the RLE algorithm is parallelized as is merging while writing the encoded data is serial. 


> 2. How did you employ concurrency mechanisms to ensure correctness, avoid deadlocks, and coordinate threads?
>    - What synchronization primitives (mutex, condition variables, semaphores) did you use? (4 points)
>    - How was each primitive used? (6 points)
>    - Justify how your design ensures correctness. (10 points)
>    - How did you design to maximize the parallelism? (4 points)

Responses.

Our first implementation did not require any synchronization primitives as the sections ran in parallel have no race conditions. We syncronize the reading of a file by waiting until all child threads have terminated using pthread_join before continuing onto the next file.
No race conditions existed due to the lack of critical regions in our code, 
and no deadlocks or livelocks could occur because there were no primatives in use. This made our first implementation determanistic.

Our final implementation utilized two synchronization primatives: a semaphore and a mutex. Additionally, we cleverly designed a linked-list to ensure the encoding maintained its order regardless of thread execution status.
The semaphore was used to gate the number of spawned threads, ensuring we only create threads based on the number specified by the user.
This semaphore would be decremented before a thread is spawned (lines 349,350), and would wait until one of the spawned threads increments the semaphore (line 139) after its finished its task.
We parse files in parallel, and only resyncronize execution after all threads have been spawned. To do this, we track the number of spawned threads via an integer. This int is gaurded by a mutex to ensure only 1 thread can increment it at a time.
Once all of the threads have reported being done, we merge and write the result structs.
All of the encoded data is kept in a Linked-list of "result(s)". Result is a struct that keeps track of the characters and counts in order per chunk that it was allocated to encode. 
Each thread is given a pointer to a result struct (line 213) packaged inside a ThreadData struct, which tells it what section of what file to encode.
The result pointer is added to a global linked_list during instantiation (lines 219-220) to maintain the ordering of the full file.  


> 3. Additional design details that are not covered in the previous two questions. 

Responses.



Performance Optimization (10 points)
------

> 1. What optimizations have you tried and which optimizations were useful in improving the compression performance over your previous parallelized implementation?

Responses.

We tried to use fseek and using mutex for that but that did not work. Then we used mmap which was the biggest jump in speedup. 
We are trying allocate the correct number of threads based on the situation but we are struggling 

Our final version of the code revolves around two optimizations: dynamic allocation of threads per file and parallel file encoding.
Initially, we were allocating all available threads per file, and parsing them in series. Although this was great for large files, splitting up small files into several chunks incurred more overhead than it was worth.
We defined a threshold 'SMALL_FILE_THRESHOLD' and allocated threads proportional to the filesize w.r.t this threshold. Doing this allows us to use less threads for small files while still chunking up large files into managable workloads. 
Additionally in our first solution, we would wait until each file was fully encoded before moving on to the next step. This ensured the correct ordering of files, but would bottleneck performance when dealing with small files.
To fix this, we utilized a semaphore to kick of thread execution on a rolling basis, and move our synchronization step (previously on a per_file basis) to after all files had been encoded.
The semaphore was initialized based on the number of available threads in the system, and we decrement this semaphore every time a thread was started. The main thread would wait until there were more threads available before continuing, preventing us from spawning too many threads.
We would increment the semaphore in each thread after it was done executing its task. 

The combination of these two changes allowed our program to spawn the minimum threads required per file, and parse files in parallel based on the available threads in the system. 

One small optimization we made was making the result structs size dynamic as well. We would resize using realloc (line 88-89) the size of the counts and chars array per result once the arrays were full.
This reduced memory usage and added robustness to different sized files.

> 2. Description of any new workloads you used to understand the performance bottlenecks.

Responses.

Errata
------

> Describe any known errors, bugs, or deviations from the requirements.

Responses.