#include "sqlite-amalgamation-3450300/sqlite3.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

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

    DIR* d;
	struct dirent* dir;
    d = opendir("C:\\Users\\ading\\Downloads\\steady state\\ThermalBranch");
	if (d == NULL) {
		printf("Couldn't open directory\n");
		return -1;
	}
    while ((dir = readdir(d)) != NULL) {
		printf("%s\n", dir->d_name);
		if (strstr(dir->d_name, ".csv") != NULL) {
			printf("found csv\n");
			char* filename1 = dir->d_name;
			printf("FILE NAME % s\n", filename1);
		}
    }
    char *filename = malloc(strlen("C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@amy#dingBRNDGEN5-gen.csv") + 1);
    if (filename == NULL) {
        printf("malloc failed\n");
        return -1;
    }
    strcpy(filename, "C:\\Users\\ading\\Downloads\\Project\\BasePrep-sum2025spcHH-LG@amy#dingBRNDGEN5-gen.csv");
    
    printf("hiiiiiiii\n");
    printf("%s\n", filename);


    char* contingency;
    char* category = NULL;
    char* nextToken;
    contingency = strtok_s(filename, "@", &nextToken); /* get the first token */

    contingency = strtok_s(NULL, "@", &nextToken); /* get the second part of the string */
    contingency = strtok_s(contingency, "-", &nextToken);
    
    if (strstr(contingency, "#") != NULL) {
        category = strtok_s(contingency, "#", &nextToken);
		contingency = strtok_s(NULL, "#", &nextToken);
    }
    printf(" %s\n", contingency);
    printf(" %s\n", category);
    
    size_t needed = snprintf(NULL, 0, "INSERT INTO contingency (`contingency name`, `NERC category`) VALUES ('%s', '%s');", contingency, category) + 1;
    char* sql_str = malloc(needed);
    if (sql_str == NULL) {
        printf("malloc faileds\n");
        return -1;
    }
    if (category != NULL) {
        sprintf_s(sql_str, needed, "INSERT INTO contingency (`contingency name`, `NERC category`) VALUES ('%s', '%s');", contingency, category);
        printf("%s\n", sql_str);
    }
	else {
		sprintf_s(sql_str, needed, "INSERT INTO contingency (`contingency name`) VALUES ('%s');", contingency);
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

char* replace_char(char* str, char find, char replace) {
    char* current_pos = strchr(str, find);
    while (current_pos) {
        *current_pos = replace;
        current_pos = strchr(current_pos, find);
    }
    return str;
}