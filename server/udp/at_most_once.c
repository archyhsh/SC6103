 /* 
    缓存区记录数据，用于检查客户端发送信息，用于at_most_once模式，相关函数为init_cache, lookup_cache, set_cache_date, insert_cache
    handle_request用于服务器端
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "at_most_once.h"
#include "../semantic_mode.h"

static CacheEntry cache[MAX_CACHE_SIZE];

/* 初始化缓存 */
void init_cache() {
    for (int i = 0; i < MAX_CACHE_SIZE; i++) {
        cache[i].in_use = 0;
        cache[i].client_ip.s_addr = INADDR_ANY;
    }
}
/* 根据requestID查询缓存区内容*/
const char* lookup_cache(uint32_t requestID, struct in_addr client_ip) {
    for (int i = 0; i < MAX_CACHE_SIZE; i++) {
        if (cache[i].in_use && cache[i].requestID == requestID && cache[i].client_ip.s_addr == client_ip.s_addr) {
            return cache[i].response;
        }
    }
    return NULL;
}
/* 向缓存区存入新请求信息*/
void insert_cache(uint32_t requestID, struct in_addr client_ip, const char* response) {
    for (int i = 0; i < MAX_CACHE_SIZE; i++) {
        if (!cache[i].in_use) {
            cache[i].in_use = 1;
            cache[i].requestID = requestID;
            cache[i].client_ip = client_ip;
            strncpy(cache[i].response, response, MAX_RESPONSE_SIZE);
            cache[i].response[MAX_RESPONSE_SIZE - 1] = '\0';
            printf("[Cache] Inserted new cache entry for client %s, request %u\n", inet_ntoa(client_ip), requestID);
            return;
        }
    }
    // 缓存满时的简单替换策略（实际可以用LRU等）
    int index = rand() % MAX_CACHE_SIZE;
    printf("[Cache] Cache full, replacing entry %d for client %s, request %u\n", index, inet_ntoa(client_ip), requestID);
    
    cache[index].requestID = requestID;
    cache[index].client_ip = client_ip;
    strncpy(cache[index].response, response, MAX_RESPONSE_SIZE);
    cache[index].response[MAX_RESPONSE_SIZE - 1] = '\0';
}
/* 服务器处理请求信息*/
const char* handle_request(uint32_t requestID, struct in_addr client_ip, const char* response, semantic_mode_t mode) {
    // 对于新请求或AT_LEAST_ONCE模式，总是返回新响应
    printf("[Server] Executing new request %u from %s, mode: %s\n", requestID, inet_ntoa(client_ip), mode == SEMANTIC_AT_MOST_ONCE ? "AT_MOST_ONCE" : "AT_LEAST_ONCE");
    // 如果是AT_MOST_ONCE模式，缓存结果
    if (mode == SEMANTIC_AT_MOST_ONCE) {
        insert_cache(requestID, client_ip, response);
    }
    
    return response;
}
