#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>

int appoint(sqlite3 *db, int venueId, const char *date, const char *start, const char *end) {
    sqlite3_stmt *stmt1;
    const char *sql1 = "INSERT INTO Appointments (venueId, date, startTime, endTime) VALUES (?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, sql1, -1, &stmt1, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    sqlite3_bind_int(stmt1, 1, venueId);
    sqlite3_bind_text(stmt1, 2, date, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt1, 3, start, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt1, 4, end, -1, SQLITE_STATIC);			        
    if (sqlite3_step(stmt1) != SQLITE_DONE) {
        fprintf(stderr, "Insert failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt1);
	return -1;
    }
    sqlite3_finalize(stmt1);

    int appointmentId = (int)sqlite3_last_insert_rowid(db);
    printf("get appointmentId: %d\n", appointmentId);
    sqlite3_stmt *stmt2;
    const char *sql2 = "INSERT OR IGNORE INTO VenueAppointments (venueId, appointmentId) VALUES (?, ?);";
    if (sqlite3_prepare_v2(db, sql2, -1, &stmt2, NULL) != SQLITE_OK) {
    	fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));                
        return -1;                              
    }
    sqlite3_bind_int(stmt2, 1, venueId);
    sqlite3_bind_int(stmt2, 2, appointmentId);
    if (sqlite3_step(stmt2) != SQLITE_DONE) {
	fprintf(stderr, "Insert failed: %s\n", sqlite3_errmsg(db));
	sqlite3_finalize(stmt2);
	return -1;
    }
    sqlite3_finalize(stmt2);

    return 0;
}

int alter(sqlite3 *db, int AppointmentId, double offset) {   
    int time_to_minutes(const char *timeStr) {
        int hours = 0, minutes = 0;
	sscanf(timeStr, "%d:%d", &hours, &minutes);
	return hours*60+minutes;
    }
    void minutes_to_time(int minutes, char *timeStr) {
	int hours = minutes / 60;
	int mins = minutes % 60;
	sprintf(timeStr, "%02d:%02d", hours, mins);
    }
    void adjust_time(sqlite3 *db, const char *startTime, const char *endTime, int AppointmentId, double offset) { 
        int startMinutes = time_to_minutes(startTime);
	int endMinutes = time_to_minutes(endTime);
	int duration = endMinutes - startMinutes;
	int offsetMinutes = (int)(offset * 60);
	startMinutes += offsetMinutes;
	endMinutes = startMinutes + duration;
	printf("Start from %d to %d, offsetMinutes: %d, duration is %d\n", startMinutes, endMinutes, offsetMinutes, duration);
        if (startMinutes < 0 || endMinutes > 1440) {
            fprintf(stderr, "Error: Adjusted time exceeds the limit of one day! \n");
            return;
        }
	char newStartTime[6], newEndTime[6];
	minutes_to_time(startMinutes, newStartTime);
	minutes_to_time(endMinutes, newEndTime);
	sqlite3_stmt *stmt;
	const char *sqlUpdate = "UPDATE Appointments SET startTime=?, endTime=? WHERE appointmentId=?;";
	if (sqlite3_prepare_v2(db, sqlUpdate, -1, &stmt, NULL) != SQLITE_OK) {
            fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
	    return;
	}
	sqlite3_bind_text(stmt, 1, newStartTime, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, newEndTime, -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 3, AppointmentId);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
            fprintf(stderr, "Update failed: %s\n", sqlite3_errmsg(db));
	    return;
	}	
        sqlite3_finalize(stmt);
    }
    sqlite3_stmt *stmt1;
    const char *sqlSearch = "SELECT startTime, endTime FROM Appointments WHERE appointmentId=?;";
    if (sqlite3_prepare_v2(db, sqlSearch, -1, &stmt1, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
	return -1;
    }
    sqlite3_bind_int(stmt1, 1, AppointmentId);
    int rc;
    if ((rc = sqlite3_step(stmt1)) == SQLITE_ROW) {
	const unsigned char *startTime = sqlite3_column_text(stmt1, 0);
	const unsigned char *endTime = sqlite3_column_text(stmt1, 1);
	printf("start time is %s, end time is %s\n", startTime, endTime);
	adjust_time(db, (const char *)startTime, (const char *)endTime, AppointmentId, offset);
    }else {
	fprintf(stderr, "Error: AppointmentId %d does not exist.\n", AppointmentId);
    }
    if (rc != SQLITE_DONE) {
	fprintf(stderr, "Query failed: %s\n", sqlite3_errmsg(db));
	return -1;
    }
    return 0;
}

void delete(sqlite3 *db, int AppointmentId) {
    sqlite3_stmt *stmt;
    const char *sqlDelete = "DELETE FROM Appointments WHERE appointmentId=?;";
    if (sqlite3_prepare_v2(db, sqlDelete, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
	return;
    }
    sqlite3_bind_int(stmt, 1, AppointmentId);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
	fprintf(stderr, "Delete failed: %s\n", sqlite3_errmsg(db));
	return;
    }
    else {
        int rowsAffected = sqlite3_changes(db);
	if (rowsAffected > 0) {
	    printf("Appointment with ID %d deleted successfully.\n", AppointmentId);
	} else {
	    fprintf(stderr, "Error: AppointmentId %d does not exist.\n", AppointmentId);
	}
    }	
    sqlite3_finalize(stmt);
}

int main() {
    sqlite3 *db;
    int rc = sqlite3_open("/home/archy/Desktop/server/CampusVenueAppointment.db", &db);
    if (rc != SQLITE_OK) {
	fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
	return -1;
    }
    //appoint(db, 1, "2025-09-05", "10:00", "16:00");
    alter(db, 1, -3.0);
    alter(db, 2, 12.5);
    alter(db, 5, 1);
    //delete(db, 5);
    sqlite3_close(db);
    return 0;
}
