
#include "tinymem.h"
#include "tm_pool.h"
#include "tm_freed.h"

Pool pool = Pool_declare();

void tm_init(){
    Pool_freed_reset(&pool);
}

inline tm_index tm_alloc(tm_size size){
    return Pool_alloc(&pool, size);
}

inline void tm_free(tm_index index){
    return Pool_free(&pool, index);
}

inline tm_size tm_sizeof(tm_index index){
    return Pool_sizeof(&pool, index);
}

inline void*  tm_void(tm_index index){
    return Pool_void(&pool, index);
}
