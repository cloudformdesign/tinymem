#include "minunit.h"
#include "tinymem.h"

#define TABLE_STANDIN NULL

#define HASH_PRIME 1677619
bool alloc_index(tm_index *indexes, tm_index index, tm_size size);


uint8_t index_hash(tm_index value, tm_size i){
    uint32_t h = (value + i) * HASH_PRIME;
    return ((h>>16) ^ (h & 0xFFFF));
}

bool check_indexes(tm_index *indexes, tm_index len){
    // All indexes where index!=0 should be equal to their hash
    tm_index i, j;
    tm_size size;
    uint8_t *data;
    for(i=0; i<len; i++){
        if(!indexes[i]) continue;
        size = tm_sizeof(indexes[i]);
        data = tm_uint8_p(indexes[i]);
        for(j=0; j<size; j++){
            if(data[j] != index_hash(i, j)){
                return false;
            }
        }
    }
    return true;
}

bool fill_indexes(tm_index *indexes, tm_index len, tm_size mod){
    // Allocate indexex of size index % mod + 1
    tm_index i;
    for(i=0; i<len; i++){
        if(!indexes[i]){
            if(!alloc_index(indexes, i, i % mod + 1)) return false;
        }
    }
    return true;
}

bool alloc_index(tm_index *indexes, tm_index index, tm_size size){
    tm_size i;
    uint8_t *data;
    indexes[index] = tm_alloc(size);
    if(!indexes[index]) return false;
    data = tm_uint8_p(indexes[index]);
    for(i=0; i<size; i++){
        data[i] = index_hash(index, i);
    }
    return check_indexes(indexes, index);
}

bool free_index(tm_index *indexes, tm_index index){
    tm_free(indexes[index]);
    indexes[index] = 0;
    return true;
}

char *test_basic(){
    // allocate and dealloc
    tm_index indexes[10];
    alloc_index(indexes, 0, 4);
    return NULL;
}

char *all_tests(){
    mu_suite_start();

    mu_run_test(test_basic);
    return NULL;
}

RUN_TESTS(all_tests);
