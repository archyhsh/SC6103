#ifndef AT_MOST_ONCE_H
#define AT_MOST_ONCE_H
#include <stdint.h>

#define MAX_CACHE_SIZE 128
#define MAX_RESPONSE_SIZE 1024
typedef struct {
    uint32_t requestID;
    char response[MAX_RESPONSE_SIZE];
    int in_use;
    char date;
} CacheEntry;
void init_cache();
void set_cache_date(CacheEntry *cache);
const char* handle_request(uint32_t requestID, const char* response);
#endif
