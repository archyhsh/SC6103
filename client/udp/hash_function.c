/* 传统的哈希函数DJB2的简单变体*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
uint32_t hash2(const char *str) { 
    /* The Dan Bernstein popuralized hash. */ 
    uint32_t hash = 5381; 
    while (*str) {
        hash += (hash << 5);
        hash ^= (uint8_t)*str++;
    }
    return hash;
};