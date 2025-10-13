#ifndef MARSHALIZE_H
#define MARSHALIZE_H

#include <stdint.h>
#include <arpa/inet.h>

int marshalizeResp(uint32_t requestID, const char *resp, char *out, size_t size)
;
#endif