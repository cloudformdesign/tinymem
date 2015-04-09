/**
 * \defgroup tinymem-lib
 * @{
 */

/**
 * \file
 *      tm_pool.h tinymem memory pool implementation
 *  \author
 *      2015 Garrett Berg, Garrett@cloudformdesign.com
 */
#ifndef __mempool_h_
#define __mempool_h_
#include <stdlib.h>
#include "tm_types.h"

/*---------------------------------------------------------------------------*/
/**
 * \brief           Pool Configuration options
 *                  define your own "tinymem_platform.h" and include it in your
 *                  Makefile to select these options
 *                  Or, link to one of the standard ones in the `platform`
 *                  folder
 */
#include "tinymem_platform.h"

/*---------------------------------------------------------------------------*/
/**
 * \brief           Internal Definitions based on platform
 */
#define TM_UPOOL_SIZE           (TM_MAX_POOL_PTRS * sizeof(tm_index))
#define TM_UPOOL_ERROR          (TM_UPOOL_SIZE + 1)
#define TM_MAX_FILLED_PTRS      (TM_MAX_POOL_PTRS / 8 + (TM_MAX_POOL_PTRS % 8 ? 1:0))
#define TM_MAX_FILLED_INT       (TM_MAX_FILLED_PTRS / sizeof(int) + \
                                    ((TM_MAX_FILLED_PTRS % sizeof(int)) ? 1:0))
#define TM_LAST_USED            ((uint8_t) (~(0x00FF >> (TM_MAX_POOL_PTRS % 8))))

#define INTBITS                 (sizeof(int) * 8)
#define MAXUINT                 ((unsigned int) 0xFFFFFFFFFFFFFFFF)

/*---------------------------------------------------------------------------*/
/**
 * \brief           Pool status bitcodes
 */
#define TM_DEFRAG_FULL  (1<<0)  // a full defrag has been requested
#define TM_DEFRAG       (1<<1)  // a fast defrag has been requested
#define TM_DEFRAG_IP    (1<<2)  // A defrag is in progress
#define TM_MOVING       (1<<3)  // the memory manager is currently moving a block
#define TM_ERROR        (1<<7)  // a memory manager internal error occurred
#define TM_ANY_DEFRAG   (TM_DEFRAG_FULL | TM_DEFRAG | TM_DEFRAG_IP)   // some defrag has been requested



/*---------------------------------------------------------------------------*/
/**
 * \brief           poolptr is used by Pool to track memory location and size
 */
typedef struct {
    tm_size size;
    tm_size ptr;
} poolptr;
#define NULL_poolptr            ((poolptr){.size=0, .ptr=0})

/*---------------------------------------------------------------------------*/
/**
 * \brief           Pool object to track all memory usage
 *                  This is the main object used by tinymem to do memory
 *                  management
 */
typedef struct {
    tm_size heap;                       //!< location of completely free memory
    tm_size stack;                      //!< used for tempalloc and tempfree, similar to standard stack
    tm_size used_bytes;                 //!< total amount of data in use out of size (does not include freed)
    tm_size used_pointers;              //!< total amount of pointers used (does not include freed)
    tm_index filled_index;              //!< faster lookup of full pointers for defragmentation
    tm_index points_index;              //!< faster lookup for unused pointers for allocation
    tm_size uheap;                      //!< heap of the upool
    tm_size ustack;                     //!< stack of the upool
    uint8_t filled[TM_MAX_FILLED_PTRS]; //!< bit array of filled pointers (only used, not freed)
    uint8_t points[TM_MAX_FILLED_PTRS]; //!< bit array of used pointers (both used and freed)
    poolptr pointers[TM_MAX_POOL_PTRS]; //!< size and location of data in pool
    uint8_t  upool[TM_UPOOL_SIZE];      //!< extra space for processing and storing freed pointers
    tm_index freed[TM_FREED_BINS];      //!< binned storage of all freed indexes (uses upool)
    uint8_t pool[TM_POOL_SIZE];         //!< Actual memory pool (very large)
} Pool;


/*---------------------------------------------------------------------------*/
/**
 * \brief           Internal use only. Declares Pool with as many initial
 *                  values as possible
 */
#define Pool_declare()  ((Pool) {       \
    .heap = 1,                          \
    .stack = TM_POOL_SIZE,              \
    .used_bytes = 1,                    \
    .used_pointers = 1,                 \
    .filled_index = 0,                  \
    .points_index = 0,                  \
    .uheap = 0,                         \
    .ustack = TM_UPOOL_SIZE,            \
    .filled = {0},                      \
    .points = {1},                      \
    .pointers = {{1, 0}},               \
    .upool = {0},                       \
    .freed = TM_FREED_BINS_DECLARE,     \
})

/*---------------------------------------------------------------------------*/
/**
 * \brief           Speeds up declaration of Freed Bins
 */
#define TM_FREED_BINS_DECLARE   {TM_UPOOL_ERROR, TM_UPOOL_ERROR, TM_UPOOL_ERROR, TM_UPOOL_ERROR,  \
                                 TM_UPOOL_ERROR, TM_UPOOL_ERROR, TM_UPOOL_ERROR, TM_UPOOL_ERROR,  \
                                 TM_UPOOL_ERROR, TM_UPOOL_ERROR, TM_UPOOL_ERROR, TM_UPOOL_ERROR,  \
                                 TM_UPOOL_ERROR, TM_UPOOL_ERROR, TM_UPOOL_ERROR, TM_UPOOL_ERROR}

/*---------------------------------------------------------------------------*/
/**
 * \brief           Pool available memory (both heap and inside freed)
 *                  This is how much space is left to allocate after a full
 *                  defragmentation
 * \return tm_size  available space in bytes
 */
#define Pool_available(pool)            (TM_POOL_SIZE - (pool)->used_bytes)
/*---------------------------------------------------------------------------*/
/**
 * \brief           Pool available pointers
 *                  This is how many more objects can be allocated
 * \return tm_size  available pointer spaces
 */
#define Pool_pointers_left(pool)        (TM_MAX_POOL_PTRS - (pool)->used_pointers)
/*---------------------------------------------------------------------------*/
/**
 * \brief           Memory remaining on the heap
 * \return tm_size  Number of bytes remaining on the heap
 */
#define Pool_heap_left(pool)            ((pool)->stack - (pool)->heap)
/*---------------------------------------------------------------------------*/
/**
 * \brief           Get the sizeof data at index in bytes
 * \return tm_size  the sizeof the data pointed to by index
 */
#define Pool_sizeof(pool, index)        ((pool)->pointers[index].size) // get size of data at index
/*---------------------------------------------------------------------------*/
/**
 * \brief           Get the status bit (0 or 1) of name
 * \return uint8_t  status bit
 */
#define Pool_status(p, name)         (((p)->pool[0]) & (name))
/*---------------------------------------------------------------------------*/
/**
 * \brief           Set the pool status of name to 1
 */
#define Pool_status_set(p, name)     ((p)->pool[0] |= (name))
/*---------------------------------------------------------------------------*/
/**
 * \brief           Set the pool status of name to 0
 */
#define Pool_status_clear(p, name)   ((p)->pool[0] &= ~(name))
/*---------------------------------------------------------------------------*/
/**
 * \brief           Convinience functions
 *                  Pool_filled* does operations on Pool's `filled` array
 *                  Pool_points* does operations on Pool's `points` array
 */
#define Pool_filled_index(index)        (index / 8)
#define Pool_filled_bit(index)          (1 << (index % 8))
#define Pool_filled_bool(pool, index)   ((pool)->filled[Pool_filled_index(index)] & Pool_filled_bit(index))
#define Pool_filled_set(pool, index)    ((pool)->filled[Pool_filled_index(index)] |=  Pool_filled_bit(index))
#define Pool_filled_clear(pool, index)  ((pool)->filled[Pool_filled_index(index)] &= ~Pool_filled_bit(index))
#define Pool_points_bool(pool, index)   ((pool)->points[Pool_filled_index(index)] & Pool_filled_bit(index))
#define Pool_points_set(pool, index)    ((pool)->points[Pool_filled_index(index)] |=  Pool_filled_bit(index))
#define Pool_points_clear(pool, index)  ((pool)->points[Pool_filled_index(index)] &= ~Pool_filled_bit(index))
/*---------------------------------------------------------------------------*/
/**
 * \brief           move memory from location at index_from to location at
 *                  index_to
 */
#define Pool_memmove(pool, index_to, index_from)  memmove(              \
            Pool_void(pool, index_to) + Pool_sizeof(pool, index_to),    \
            Pool_void(pool, index_from),                                \
            Pool_sizeof(pool, index_from)                               \
        )
/*---------------------------------------------------------------------------*/
/**
 * \brief           location of index
 * \return tm_size  location in relation to pool.pool
 */
#define Pool_location(pool, index)              ((pool)->pointers[index].ptr)  // location of pointer inside pool
/*---------------------------------------------------------------------------*/
/**
 * \brief           set location of index to loc
 */
#define Pool_location_set(pool, index, loc)     (Pool_location(pool, index) = loc)
/*---------------------------------------------------------------------------*/
/**
 * \brief           cast a void pointer of location
 */
#define Pool_location_void(p, loc)           ((void*)(p)->pool + (loc))    // pointer of location

/*---------------------------------------------------------------------------*/
/**
 * \brief           allocate a new pool using system malloc
 * \return *Pool
 */
Pool*           Pool_new();
/*---------------------------------------------------------------------------*/
/**
 * \brief           delete (free) Pool object
 */
#define Pool_del(pool)  (free(pool))
/*---------------------------------------------------------------------------*/
/**
 * \brief           allocate memory from pool
 * \param pool      pointer to Pool struct
 * \param size      size of pointer to allocate
 * \return          tm_index corresponding to memory location
 *                  On error or if not enough memory, return value == 0
 */
tm_index        Pool_alloc(Pool *pool, tm_size size);
/*---------------------------------------------------------------------------*/
/**
 * \brief           free allocated memory from pool
 * \param pool      pointer to Pool struct
 * \param index     tm_index to free
 */
void            Pool_free(Pool *pool, tm_index index);
/*---------------------------------------------------------------------------*/
/**
 * \brief           perform a full (non-threaded) defragmentation of pool
 */
bool            Pool_defrag_full(Pool *pool);
/*---------------------------------------------------------------------------*/
/**
 * \brief           cast a void pointer from index
 * \param pool      pointer to Pool struct
 * \param index     tm_index to get pointer to
 * \return          void* pointer to actual data
 */
inline void*    Pool_void(Pool *pool, tm_index index);

/*---------------------------------------------------------------------------*/
/**
 * \brief           upool allocation and freeing. Used for internal methods
 */
tm_index Pool_ualloc(Pool *pool, tm_size size);
bool Pool_ufree(Pool *pool, tm_index index);
inline void *Pool_uvoid(Pool *pool, tm_index index);
#define Pool_upool_get_index(pool, index)  (((tm_index *)((pool)->upool))[index])
#define Pool_upool_set_index(pool, index, value)  (((tm_index *)((pool)->upool))[index] = value)
#define Pool_uheap_left(pool)           (pool->ustack - pool->uheap)
#define Pool_ustack_used(pool)          (TM_UPOOL_SIZE - pool->ustack)

/*---------------------------------------------------------------------------*/
/**
 * \brief           clear upool (loosing all data)
 */
void Pool_upool_clear(Pool *pool);
/*---------------------------------------------------------------------------*/
/**
 * \brief           clear freed array (loosing all data)
 */
void Pool_freed_reset(Pool *pool);

/*---------------------------------------------------------------------------*/
/**
 * \brief           Various data type casts
 */
#define Pool_char_p(pool, index)        ((char *)Pool_void(pool, index))
#define Pool_int8_p(pool, index)        ((int8_t *)Pool_void(pool, index))
#define Pool_uint8_p(pool, index)       ((uint8_t *)Pool_void(pool, index))
#define Pool_int16_p(pool, index)       ((int16_t *)Pool_void(pool, index))
#define Pool_uint16_p(pool, index)      ((uint16_t *)Pool_void(pool, index))
#define Pool_int32_p(pool, index)       ((int32_t *)Pool_void(pool, index))
#define Pool_uint32_p(pool, index)      ((uint32_t *)Pool_void(pool, index))

#endif
/** @} */
