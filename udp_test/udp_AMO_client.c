#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "hash_function.h"
#include "marshalize.h"
#include "demarshalize.h"
#include "at_most_once.h"

#define PORT 12345

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    init_cache();
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error socket");
        exit(1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("192.168.8.128");
    server_addr.sin_port = htons(PORT);

    // if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    //     perror("Error bind");
    //     close(sockfd);
    //     exit(1);
    // }

    // printf("Server UDP listening on port  %d\n", PORT);

    while (1) {
        char req[] = "create,1,2025-09-12,10:00,12:30";
        char message[1024];
        size_t msg_len = marshalizeReq(req, message, sizeof(message));    	
        sendto(sockfd, message, msg_len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        printf("Sent msg %s to server", message);
        char buffer[1024];
        socklen_t addr_len = sizeof(server_addr);
        ssize_t received_len = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&server_addr, &addr_len);
        if (received_len < 0) {
            perror("接收失败");
            continue;
        }
        buffer[received_len] = '\0';
        printf("收到来自服务器 %s:%d 的响应 \n",
               inet_ntoa(server_addr.sin_addr),
               ntohs(server_addr.sin_port));
        char *demar_data[10];
	   uint32_t requestID;
	   int parameters = demarshalize(buffer, demar_data, received_len, &requestID);
	   const char* cached = lookup_cache(requestID);
	   if (cached != NULL) {
            printf("find existed requestID, request is %s", cached);
        }
        else {
        	  printf("Cannot find requestID %d in buffer!", requestID);
        }
        sleep(1);
        break;
    }
    close(sockfd);
    return 0;
}
