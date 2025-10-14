#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H
#include <arpa/inet.h>
// 客户端本地地址信息结构
#define PORT 12345
int udp_client_init(const char *server_ip, struct sockaddr_in *server_addr);
ssize_t udp_send(int sockfd, const char *buffer, size_t len, struct sockaddr_in *server_addr, socklen_t addr_len);
ssize_t udp_recv(int sockfd, char *buffer, size_t bufsize, struct sockaddr_in *server_addr, socklen_t *addr_len);
#endif