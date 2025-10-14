#ifndef OPERATE_H
#define OPERATE_H
#include <sqlite3.h>

int query(sqlite3 *db, int sockfd, int venueId, const char *date, const char *start, const char *end, int appointmentId, char *occupied_slots, size_t slots_size);
int appoint(sqlite3 *db, int sockfd, int venueId, const char *date, const char *start, const char *end, uint32_t *appointmentId, char *occupied_slots, size_t slots_size);
int alter(sqlite3 *db, int sockfd, uint32_t appointmentId, double offset, char *newStartOut, char *newEndOut, char *occupied_slots, size_t slots_size);
void deleteAppointment(sqlite3 *db, int sockfd, uint32_t AppointmentId);
int hook(sqlite3 *db, const char *client_addr, const int port, const int venueId, const int startTime, const int duration);
int duplicate(sqlite3 *db, int sockfd, uint32_t appointmentID, uint32_t *rAppointmentID, char *occupied_slots, size_t slots_size);

#endif