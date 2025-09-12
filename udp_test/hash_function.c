#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
uint32_t hash2(const char *str) { 
    /* The Dan Bernstein popuralized hash. */ 
    uint32_t hash = 5381; 
    for (int i = 0; i < (int)strlen(str); i++){ 
        hash += (hash << 5); 
        hash ^= (uint8_t)str[i]; 
    } 
    return hash; 
};