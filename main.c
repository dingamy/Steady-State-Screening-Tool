#include "sqlite-amalgamation-3450300/sqlite3.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

int main(int argc, char* argv[]) {
    printf("beginning\n");

    
    
    char* file = "database.db";
    sqlite3 *db = NULL;
    int rc = 0;
    sqlite3_initialize();
    rc = sqlite3_open_v2(file, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    if (rc != SQLITE_OK) {
        printf("not ok\n");
        sqlite3_close(db);
        exit(-1);
    }
    // DROPS EXISTING TABLE contingency
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, "DROP TABLE IF EXISTS contingency;", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("not ok2\n");
        sqlite3_close(db);
        exit(-1);
    }
    rc = sqlite3_step(stmt);
    while (rc != SQLITE_DONE) {
        printf("error\n");
        return -1;
    }
    sqlite3_finalize(stmt);

    // CREATES NEW TABLE contingency
    stmt = NULL;
    rc = sqlite3_prepare_v2(db, "CREATE TABLE contingency (`contingency name` text NOT NULL, `NERC category` text);", -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("not ok2\n");
        sqlite3_close(db);
        exit(-1);
    }
    rc = sqlite3_step(stmt);
    while (rc != SQLITE_DONE) {
        printf("error\n");
        return -1;
    }
    sqlite3_finalize(stmt);

	// INSERTING VALUES INTO TABLE contingency
    int count = 0;
    DIR* d;
	struct dirent* dir;
    d = opendir("C:\\Users\\ading\\Downloads\\steady state\\ThermalBranch");
	if (d == NULL) {
		printf("Couldn't open directory\n");
		return -1;
	}
    while ((dir = readdir(d)) != NULL) {

		if (strstr(dir->d_name, ".csv") != NULL) {
            printf("%s\n", dir->d_name);
            char* filename = dir->d_name;
            char* contingency;
            char* category = NULL;
            char* nextToken;
            contingency = strtok_s(filename, "@", &nextToken); 
            contingency = strtok_s(NULL, "@", &nextToken);
            contingency = strtok_s(contingency, "-", &nextToken);

            if (strstr(contingency, "#") != NULL) {
                category = strtok_s(contingency, "#", &nextToken);
                contingency = strtok_s(NULL, "#", &nextToken);
            }
     
            size_t needed = snprintf(NULL, 0, "INSERT INTO contingency (`contingency name`, `NERC category`) VALUES ('%s', '%s');", contingency, category) + 1;
            char* sql_str = malloc(needed);
            if (sql_str == NULL) {
                printf("malloc faileds\n");
                return -1;
            }
            if (category != NULL) {
                sprintf_s(sql_str, needed, "INSERT INTO contingency(`contingency name`, `NERC category`) VALUES('%s', '%s');", contingency, category);
                printf("%s\n", sql_str);
            }
            else {
                sprintf_s(sql_str, needed, "INSERT INTO contingency (`contingency name`) VALUES ('%s');", contingency);
                printf("%s\n", sql_str);
            }

            stmt = NULL;
            rc = sqlite3_prepare_v2(db, sql_str, -1, &stmt, NULL);
            if (rc != SQLITE_OK) {
                printf("not ok2\n");
                sqlite3_close(db);
                exit(-1);
            }

            rc = sqlite3_step(stmt);
            while (rc != SQLITE_DONE) {
                printf("error\n");
                return -1;
            }

            printf("done\n");
            count++;
            sqlite3_finalize(stmt);
            free(sql_str);
		}
    }
	printf("count: %d\n", count);
    sqlite3_close(db);
    sqlite3_shutdown();
}