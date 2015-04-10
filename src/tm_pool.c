#include <stdlib.h>
#include "dbg.h"
#include "tm_pool.h"
#include "tm_freed.h"

#define IS_LESS(pool, v1, v2)  (Pool_location(pool, v1) < Pool_location(pool, v2))
void siftDown(Pool *pool, tm_index *a, int16_t start, int16_t count);
#define SWAP(r,s)  do{tm_index t=r; r=s; s=t;} while(0)


void heap_sort(Pool *pool, tm_index *a, int16_t count){
    int16_t start, end;

    /* heapify */
    for (start = (count-2)/2; start >=0; start--) {
        siftDown(pool, a, start, count);
    }

    for (end=count-1; end > 0; end--) {
        SWAP(a[end], a[0]);
        siftDown(pool, a, 0, end);
    }
}


void siftDown(Pool *pool, tm_index *a, int16_t start, int16_t end){
    int16_t root = start;

    while ( root*2+1 < end ) {
        int16_t child = 2*root + 1;
        if ((child + 1 < end) && IS_LESS(pool, a[child], a[child+1])) {
            child += 1;
        }
        if (IS_LESS(pool, a[root], a[child])) {
            SWAP(a[child], a[root]);
            root = child;
        }
        else
            return;
    }
}


void bubble_sort(Pool *pool, tm_index *array, tm_index len){
    // sort indexes by location
    tm_index i, j;
    tm_index swap;
    bool workdone = false;
    for (i = 0; i < len; i++){
        workdone = false;
        for (j = 0; j < len - 1; j++){
            if (Pool_location(pool, array[j]) < Pool_location(pool, array[j-1])){
                swap       = array[j];
                array[j]   = array[j-1];
                array[j-1] = swap;
                workdone = true;
            }
        }
        if(! workdone) break; // no values were moved, list is sorted
    }
}


/* ##################################################
 * ###  Pool Methods
 */

Pool *Pool_new(){
    // Malloc space we need
    Pool *pool = malloc(sizeof(Pool));
    if(!pool) return NULL;
    *pool = Pool_declare();
    Pool_freed_reset(pool);
    return pool;
}


void Pool_upool_clear(Pool *pool){
    pool->uheap = 0;
    pool->ustack = TM_UPOOL_SIZE;
    Pool_freed_reset(pool);
}


void Pool_freed_reset(Pool *pool){
    tm_index i;
    for(i=0; i<TM_FREED_BINS; i++){
        pool->freed[i] = TM_UPOOL_ERROR;
    }
    // there are no freed values anymore, only filled ones
    for(i=0; i<TM_MAX_FILLED_PTRS; i++){
        pool->points[i] = pool->filled[i];
    }
    pool->points[0]     |= 1;  // NULL needs to be taken
    pool->points[i-1]   |= TM_LAST_USED;  // prevent use of values that don't exist
}

inline void *Pool_void(Pool *pool, tm_index index){
    // get a void pointer to data from pool index.
    if(!index) return NULL;
    return Pool_location_void(pool, Pool_location(pool, index));
}

tm_index Pool_find_index(Pool *pool){
    tm_index index, i, b;
    unsigned int bit;
    unsigned int *points = (unsigned int *)pool->points;
    for(i=0; i<TM_MAX_FILLED_INT; i++){
        if(points[i] != MAXUINT){
            // There is an empty value
            bit = 1;
            for(b=0; b<INTBITS; b++){
                if(! (points[i] & bit)){
                    index = i * INTBITS + b;
                    return index;
                }
                bit = bit << 1;
            }
        }
    }
    return 0;
}


tm_index Pool_alloc(Pool *pool, tm_index size){
    tm_index index = Pool_freed_getsize(pool, size);
    if(index) return index;
    if(size > Pool_available(pool)) return 0;

    if(size > Pool_heap_left(pool)){
        Pool_status_set(pool, TM_DEFRAG);
        // TODO: this is the "simplest" implementation.
        //      Threading is going to be main one long term
        Pool_defrag_full(pool);
        if(size > Pool_heap_left(pool)){
            Pool_status_set(pool, TM_ERROR);
            return 0;
        }
    }
    // find an unused index
    if(!Pool_pointers_left(pool)){
        return 0;
    }
    index = Pool_find_index(pool);
    if(!index){
        Pool_status_set(pool, TM_DEFRAG);
        Pool_defrag_full(pool);  // TODO: simple implemntation
        index = Pool_find_index(pool);
        if((!Pool_pointers_left(pool)) || (!index)){
            Pool_status_set(pool, TM_ERROR);
            return 0;
        }
    }
    Pool_filled_set(pool, index);
    Pool_points_set(pool, index);
    pool->pointers[index] = (poolptr) {.size = size, .ptr = pool->heap};
    pool->heap += size;
    pool->used_bytes += size;
    pool->used_pointers++;
    return index;
}


void Pool_free(Pool *pool, tm_index index){
    Pool_filled_clear(pool, index);
    pool->used_bytes -= Pool_sizeof(pool, index);
    pool->used_pointers--;
    Pool_freed_append(pool, index);  // TODO: if false set defrag flag
}


bool Pool_defrag_full(Pool *pool){
    tm_index prev_index, index, i;
    tm_index len = 0;
    Pool_status_set(pool, TM_DEFRAG_IP);
    Pool_status_clear(pool, TM_DEFRAG | TM_DEFRAG_FULL);

    pool->used_bytes = 1;
    pool->used_pointers = 1;

    // this function completely deallocates -- there are no freed values or anything
    // else in the upool
    Pool_upool_clear(pool);

    // Move used indexes into upool andand sort them
    for(index=0; index<TM_MAX_POOL_PTRS; index++){
        if(Pool_filled_bool(pool, index)){
            Pool_upool_set_index(pool, len, index);
            len++;
        }
    }
    if(! len){
        pool->heap = 1;
        return false;
    }

    heap_sort(pool, (tm_index *)pool->upool, len);  // kind of a pun, sorting the heap... haha

    // we now have sorted indexes by location. We just need to
    // move all memory to the left
    // First memory can be moved to loc 1
    // TODO: use macro Pool_memmove(pool, to, from)
    index = Pool_upool_get_index(pool, 0);
    // memmove(to, from, size)
    memmove(Pool_location_void(pool, 1), Pool_void(pool, index), Pool_sizeof(pool, index));
    Pool_location_set(pool, index, 1);
    pool->used_bytes += Pool_sizeof(pool, index);
    pool->used_pointers++;

    prev_index = index;
    // rest of memory is packeduse2
    for(i=1; i<len; i++){
        index = Pool_upool_get_index(pool, i);
        memmove(
            Pool_void(pool, prev_index) + Pool_sizeof(pool, prev_index),
            Pool_void(pool, index),
            Pool_sizeof(pool, index)
        );
        Pool_location_set(pool, index, Pool_location(pool, prev_index) + Pool_sizeof(pool, prev_index));
        pool->used_bytes += Pool_sizeof(pool, index);
        pool->used_pointers++;
        prev_index = index;
    }

    // heap can now move left
    pool->heap = Pool_location(pool, index) + Pool_sizeof(pool, index);
    Pool_status_clear(pool, TM_DEFRAG_IP);
    return true;
}


/* upool allocation and freeing. Used for internal methods
 */
tm_index Pool_ualloc(Pool *pool, tm_size size){
    // The upool ASSUMES that all blocks are the same size. Make sure this is always true.
    tm_index location;

    if(Pool_ustack_used(pool)) {
        // free pointers available
        location = Pool_upool_get_index(pool, pool->ustack / 2);
        pool->ustack += 2;
        return location;
    }
    if(size > Pool_uheap_left(pool)) return TM_UPOOL_ERROR;
    pool->uheap += size;
    return pool->uheap - size;
}


bool Pool_ufree(Pool *pool, tm_index location){
    tm_index left = Pool_uheap_left(pool);
    tmdebug("uheap_left=%u", left);
    if(left < 2){
        return false;
    }
    pool->ustack-=2;
    Pool_upool_set_index(pool, pool->ustack / 2, location);
    return true;
}


inline void *Pool_uvoid(Pool *pool, tm_index location){
    if(location >= TM_UPOOL_ERROR) return NULL;
    return pool->upool + location;
}

