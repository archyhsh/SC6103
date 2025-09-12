#ifndef UNMARSHALIZE_H
#define UNMARSHALIZE_H

#include <stdint.h>
#include <arpa/inet.h>

size_t demarshalize_single(char *buffer, char **out);
int demarshalize(char *input, char *out[], size_t size, uint32_t *requestID);

#endif