#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <time.h>
#include <string.h>
/* 解析时间模板，用于修改*/
int parseTime(const char *date, const char *timepoint) {
    char datetime[32];
    struct tm tm_time; // incomplete time_t
    memset(&tm_time, 0, sizeof(struct tm));
    snprintf(datetime, sizeof(datetime), "%s %s", date, timepoint);
    if (strptime(datetime, "%Y-%m-%d %H:%M", &tm_time) == NULL) {
        fprintf(stderr, "Failed to parse datetime string: %s\n", datetime);
        return -1;
    }
    return (int)mktime(&tm_time);
}
