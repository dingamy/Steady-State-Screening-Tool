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
        "DROP TABLE IF EXISTS contingency;",
        "CREATE TABLE contingency (`contingency name` TEXT PRIMARY KEY NOT NULL, `NERC category` TEXT);",
        "DROP TABLE IF EXISTS scenarios;",
        "CREATE TABLE scenarios (`scenario name` TEXT PRIMARY KEY NOT NULL, `season` TEXT, year INT, load FLOAT);"
    };
    sqlite3_stmt* stmt = NULL;
    for (int i = 0; i < 4; i++) {
        stmt = NULL;
        rc = sqlite3_prepare_v2(db, queries[i], -1, &stmt, NULL);
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
            if (!filename) {
                printf("strdup failed\n");
                closedir(d);
                return -1;
            }
            printf("File: %s\n", filename);
            char* category = NULL;
            char* contingency = malloc(500);
            contingency = strtok(filename, "@");
            contingency = strtok(NULL, "@");
            contingency = strtok(contingency, "-");
			printf("contingency: %s\n", contingency);
			printf("category: %s\n", category);

            const char* contingencies[10] = {NULL};
            if (strstr(contingency, "#") != NULL) {
                category = strtok(contingency, "#");
                contingency = strtok(NULL, "#");
                contingencies[0] = contingency;
                int i = 1;
                contingency = strtok(NULL, "#");
                while (contingency != NULL) {
                    contingencies[i] = contingency;
                    i++;
                    contingency = strtok(NULL, "#");
                }
                printf("continges %s\n", contingency);
				for (int j = 0; j < i; j++) {
					printf("%d contingency: %s\n", j, contingencies[j]);
				}   
                if (i > 1) {
                    qsort(contingencies, 3, sizeof(contingencies[0]), compareStrings);
                }
                printf("hai");
                for (int j = 0; j < i; j++) {
                    printf("%d contingency: %s\n", j, contingencies[j]);
                }
                char* result = malloc(256);
                if (result == NULL) {
                    printf("malloc failed\n");
                    return -1;
                }
                for (int j = 0; j < 10; j++) {
				    printf("contingency: %s\n", contingencies[j]);
					if (contingencies[j] == NULL) {
                        continue;
					} else if (j == 0) {
                        strcpy(result, contingencies[0]);
						printf("sort: %s\n", result);
						printf("contingency: %s\n", contingencies[0]);
                    } else {
                        strcat(result, ", ");
                        strcat(result, contingencies[i]);
                    }
				}

				contingency = realloc(contingency, strlen(result) + 1);
                if (contingency == NULL) {
                    printf("realloc failed\n");
                    return -1;
                }
                strcpy(contingency, result);
				free(result);
            }
            printf("success %s\n", contingency);
			printf("category %s\n", category);
			
            size_t needed = snprintf(NULL, 0, "INSERT INTO contingency (`contingency name`, `NERC category`) VALUES ('%s', '%s');", contingency, category) + 1;
            char* sql_str = malloc(needed);
            if (sql_str == NULL) {
                printf("malloc failed\n");
                return -1;
            }
            if (category != NULL) {
                sprintf_s(sql_str, needed, "INSERT INTO contingency(`contingency name`, `NERC category`) VALUES('%s', '%s');", contingency, category);
                printf("sql_str: %s\n", sql_str);
            }
            else {
                sprintf_s(sql_str, needed, "INSERT INTO contingency (`contingency name`) VALUES ('%s');", contingency);
                printf("sql_str: %s\n", sql_str);
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
            printf("done1\n");
            free(sql_str);
            printf("done2\n");
			if (contingencies[0] != NULL) {
				free(contingency);
                printf("done3\n");
			}
        
            printf("done4\n");
		}
    }
	printf("count: %d\n", count);
    sqlite3_close(db);
    sqlite3_shutdown();
}

