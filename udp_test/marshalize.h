#ifndef MARSHALIZE_H
#define MARSHALIZE_H

#include <stdint.h>
#include <arpa/inet.h>

size_t marshalize_single(char *buf, const char *str);
int marshalizeReq(char *req, char* out, size_t size);
int marshalizeResp(uint32_t requestID, const char *resp, char *out, size_t size);
#endif