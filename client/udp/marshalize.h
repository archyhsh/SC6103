#ifndef MARSHALIZE_H
#define MARSHALIZE_H

#include <stdint.h>
#include <arpa/inet.h>
#include "../semantic_mode.h"

int marshalizeReq(char *req, char *out, size_t size, uint32_t *requestID, semantic_mode_t mode);

#endif