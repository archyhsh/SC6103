#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include "udp/udp_client.h"
#include "udp/marshalize.h"
#include "udp/demarshalize.h"
#include "udp/at_most_once.h"
#include "request.h"
#include "parseTime.h"
#include "config.h"
/* 小工具：用于校验新请求日期合法*/
int validate_date(const char *dateStr) {
    struct tm input_tm = {0};
    time_t now = time(NULL);
    struct tm today_tm;
    localtime_r(&now, &today_tm);
    if (strptime(dateStr, "%Y-%m-%d", &input_tm) == NULL) {
        printf("日期格式错误，请输入 YYYY-MM-DD 格式\n");
        return -1;
    }
    time_t input_time = mktime(&input_tm);
    today_tm.tm_hour = today_tm.tm_min = today_tm.tm_sec = 0;
    time_t today_time = mktime(&today_tm);
    double diff_days = difftime(input_time, today_time) / (60 * 60 * 24);
    if (diff_days < 0) {
        printf("错误：预约日期不能早于今天！\n");
        return -2;
    }
    if (diff_days > 7) {
        printf("错误：预约日期不能超过一周！\n");
        return -3;
    }
    return 0;
}
/* 小工具：用于校验新请求时间点合法*/
int validate_time(const char *timeStr) {
    int hour, minute;
    if (sscanf(timeStr, "%d:%d", &hour, &minute) != 2) {
        return -1; 
    }
    if (hour < 0 || hour > 23) {
        return -2; 
    }
    if (!(minute == 0 || minute == 30)) {
        return -3; 
    }
    return 0;
}
/* 
1、主流程: 客户端本地初始化缓存-> 向服务器发送初始化请求（返回场所列表）->用户选择6个功能进行键入->根据键入内容发送请求->返回信息后反馈给客户端
2、六个方法：创建新预约、修改现有预约时间段、删除现有预约、基于现有预约生成新预约、订阅新场所、退出
*/
int main() {
    struct sockaddr_in server_addr;
    int sockfd = udp_client_init(SERVER_IP, &server_addr);
    if (sockfd < 0) return 1;
    // at least once 配置入口： init_cache_with_params(CACHE_SIZE, UDP_TIMEOUT_MS, MAX_RETRIES);
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
    printf("请输入命令 (create_new(创建一个新的预约) / alter_exist(修改现有预约时间段) / delete_exist(删除现有预约) / duplicate_exist(基于现有预约生成新预约) / subscribe_venue(订阅场所信息) / exit(退出)):\n");
    while (1) {
        printf("\n> ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        command[strcspn(command, "\n")] = '\0';
        if (strcmp(command, "create_new") == 0) {
            int venueId;
            char date[16], startTime[8], endTime[8];
            printf("现有场所为: %s\n请输入您要预约的场所ID: \n> ", response);
            scanf("%d", &venueId);
            getchar();
            while (1) {
                printf("请输入您要预约的日期:(YYYY-MM-DD):\n> ");
                scanf("%s", date);
                getchar();
                int tmp = validate_date(date);
                if (tmp == -1) {
            	     printf("请输入合法日期格式(YYYY-MM-DD)\n");
                } else if (tmp == -2) {
            	     printf("预约时间应不早于今天！");
                } else if (tmp == -3) {
            	     printf("预约时间不能超过一周！");
                } else {
                    break;
                }
            }
            while (1) {
                printf("请输入您预约的开始时间 (HH:MM) (整点或半点):\n> ");
		      scanf("%s", startTime);
		      getchar();
		      int tmp1 = validate_time(startTime);
                printf("请输入您预约的结束时间 (HH:MM) (整点或半点):\n> ");
                scanf("%s", endTime);
		      getchar();
		      int tmp2 = validate_time(endTime);
                if (strcmp(startTime, endTime) < 0) {
                    break;
                } else if (tmp1 < 0 || tmp2 < 0) {
                	printf("请输入正确的时间！");
                } else {
                	printf("请输入您预约的结束时间 (HH:MM) (整点或半点):\n> ");
                }
            }
            char req[128];
            snprintf(req, sizeof(req), "create,%d,%s,%s,%s", venueId, date, startTime, endTime);
            if (send_request(sockfd, &server_addr, req, response, sizeof(response), reqID) > 0) {
                if (strncmp(response, "ERR", 3) == 0) {
                    printf("处理您的新建预约申请发生了错误: %s\n", response);
                } else {
                    int appointmentId = atoi(response + 17); // "OK|appointmentId=123456"
                    printf("您的修改预约申请已办理成功！请保存好你的预约ID: %d\n", appointmentId);
                }
            }
        }
        else if (strcmp(command, "alter_exist") == 0) {
            int appointmentId;
            float newDuration;
            printf("请输入预约ID: \n>  ");
            scanf("%d", &appointmentId);
            getchar();
            printf("您不能改变持续时间，但可以调整时间段，输入正数代表推迟，输入负数代表提前: 单位：小时\n");
            scanf("%f", &newDuration);
            getchar();
            char req[128];
            snprintf(req, sizeof(req), "alter,%d,%.2f", appointmentId, newDuration);
            if (send_request(sockfd, &server_addr, req, response, sizeof(response), reqID) > 0) {
                if (strncmp(response, "ERR", 3) == 0) {
                    printf("处理您的修改预约申请发生了错误: %s\n", response);
                } else {
                	char timePeriod[16];
                	const char *pos = strstr(response, "timePeriod="); // "OK|timePeriod=123456"
                    if (pos) {
                        strcpy(timePeriod, pos + strlen("timePeriod="));
                        printf("您的修改预约申请已办理成功！新的预约时间段为: %s\n", timePeriod);
                    }
                }
            }
        }
        else if (strcmp(command, "delete_exist") == 0) {
            int appointmentId;
            printf("删除预约操作不可撤销，如果确认，请输入您的预约ID: \n>  ");
            scanf("%d", &appointmentId);
            getchar();

            char req[128];
            snprintf(req, sizeof(req), "delete,%d", appointmentId);

            if (send_request(sockfd, &server_addr, req, response, sizeof(response), reqID) > 0) {
                if (strncmp(response, "ERR", 3) == 0) {
                    printf("处理您的删除预约申请发生了错误: %s\n", response);
                } else {
                    printf("您的删除预约申请已办理成功!\n");
                }
            }
        }
        else if (strcmp(command, "duplicate_exist") == 0) {
            int appointmentId;
            printf("请输入预约ID: \n>  ");
            scanf("%d", &appointmentId);
            getchar();
            char req[128];
            snprintf(req, sizeof(req), "duplicate,%d", appointmentId);

            if (send_request(sockfd, &server_addr, req, response, sizeof(response), reqID) > 0) {
                if (strncmp(response, "ERR", 3) == 0) {
                    printf("处理您的复制预约申请发生了错误: %s\n", response);
                } else {
                	int appointmentId = atoi(response + 17); // "OK|appointmentId=123456"
                    printf("您的新预约申请已办理成功！请保存好你的预约ID: %d\n", appointmentId);
                }
            }
        }
        else if (strcmp(command, "subscribe_venue") == 0) {
            int venueId, duration;
            double durationHour;
            time_t now = time(NULL);
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
                    printf("处理您的订阅场所申请发生了错误: %s\n", response);
                } else {
                    printf("✅ 订阅场所模式开启！剩余时长为: %.lf 小时\n", durationHour);
                    // 待机状态，客户端计时过程中实现:等待服务器消息+检测订阅超时，超时后恢复正常通信功能
                    // based on chatgpt: select & poll
                    fd_set readfds;
                    struct timeval tv;
                    while (1) {
                        FD_ZERO(&readfds);
                        FD_SET(sockfd, &readfds);
                        tv.tv_sec = 1;
                        tv.tv_usec = 0;
                        int ret = select(sockfd + 1, &readfds, NULL, NULL, &tv);
                        if (difftime(time(NULL), startTs) >= durationHour * 3600) {
                            printf("⏰ 订阅时长已到，退出待机模式。\n");
                            break;
                        }
                        if (ret > 0 && FD_ISSET(sockfd, &readfds)) {
                            char msg[128];
                            socklen_t addr_len = sizeof(server_addr);
                            ssize_t received_len = recvfrom(sockfd, msg, sizeof(msg)-1, 0, (struct sockaddr *)&server_addr, &addr_len);
                            if (received_len > 0) {
                                msg[received_len] = '\0';
                                uint32_t requestID;
                                char *demar_data[10];
                                demarshalize(msg, demar_data, received_len, &requestID);
                                printf("subscribe callback from server: %s\n", demar_data[0]);
                            }
                        } else if (ret < 0) {
                            perror("select error");
                            break;
                        }
                     // ret == 0 -> 超时，继续循环判断时间
                   }
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