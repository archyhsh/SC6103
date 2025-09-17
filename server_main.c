#include "udp/udp_server.h"
#include "udp/marshalize.h"
#include "udp/demarshalize.h"
#include "udp/at_most_once.h"
#include <stdio.h>
#include <arpa/inet.h>

int main() {
    int sockfd = udp_server_init(PORT);
    if (sockfd < 0) return 1;
    printf("Server listening on port %d\n", PORT);
    init_cache();

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        char buffer[1024];

        ssize_t received_len = udp_recv(sockfd, buffer, 1024, &client_addr, &addr_len);
        if (received_len < 0) continue;

        buffer[received_len] = '\0';
        printf("received from %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        char *demar_data[10];
        uint32_t requestID;
//        int parameters = demarshalize(buffer, demar_data, received_len, &requestID);
        demarshalize(buffer, demar_data, received_len, &requestID);
        const char *rawResp = "this is a test msg";
        handle_request(requestID, rawResp);
        char message[1024];
        size_t respLen = marshalizeResp(requestID, rawResp, message, sizeof(message));
        printf("the sent msg is of %ld length\n", respLen);
        udp_send(sockfd, message, respLen, &client_addr, addr_len);
    }

    close(sockfd);
    return 0;
}
