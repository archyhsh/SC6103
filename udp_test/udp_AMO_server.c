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
int main(){
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1234];
    socklen_t addr_len = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
	perror("Socket Error");
	exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind error");
	   close(sockfd);
	   exit(1);
    }
    printf("Server UDP listened on Port  %d\n", PORT);

    init_cache();
    
    while(1) {
        ssize_t received_len = recvfrom(sockfd, buffer, sizeof(buffer)-1, 0, (struct sockaddr*)&client_addr, &addr_len);
	   if (received_len < 0) {
	       perror("Error receiving the msg");
	       continue;
        }
	   buffer[received_len] = '\0';
	   printf("received from %s:%d len is %ld\n",
	       inet_ntoa(client_addr.sin_addr),
	       ntohs(client_addr.sin_port),
	       received_len);
	   char *demar_data[10];
	   uint32_t requestID;
	   int parameters = demarshalize(buffer, demar_data, received_len, &requestID);
	   printf("%d, %d\n", requestID, parameters);
	   for (int i = 0; i < parameters; i++) {
	       printf("item %d: %s\n", i, demar_data[i]);
	   }

        const char *rawResp = "this is a test msg";
        const char *response = handle_request(requestID, rawResp);
        char message[1024];
        size_t respLen = marshalizeResp(requestID, response, message, sizeof(message));
	   
	   if(sendto(sockfd, message, respLen, 0, (struct sockaddr *)&client_addr, addr_len) < 0) {
            perror("response to client has failed!!!");
	   } else {
	       printf("Sent back to    %s:%d\n",
	        inet_ntoa(client_addr.sin_addr),
		   ntohs(client_addr.sin_port));
	   }
    }
    close(sockfd);
    return 0;
}
            
