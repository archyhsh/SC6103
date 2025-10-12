/* 服务器端udp*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#define PORT 12345
/* 初始化服务器信息*/
int udp_server_init() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket error");
        return -1;
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error");
        close(sockfd);
        return -1;
    }
    return sockfd;
}
/* 发送函数 */
ssize_t udp_recv(int sockfd, char *buffer, size_t bufsize, struct sockaddr_in *client_addr, socklen_t *addr_len) {
    // sockfd from init(), char buffer[], socklen_t addr_len = sizeof(client_addr);
    return recvfrom(sockfd, buffer, bufsize, 0, (struct sockaddr *)client_addr, addr_len);
}
/* 接收函数 */
ssize_t udp_send(int sockfd, const char *buffer, size_t len, struct sockaddr_in *client_addr, socklen_t addr_len) {
    return sendto(sockfd, buffer, len, 0, (struct sockaddr *)client_addr, addr_len);
}