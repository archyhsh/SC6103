/* 
    缓存区记录数据，用于检查客户端发送信息，用于at_most_once模式，相关函数为init_cache, lookup_cache, set_cache_date, insert_cache
    handle_request用于服务器端
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
/* 设置新请求信息时间（过期清理）*/
void set_cache_date(CacheEntry *cache) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    snprintf(cache->date, sizeof(cache->date), "%04d-%02d-%02d",
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday);
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
/* 服务器处理请求信息*/
const char* handle_request(uint32_t requestID, const char* response) {
    const char* cached = lookup_cache(requestID);
    if (cached != NULL) {
        printf("[Server] Duplicate request %u, returning cached response.\n", requestID);
        return cached;
    }
    insert_cache(requestID, response);
    printf("[Server] Executed new request %u.\n", requestID);
    return response;
}
