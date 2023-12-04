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
    if ((prologue = mem_sbrk(TAGS_SIZE * 2)) == (void *) -1) {
        perror("mem_sbrk");
        return -1;
    }
    epilogue = (block_t *) ((char *) prologue + TAGS_SIZE);
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
    (void) size;  // avoid unused variable warnings
    // TODO
    if (size <= 0) {
        fprintf(stderr, "Error: size is less than or equal to 0\n");
        return NULL;
    }

    long requiredSize = align(size);
    block_t *new = flist_first;

    if (MINBLOCKSIZE > requiredSize + TAGS_SIZE)
        requiredSize = MINBLOCKSIZE;
    else
        requiredSize += TAGS_SIZE;

    if (flist_first != NULL) {
        int firstFlag = 1;
        while (block_flink(new) != flist_first || firstFlag) {
            if (new->size == requiredSize) {
                pull_free_block(new);
                block_set_size_and_allocated(new, requiredSize, 1);
                //                printf("payload address: %p\n", (void *) (new
                //                + 1));
                return new + 1;
            }
            if (new->size >= requiredSize + MINBLOCKSIZE) {
                block_set_size(new, block_size(new) - requiredSize);
                new = block_next(new);
                block_set_size_and_allocated(new, requiredSize, 1);
                //                printf("payload address: %p\n", (void *) (new
                //                + 1));
                return new + 1;
            }
            firstFlag = 0;
            new = block_flink(new);
        }
        if (block_next_size(new) == TAGS_SIZE) {
            //            printf("requiredSize - block_size(new) = %ld\n",
            //            requiredSize - block_size(new));
            long addon;
            int reqSizeLarger = 0;
            if (block_size(new) < requiredSize) {
                addon = requiredSize - block_size(new);
            } else {
                addon = 2 * MINBLOCKSIZE - (block_size(new) - requiredSize);
                reqSizeLarger = 1;
                if (addon < 0) {
                    block_set_size(new, block_size(new) - requiredSize);
                    new = block_next(new);
                    block_set_size_and_allocated(new, requiredSize, 1);
                    return new + 1;
                }
            }
            if (mem_sbrk(addon) == (void *) -1) {
                perror("mem_sbrk");
                return NULL;
            }
            if (reqSizeLarger) {
                block_set_size(new, 2 * MINBLOCKSIZE);
                new = block_next(new);
            } else {
                pull_free_block(new);
            }
            block_set_size_and_allocated(new, requiredSize, 1);
            epilogue = block_next(new);
            block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);
            return new + 1;
        }
    }
    if (mem_sbrk(requiredSize) == (void *) -1) {
        perror("mem_sbrk");
        return NULL;
    }
    new = epilogue;
    block_set_size_and_allocated(new, requiredSize, 1);
    epilogue = block_next(new);
    block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);

    //    printf("payload address: %p\n", (void *) (new + 1));
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
    (void) ptr;  // avoid unused variable warnings
    // TODO=
    block_t *freed = payload_to_block(ptr);
    if (ptr != NULL) {
        if (!block_next_allocated(freed)) {
            pull_free_block(block_next(freed));
            block_set_size_and_allocated(
                    freed, block_size(freed) + block_next_size(freed), 0);
        }
        if (!block_prev_allocated(freed)) {
            pull_free_block(block_prev(freed));
            long freedSize = block_size(freed);
            freed = block_prev(freed);
            block_set_size_and_allocated(freed, freedSize + block_size(freed),
                                         0);
        } else
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
    (void) ptr, (void) size;  // avoid unused variable warnings
    // TODO
    long requiredSize = align(size);
    block_t *new = payload_to_block(ptr);
    if (MINBLOCKSIZE > requiredSize + TAGS_SIZE)
        requiredSize = MINBLOCKSIZE;
    else
        requiredSize += TAGS_SIZE;
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    } else if (ptr == NULL) {
        return mm_malloc(size);
    } else {
        if (requiredSize == block_size(new)) {
            return ptr;
        }
        if (requiredSize <= block_size(new)) {
            mm_free(ptr);
            return mm_malloc(size);
        } else {
            if (!block_next_allocated(new)) {
                long bbNextSize = block_next_size(new) + block_size(new);
                if (bbNextSize == requiredSize) {
                    pull_free_block(block_next(new));
                    block_set_size_and_allocated(
                            new, block_size(new) + block_next_size(new), 1);
                    return new + 1;
                }
                if (bbNextSize > requiredSize + MINBLOCKSIZE) {
//                    pull_free_block(block_next(new));
//                    printf("new->payload before move: %p\n", (void *) new->payload);
//                    memcpy((char *) new + bbNextSize - requiredSize + 1, new->payload, block_size(new) - TAGS_SIZE);
//                    mm_free(ptr);
//                    block_set_size_and_allocated(new, bbNextSize - requiredSize, 0);
//                    new = (block_t *) ((char *) new + bbNextSize - requiredSize);
//                    block_set_size_and_allocated(new, requiredSize, 1);
//                    printf("new->payload after move: %p\n", (void *) new->payload);
//                    return new + 1;
//                    block_set_size_and_allocated(new, requiredSize, 1);
//                    new = block_next(new);
//                    block_set_size_and_allocated(new, bbNextSize - requiredSize, 0);
//                    insert_free_block(new);
//                    return block_prev(new) + 1;
                }
            }
            if (!block_prev_allocated(new)) {
                long bbPrevSize = block_prev_size(new) + block_size(new);
                if (bbPrevSize == requiredSize) {
                    pull_free_block(block_prev(new));
                    memmove(block_prev(new)->payload, new->payload, block_size(new) - TAGS_SIZE);
                    block_set_size_and_allocated(block_prev(new), requiredSize, 1);
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
                block_next_size(new) + block_size(new) - requiredSize < MINBLOCKSIZE ||
                block_prev_size(new) + block_size(new) - requiredSize < MINBLOCKSIZE) {
                int firstFlag = 1;
                block_t *cur = flist_first;
                if (flist_first != NULL) {
                    while (block_flink(cur) != flist_first || firstFlag) {
                        if (cur->size == requiredSize) {
                            pull_free_block(cur);
                            block_set_size_and_allocated(cur, requiredSize, 1);
                            if (cur != payload_to_block(ptr)) {
                                memmove(cur->payload, ptr, new->size - TAGS_SIZE);
//                                printf("cur payload size == : %ld\n", cur->size - TAGS_SIZE);
//                                printf("new payload size == : %ld\n", new->size - TAGS_SIZE);
                                mm_free(ptr);
                            }
                            //                printf("payload address: %p\n", (void *) (new
                            //                + 1));
                            return cur + 1;
                        }
                        if (cur->size >= requiredSize + MINBLOCKSIZE) {
                            block_set_size(cur, block_size(cur) - requiredSize);
                            cur = block_next(cur);
                            if (cur != payload_to_block(ptr)) {
                                memmove(cur->payload, new->payload, new->size - TAGS_SIZE);
////                                printf("cur payload size >= : %ld\n", cur->size - TAGS_SIZE);
////                                printf("new payload size >= : %ld\n", new->size - TAGS_SIZE);
                            }
                            block_set_size_and_allocated(cur, requiredSize, 1);
////                                            printf("payload address: %p\n", (void *) (new
////                                            + 1));
                            mm_free(ptr);
                            return cur + 1;
                        }
                        firstFlag = 0;
                        cur = block_flink(cur);
                    }
                }


                if (mem_sbrk(requiredSize) == (void *) -1) {
                    printf("required_size: %ld\n", requiredSize);
                    perror("mem_sbrk");
                    return NULL;
                }
                new = memmove(epilogue + 1, new->payload, new->size - TAGS_SIZE);
                new = payload_to_block(new);
                block_set_size_and_allocated(new, requiredSize, 1);
                block_set_size(new, requiredSize);
//                printf("block_next(new): %p\n", (void *) block_next(new));
//                printf("(block_t *) ((char *) mem_heap_hi() - TAGS_SIZE): %p\n",
//                       (void *) ((char *) mem_heap_hi() - TAGS_SIZE));
                epilogue = block_next(new);
                block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);
                mm_free(ptr);
//                printf("new: %p\n", (void *) payload_to_block(new));
//                printf("new start size: %ld\n", block_size(payload_to_block(new)));
//                printf("new start allocated: %d\n",  block_allocated(payload_to_block(new)));
//                printf("new end size: %ld\n", block_end_size(payload_to_block(new)));
//                printf("new end allocated: %d\n", block_end_allocated(payload_to_block(new)));
//                printf("new end tag: %p\n", (void *) block_end_tag(payload_to_block(new)));
//                printf("payload[0] after memmove: %ld\n", payload_to_block(new)->payload[0]);
//                printf("payload[1] after memmove: %ld\n",  payload_to_block(new)->payload[1]);
                return new + 1;
            }
        }
    }

    return NULL;
}
