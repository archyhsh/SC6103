/* 序列化传递信息: 哈希信息内容作为唯一标识符ID，在序列化过程中，先传递参数长度，再传递参数，便于反序列化 */

#include <stdio.h>
#include "hash_function.h"
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "at_most_once.h"

static size_t marshalize_single(char *buf, const char *str) {
    // requestID for record, string marshalize with their individual length
    uint32_t len = strlen(str);
    uint32_t net_len = htonl(len);
    memcpy(buf, &net_len, sizeof(net_len));
    memcpy(buf + sizeof(net_len), str, len);
    return sizeof(net_len) + len;
}

int marshalizeResp(uint32_t requestID, const char *resp, char *out, size_t size) {
    int offset = 0;
    uint32_t net_id = htonl(requestID);
    memcpy(out + offset, &net_id, sizeof(net_id));
    offset += sizeof(net_id);
    offset += marshalize_single(out + offset, resp);

    return offset;
}

