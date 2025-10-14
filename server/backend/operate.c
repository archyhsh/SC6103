#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <time.h>
#include "db_init.h"
#include "parseTime.h"
#include "../udp/hash_function.h"
#include "../udp/udp_server.h"
#include "../udp/marshalize.h"
/* 回调函数 记录IP与端口，触发后发送信息*/
static int subscribeCallback(sqlite3 *db, int sockfd, int venueId, char *msg) {
    sqlite3_stmt *stmt;
    const char *sql =    "SELECT IP, PORT, startTime, duration FROM ClientIP WHERE hookVenueId = ? AND (strftime('%s','now') + 0) BETWEEN startTime AND (startTime + duration);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    sqlite3_bind_int(stmt, 1, venueId);	
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *client_addr = (const char *)sqlite3_column_text(stmt, 0);
        const int port = sqlite3_column_int(stmt, 1);
        printf("HOOK! send to %s\n", client_addr);
        // sendto client for date+start+end, no matter create/alter/duplicate/delete
        struct sockaddr_in cliaddr;
        memset(&cliaddr, 0, sizeof(cliaddr));
        cliaddr.sin_family = AF_INET;
        cliaddr.sin_port = htons(port);
        inet_pton(AF_INET, client_addr, &cliaddr.sin_addr);
        socklen_t addr_len = sizeof(cliaddr);
        uint32_t requestID = 0;
        char message[1024];
        size_t respLen = marshalizeResp(requestID, msg, message, sizeof(message));
        printf("the callback msg is of %ld length\n", respLen);
        udp_send(sockfd, message, respLen, &cliaddr, addr_len);
    }
    sqlite3_finalize(stmt);
    return 0;
}
/* 新建预约函数，失败会返回相应错误码*/
int appoint(sqlite3 *db, int sockfd, int venueId, const char *date, const char *start, const char *end, uint32_t *appointmentId) {
    sqlite3_stmt *stmt0;
    const char *sql0 = "SELECT startTime, endTime FROM Appointments WHERE venueId = ? and date = ?;";
    if (sqlite3_prepare_v2(db, sql0, -1, &stmt0, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    sqlite3_bind_int(stmt0, 1, venueId);
    sqlite3_bind_text(stmt0, 2, date, -1, SQLITE_STATIC);
    while (sqlite3_step(stmt0) == SQLITE_ROW) {
        const char *curStart = (const char *)sqlite3_column_text(stmt0, 0);
        const char *curEnd = (const char *)sqlite3_column_text(stmt0, 1);
        if (strcmp(start, curEnd) < 0 && strcmp(end, curStart) > 0) {
        	printf("The appointment is invalid!\n");
        	return -2;
        }
    }
    sqlite3_finalize(stmt0);
    sqlite3_stmt *stmt1;
    time_t now = time(NULL); 
    size_t buffer_size = strlen(date) + strlen(start) + strlen(end) + 20;
    char buffer[buffer_size];
    snprintf(buffer, sizeof(buffer), "%ld%d%s%s%s", (long)now, venueId, date, start, end);
    *appointmentId = hash2(buffer);
    printf("get appointmentId: %u\n", *appointmentId);
    const char *sql1 = "INSERT INTO Appointments (appointmentId, venueId, date, startTime, endTime) VALUES (?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, sql1, -1, &stmt1, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    sqlite3_bind_int(stmt1, 1, *appointmentId);
    sqlite3_bind_int(stmt1, 2, venueId);
    sqlite3_bind_text(stmt1, 3, date, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt1, 4, start, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt1, 5, end, -1, SQLITE_STATIC);			        
    if (sqlite3_step(stmt1) != SQLITE_DONE) {
        fprintf(stderr, "Insert failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt1);
        return -1;
    }
    sqlite3_finalize(stmt1);
    char msg[1024];
    snprintf(msg, sizeof(msg), "Someone created a new appointment on %s %s-%s", date, start, end);
    subscribeCallback(db, sockfd, venueId, msg);
    return 0;
}
/* 时间相关函数，用于修改预约时间段*/
static int time_to_minutes(const char *timeStr) {
    int hours = 0, minutes = 0;
    sscanf(timeStr, "%d:%d", &hours, &minutes);
    return hours*60+minutes;
}
/* 时间相关函数，用于修改预约时间段*/
static void minutes_to_time(int minutes, char *timeStr) {
    int hours = minutes / 60;
    int mins = minutes % 60;
    sprintf(timeStr, "%02d:%02d", hours, mins);
}
/* 修改预约时间段，发生错误会返回相应错误码*/
static int adjust_time(sqlite3 *db, const int venueId, const char *date, const char *startTime, const char *endTime, uint32_t appointmentId, double offset, char *newStartOut, char *newEndOut) { 
    int startMinutes = time_to_minutes(startTime);
    int endMinutes = time_to_minutes(endTime);
    int duration = endMinutes - startMinutes;
    int offsetMinutes = (int)(offset * 60);
    startMinutes += offsetMinutes;
    endMinutes = startMinutes + duration;
    printf("Start from %d to %d, offsetMinutes: %d, duration is %d\n", startMinutes, endMinutes, offsetMinutes, duration);
    if (startMinutes < 0 || endMinutes > 1440) {
        fprintf(stderr, "Error: Adjusted time exceeds the limit of one day! \n");
            return -3;
        }
    char newStartTime[6], newEndTime[6];
    minutes_to_time(startMinutes, newStartTime);
    minutes_to_time(endMinutes, newEndTime);
    sqlite3_stmt *stmt0;
    const char *sql0 = "SELECT appointmentId, startTime, endTime FROM Appointments WHERE venueId = ? and date = ?;";
    if (sqlite3_prepare_v2(db, sql0, -1, &stmt0, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    sqlite3_bind_int(stmt0, 1, venueId);
    sqlite3_bind_text(stmt0, 2, date, -1, SQLITE_STATIC);
    while (sqlite3_step(stmt0) == SQLITE_ROW) {
        const int curAppId = sqlite3_column_int(stmt0, 0);
        const char *curStart = (const char *)sqlite3_column_text(stmt0, 1);
        const char *curEnd = (const char *)sqlite3_column_text(stmt0, 2);
        if (curAppId == appointmentId) {
            continue;
        }
        if (strcmp(newStartTime, curEnd) < 0 && strcmp(newEndTime, curStart) > 0) {
            printf("The alter of your appointment is invalid!\n");
            return -2;
        }
    }
    sqlite3_finalize(stmt0);
    sqlite3_stmt *stmt1;
    const char *sql1 = "UPDATE Appointments SET startTime=?, endTime=? WHERE appointmentId=?;";
    if (sqlite3_prepare_v2(db, sql1, -1, &stmt1, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    sqlite3_bind_text(stmt1, 1, newStartTime, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt1, 2, newEndTime, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt1, 3, appointmentId);
    if (sqlite3_step(stmt1) != SQLITE_DONE) {
        fprintf(stderr, "Update failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }	
    sqlite3_finalize(stmt1);
    strcpy(newStartOut, newStartTime);
    strcpy(newEndOut, newEndTime);
    return 0;
}
/* 修改接口，实际更改数据库步骤在adjust_time*/
int alter(sqlite3 *db, int sockfd, uint32_t appointmentId, double offset, char *newStartOut, char *newEndOut) {   
    sqlite3_stmt *stmt0;
    const char *sql0 = "SELECT venueID, date, startTime, endTime FROM Appointments WHERE appointmentId=?;";
    if (sqlite3_prepare_v2(db, sql0, -1, &stmt0, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
	   return -1;
    }
    sqlite3_bind_int(stmt0, 1, appointmentId);
    int rc;
    if ((rc = sqlite3_step(stmt0)) == SQLITE_ROW) {
    	   const int venueId = sqlite3_column_int(stmt0, 0);
    	   const char *date = (const char *)sqlite3_column_text(stmt0, 1);
	   const char *startTime = (const char *)sqlite3_column_text(stmt0, 2);
	   const char *endTime = (const char *)sqlite3_column_text(stmt0, 3);
	   int tmp = adjust_time(db, venueId, date, startTime, endTime, appointmentId, offset, newStartOut, newEndOut);
	   if (tmp < 0) {
	       return tmp;
	   }
	   if (sqlite3_prepare_v2(db, sql0, -1, &stmt0, NULL) != SQLITE_OK) {
            fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
            return -1;
        }
        char msg[1024];
        snprintf(msg, sizeof(msg), "Someone change his appointment on %s to %s-%s", date, newStartOut, newEndOut);
        subscribeCallback(db, sockfd, venueId, msg);
        sqlite3_finalize(stmt0);
        return 0;
    }else {
	   fprintf(stderr, "Error: AppointmentId %u does not exist.\n", appointmentId);
	   sqlite3_finalize(stmt0);
	   return -4;
    }
}
/* 删除预约*/
void deleteAppointment(sqlite3 *db, int sockfd, uint32_t appointmentId) {
    sqlite3_stmt *stmt0;
    const char *sql0 = "SELECT venueID, date, startTime, endTime FROM Appointments WHERE appointmentId=?;";
    if (sqlite3_prepare_v2(db, sql0, -1, &stmt0, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
	   return;
    }
    sqlite3_bind_int(stmt0, 1, appointmentId);
    sqlite3_step(stmt0);
    const int venueId = sqlite3_column_int(stmt0, 0);
    sqlite3_finalize(stmt0);
    sqlite3_stmt *stmt1;
    const char *sql1 = "DELETE FROM Appointments WHERE appointmentId=?;";
    if (sqlite3_prepare_v2(db, sql1, -1, &stmt1, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
	return;
    }
    sqlite3_bind_int(stmt1, 1, appointmentId);
    if (sqlite3_step(stmt1) != SQLITE_DONE) {
	fprintf(stderr, "Delete failed: %s\n", sqlite3_errmsg(db));
	return;
    }
    else {
        int rowsAffected = sqlite3_changes(db);
	if (rowsAffected > 0) {
	    printf("Appointment with ID %d deleted successfully.\n", appointmentId);
	} else {
	    fprintf(stderr, "Error: AppointmentId %d does not exist.\n", appointmentId);
	}
    }	
    char msg[1024];
    snprintf(msg, sizeof(msg), "Someone delete his appointment");
    subscribeCallback(db, sockfd, venueId, msg);
    sqlite3_finalize(stmt1);
}
/* 根据场所ID，接收IP和端口记录到数据库，接收开始时间与总时长，超时后自动忽略订阅信息*/
int hook(sqlite3 *db, const char *client_addr, const int port, const int venueId, const int startTime, const int duration) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO ClientIP (IP, PORT, hookVenueId, startTime, duration) VALUES (?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
	   return -1;
    }
    sqlite3_bind_text(stmt, 1, client_addr, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, port);
    sqlite3_bind_int(stmt, 3, venueId);
    sqlite3_bind_int(stmt, 4, startTime); 
    sqlite3_bind_int(stmt, 5, duration); 
    if (sqlite3_step(stmt) != SQLITE_DONE) {
	   fprintf(stderr, "Hook failed: %s\n", sqlite3_errmsg(db));
	   return -1;
    }
    sqlite3_finalize(stmt);
    return 0;
}
/* 根据ID复制预约信息至后一天，发生错误会返回相应的错误码*/
int duplicate(sqlite3 *db, int sockfd, uint32_t appointmentID, uint32_t *rAppointmentID) {
	sqlite3_stmt *stmt;
	const char *sql = "SELECT venueId, date, startTime, endTime from Appointments where appointmentId=?;";
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
         fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
         return -1;
     }
     sqlite3_bind_int(stmt, 1, appointmentID);
     int rc = sqlite3_step(stmt);
     if (rc != SQLITE_ROW) {
         fprintf(stderr, "Invalid appointmentID!\n");
         sqlite3_finalize(stmt);
         return -3;
     }	
     const int venueId = sqlite3_column_int(stmt, 0);
     const char *date = (const char *)sqlite3_column_text(stmt, 1);
     const char *start = (const char *)sqlite3_column_text(stmt, 2);
     const char *end = (const char *)sqlite3_column_text(stmt, 3);
     char newDate[16];
     {
         struct tm tm_date = {0};
         strptime(date, "%Y-%m-%d", &tm_date);
         tm_date.tm_mday += 1;
         time_t target_time = mktime(&tm_date);
         if (difftime(target_time, time(NULL)) > 7 * 24 * 3600) {
             fprintf(stderr, "❌ Duplicate failed: new appointment exceeds one week from now!\n");
             sqlite3_finalize(stmt);
             return -4;
         }
         strftime(newDate, sizeof(newDate), "%Y-%m-%d", &tm_date);
     }
     char start_copy[8], end_copy[8];
     strncpy(start_copy, start, sizeof(start_copy));
     strncpy(end_copy, end, sizeof(end_copy));
     start_copy[sizeof(start_copy)-1] = '\0';
     end_copy[sizeof(end_copy)-1] = '\0';
     sqlite3_finalize(stmt);     
	return appoint(db, sockfd, venueId, newDate, start_copy, end_copy, rAppointmentID);
}
