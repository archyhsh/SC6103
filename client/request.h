#ifndef REQUEST_H
#define REQUEST_H
#include <arpa/inet.h>
typedef enum {
    SEMANTIC_AT_MOST_ONCE,
    SEMANTIC_AT_LEAST_ONCE
} semantic_mode_t;
int send_request(int sockfd, struct sockaddr_in *server_addr, char *req, char *response_buf, size_t buf_size, semantic_mode_t mode);

#endif