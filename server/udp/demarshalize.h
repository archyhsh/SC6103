#ifndef UNMARSHALIZE_H
#define UNMARSHALIZE_H

#include <stdint.h>
#include <arpa/inet.h>
#include "../semantic_mode.h"

int demarshalize(char *input, char *out[], ssize_t size, uint32_t *requestID, semantic_mode_t *mode);

#endif