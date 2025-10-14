#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "udp/udp_client.h"
#include "udp/marshalize.h"
#include "udp/demarshalize.h"
#include "udp/at_most_once.h"
#include "semantic_mode.h"

/* 客户端发送请求，之后接收响应*/
/* 传入的requestID并没有实值*/
int send_request(int sockfd, struct sockaddr_in *server_addr, char *req, char *response_buf, size_t buf_size, semantic_mode_t mode) {
    printf("\nCurrently using mode: %s\n", mode == SEMANTIC_AT_MOST_ONCE ? "AT_MOST_ONCE" : "AT_LEAST_ONCE");
    char message[1024];
    uint32_t requestID;
    size_t msg_len = marshalizeReq(req, message, sizeof(message), &requestID, mode); 
    const char* cached_response = lookup_cache(requestID);
    if (cached_response != NULL) {
        printf("Using cached response for request %u\n", requestID);
        if (response_buf != NULL) {
            strncpy(response_buf, cached_response, buf_size-1);
            response_buf[buf_size-1] = '\0';
        }
        return -2;
    }

    // 发送和重试逻辑
    int max_attempts = 3;
    int timeout_ms = 2000;
    
    for (int attempt = 0; attempt < max_attempts; attempt++) {
        // 发送请求
        if (udp_send(sockfd, message, msg_len, server_addr, sizeof(*server_addr)) < 0) {
            perror("Send failed");
            if (attempt == max_attempts - 1) return -1;
            continue;
        }
        
        // 设置接收超时
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        // 接收响应
        char buffer[1024];
        socklen_t addr_len = sizeof(*server_addr);
        ssize_t received_len = udp_recv(sockfd, buffer, sizeof(buffer)-1, server_addr, &addr_len);
        
        if (received_len > 0) {
            buffer[received_len] = '\0';
            
            printf("Recived response from server %s:%d\n",
                   inet_ntoa(server_addr->sin_addr),
                   ntohs(server_addr->sin_port));
            
            // 解析响应
            char *demar_data[10];
            int count = demarshalize(buffer, demar_data, received_len);
            
            if (count > 0 && response_buf != NULL) {
                strncpy(response_buf, demar_data[0], buf_size-1);
                response_buf[buf_size-1] = '\0';
                
                // !!! 客户端缓存响应结果 !!!
                insert_cache(requestID, demar_data[0]);
                printf("Cached response for request %u\n", requestID);
                
                // 清理临时数据
                for (int i = 0; i < count; i++) {
                    free(demar_data[i]);
                }
            }
            
            return count;
        } else {
            if (attempt < max_attempts - 1) {
                printf("udp transmission timeout, retrying...\n");
            } else {
                printf("ERROR! udp transmission failed!\n");
            }
        }
    }
    
    return -1;
} 