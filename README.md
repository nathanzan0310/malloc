```
 __  __       _ _
|  \/  | __ _| | | ___   ___
| |\/| |/ _` | | |/ _ \ / __|
| |  | | (_| | | | (_) | (__
|_|  |_|\__,_|_|_|\___/ \___|
```
You should provide a README file which documents the following: 
• a description of your strategy for maintaining compaction (i.e. how are you preventing your heap from turning into a bunch of tiny free blocks?) 
• your mm_realloc() implementation strategy, more specifically what optimizations or deliberate choices did you make to improve throughput and / or utilization 
• your strategy to achieve high throughput (see next point on Performance) 
• unresolved bugs with your program 
• any other optimizations

My strategy to maintain compaction was to only mem_sbrk when I couldn't find a free block big enough through the 
free list and to ensure that whenever I split a block I left at least MIN_BLOCK_SIZE. When I did need to mem_sbrk, I 
first checked if the last free block was right before the epilogue, and if so then all I had to sbrk was the difference between
required size and block_size(last free block). Another case I checked where the last free block was right before the 
epilogue was if required size + 2 * MINBLOCKSIZE  < block_size(last free block) so that I could sbrk just enough to
allocate the requested size and produce a free block of useful size.

My mm_realloc strategy was to check if the previous and next blocks were unallocated, and if so, see if I could combine
the given block with one or both, so I didn't have to sbrk more. I employed the same split principles as in malloc.
Unfortunately, I wasn't able to successfully implement the cases where next was unallocated and the combined space
of next and given was > requested size as I kept getting original payload not preserved errors; nor was I able to 
implement the case where I checked both previous and next blocks at the same time. However, I was still able to achieve 
more than 60% utilization in both realloc traces. If the given block was requested to shrink, I simply mm_free(ptr) and
malloc(size). I avoided calls to malloc unless necessary as it is very expensive. If prev and next were both 
allocated or not big enough with given block or there were no free blocks large enough, I sbrk'ed the required size.
This is the only time I ever used sbrk in order to reduce as much unnecessary space as possible.
