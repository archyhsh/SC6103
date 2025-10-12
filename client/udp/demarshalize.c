/* 反序列化信息， 占位ID获取传来的信息ID，之后用一个字符串列表记录传送来的参数 */

#include <stdio.h>
#include "hash_function.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

size_t demarshalize_single(char *buffer, char **out) {
    // unmarshalize request and string with their length
    uint32_t len;
    memcpy(&len, buffer, 4);
    len = ntohl(len);
    *out = (char*)malloc(len + 1);
    memcpy(*out, buffer + 4, len);
    (*out)[len] = '\0';
    return 4 + len;
}

int demarshalize(char *input, char *out[], ssize_t size) {
    ssize_t offset = 0;
    // uint32_t requestID;
    // memcpy(&requestID, input + offset, 4);
    // offset += 4;
    int index = 0;
    printf("%ld, %ld\n", offset, size);
    while (offset + 4 < size) {
    	   uint32_t len;
        memcpy(&len, input + offset, 4);
        printf("%d\n", len);
        len = ntohl(len);
        printf("%d\n", len); 
        if (offset + 4 + len > size) {
            break;
        }
        offset += demarshalize_single(input + offset, &out[index]);
        index++;
    }
    return index;
}