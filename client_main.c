#include "udp/udp_client.h"
#include "udp/marshalize.h"
#include "udp/demarshalize.h"
#include "udp/at_most_once.h"
#include <stdio.h>
#include <arpa/inet.h>

int main() {
    struct sockaddr_in server_addr;
    int sockfd = udp_client_init("192.168.8.128", &server_addr);
    if (sockfd < 0) return 1;
    init_cache();

    while (1) {
        char req[] = "create,1,2025-09-12,10:00,12:30";
        char message[1024];
        size_t msg_len = marshalizeReq(req, message, sizeof(message)); 
        udp_send(sockfd, message, msg_len, (struct sockaddr_in *)&server_addr, sizeof(server_addr));
        printf("Sent marshalized msg to server, length is %ld\n", msg_len);
        char buffer[1024];
        socklen_t addr_len = sizeof(server_addr);
        ssize_t received_len = udp_recv(sockfd, buffer, 1024, (struct sockaddr_in *)&server_addr, &addr_len);
        if (received_len < 0) {
            perror("接收失败");
            continue;
        }
        buffer[received_len] = '\0';
        printf("收到来自服务器 %s:%d 的响应, 信息长度为%ld \n",
               inet_ntoa(server_addr.sin_addr),
               ntohs(server_addr.sin_port),
               received_len);
        char *demar_data[10];
	   uint32_t requestID;
	   int count = demarshalize(buffer, demar_data, received_len, &requestID);
	   for (int i = 0; i < count; i++) {
	   	printf("response is: %s\n", demar_data[i]);
	   }
//	   printf("response is: %s", demar_data[0]);
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