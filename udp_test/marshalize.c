#include <stdio.h>
#include "hash_function.h"
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

size_t marshalize_single(char *buf, const char *str) {
    // requestID for record, string marshalize with their individual length
    uint32_t len = strlen(str);
    uint32_t net_len = htonl(len);
    memcpy(buf, &net_len, sizeof(net_len));
    memcpy(buf + sizeof(net_len), str, len);
    return sizeof(net_len) + len;
}

int marshalize(char *req, char *out, size_t size) {
	char *tmp[10];
	int count = 0;
	int offset = 0;
	char *period = strtok(req, ",");
	while (period != NULL && count < 10) {
		tmp[count++] = period;
		period = strtok(NULL, ",");
	}
	uint32_t requestID = hash2(req);
	uint32_t net_id = htonl(requestID);
	memcpy(out + offset, &net_id, sizeof(net_id));
	offset += sizeof(net_id);
	for (int i = 0; i < count; i++) {
	    offset += marshalize_single(out + offset, tmp[i]);	
	}
	return offset;
}
