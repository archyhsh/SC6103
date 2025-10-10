#ifndef REQUEST_H
#define REQUEST_H
#include <arpa/inet.h>

int send_request(int sockfd, struct sockaddr_in *server_addr, char *req, char *response_buf, size_t buf_size, uint32_t requestID);

#endif