# virtual_memory_manager

## Project 3 Report

In part 1 of the project, we implemented a virtual memory manager with equal physical and virtual memory sizes. We first extracted offset and the logical page number with shifting and masks. The algorithm first checks the TLB table and if it is a hit it will return the physical page number which further is used for finding the corresponding physical address and value in the main memory. If the search is unsuccessful, it will check the page table with the given logical page number as the index. Finally, if the value is not in the memory, it will read 1024 bytes from the backing storage. In the end of each iteration, the TLB table is updated to represent the most recent access. 

In the second part, we used a page table struct in addition to the TLB struct that was given in part 1 to add an additional valid bit integer. The valid bit is used to keep track of whether the requested frame is in the memory or not. If these frames are overwritten later, the valid bit in these tables are set to 0. The pages are replaced by either of FIFO or LRU page replacement policies. FIFO replaces the first entered page in the memory with the new page. For LRU, we created an array of ints of size PHYSICAL_PAGES which records the number of iterations a page is not used. We increase the value of each index that is not used on that iteration and assign zero for the used memory address. When there is no space on the memory, we select the highest index as the victim of reallocation.


To run the first part, after compiling part1.c , the following arguments are needed
BACKING_STORE.bin addresses.txt.

To run the second part and for FIFO use:
```
./part2 BACKING_STORE.bin addresses.txt -p 0 
```

for LRU use:
```
./part2 BACKING_STORE.bin addresses.txt -p 1 
```
