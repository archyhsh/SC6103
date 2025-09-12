#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "hash_function.h"
#include "marshalize.h"

#define PORT 12345

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char req[] = "create,1,2025-09-12,10:00,12:30";
    char message[1024];
    size_t msg_len = marshalize(req, message, sizeof(message));
    
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

    
    sendto(sockfd, message, msg_len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Sent msg %s to server", message);

    close(sockfd);
    return 0;
}
