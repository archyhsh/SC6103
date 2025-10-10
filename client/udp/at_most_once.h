#ifndef AT_MOST_ONCE_H
#define AT_MOST_ONCE_H
#include <stdint.h>

#define MAX_CACHE_SIZE 128
#define MAX_RESPONSE_SIZE 1024
#define DATE_STR_LEN 11   // "YYYY-MM-DD" + '\0'
typedef struct {
    uint32_t requestID;
    char msg[MAX_RESPONSE_SIZE];
    char response[MAX_RESPONSE_SIZE];
    int in_use;
    char date[DATE_STR_LEN];
} CacheEntry;
void init_cache();
const char* lookup_cache(uint32_t requestID);
void set_cache_date(CacheEntry *cache);
void insert_cache(uint32_t requestID, const char* response);
const char* handle_request(uint32_t requestID, const char* response);
const char* handle_response(uint32_t requestID, const char* response);
#endif

