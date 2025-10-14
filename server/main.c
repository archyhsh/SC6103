#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sqlite3.h>
#include "udp/udp_server.h"
#include "udp/marshalize.h"
#include "udp/demarshalize.h"
#include "udp/at_most_once.h"
#include "backend/db_init.h"
#include "backend/operate.h"
#include "semantic_mode.h"

int main() {
    int sockfd = udp_server_init();
    if (sockfd < 0) return 1;
    printf("Server listening on port %d\n", PORT);
    /* 初始化缓存和数据库*/
    init_cache();
    sqlite3 *db;
    init_db();
    int rc = sqlite3_open("./backend/CampusVenueAppointment.db", &db);
    if (rc != SQLITE_OK) {
	   fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
	   return -1;
    }
    /*轮询*/
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        char buffer[1024];
        char resp[1024];
        char occupied_slots[1024]; 
        size_t slots_size = 1024;

        ssize_t received_len = udp_recv(sockfd, buffer, 1024, &client_addr, &addr_len);
        if (received_len < 0) continue;

        buffer[received_len] = '\0';
        printf("received from %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        char *demar_data[10];
        uint32_t requestID;
        semantic_mode_t mode;
        int parameters = demarshalize(buffer, demar_data, received_len, &requestID, &mode);
        /* 重复的at most once请求直接返回记录在缓存中的response*/
        if (mode == 0) {
            const char* cached = lookup_cache(requestID, client_addr.sin_addr);
            if (cached != NULL) {
                printf("[Server AT_MOST_ONCE] Duplicate request %u from %s, returning cached response.\n", requestID, inet_ntoa(client_addr.sin_addr));
                strcpy(resp, cached);
                char message[1024];
                size_t respLen = marshalizeResp(requestID, resp, message, sizeof(message));
                printf("Sending cached response of %ld bytes to %s\n", respLen, inet_ntoa(client_addr.sin_addr));
                udp_send(sockfd, message, respLen, &client_addr, addr_len);
                for (int i = 0; i < parameters; i++) {
                    free(demar_data[i]);
                }
                continue;
            }
        } 
        printf("[Server] Processing new request %u from %s, mode: %s\n", requestID, inet_ntoa(client_addr.sin_addr), mode == SEMANTIC_AT_MOST_ONCE ? "AT_MOST_ONCE" : "AT_LEAST_ONCE");
        if (strcmp(demar_data[0], "init") == 0) {
            snprintf(resp, sizeof(resp), "OK|venues=1:TR62,2:TR68");
        }
        else if (strcmp(demar_data[0], "query") == 0) {
            if (parameters >= 3) {
                int venueId = atoi(demar_data[1]);
                char *date = demar_data[2];
                int tmp = query(db, sockfd, venueId, date, NULL, NULL, -1, occupied_slots, slots_size);
                if (tmp == -1) {
                    snprintf(resp, sizeof(resp), "ERR|System error!");
                } else {
                    snprintf(resp, sizeof(resp), "OK|occupied time period on %s: %s\n", date, occupied_slots);
                }
            } else {
                snprintf(resp, sizeof(resp), "ERR|invalid arguments for query function!");
            }
        }
        else if (strcmp(demar_data[0], "create") == 0) {
            uint32_t appointmentId;
            if (parameters >= 5) {
                int venueId = atoi(demar_data[1]);
                char *date = demar_data[2];
                char *start = demar_data[3];
                char *end = demar_data[4];
                int tmp = appoint(db, sockfd, venueId, date, start, end, &appointmentId, occupied_slots, slots_size);
                if (tmp == -1) {
                    snprintf(resp, sizeof(resp), "ERR|System error!");
                } else if (tmp == -2) {
                    snprintf(resp, sizeof(resp), "ERR|Appointment time period not available! Already occupied time period for this spot on %s: %s\n", date, occupied_slots);
                } else {
                    snprintf(resp, sizeof(resp), "OK|appointmentId=%u", appointmentId);
                }
            } else {
                snprintf(resp, sizeof(resp), "ERR|invalid arguments for create function!");
            }
        }
        else if (strcmp(demar_data[0], "alter") == 0) {
            if (parameters >= 3) {
                int appointmentId = atoi(demar_data[1]);
                double duration = atof(demar_data[2]);
                char newStart[6], newEnd[6];
                int tmp = alter(db, sockfd, appointmentId, duration, newStart, newEnd, occupied_slots, slots_size);
                if (tmp == -1) {
                    snprintf(resp, sizeof(resp), "ERR|System error!");
                } else if (tmp == -2) {
                    snprintf(resp, sizeof(resp), "ERR|Appointment time period not available! Already occupied time period for this spot on this day: %s\n", occupied_slots);
                } else if (tmp == -3) {
                    snprintf(resp, sizeof(resp), "ERR|Appointment time cannot start on the previous day or end on the next day!");
                } else if (tmp == -4) {
                    snprintf(resp, sizeof(resp), "ERR|Invalid AppointmentID!");
                }else {
                    snprintf(resp, sizeof(resp), "OK|timePeriod=%s-%s", newStart, newEnd);
                }
            } else {
                snprintf(resp, sizeof(resp), "ERR|invalid arguments for alter function!");
            }
        }
        else if (strcmp(demar_data[0], "hook") == 0) {
            if (parameters >= 4) {
                int venueId = atoi(demar_data[1]);
                long ts = atol(demar_data[2]);
                int duration = atoi(demar_data[3]);
                hook(db, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), venueId, ts, duration);
                snprintf(resp, sizeof(resp), "OK|hook registered successfully!");
            } else {
                snprintf(resp, sizeof(resp), "ERR|invalid hook format");
            }
        }
        else if (strcmp(demar_data[0], "duplicate") == 0) {
            if (parameters >= 2) {
                uint32_t appointmentId = atoi(demar_data[1]);
                uint32_t rAppointmentId;
                int tmp = duplicate(db, sockfd, appointmentId, &rAppointmentId, occupied_slots, slots_size);
                if (tmp == -1) {
                    snprintf(resp, sizeof(resp), "ERR|System error!");
                } else if (tmp == -2) {
                    snprintf(resp, sizeof(resp), "ERR|Appointment time period not available! Already occupied time period for this spot on this day: %s\n", occupied_slots);
                } else if (tmp == -3) {
                    snprintf(resp, sizeof(resp), "ERR|Invalid appointmentId!");
                } else if (tmp == -4) {
                    snprintf(resp, sizeof(resp), "ERR|Duplicate an out-of-range appointment!");
                }else {
                    snprintf(resp, sizeof(resp), "OK|appointmentId=%u", rAppointmentId);
                }
            } else {
                snprintf(resp, sizeof(resp), "ERR|invalid duplicate format");
            }
        }
        else if (strcmp(demar_data[0], "delete") == 0) {
            if (parameters >= 2) {
                uint32_t appointmentId = atoi(demar_data[1]);
                deleteAppointment(db, sockfd, appointmentId);
                snprintf(resp, sizeof(resp), "OK|your appointment has been deleted successfully!");
            } else {
                snprintf(resp, sizeof(resp), "ERR|invalid delete format");
            }
        }
        else {
            snprintf(resp, sizeof(resp), "ERR|unknown command");
        }
        /* 发送响应消息*/
        handle_request(requestID, client_addr.sin_addr, resp, mode);
        char message[1024];
        size_t respLen = marshalizeResp(requestID, resp, message, sizeof(message));
        printf("the sent msg is %s\n", resp);
        udp_send(sockfd, message, respLen, &client_addr, addr_len);
        for (int i = 0; i < parameters; i++) {
            free(demar_data[i]);
        }
    }

    close(sockfd);
    return 0;
}
