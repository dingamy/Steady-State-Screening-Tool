#include "sqlite-amalgamation-3450300/sqlite3.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
int COUNT = 0; // number of files processed

void handle_error(sqlite3* db, const char* msg) {
    fprintf(stderr, "%s: %s\n", msg, sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(-1);
}

void createScenarioTable(sqlite3* db, const char *path) {

    sqlite3_stmt* stmt = NULL;
    int rc = 0;
 
    DIR* d;
    struct dirent* dir;
    d = opendir(path);

    if (d == NULL) {
        printf("Couldn't open directory\n");
        sqlite3_close(db);
        return -1;
    }

    while ((dir = readdir(d)) != NULL) {
		if (dir->d_type == DT_DIR) { // recursively traverse through directories
            char filePath[1024];
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
                continue;
            snprintf(filePath, sizeof(filePath), "%s/%s", path, dir->d_name);
            createScenarioTable(db, filePath);
		}
        if (strstr(dir->d_name, ".csv") != NULL) { // if it's a csv file then we process
            char* filename = dir->d_name;
            char* filenameCopy = strdup(filename);
            if (!filename) {
                handle_error(db, "strdup error");
                closedir(d);
                sqlite3_close(db);
                return -1;
            }

            printf("File: %s\n", filename);

            // parsing the filename to get column data
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
            if (load[0] > 65) { // comparing ascii value, if (w)"in" or (s)"um" then load is 100, otherwise load is specified in filename
                load = "100";
            }
            printf("scenario: %s\n", scenario);
            printf("study: %s\n", study);
            printf("season: %s\n", season);
            printf("year: %s\n", year);
            printf("load: %s\n", load);
            printf("topology: %s\n", topology);

            if (strcmp(contingency, "System") == 0) { // edge case
                contingency = "INTACT";
                category = "basecase";
            }

            if (contingency[0] == 'P') { // if category exists, then it starts with P followed by 1 or 2 digits
                category = strtok(contingency, "#");
                contingency = strtok(NULL, "");

            }
            printf("contingency: %s\n", contingency);
            printf("category: %s\n", category);

			// allocating memory for the sql string
            size_t needed = snprintf(NULL, 0, "INSERT OR IGNORE INTO Scenarios (`Scenario Name`, `Study`, `Season`, `Year`, `Load`, `Topology`, `Contingency Name`, `Ncat`) VALUES ('%s', '%s', '%s', '%d', '%f', '%s', '%s', '%s');", scenario, study, season, atoi(year), atof(load), topology, contingency, category) + 1;
            char* sql_str = malloc(needed);
            if (sql_str == NULL) {
                printf("malloc failed\n");
                sqlite3_close(db);
                return -1;
            }
			// adding the data to the sql string
            if (category != NULL) {
                sprintf_s(sql_str, needed, "INSERT OR IGNORE INTO Scenarios(`Scenario Name`, `Study`, `Season`, `Year`, `Load`, `Topology`, `Contingency Name`, `Ncat`) VALUES('%s', '%s', '%s', '%d', '%f', '%s', '%s', '%s');", scenario, study, season, atoi(year), atof(load), topology, contingency, category);
            }
            else {
                sprintf_s(sql_str, needed, "INSERT OR IGNORE INTO Scenarios (`Scenario Name`, `Study`, `Season`, `Year`, `Load`, `Topology`, `Contingency Name`) VALUES ('%s', '%s', '%s', '%d', '%f', '%s', '%s');", scenario, study, season, atoi(year), atof(load), topology, contingency);
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
                sqlite3_close(db);
                return -1;
            }

            COUNT++;
            sqlite3_finalize(stmt);
            free(sql_str);

        }
    }
}

int main(int argc, char* argv[]) {
    char* file = "database.db";
    sqlite3* db = NULL;
    int rc = 0;
    sqlite3_initialize();
    rc = sqlite3_open_v2(file, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    if (rc != SQLITE_OK) {
        handle_error(db, "Cannot open database");
        printf("not ok\n");
        sqlite3_close(db);
        exit(-1);
    }

    // initializing tables
    const char* queries[] = {
        "DROP TABLE IF EXISTS Scenarios;",
        "CREATE TABLE Scenarios (`Scenario Name` TEXT PRIMARY KEY, `Study` TEXT, Season TEXT, Year INT, Load FLOAT, Topology TEXT, `Contingency Name` TEXT, Ncat TEXT);",
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
    // populating tables with data
	createScenarioTable(db, ".");

    printf("count: %d\n", COUNT);
	return 0;
}

