#include "sqlite-amalgamation-3450300/sqlite3.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
int compareStrings(const void* a, const void* b) {

	if (*(const char**)a  == NULL || *(const char**)b == NULL) {
		return 0;
	}
    return strcmp(*(const char**)a, *(const char**)b);
}
void handle_error(sqlite3* db, const char* msg) {
    fprintf(stderr, "%s: %s\n", msg, sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(-1);
}

//void test() {
//	char* arr[10] = { "a", "b", "c", "d", "e", "f", NULL, NULL, NULL, NULL };
//    int i = 6;
//	arr = realloc(arr, 6 * sizeof(char*));
//}

int main(int argc, char* argv[]) {

    char* file = "database.db";
    sqlite3 *db = NULL;
    int rc = 0;
    sqlite3_initialize();
    rc = sqlite3_open_v2(file, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    if (rc != SQLITE_OK) {
        handle_error(db, "Cannot open database");
        printf("not ok\n");
        sqlite3_close(db);
        exit(-1);
    }
    const char* queries[] = {
        "DROP TABLE IF EXISTS Scenarios;",
        "CREATE TABLE Scenarios (`Scenario Name` TEXT, `Study` TEXT, Season TEXT, Year INT, Load FLOAT, Topology TEXT, `Contingency Name` TEXT, Ncat TEXT);",
    };
    sqlite3_stmt* stmt = NULL;
    for (int i = 0; i < 2; i++) {
        stmt = NULL;
        rc = sqlite3_prepare_v2(db, queries[i], -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            handle_error(db, "prepare error");
            sqlite3_close(db);
            exit(-1);
        }
        rc = sqlite3_step(stmt);
        while (rc != SQLITE_DONE) {
            handle_error(db, "statement error");
            return -1;
        }
        sqlite3_finalize(stmt);
    }
    // inserting into contingency value
    int count = 0;
    DIR* d;
	struct dirent* dir;
    d = opendir("C:\\Users\\ading\\Downloads\\steady state\\Voltage");

	if (d == NULL) {
		printf("Couldn't open directory\n");
		return -1;
	}

    while ((dir = readdir(d)) != NULL) {
        if (strstr(dir->d_name, ".csv") != NULL) {

            char* filename = dir->d_name;
            //char filename[] = "Base-s142030-LG-islnd0@P1#SPC#GREATPLAINSGENERATION-branch.csv";
			char* filenameCopy = strdup(filename);
            if (!filename) {
                handle_error(db, "strdup error");
                closedir(d);
                return -1;
            }
            printf("File: %s\n", filename);
            char* category = NULL;
            char* contingency;
            contingency = strtok(filename, "@");
            contingency = strtok(NULL, "@");
            contingency = strtok(contingency, "-");
			

            char* scenario = strtok(filenameCopy, "@");
            char* study = strtok(strdup(scenario), "-");
			char* season = strtok(NULL, "-");
			char* topology = strtok(NULL, "");
            char* year = strdup(season) + 3;
            char* load = season + 1;
            load[2] = '\0';
            if (season[0] == 's' || season[0] == 'S') {
				season = "Summer";
			}
            if (season[0] == 'w' || season[0] == 'W') {
				season = "Winter";
			}
            if (load[0] > 65) {
				load = "100";
            }

            printf("scenario: %s\n", scenario);
			printf("study: %s\n", study);
			printf("season: %s\n", season);
			printf("year: %s\n", year);
            printf("load: %s\n", load);
			printf("topology: %s\n", topology);
			
            if (strcmp(contingency, "System") == 0) {
                contingency = "INTACT";
                category = "basecase";
            }

            if (contingency[0] == 'P') {
                category = strtok(contingency, "#");
                contingency = strtok(NULL, "");
	
            }
            printf("contingency: %s\n", contingency);
            printf("category: %s\n", category);

            size_t needed = snprintf(NULL, 0, "INSERT INTO Scenarios (`Scenario Name`, `Study`, `Season`, `Year`, `Load`, `Topology`, `Contingency Name`, `Ncat`) VALUES ('%s', '%s', '%s', '%d', '%f', '%s', '%s', '%s');", scenario, study, season, atoi(year), atof(load), topology, contingency, category) + 1;
            char* sql_str = malloc(needed);
            if (sql_str == NULL) {
                printf("malloc failed\n");
                return -1;
            }
            if (category != NULL) {
                sprintf_s(sql_str, needed, "INSERT INTO Scenarios(`Scenario Name`, `Study`, `Season`, `Year`, `Load`, `Topology`, `Contingency Name`, `Ncat`) VALUES('%s', '%s', '%s', '%d', '%f', '%s', '%s', '%s');", scenario, study, season, atoi(year), atof(load), topology, contingency, category);
                //printf("sql_str: %s\n", sql_str);
            }
            else {
                sprintf_s(sql_str, needed, "INSERT INTO Scenarios (`Scenario Name`, `Study`, `Season`, `Year`, `Load`, `Topology`, `Contingency Name`) VALUES ('%s', '%s', '%s', '%d', '%f', '%s', '%s');", scenario, study, season, atoi(year), atof(load), topology, contingency);
                //printf("sql_str: %s\n", sql_str);
            }

            stmt = NULL;
            rc = sqlite3_prepare_v2(db, sql_str, -1, &stmt, NULL);
            if (rc != SQLITE_OK) {
                handle_error(db, "prepare error");
                sqlite3_close(db);
                exit(-1);
            }

            rc = sqlite3_step(stmt);
            while (rc != SQLITE_DONE) {
                handle_error(db, "statement error");
                return -1;
            }

            count++;
            sqlite3_finalize(stmt);
            free(sql_str);

		}
    }
	printf("count: %d\n", count);
    sqlite3_close(db);
    sqlite3_shutdown();
}

