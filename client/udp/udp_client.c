#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345
int udp_client_init(const char *ip, struct sockaddr_in *server_addr) {
    // ip: "192.168.8.128"
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket error");
        return -1;
    }
    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(PORT);
    server_addr->sin_addr.s_addr = inet_addr(ip);
    if (server_addr->sin_addr.s_addr == INADDR_NONE) {
        perror("invalid IP address");
        close(sockfd);
        return -1;
    }
    return sockfd;
}
ssize_t udp_send(int sockfd, const char *buffer, size_t len, struct sockaddr_in *server_addr, socklen_t addr_len) {
    //
    return sendto(sockfd, buffer, len, 0, (struct sockaddr *)server_addr, addr_len);
}
ssize_t udp_recv(int sockfd, char *buffer, size_t bufsize, struct sockaddr_in *server_addr, socklen_t *addr_len) {
    // sockfd from init(), char buffer[], socklen_t addr_len = sizeof(client_addr);
    return recvfrom(sockfd, buffer, bufsize, 0, (struct sockaddr *)server_addr, addr_len);
}
