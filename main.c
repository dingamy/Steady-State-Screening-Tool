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
    d = opendir("C:\\Users\\ading\\Downloads\\steady state\\ThermalBranch");

	if (d == NULL) {
		printf("Couldn't open directory\n");
		return -1;
	}

    while ((dir = readdir(d)) != NULL) {
        if (strstr(dir->d_name, ".csv") != NULL) {

            char* filename = dir->d_name;
            //char filename[] = "Base-sum2030-LG-islnd0@P1#SPC#GREATPLAINSGENERATION-branch.csv";
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
			printf("contingency: %s\n", contingency);
            contingency = strtok(contingency, "-");
			printf("contingency: %s\n", contingency);

            char* scenario = strtok(filenameCopy, "@");
            char* study = strtok(strdup(scenario), "-");
			char* season = strtok(NULL, "-");
            printf("scenario: %s\n", scenario);
			printf("study: %s\n", study);
			printf("season: %s\n", season);
           
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

            size_t needed = snprintf(NULL, 0, "INSERT INTO Scenarios (`Contingency Name`, `Ncat`) VALUES ('%s', '%s');", contingency, category) + 1;
            char* sql_str = malloc(needed);
            if (sql_str == NULL) {
                printf("malloc failed\n");
                return -1;
            }
            if (category != NULL) {
                sprintf_s(sql_str, needed, "INSERT INTO Scenarios(`Contingency Name`, `Ncat`) VALUES('%s', '%s');", contingency, category);
                printf("sql_str: %s\n", sql_str);
            }
            else {
                sprintf_s(sql_str, needed, "INSERT INTO Scenarios (`Contingency Name`) VALUES ('%s');", contingency);
                printf("sql_str: %s\n", sql_str);
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

