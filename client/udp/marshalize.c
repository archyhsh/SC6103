/* 序列化传递信息: 哈希信息内容作为唯一标识符ID，在序列化过程中，先传递参数长度，再传递参数，便于反序列化 */

#include <stdio.h>
#include "hash_function.h"
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "at_most_once.h"
#include "../semantic_mode.h"

static size_t marshalize_single(char *buf, const char *str) {
    // requestID for record, string marshalize with their individual length
    uint32_t len = strlen(str);
    uint32_t net_len = htonl(len);
    memcpy(buf, &net_len, sizeof(net_len));
    memcpy(buf + sizeof(net_len), str, len);
    return sizeof(net_len) + len;
}
int marshalizeReq(char *req, char *out, size_t size, uint32_t *requestID, semantic_mode_t mode) {
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
	uint32_t requestID_tmp = hash2(req);
	if (requestID != NULL) {
        *requestID = requestID_tmp;
    }
	/* 序列化 requestID */
	uint32_t net_id = htonl(requestID_tmp);
	memcpy(out + offset, &net_id, sizeof(net_id));
	offset += sizeof(net_id);
	/* 序列化 语义模式 */
	uint32_t mode_id = (uint32_t)mode;
	uint32_t net_mode = htonl(mode_id);
    memcpy(out + offset, &net_mode, sizeof(net_mode));
    offset += sizeof(net_mode);
	for (int i = 0; i < count; i++) {
	    offset += marshalize_single(out + offset, tmp[i]);	
	}
	return offset;
}

