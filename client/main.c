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
#include "config.h"
#include "semantic_mode.h"
/* 小工具：用于校验新请求日期合法*/
int validate_date(const char *dateStr) {
    struct tm input_tm = {0};
    time_t now = time(NULL);
    struct tm today_tm;
    localtime_r(&now, &today_tm);
    if (strptime(dateStr, "%Y-%m-%d", &input_tm) == NULL) {
        //printf("Invalid date format. Please use YYYY-MM-DD.\n");
        return -1;
    }
    time_t input_time = mktime(&input_tm);
    today_tm.tm_hour = today_tm.tm_min = today_tm.tm_sec = 0;
    time_t today_time = mktime(&today_tm);
    double diff_days = difftime(input_time, today_time) / (60 * 60 * 24);
    if (diff_days < 0) {
        //printf("Error: Appointment date cannot be earlier than today!\n");
        return -2;
    }
    if (diff_days > 7) {
        //printf("Error: Appointment date must be within one week from today!\n");
        return -3;
    }
    return 0;
}
/* 小工具：用于校验新请求时间点合法*/
int validate_time(const char *timeStr) {
    int hour, minute;
    char extra;
    if (sscanf(timeStr, "%d:%d%c", &hour, &minute, &extra) != 2) {
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
    init_cache();
    semantic_mode_t mode;
    char response[1024];
    char command[50];
    char venues_list[1024];

    if (send_request(sockfd, &server_addr, "init", response, sizeof(response), 0) == -1) {
        printf("UDP transmission failed!\n");
        printf("Exiting.....................\n");
        close(sockfd);
        return 0;
    }

    if (strncmp(response, "ERR", 3) == 0) {
        printf("[INIT FAILED] %s\n", response);
    } else {
        printf("[INIT SUCCESS] Available venues: %s\n", response);
        strncpy(venues_list, response, sizeof(venues_list) - 1);
        venues_list[sizeof(venues_list) - 1] = '\0';
    }

    printf("\n\033[1;92m   WELCOME TO THE APPOINTMENT BOOKING SYSTEM  \033[0m\n");

    while (1) {
        printf("\033[1;33m┌───────────────────────────────────────────────┐\033[0m\n");
        printf("\033[1;33m│\033[0m \033[1mSelect Communication Semantic Mode            \033[33m│\033[0m\n");
        printf("\033[1;33m├───────────────────────────────────────────────┤\033[0m\n");
        printf("\033[1;33m│\033[0m 0 - At_Most_Once  (Recommended for queries)   \033[1;33m│\033[0m\n");
        printf("\033[1;33m│\033[0m 1 - At_Least_Once (Recommended for booking)   \033[1;33m│\033[0m\n");
        printf("\033[1;33m└───────────────────────────────────────────────┘\033[0m\n");
        printf("\033[1;37mYour choice (0/1): \033[0m");

        int input;
        if (scanf("%d", &input) != 1) {
            printf("Invalid input format!\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();

        if (input == 0) {
            mode = SEMANTIC_AT_MOST_ONCE;
            printf("At_Most_Once mode selected.\n");
            break;
        } else if (input == 1) {
            mode = SEMANTIC_AT_LEAST_ONCE;
            printf("At_Least_Once mode selected.\n");
            break;
        } else {
            printf("Invalid choice. Please enter 0 or 1.\n");
        }
    }

    printf("\n\033[1;33m┌────────────────────────────────────────────────────┐\033[0m\n");
    printf("\033[1;33m│\033[0m \033[1mAvailable Commands\033[0m                                 \033[1;33m│\033[0m\n");
    printf("\033[1;33m├────────────────────────────────────────────────────┤\033[0m\n");
    printf("\033[1;33m│\033[0m query_venue     - Check venue availability         \033[1;33m│\033[0m\n");
    printf("\033[1;33m│\033[0m create_new      - Book a new appointment           \033[1;33m│\033[0m\n");
    printf("\033[1;33m│\033[0m alter_exist     - Modify an existing appointment   \033[1;33m│\033[0m\n");
    printf("\033[1;33m│\033[0m delete_exist    - Cancel an existing appointment   \033[1;33m│\033[0m\n");
    printf("\033[1;33m│\033[0m duplicate_exist - Duplicate to next day            \033[1;33m│\033[0m\n");
    printf("\033[1;33m│\033[0m subscribe_venue - Subscribe to venue updates       \033[1;33m│\033[0m\n");
    printf("\033[1;33m│\033[0m exit            - Exit the system                  \033[1;33m│\033[0m\n");
    printf("\033[1;33m└────────────────────────────────────────────────────┘\033[0m\n");

    while (1) {
        printf("\n> ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }
        command[strcspn(command, "\n")] = '\0';
        if (strcmp(command, "query_venue") == 0) {
            int venueId;
            char date[16];
            printf("Available venues: %s\nEnter venue ID: \n> ", venues_list);
            scanf("%d", &venueId);
            getchar();

            while (1) {
                printf("Enter date (YYYY-MM-DD): \n> ");
                scanf("%s", date);
                getchar();
                int tmp = validate_date(date);
                if (tmp == -1) {
                    printf("Invalid date format (YYYY-MM-DD).\n");
                } else if (tmp == -2) {
                    printf("Date cannot be earlier than today.\n");
                } else if (tmp == -3) {
                    printf("Date must be within one week from today.\n");
                } else {
                    break;
                }
            }

            char req[128];
            snprintf(req, sizeof(req), "query,%d,%s", venueId, date);
            int tmp_response = send_request(sockfd, &server_addr, req, response, sizeof(response), mode);

            if (tmp_response > 0) {
                if (strncmp(response, "ERR", 3) == 0) {
                    printf("Error processing your query request: %s\n", response);
                } else {
                    char occupied_slots[1024];
                    if (sscanf(response, "OK|occupied time period is %[^\n]", occupied_slots) == 1) {
                        printf("This date occupied slots are: %s\n", occupied_slots);
                    }
                }
            } else if (tmp_response == -1) {
                printf("System Error!\n");
            }
        } else if (strcmp(command, "create_new") == 0) {
            int venueId;
            char date[16], startTime[8], endTime[8];
            printf("Available venues: %s\nEnter venue ID: \n> ", venues_list);
            scanf("%d", &venueId);
            getchar();

            while (1) {
                printf("Enter date (YYYY-MM-DD): \n> ");
                scanf("%s", date);
                getchar();
                int tmp = validate_date(date);
                if (tmp == -1) {
                    printf("Invalid date format (YYYY-MM-DD).\n");
                } else if (tmp == -2) {
                    printf("Date cannot be earlier than today.\n");
                } else if (tmp == -3) {
                    printf("Date must be within one week from today.\n");
                } else {
                    break;
                }
            }

            while (1) {
                printf("Enter start time (HH:MM, on the hour or half-hour): \n> ");
                scanf("%s", startTime);
                getchar();
                int tmp1 = validate_time(startTime);

                printf("Enter end time (HH:MM, on the hour or half-hour): \n> ");
                scanf("%s", endTime);
                getchar();
                int tmp2 = validate_time(endTime);

                if (strcmp(startTime, endTime) < 0) {
                    break;
                } else if (tmp1 < 0 || tmp2 < 0) {
                    printf("Invalid time format.\n");
                } else {
                    printf("End time must be after start time.\n");
                }
            }

            char req[128];
            snprintf(req, sizeof(req), "create,%d,%s,%s,%s", venueId, date, startTime, endTime);
            int tmp_response = send_request(sockfd, &server_addr, req, response, sizeof(response), mode);

            if (tmp_response > 0) {
                if (strncmp(response, "ERR", 3) == 0) {
                    printf("Error processing your booking request: %s\n", response);
                } else {
                    int appointmentId = atoi(response + 17); // "OK|appointmentId=123456"
                    printf("Booking successful! Your appointment ID is: %u\n", appointmentId);
                }
            } else if (tmp_response == -1) {
                printf("System Error!\n");
            }
        }
        else if (strcmp(command, "alter_exist") == 0) {
            uint32_t appointmentId;
            float newDuration;
            printf("Enter your appointment ID: \n> ");
            scanf("%u", &appointmentId);
            getchar();

            printf("You cannot change duration, but you can shift the time slot.\n");
            printf("Enter offset in hours (positive to delay, negative to advance): \n> ");
            scanf("%f", &newDuration);
            getchar();

            char req[128];
            time_t now = time(NULL);
            snprintf(req, sizeof(req), "alter,%u,%.2f,%ld", appointmentId, newDuration, now);
            int tmp_response = send_request(sockfd, &server_addr, req, response, sizeof(response), mode);

            if (tmp_response > 0) {
                if (strncmp(response, "ERR", 3) == 0) {
                    printf("Error processing your modification request: %s\n", response);
                } else {
                    char timePeriod[16];
                    const char *pos = strstr(response, "timePeriod=");
                    if (pos) {
                        strcpy(timePeriod, pos + strlen("timePeriod="));
                        printf("Modification successful! New time slot: %s\n", timePeriod);
                    }
                }
            } else if (tmp_response == -1) {
                printf("System Error!\n");
            }
        }
        else if (strcmp(command, "delete_exist") == 0) {
            uint32_t appointmentId;
            printf("Deletion is irreversible. Enter your appointment ID to confirm: \n> ");
            scanf("%u", &appointmentId);
            getchar();

            char req[128];
            snprintf(req, sizeof(req), "delete,%u", appointmentId);
            int tmp_response = send_request(sockfd, &server_addr, req, response, sizeof(response), mode);

            if (tmp_response >= 0) {
                if (strncmp(response, "ERR", 3) == 0) {
                    printf("Error processing your cancellation request: %s\n", response);
                } else {
                    printf("Appointment successfully cancelled!\n");
                }
            } else if (tmp_response == -1) {
                printf("System Error!\n");
            }
        }
        else if (strcmp(command, "duplicate_exist") == 0) {
            uint32_t appointmentId;
            printf("Enter your appointment ID: \n> ");
            scanf("%u", &appointmentId);
            getchar();

            char req[128];
            time_t now = time(NULL);
            snprintf(req, sizeof(req), "duplicate,%u,%ld", appointmentId, now);
            int tmp_response = send_request(sockfd, &server_addr, req, response, sizeof(response), mode);

            if (tmp_response > 0) {
                if (strncmp(response, "ERR", 3) == 0) {
                    printf("Error processing your duplication request: %s\n", response);
                } else {
                    int newAppointmentId = atoi(response + 17); // "OK|appointmentId=123456"
                    printf("Duplication successful! Your new appointment ID is: %u\n", newAppointmentId);
                }
            } else if (tmp_response == -1) {
                printf("System Error!\n");
            }
        }
        else if (strcmp(command, "subscribe_venue") == 0) {
            int venueId;
            double durationHour;
            printf("Available venues: %s\n> ", venues_list);
            printf("Enter venue ID to subscribe: \n> ");
            scanf("%d", &venueId);
            getchar();

            printf("Enter subscription duration (in hours): \n> ");
            scanf("%lf", &durationHour);
            getchar();

            int duration = (int)(durationHour * 3600);
            char req[128];
            time_t now = time(NULL);
            snprintf(req, sizeof(req), "hook,%d,%ld,%d", venueId, now, duration);
            int tmp_response = send_request(sockfd, &server_addr, req, response, sizeof(response), mode);

            if (tmp_response > 0) {
                if (strncmp(response, "ERR", 3) == 0) {
                    printf("Error processing your subscription request: %s\n", response);
                } else {
                    printf("✅ Subscription activated! Duration: %.1f hour(s)\n", durationHour);

                    double total_seconds = durationHour * 3600;
                    time_t end_time = now + (time_t)total_seconds;

                    fd_set readfds;
                    struct timeval tv;

                    while (1) {
                        time_t now = time(NULL);
                        double remaining = difftime(end_time, now);

                        if (remaining <= 0) {
                            printf("\r\033[K⏰ Subscription expired. Exiting listening mode.\n");
                            break;
                        }

                        int secs = (int)remaining;
                        int hours = secs / 3600;
                        int mins = (secs % 3600) / 60;
                        int seconds = secs % 60;
                        printf("\r\033[K⏳ Subscription active — %02d:%02d:%02d remaining... ", hours, mins, seconds);
                        fflush(stdout);

                        FD_ZERO(&readfds);
                        FD_SET(sockfd, &readfds);
                        tv.tv_sec = 1;
                        tv.tv_usec = 0;
                        int ret = select(sockfd + 1, &readfds, NULL, NULL, &tv);
                        if (ret > 0 && FD_ISSET(sockfd, &readfds)) {
                            char msg[128];
                            socklen_t addr_len = sizeof(server_addr);
                            ssize_t received_len = recvfrom(sockfd, msg, sizeof(msg)-1, 0, (struct sockaddr *)&server_addr, &addr_len);
                            if (received_len > 0) {
                                msg[received_len] = '\0';
                                char *demar_data[10];
                                demarshalize(msg, demar_data, received_len);
                                printf("\r\033[K🔔 Notification: %s\n", demar_data[0]);

                                remaining = difftime(end_time, time(NULL));
                                if (remaining > 0) {
                                    secs = (int)remaining;
                                    hours = secs / 3600;
                                    mins = (secs % 3600) / 60;
                                    seconds = secs % 60;
                                    printf("⏳ Subscription active — %02d:%02d:%02d remaining... ",
                                           hours, mins, seconds);
                                    fflush(stdout);
                                }
                            }
                        } else if (ret < 0) {
                            perror("select error");
                            break;
                        }
                    }
                }
            } else if (tmp_response == -1) {
                printf("System Error!\n");
            }
        }
        else if (strcmp(command, "exit") == 0) {
            printf("Exiting system. Goodbye!\n");
            break;
        }
        else {
            printf("Invalid command. Please try again.\n");
        }
    }

    close(sockfd);
    return 0;
}