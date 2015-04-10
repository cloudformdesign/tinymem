
#ifndef __memptr_h
#define __memptr_h
#include <stdint.h>
#include <stdbool.h>

#define tmdebug(...)      do{printf("[DEBUG](%s,%u):", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n");}while(0)

typedef uint16_t tm_index;
typedef uint16_t tm_size;

#endif
