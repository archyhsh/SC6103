/* 
    handle_response用于客户端
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "at_most_once.h"
static CacheEntry cache[MAX_CACHE_SIZE];
/* 初始化缓存 */
void init_cache() {
    for (int i = 0; i < MAX_CACHE_SIZE; i++) {
        cache[i].in_use = 0;
    }
}
/* 根据requestID查询缓存区内容*/
const char* lookup_cache(uint32_t requestID) {
    for (int i = 0; i < MAX_CACHE_SIZE; i++) {
        if (cache[i].in_use && cache[i].requestID == requestID) {
            return cache[i].response;
        }
    }
    return NULL;
}
/* 向缓存区存入新请求信息*/
void insert_cache(uint32_t requestID, const char* response) {
    for (int i = 0; i < MAX_CACHE_SIZE; i++) {
        if (!cache[i].in_use) {
            cache[i].in_use = 1;
            cache[i].requestID = requestID;
            strncpy(cache[i].response, response, MAX_RESPONSE_SIZE);
            set_cache_date(&cache[i]);
            return;
        }
    }
}
/* 确认返回信息*/
const char* handle_response(uint32_t requestID, const char* response) {
    const char* cached = lookup_cache(requestID);
    if (cached != NULL) {
        printf("[Client] Already cached response for request %u.\n", requestID);
        return cached;
    }
    insert_cache(requestID, response);
    printf("[Client] Stored new response for request %u.\n", requestID);
    return response;
}