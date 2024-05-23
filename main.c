#include "sqlite-amalgamation-3450300/sqlite3.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
    printf("beginning\n");

    
    
    char* file = "database.db"; /* default to temp db */
    sqlite3 *db = NULL;
    int rc = 0;
    sqlite3_initialize();
    rc = sqlite3_open_v2(file, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    if (rc != SQLITE_OK) {
        printf("not ok\n");
        sqlite3_close(db);
        exit(-1);
    }
    printf("ok\n");

    char filename[] = "C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@BRNDGEN5-gen.csv";
 
    char* contingency;
    char* category = NULL;
    contingency = strtok(filename, "@"); /* get the first token */
    contingency = strtok(NULL, "@"); /* get the second part of the string */
    contingency = strtok(contingency, "-");
    
    if (strstr(contingency, "#") != NULL) {
        category = strtok(contingency, "#");
		contingency = strtok(NULL, "#");
    }
    printf(" %s\n", contingency);
    printf(" %s\n", category);
    
    size_t needed = snprintf(NULL, 0, "INSERT INTO contingency (`contingency name`, `NERC category`) VALUES ('%s', '%s');", contingency, category) + 1;
    char* sql_str = malloc(needed);
    if (sql_str == NULL) {
        perror("malloc");
        return -1;
    }
    if (category != NULL) {
        sprintf(sql_str, "INSERT INTO contingency (`contingency name`, `NERC category`) VALUES ('%s', '%s');", contingency, category);
        printf("%s\n", sql_str);
    }
	else {
		sprintf(sql_str, "INSERT INTO contingency (`contingency name`) VALUES ('%s');", contingency);
		printf("%s\n", sql_str);
	}

    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql_str, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        printf("not ok2\n");
        sqlite3_close(db);
        exit(-1);
    }
    printf("ok2\n");
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        continue;
    }
    
	printf("done\n");
    sqlite3_finalize(stmt);

    free(sql_str);
    sqlite3_close(db);
    sqlite3_shutdown();
}