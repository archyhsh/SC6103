#ifndef OPERATE_H
#define OPERATE_H
#include <sqlite3.h>

int appoint(sqlite3 *db, int venueId, const char *date, const char *start, const char *end, uint32_t *appointmentId);
int alter(sqlite3 *db, uint32_t appointmentId, double offset, char *newStartOut, char *newEndOut);
void deleteAppointment(sqlite3 *db, uint32_t *AppointmentId);
int hook(sqlite3 *db, const char *client_addr, const int venueId, const int startTime, const int duration);

#endif