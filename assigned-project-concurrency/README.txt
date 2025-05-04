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

Our current implementation does not require any synchronization primitives as the sections ran in parallel have no race conditions. We syncronize the reading of a file by waiting until all child threads have terminated using pthread_join before continuing onto the next file.
We plan on revisiting our implementation to better use synchronization primitives to increase our speed.
But, as the design is currently, we know that our algorithm is consistent across runs because there exist no runtime-specific errors. 
No race conditions exist due to the lack of critical regions in our code, and no deadlocks or livelocks can occur because there are no primatives in use.
This makes running determanistic.  

> 3. Additional design details that are not covered in the previous two questions. 

Responses.



Performance Optimization (10 points)
------

> 1. What optimizations have you tried and which optimizations were useful in improving the compression performance over your previous parallelized implementation?

Responses.

We tried to use fseek and using mutex for that but that did not work. Then we used mmap which was the biggest jump in speedup. 
We are trying allocate the correct number of threads based on the situation but we are struggling 
> 2. Description of any new workloads you used to understand the performance bottlenecks.

Responses.

Errata
------

> Describe any known errors, bugs, or deviations from the requirements.

Responses.