#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "udp/udp_client.h"
#include "udp/marshalize.h"
#include "udp/demarshalize.h"
#include "udp/at_most_once.h"
#include "request.h"
#include "parseTime.h"


// server msg type should be like:
// 1. OK|appointmentId=123456    OK|hooked    OK|venues=TR61,TR62,TR63
// 2. ERR|InvalidTime      ERR|VenueNotAvailable     ERR|AppointmentNotFound


int main() {
    struct sockaddr_in server_addr;
    int sockfd = udp_client_init("192.168.8.128", &server_addr);
    if (sockfd < 0) return 1;
    init_cache();
    uint32_t reqID = 0;
    char response[1024];
    char command[50];
    if (send_request(sockfd, &server_addr, "init", response, sizeof(response), reqID) > 0) {
        if (strncmp(response, "ERR", 3) == 0) {
            printf("[INIT失败] %s\n", response);
        } else {
            printf("[INIT成功] 场所列表: %s\n", response);
        }
    }
    printf("欢迎使用预约系统！\n");
    printf("请输入命令 (create_new / alter_exist / subscribe_venue / exit):\n");
    while (1) {
        printf("\n> ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        command[strcspn(command, "\n")] = '\0';
        if (strcmp(command, "create_new") == 0) {
            int venueId;
            char date[16], startTime[8], endTime[8];
            printf("请输入您要预约的场所ID:\n> ");
            scanf("%d", &venueId);
            getchar();
            printf("请输入您要预约的日期:(YYYY-MM-DD):\n> ");
            scanf("%s", date);
            getchar();
            printf("请输入您预约的开始时间 (HH:MM) (整点或半点):\n> ");
		  scanf("%s", startTime);
		  getchar();
            printf("请输入您预约的结束时间 (HH:MM) (整点或半点):\n> ");
            scanf("%s", endTime);
		  getchar();
            if (strcmp(startTime, endTime) >= 0) {
                break;
            }
            char req[128];
            snprintf(req, sizeof(req), "create,%d,%s,%s,%s", venueId, date, startTime, endTime);
            if (send_request(sockfd, &server_addr, req, response, sizeof(response), reqID) > 0) {
                if (strncmp(response, "ERR", 3) == 0) {
                    printf("[预约失败] %s\n", response);
                } else {
                    int appointmentId = atoi(response + 17); // "OK|appointmentId=123456"
                    printf("✅ 预约成功！appointmentId = %d\n", appointmentId);
                }
            }
        }
        else if (strcmp(command, "alter_exist") == 0) {
            int appointmentId;
            float newDuration;
            printf("请输入预约ID: \n>  ");
            scanf("%d", &appointmentId);
            getchar();
            printf("您不能改变持续时间，只能尝试调整起始时间，输入正数代表推迟，输入负数代表提前: 单位：小时\n");
            scanf("%f", &newDuration);
            getchar();

            char req[128];
            snprintf(req, sizeof(req), "alter,%d,%.2f", appointmentId, newDuration);

            if (send_request(sockfd, &server_addr, req, response, sizeof(response), reqID) > 0) {
                if (strncmp(response, "ERR", 3) == 0) {
                    printf("[修改预约失败] %s\n", response);
                } else {
                	char timePeriod[16];
                	const char *pos = strstr(response, "timePeriod="); // "OK|appointmentId=123456"
                    if (pos) {
                        strcpy(timePeriod, pos + strlen("timePeriod="));
                        printf("✅ 修改预约成功！新的预约时间段为: %s\n", timePeriod);
                    }
                }
            }
        }
        else if (strcmp(command, "subscribe_venue") == 0) {
            int venueId, duration;
            time_t now = time(NULL);
            double durationHour;
            struct tm *tm_info = localtime(&now);
            char date[11];
            char timepoint[6];
            strftime(date, sizeof(date), "%Y-%m-%d", tm_info);
            strftime(timepoint, sizeof(timepoint), "%H:%M", tm_info);
            long startTs = parseTime(date, timepoint);
            printf("请输入您想订阅的场所ID: \n>  ");
            scanf("%d", &venueId);
            getchar();
            printf("请输入您希望订阅的时长（单位：小时）: \n>  ");
            scanf("%lf", &durationHour);
            getchar();
            duration = durationHour * 3600;
            char req[128];
            snprintf(req, sizeof(req), "hook,%d,%ld,%d", venueId, startTs, duration);
            if (send_request(sockfd, &server_addr, req, response, sizeof(response), reqID) > 0) {
                if (strncmp(response, "ERR", 3) == 0) {
                    printf("[订阅场所推送失败] %s\n", response);
                } else {
                    printf("✅ 订阅场所模式开启！剩余时长为: %.lf 小时\n", durationHour);
                    // 待机状态，只能接受服务器发送来的消息，无法返回response，也无法发送新的申请
                }
            }
        }
        else if (strcmp(command, "exit") == 0) {
            printf("退出系统，再见！\n");
            break;
        }
        else {
            printf("无效命令，请重试。\n");
        }
    }
    close(sockfd);
    return 0;
}