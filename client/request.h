#ifndef REQUEST_H
#define REQUEST_H
#include <arpa/inet.h>
#include "semantic_mode.h"
int send_request(int sockfd, struct sockaddr_in *server_addr, char *req, char *response_buf, size_t buf_size, semantic_mode_t mode);

#endif