/* 
    handle_response用于客户端
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "at_most_once.h"

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