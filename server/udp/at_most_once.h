#ifndef AT_MOST_ONCE_H
#define AT_MOST_ONCE_H
#include <stdint.h>
#include <arpa/inet.h>
#include "../semantic_mode.h"
#define MAX_CACHE_SIZE 128
#define MAX_RESPONSE_SIZE 1024

typedef struct {
    uint32_t requestID;
    struct in_addr client_ip;
    char response[MAX_RESPONSE_SIZE];
    int in_use;
} CacheEntry;
void init_cache();
const char* lookup_cache(uint32_t requestID, struct in_addr client_ip);
void insert_cache(uint32_t requestID, struct in_addr client_ip, const char* response);
const char* handle_request(uint32_t requestID, struct in_addr client_ip, const char* response, semantic_mode_t mode);
#endif

