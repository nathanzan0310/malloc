#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * BEFORE GETTING STARTED:
 *
 * Familiarize yourself with the functions and constants/variables
 * in the following included files.
 * This will make the project a LOT easier as you go!!
 *
 * The diagram in Section 4.1 (Specification) of the handout will help you
 * understand the constants in mm.h
 * Section 4.2 (Support Routines) of the handout has information about
 * the functions in mminline.h and memlib.h
 */
#include "./memlib.h"
#include "./mm.h"
#include "./mminline.h"

block_t *prologue;
block_t *epilogue;

// rounds up to the nearest multiple of WORD_SIZE
static inline long align(long size) {
    return (((size) + (WORD_SIZE - 1)) & ~(WORD_SIZE - 1));
}

// Gets the required size of requested size block
static inline long reqSize(long size) {
    long requiredSize = align(size);
    if (MINBLOCKSIZE > requiredSize + TAGS_SIZE)
        requiredSize = MINBLOCKSIZE;
    else
        requiredSize += TAGS_SIZE;
    return requiredSize;
}

/*
 *                             _       _ _
 *     _ __ ___  _ __ ___     (_)_ __ (_) |_
 *    | '_ ` _ \| '_ ` _ \    | | '_ \| | __|
 *    | | | | | | | | | | |   | | | | | | |_
 *    |_| |_| |_|_| |_| |_|___|_|_| |_|_|\__|
 *                       |_____|
 *
 * initializes the dynamic storage allocator (allocate initial heap space)
 * arguments: none
 * returns: 0, if successful
 *         -1, if an error occurs
 */
int mm_init(void) {
    flist_first = NULL;
    if ((prologue = mem_sbrk(TAGS_SIZE * 2)) == (void *)-1) {
        perror("mem_sbrk");
        return -1;
    }
    epilogue = (block_t *)((char *)prologue + TAGS_SIZE);
    block_set_size_and_allocated(prologue, TAGS_SIZE, 1);
    block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);
    return 0;
}

/*     _ __ ___  _ __ ___      _ __ ___   __ _| | | ___   ___
 *    | '_ ` _ \| '_ ` _ \    | '_ ` _ \ / _` | | |/ _ \ / __|
 *    | | | | | | | | | | |   | | | | | | (_| | | | (_) | (__
 *    |_| |_| |_|_| |_| |_|___|_| |_| |_|\__,_|_|_|\___/ \___|
 *                       |_____|
 *
 * allocates a block of memory and returns a pointer to that block's payload
 * arguments: size: the desired payload size for the block
 * returns: a pointer to the newly-allocated block's payload (whose size
 *          is a multiple of ALIGNMENT), or NULL if an error occurred
 */
void *mm_malloc(long size) {
    (void)size;  // avoid unused variable warnings
    // TODO
    if (size <= 0) {
        fprintf(stderr, "Error: size is less than or equal to 0\n");
        return NULL;
    }

    long requiredSize = reqSize(size);
    block_t *new = flist_first;

    if (flist_first != NULL) {
        int firstFlag = 1;
        while (block_flink(new) != flist_first || firstFlag) {
            // Search through free list for suitable block
            if (new->size == requiredSize) {
                pull_free_block(new);
                block_set_size_and_allocated(new, requiredSize, 1);
                return new + 1;
            }
            if (new->size >= requiredSize + MINBLOCKSIZE) {
                // If splitting, make sure leftover > MINBLOCKSIZE
                block_set_size(new, block_size(new) - requiredSize);
                new = block_next(new);
                block_set_size_and_allocated(new, requiredSize, 1);
                return new + 1;
            }
            firstFlag = 0;
            new = block_flink(new);
        }
        if (block_next_size(new) == TAGS_SIZE) {
            // Last free block right before epilogue, can mem_sbrk less
            long addon;
            int reqSizeSmaller = 0;
            if (block_size(new) <
                requiredSize)  // reqSize > free block size, sbrk the difference
                addon = requiredSize - block_size(new);
            else {
                // reqSize < free block size, sbrk enough so that leftover is
                // useful
                addon = 2 * MINBLOCKSIZE - (block_size(new) - requiredSize);
                reqSizeSmaller = 1;
                if (addon < 0) {
                    // if difference is large enough, split without sbrk
                    block_set_size(new, block_size(new) - requiredSize);
                    new = block_next(new);
                    block_set_size_and_allocated(new, requiredSize, 1);
                    return new + 1;
                }
            }
            if (mem_sbrk(addon) == (void *)-1) {
                perror("mem_sbrk");
                return NULL;
            }
            if (reqSizeSmaller) {
                // If reqSizeSmaller i.e. need to split after sbrk, then resize
                // free block
                block_set_size(new, 2 * MINBLOCKSIZE);
                new = block_next(new);
            } else
                // otherwise remove free block
                pull_free_block(new);

            block_set_size_and_allocated(new, requiredSize, 1);
            epilogue = block_next(new);
            block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);
            return new + 1;
        }
    }
    if (mem_sbrk(requiredSize) == (void *)-1) {
        // if no space, just extend by required size and allocate
        perror("mem_sbrk");
        return NULL;
    }
    new = epilogue;
    block_set_size_and_allocated(new, requiredSize, 1);
    epilogue = block_next(new);
    block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);

    return new + 1;
}

/*                              __
 *     _ __ ___  _ __ ___      / _|_ __ ___  ___
 *    | '_ ` _ \| '_ ` _ \    | |_| '__/ _ \/ _ \
 *    | | | | | | | | | | |   |  _| | |  __/  __/
 *    |_| |_| |_|_| |_| |_|___|_| |_|  \___|\___|
 *                       |_____|
 *
 * frees a block of memory, enabling it to be reused later
 * arguments: ptr: pointer to the block's payload
 * returns: nothing
 */
void mm_free(void *ptr) {
    (void)ptr;  // avoid unused variable warnings
    // TODO=
    block_t *freed = payload_to_block(ptr);

    if (ptr != NULL) {
        if (!block_next_allocated(freed)) {
            // coalesce with previous free
            pull_free_block(block_next(freed));
            block_set_size_and_allocated(
                freed, block_size(freed) + block_next_size(freed), 0);
        }
        if (!block_prev_allocated(freed)) {
            // coalesce with next free
            pull_free_block(block_prev(freed));
            long freedSize = block_size(freed);
            freed = block_prev(freed);
            block_set_size_and_allocated(freed, freedSize + block_size(freed),
                                         0);
        } else
            // otherwise allocate free and add to list
            block_set_allocated(freed, 0);
        insert_free_block(freed);
    }
}

/*
 *                                            _ _
 *     _ __ ___  _ __ ___      _ __ ___  __ _| | | ___   ___
 *    | '_ ` _ \| '_ ` _ \    | '__/ _ \/ _` | | |/ _ \ / __|
 *    | | | | | | | | | | |   | | |  __/ (_| | | | (_) | (__
 *    |_| |_| |_|_| |_| |_|___|_|  \___|\__,_|_|_|\___/ \___|
 *                       |_____|
 *
 * reallocates a memory block to update it with a new given size
 * arguments: ptr: a pointer to the memory block's payload
 *            size: the desired new payload size
 * returns: a pointer to the new memory block's payload
 */
void *mm_realloc(void *ptr, long size) {
    (void)ptr, (void)size;  // avoid unused variable warnings
    // TODO
    long requiredSize = reqSize(size);
    block_t *new = payload_to_block(ptr);

    if (size == 0) {
        mm_free(ptr);
        return NULL;
    } else if (ptr == NULL) {
        return mm_malloc(size);
    } else {
        if (requiredSize == block_size(new)) {
            return ptr;
        } else if (requiredSize < block_size(new)) {
            // if block is shrinking, just free and re-malloc
            mm_free(ptr);
            return mm_malloc(size);
        } else {
            if (!block_next_allocated(new)) {
                // coalesce with next block if suitable
                long bbNextSize = block_next_size(new) + block_size(new);
                if (bbNextSize == requiredSize) {
                    pull_free_block(block_next(new));
                    block_set_size_and_allocated(
                        new, block_size(new) + block_next_size(new), 1);
                    return new + 1;
                }
            }
            if (!block_prev_allocated(new)) {
                // coalesce with previous block if suitable
                long bbPrevSize = block_prev_size(new) + block_size(new);
                if (bbPrevSize == requiredSize) {
                    pull_free_block(block_prev(new));
                    memmove(block_prev(new)->payload, new->payload,
                            block_size(new) - TAGS_SIZE);
                    block_set_size_and_allocated(block_prev(new), requiredSize,
                                                 1);
                    return block_prev(new) + 1;
                }
                if (bbPrevSize > requiredSize + MINBLOCKSIZE) {
                    block_set_size(block_prev(new), bbPrevSize - requiredSize);
                    void *ogPayload = new->payload;
                    long ogPayloadSize = block_size(new) - TAGS_SIZE;
                    new = block_next(block_prev(new));
                    memmove(new->payload, ogPayload, ogPayloadSize);
                    block_set_size_and_allocated(new, requiredSize, 1);
                    return new + 1;
                }
            }
            if ((block_prev_allocated(new) && block_next_allocated(new)) ||
                block_next_size(new) + block_size(new) - requiredSize <
                    MINBLOCKSIZE ||
                block_prev_size(new) + block_size(new) - requiredSize <
                    MINBLOCKSIZE) {
                // if can't coalesce, search for first suitable free block and
                // reallocate
                int firstFlag = 1;
                block_t *cur = flist_first;
                while (cur != NULL &&
                       (block_flink(cur) != flist_first || firstFlag)) {
                    if (cur->size == requiredSize) {
                        // copy over original payload to cur and free original
                        pull_free_block(cur);
                        block_set_size_and_allocated(cur, requiredSize, 1);
                        memmove(cur->payload, ptr, new->size - TAGS_SIZE);
                        mm_free(ptr);
                        return cur + 1;
                    }
                    if (cur->size >= requiredSize + MINBLOCKSIZE) {
                        // copy over original payload to cur and free original
                        block_set_size(cur, block_size(cur) - requiredSize);
                        cur = block_next(cur);
                        block_set_size_and_allocated(cur, requiredSize, 1);
                        memmove(cur->payload, ptr, new->size - TAGS_SIZE);
                        mm_free(ptr);
                        return cur + 1;
                    }
                    firstFlag = 0;
                    cur = block_flink(cur);
                }
            }
        }
    }
    if (mem_sbrk(requiredSize) == (void *)-1) {
        // if no suitable block, just extend by reqSize
        perror("mem_sbrk");
        return NULL;
    }
    new = memmove(epilogue + 1, new->payload, new->size - TAGS_SIZE);
    // copy over original payload to new and free original
    new = payload_to_block(new);
    block_set_size_and_allocated(new, requiredSize, 1);
    block_set_size(new, requiredSize);
    epilogue = block_next(new);
    block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);
    mm_free(ptr);
    return new + 1;
}
