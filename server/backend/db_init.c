#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>

// only conduct this program for database initialize when you use the repo for the first time
/* 初始化场所信息*/
static void createVenueTable(sqlite3 *db) {
    const char *sql = 
	"CREATE TABLE IF NOT EXISTS Venues("
     "venueId INTEGER PRIMARY KEY AUTOINCREMENT,"
	"name TEXT NOT NULL);";
    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
    	   fprintf(stderr, "SQL error in createVenueTable: %s\n", err);
        sqlite3_free(err);
    }
}
/* 初始化预约信息*/
static void createAppointmentTable(sqlite3 *db) {
    const char *sql = 
	"CREATE TABLE IF NOT EXISTS Appointments("
	"id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "appointmentId INTEGER NOT NULL,"
	"venueId INTEGER NOT NULL,"
    "date TEXT NOT NULL,"
    "startTime TEXT NOT NULL," 
    "endTime TEXT NOT NULL);";
    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
    	   fprintf(stderr, "SQL error in createAppointmentTable: %s\n", err);
        sqlite3_free(err);
    }
}
/* 初始化订阅信息*/
static void createIpTable(sqlite3 *db) {
    const char *sql = 
	"CREATE TABLE IF NOT EXISTS ClientIP("
	"id INTEGER PRIMARY KEY AUTOINCREMENT,"
     "IP TEXT NOT NULL,"
     "PORT INTEGER NOT NULL,"
     "hookVenueId INTEGER NOT NULL,"
     "startTime INTEGER NOT NULL,"  // try unix timestamp
     "Duration INTEGER NOT NULL);";
    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
    	   fprintf(stderr, "SQL error in createIpTable: %s\n", err);
        sqlite3_free(err);
    }
}

/* 插入场所信息（假设不删除）*/
static void insert_venue(sqlite3 *db, const char *name) {
    const char *sql = "INSERT OR REPLACE INTO Venues(name) VALUES(?);";
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "Insert error: %s\n", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
}
/* 初始化数据库 */
int init_db() {
    sqlite3 *db = NULL;
    // it is told that no need for create a new db, open one if not exists will create one automatically https://stackoverflow.com/questions/41693599/create-a-sqlite-database-in-c
    if (sqlite3_open("/home/archy/Desktop/server/backend/CampusVenueAppointment.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    createVenueTable(db);
    createAppointmentTable(db);
    createIpTable(db);
    const char *Venue1 = "TR61";
    const char *Venue2 = "TR68";
    insert_venue(db, Venue1);
    insert_venue(db, Venue2);

    sqlite3_close(db);
    return 0;
}

