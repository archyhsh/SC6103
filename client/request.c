#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "udp/udp_client.h"
#include "udp/marshalize.h"
#include "udp/demarshalize.h"
#include "udp/at_most_once.h"
/* 客户端发送请求，之后接收响应*/
int send_request(int sockfd, struct sockaddr_in *server_addr, char *req, char *response_buf, size_t buf_size, uint32_t requestID) {
    char message[1024];
    size_t msg_len = marshalizeReq(req, message, sizeof(message)); 
    if (udp_send(sockfd, message, msg_len, server_addr, sizeof(*server_addr)) < 0) {
        perror("发送失败");
        return -1;
    }
    printf("Sent marshalized msg to server, length is %ld\n", msg_len);

    char buffer[1024];
    socklen_t addr_len = sizeof(*server_addr);
    ssize_t received_len = udp_recv(sockfd, buffer, sizeof(buffer)-1, server_addr, &addr_len);
    if (received_len < 0) {
        perror("接收失败");
        return -1;
    }
    buffer[received_len] = '\0';

    printf("收到来自服务器 %s:%d 的响应, 信息长度为%ld \n",
           inet_ntoa(server_addr->sin_addr),
           ntohs(server_addr->sin_port),
           received_len);
    char *demar_data[10];
    int count = demarshalize(buffer, demar_data, received_len, &requestID);
    if (count > 0 && response_buf != NULL) { 
    	   strncpy(response_buf, demar_data[0], buf_size-1); 
    	   response_buf[buf_size-1] = '\0'; 
    }
    return count;  
}