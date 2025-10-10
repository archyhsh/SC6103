#include <stdio.h>
#include "hash_function.h"
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "at_most_once.h"

size_t marshalize_single(char *buf, const char *str) {
    // requestID for record, string marshalize with their individual length
    uint32_t len = strlen(str);
    uint32_t net_len = htonl(len);
    memcpy(buf, &net_len, sizeof(net_len));
    memcpy(buf + sizeof(net_len), str, len);
    return sizeof(net_len) + len;
}

int marshalizeReq(char *req, char *out, size_t size) {
	char *tmp[10];
	char buffer[1024];
     strncpy(buffer, req, sizeof(buffer));
	buffer[sizeof(buffer)-1] = '\0';
	int count = 0;
	int offset = 0;
	char *period = strtok(buffer, ",");
	while (period != NULL && count < 10) {
		tmp[count++] = period;
		period = strtok(NULL, ",");
	}
	uint32_t requestID = hash2(req);
	insert_cache(requestID, req);
	uint32_t net_id = htonl(requestID);
	memcpy(out + offset, &net_id, sizeof(net_id));
	offset += sizeof(net_id);
	for (int i = 0; i < count; i++) {
	    offset += marshalize_single(out + offset, tmp[i]);	
	}
	return offset;
}

int marshalizeResp(uint32_t requestID, const char *resp, char *out, size_t size) {
    int offset = 0;
    uint32_t net_id = htonl(requestID);
    memcpy(out + offset, &net_id, sizeof(net_id));
    offset += sizeof(net_id);
    offset += marshalize_single(out + offset, resp);

    return offset;
}

