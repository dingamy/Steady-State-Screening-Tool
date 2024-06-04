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

void populateScenCont(sqlite3* db, const char *path) {
    sqlite3_stmt* stmt = NULL;
    int rc = 0;
    DIR* d;
    struct dirent* dir;
    d = opendir(path);
    if (d == NULL) {
        printf("Couldn't open directory\n");
        sqlite3_close(db);
        exit(-1);
    }

    while ((dir = readdir(d)) != NULL) {
		if (dir->d_type == DT_DIR) { // recursively traverse through directories
            char filePath[1024];
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
                continue;
            snprintf(filePath, sizeof(filePath), "%s/%s", path, dir->d_name);
            populateScenCont(db, filePath);
		}
        if (strstr(dir->d_name, ".csv") != NULL) { // if it's a csv file then we process
            char* filename = strdup(dir->d_name);
            if (!filename) {
                handle_error(db, "strdup error");
                closedir(d);
                exit(-1);
            }

            printf("File: %s\n", filename);

            // parsing the filename to get column data
            char* category = NULL;
            char* contingency;
            contingency = strtok(filename, "@");
            contingency = strtok(NULL, "@");
            contingency = strtok(contingency, "-");
            char* scenario = strdup(filename);
            if (!scenario) {
                handle_error(db, "strdup error");
                free(filename);
                closedir(d);
                exit(-1);
            }
            scenario = strtok(scenario, "@");

            char* study = strdup(scenario);
            if (!study) {
                handle_error(db, "strdup error");
                free(filename);
                free(scenario);
                closedir(d);
                exit(-1);
            }
            study = strtok(study, "-");
            char* season = strtok(NULL, "-");
            char* topology = strtok(NULL, "");
            char* year = strdup(season);
            if (!filename) {
                handle_error(db, "strdup error");
                free(filename);
                free(scenario);
                free(study);
                closedir(d);
                exit(-1);
            }
            year += 3;
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
            //printf("scenario: %s\n", scenario);
            //printf("study: %s\n", study);
            //printf("season: %s\n", season);
            //printf("year: %s\n", year);
            //printf("load: %s\n", load);
            //printf("topology: %s\n", topology);

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

            // inserting data into scenarios table
			// allocating memory for the sql string
            size_t needed = snprintf(NULL, 0, "INSERT OR IGNORE INTO Scenarios (`Scenario Name`, `Study`, `Season`, `Year`, `Load`, `Topology`) VALUES ('%s', '%s', '%s', '%d', '%f', '%s');", scenario, study, season, atoi(year), atof(load), topology) + 1;
            char* sql_str = malloc(needed);
            if (sql_str == NULL) {
                printf("malloc failed\n");
                free(filename);
                free(scenario);
                free(study);
                sqlite3_close(db);
                return -1;
            }
			// adding the data to the sql string
            if (category != NULL) {
                snprintf(sql_str, needed, "INSERT OR IGNORE INTO Scenarios(`Scenario Name`, `Study`, `Season`, `Year`, `Load`, `Topology`) VALUES('%s', '%s', '%s', '%d', '%f', '%s');", scenario, study, season, atoi(year), atof(load), topology);
            }
            else {
                snprintf(sql_str, needed, "INSERT OR IGNORE INTO Scenarios (`Scenario Name`, `Study`, `Season`, `Year`, `Load`, `Topology`) VALUES ('%s', '%s', '%s', '%d', '%f', '%s');", scenario, study, season, atoi(year), atof(load), topology);
            }

            stmt = NULL;
            rc = sqlite3_prepare_v2(db, sql_str, -1, &stmt, NULL);
            if (rc != SQLITE_OK) {
                handle_error(db, "prepare error");
                free(filename);
                free(scenario);
                free(study);
                free(sql_str);
                exit(-1);
            }

            rc = sqlite3_step(stmt);
            while (rc != SQLITE_DONE) {
                handle_error(db, "statement error");
                free(filename);
                free(scenario);
                free(study);
                free(sql_str);
                exit(-1);
            }
            sqlite3_finalize(stmt);
            free(sql_str);
			// inserting data into contingency table
            // allocating memory for the sql string
            needed = snprintf(NULL, 0, "INSERT OR IGNORE INTO Contingency (`Contingency Name`, `NERC Category`) VALUES ('%s', '%s');", contingency, category) + 1;
            sql_str = malloc(needed);
            if (sql_str == NULL) {
                printf("malloc failed\n");
                free(filename);
                free(scenario);
                free(study);
                sqlite3_close(db);
                exit(-1);
            }
            // adding the data to the sql string
            if (category != NULL) {
                snprintf(sql_str, needed, "INSERT OR IGNORE INTO Contingency (`Contingency Name`, `NERC Category`) VALUES('%s', '%s');", contingency, category);
            }
            else {
                snprintf(sql_str, needed, "INSERT OR IGNORE INTO Contingency (`Contingency Name`) VALUES ('%s');", contingency);
            }

            rc = sqlite3_prepare_v2(db, sql_str, -1, &stmt, NULL);
            if (rc != SQLITE_OK) {
                handle_error(db, "prepare error");
                free(filename);
                free(scenario);
                free(study);
                free(sql_str);
                exit(-1);
            }
            rc = sqlite3_step(stmt);
            while (rc != SQLITE_DONE) {
                handle_error(db, "statement error");
                free(filename);
                free(scenario);
                free(study);
                free(sql_str);
                exit(-1);
            }

            COUNT++;
            sqlite3_finalize(stmt);
            stmt = NULL;
            free(filename);
            free(scenario);
            free(study);
            free(sql_str);

        }

    }
}
char* getContingency(char* path) {

	char* pathCopy = strdup(path);
    char* contingency = strtok(pathCopy, "@");
    printf("contingency1%s\n", contingency);
    contingency = strtok(NULL, "@");
    printf("contingency2%s\n", contingency);
    contingency = strtok(contingency, "-");
    printf("contingency3%s\n", contingency);
    if (strcmp(contingency, "System") == 0) { // edge case
        contingency = "INTACT";
    }
    if (contingency[0] == 'P') { // if category exists, then it starts with P followed by 1 or 2 digits
        strtok(contingency, "#");
        contingency = strtok(NULL, "");
    }
	
    printf("contingency4%s\n", contingency);
    return contingency;
}
char* getScenario(char* path) {
    char* pathCopy = strdup(path);
    char* scenario = strdup(pathCopy);
    scenario = strtok(scenario, "@");
	return scenario;
}
char* getSeason(char* path) {
	char* pathCopy = strdup(path);
	char* season = strtok(pathCopy, "-");
	season = strtok(NULL, "-");
	printf("season: %s\n", season);
	return season;
}
void populateBusTables(sqlite3* db, char* path) {
    int linecount = 0;
    sqlite3_stmt* stmt = NULL;
    sqlite3_stmt* stmt2 = NULL;
    int rc = 0;

    DIR* d;
    struct dirent* dir;
    d = opendir(path);
    if (d == NULL) {
        printf("Couldn't open directory\n");
        sqlite3_close(db);
        exit(-1);
    }

    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR) { // recursively traverse through directories
            char filePath[1024];
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
                continue;
            snprintf(filePath, sizeof(filePath), "%s/%s", path, dir->d_name);
            populateBusTables(db, filePath);
        }
        if (strstr(dir->d_name, ".csv") != NULL) { // if it's a csv file then we process
            char* filename = strdup(dir->d_name);
            //char* filename = "Base-sum2030-LGspcHH@BRNDSHUNT-volt.csv";
            if (!filename) {
                handle_error(db, "strdup error");
                closedir(d);
                exit(-1);
            }

            size_t needed = snprintf(NULL, 0, "INSERT OR IGNORE INTO BUS (`Bus Number`, `Bus Name`, `Area`, `Zone`, `Owner`, `Voltage Base`, `criteria_nlo`, `criteria_nhi`, `criteria_elo`, `criteria_ehi`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);") + 100;
            char* sql_str = malloc(needed);
            if (sql_str == NULL) {
                printf("malloc failed\n");
                return -1;
            }
            snprintf(sql_str, needed, "INSERT OR IGNORE INTO BUS (`Bus Number`, `Bus Name`, `Area`, `Zone`, `Owner`, `Voltage Base`, `criteria_nlo`, `criteria_nhi`, `criteria_elo`, `criteria_ehi`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
            rc = sqlite3_prepare_v2(db, sql_str, -1, &stmt, NULL);
            if (rc != SQLITE_OK) {
                handle_error(db, "prepare error");
                exit(-1);
            }
            char* contingency = getContingency(filename);
            char* scenario = getScenario(filename);
            size_t needed2 = snprintf(NULL, 0, "INSERT OR IGNORE INTO `Bus Simulation Results` (`Scenario Name`, `Contingency Name`, `Bus Number`, `stat`, `bus_pu`, `bus_angle`, `violate`, `exception`) VALUES ('%s', '%s', ?, ?, ?, ?, ?, ?);", scenario, contingency) + 100;
            char* sql_str2 = malloc(needed2);
            if (sql_str2 == NULL) {
                printf("malloc failed\n");
                return -1;
            }
            snprintf(sql_str2, needed2, "INSERT OR IGNORE INTO `Bus Simulation Results` (`Scenario Name`, `Contingency Name`, `Bus Number`, `stat`, `bus_pu`, `bus_angle`, `violate`, `exception`) VALUES (?, ?, ?, ?, ?, ?, ?, ?);");
            rc = sqlite3_prepare_v2(db, sql_str2, -1, &stmt2, NULL);
            if (rc != SQLITE_OK) {
                handle_error(db, "prepare error");
                exit(-1);
            }
         
            char* line = malloc(1024);

            char filePath[1024];
            snprintf(filePath, sizeof(filePath), ".\\Voltage\\%s", dir->d_name);
            
            FILE* file = fopen(filePath, "r");
			if (file == NULL) {
				printf("Couldn't open file\n");
				sqlite3_close(db);
				exit(-1);
			}
            fgets(line, 1024, file); // gets rid of column name
			printf("line: %s\n", line);

    
            while (fgets(line, 1024, file) != NULL) {
                char* bus_number = strtok(line, ",");
                char* bus_name = strtok(NULL, ",");
                char* area = strtok(NULL, ",");
                char* zone = strtok(NULL, ",");
                char* owner = strtok(NULL, ",");
                char* voltage_base = strtok(NULL, ",");
                char* criteria_nlo = strtok(NULL, ",");
                char* criteria_nhi = strtok(NULL, ",");
                char* criteria_elo = strtok(NULL, ",");
                char* criteria_ehi = strtok(NULL, ",");
                char* stat = strtok(NULL, ",");
                char* bus_pu = strtok(NULL, ",");
                char* bus_angle = strtok(NULL, ",");
                char* violate = strtok(NULL, ",");
                char* exception = strtok(NULL, ",");

                //printf("line: %s\n", line);
                printf("bus_number: %s\n", bus_number);
                //printf("bus_name: %s\n", bus_name);
                //printf("area: %s\n", area);
                //printf("zone: %s\n", zone);
                //printf("owner: %s\n", owner);
                //printf("voltage_base: %s\n", voltage_base);
                //printf("criteria_nlo: %s\n", criteria_nlo);
                //printf("criteria_nhi: %s\n", criteria_nhi);
                //printf("criteria_elo: %s\n", criteria_elo);
                //printf("criteria_ehi: %s\n", criteria_ehi);
                //printf("stat: %s\n", stat);
                //printf("bus_pu: %s\n", bus_pu);
                //printf("bus_angle: %s\n", bus_angle);
                //printf("violate: %d\n", atoi(violate));
                //printf("exception: %s\n", exception);
				
                sqlite3_bind_int64(stmt, 1, atoi(bus_number), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, bus_name, -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 3, atoi(area), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 4, atoi(zone), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 5, atoi(owner), -1, SQLITE_STATIC);
                sqlite3_bind_double(stmt, 6, atof(voltage_base), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 1, atoi(bus_number), -1, SQLITE_STATIC);
                sqlite3_bind_double(stmt, 8, atof(criteria_nhi), -1, SQLITE_STATIC);
                sqlite3_bind_double(stmt, 9, atof(criteria_elo), -1, SQLITE_STATIC);
                sqlite3_bind_double(stmt, 10, atof(criteria_ehi), -1, SQLITE_STATIC);
                rc = sqlite3_step(stmt);
                if ((rc != SQLITE_DONE) && (rc != SQLITE_ROW)) {
                    handle_error(db, "step error");
                    sqlite3_close(db);
                    exit(-1);
                }
                sqlite3_reset(stmt);
                sqlite3_clear_bindings(stmt);
                sqlite3_bind_text(stmt2, 1, scenario, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt2, 2, contingency, -1, SQLITE_STATIC);
                sqlite3_bind_int64(stmt2, 3, atoi(bus_number), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt2, 4, atoi(stat), -1, SQLITE_STATIC);
                sqlite3_bind_double(stmt2, 5, atof(bus_pu), -1, SQLITE_STATIC);
                sqlite3_bind_double(stmt2, 6, atof(bus_angle), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt2, 7, atoi(violate), -1, SQLITE_STATIC);

                if (exception == NULL) {
                    sqlite3_bind_int(stmt2, 8, 0, SQLITE_STATIC);
                }
                else {
                    sqlite3_bind_int(stmt2, 8, atoi(exception), -1, SQLITE_STATIC);
                }
                rc = sqlite3_step(stmt2);
                if ((rc != SQLITE_DONE) && (rc != SQLITE_ROW)) {
                    handle_error(db, "step error");
                    sqlite3_close(db);
                    exit(-1);
                }
                sqlite3_reset(stmt2);
                sqlite3_clear_bindings(stmt2);
                linecount++;
            }

			fclose(file);
            stmt = NULL;
            stmt2 = NULL;
            free(sql_str);
            free(sql_str2);
			free(line);
            sqlite3_finalize(stmt);
            sqlite3_finalize(stmt2);

        }
    }
    closedir(d);
}
void populateBranchTables(sqlite3* db, char* path) {
    int linecount = 0;
    sqlite3_stmt* stmt = NULL;
    sqlite3_stmt* stmt2 = NULL;
	sqlite3_stmt* stmt3 = NULL;
	sqlite3_stmt* stmt4 = NULL;
	sqlite3_stmt* stmt5 = NULL;
    int rc = 0;

    DIR* d;
    struct dirent* dir;
    d = opendir(path);
    if (d == NULL) {
        printf("Couldn't open directory\n");
        sqlite3_close(db);
        exit(-1);
    }

    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR) { // recursively traverse through directories
            char filePath[1024];
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
                continue;
            snprintf(filePath, sizeof(filePath), "%s/%s", path, dir->d_name);
            populateBranchTables(db, filePath);
        }
        if (strstr(dir->d_name, ".csv") != NULL) { // if it's a csv file then we process
            char* filename = strdup(dir->d_name);
            //char* filename = "Base-sum2030-LGspcHH@BRNDSHUNT-volt.csv";
            if (!filename) {
                handle_error(db, "strdup error");
                closedir(d);
                exit(-1);
            }
            printf("filename: %s\n", filename);
            size_t needed = snprintf(NULL, 0, "INSERT OR IGNORE INTO Branch (`Branch Name`, `Metered Bus Number`, `Other Bus Number`, `Bus ID`, `Voltage Base`, `RateA sum`, `RateB sum`, `RateC sum`, `RateA win`, `RateB win`, `RateC win`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
			char* sql_str = malloc(needed);
			if (sql_str == NULL) {
				printf("malloc failed\n");
				exit(-1);
			}
			snprintf(sql_str, needed, "INSERT OR IGNORE INTO Branch (`Branch Name`, `Metered Bus Number`, `Other Bus Number`, `Bus ID`, `Voltage Base`, `RateA sum`, `RateB sum`, `RateC sum`, `RateA win`, `RateB win`, `RateC win`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
			rc = sqlite3_prepare_v2(db, sql_str, -1, &stmt, NULL);
			if (rc != SQLITE_OK) {
				handle_error(db, "prepare error1");
				exit(-1);
			}
			char* contingency = getContingency(filename);
			printf("contingency: %s\n", contingency);
			char* scenario = getScenario(filename);
            char* season = getSeason(filename);

            size_t needed2 = snprintf(NULL, 0, "INSERT OR IGNORE INTO `Branch Simulation Results` (`Scenario Name`, `Contingency Name`, `Branch Name`, `stat`, `p_metered`, `p_other`, `q_metered`, `q_other`, `amp_angle_metered`, `amp_angle_other`, `amp_metered`, `amp_other`, `ploss`, `qloss`, `violate`, `exception`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
			char* sql_str2 = malloc(needed2);
            if (sql_str2 == NULL) {
				printf("malloc failed\n");
				exit(-1);   
            }
            snprintf(sql_str2, needed2, "INSERT OR IGNORE INTO `Branch Simulation Results` (`Scenario Name`, `Contingency Name`, `Branch Name`, `stat`, `p_metered`, `p_other`, `q_metered`, `q_other`, `amp_angle_metered`, `amp_angle_other`, `amp_metered`, `amp_other`, `ploss`, `qloss`, `violate`, `exception`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
			rc = sqlite3_prepare_v2(db, sql_str2, -1, &stmt2, NULL);
            if (rc != SQLITE_OK) {
				handle_error(db, "prepare error2");
				exit(-1);
            }

            size_t needed3 = snprintf(NULL, 0, "SELECT exists(SELECT * FROM `Branch` WHERE `Branch Name` = ?);");
			char* sql_str3 = malloc(needed3);
			if (sql_str3 == NULL) {
				printf("malloc failed\n");
				exit(-1);
			}
			snprintf(sql_str3, needed3, "SELECT exists(SELECT * FROM `Branch` WHERE `Branch Name` = ?);");
			rc = sqlite3_prepare_v2(db, sql_str3, -1, &stmt3, NULL);
            if (rc != SQLITE_OK) {
                handle_error(db, "prepare error3");
                exit(-1);
            }

            size_t needed4 = snprintf(NULL, 0, "UPDATE Branch SET `RateA sum` = ?, `RateB sum` = ?, `RateC sum` = ? WHERE `Branch Name` = ?;");
            char* sql_str4 = malloc(needed4);
            if (sql_str4 == NULL) {
                printf("malloc failed\n");
                exit(-1);
            }
            snprintf(sql_str4, needed4, "UPDATE Branch SET `RateA sum` = ?, `RateB sum` = ?, `RateC sum` = ? WHERE `Branch Name` = ?;");
            rc = sqlite3_prepare_v2(db, sql_str4, -1, &stmt4, NULL);
            if (rc != SQLITE_OK) {
                handle_error(db, "prepare error4");
                exit(-1);
            }

            size_t needed5 = snprintf(NULL, 0, "UPDATE Branch SET `RateA win` = ?, `RateB win` = ?, `RateC win` = ? WHERE `Branch Name` = ?;");
            char* sql_str5 = malloc(needed4);
            if (sql_str5 == NULL) {
                printf("malloc failed\n");
                exit(-1);
            }
            snprintf(sql_str5, needed5, "UPDATE Branch SET `RateA win` = ?, `RateB win` = ?, `RateC win` = ? WHERE `Branch Name` = ?;");
            rc = sqlite3_prepare_v2(db, sql_str5, -1, &stmt5, NULL);
            if (rc != SQLITE_OK) {
                handle_error(db, "prepare error4");
                exit(-1);
            }

			char* line = malloc(1024);
			char filePath[1024];
			snprintf(filePath, sizeof(filePath), ".\\ThermalBranch\\%s", dir->d_name);
			FILE* file = fopen(filePath, "r");
			if (file == NULL) {
				printf("Couldn't open file\n");
				sqlite3_close(db);
				exit(-1);
            }
			fgets(line, 1024, file); // gets rid of column name
			printf("line: %s\n", line);
            while (fgets(line, 1024, file) != NULL) {
				char* branch_name = strtok(line, ",");
				char* metered_bus_number = strtok(NULL, ",");
				char* other_bus_number = strtok(NULL, ",");
				char* branch_id = strtok(NULL, ",");
				char* voltage_base = strtok(NULL, ",");
                char* rateA = strtok(NULL, ",");
				char* rateB = strtok(NULL, ",");
				char* rateC = strtok(NULL, ",");
				char* p_metered = strtok(NULL, ",");
				char* p_other = strtok(NULL, ",");
				char* q_metered = strtok(NULL, ",");
				char* q_other = strtok(NULL, ",");
				char* amp_angle_metered = strtok(NULL, ",");
				char* amp_angle_other = strtok(NULL, ",");
				char* amp_metered = strtok(NULL, ",");
				char* amp_other = strtok(NULL, ",");
				char* ploss = strtok(NULL, ",");
				char* qloss = strtok(NULL, ",");
				char* stat = strtok(NULL, ",");
				char* violate = strtok(NULL, ",");
				char* exception = strtok(NULL, ",");
                
				//printf("branch_name: %s\n", branch_name);
				//printf("metered_bus_number: %s\n", metered_bus_number);
				//printf("other_bus_number: %s\n", other_bus_number);
				//printf("branch_id: %s\n", branch_id);
				//printf("voltage_base: %s\n", voltage_base);
				//printf("rateA: %s\n", rateA);
				//printf("rateB: %s\n", rateB);
				//printf("rateC: %s\n", rateC);
				//printf("p_metered: %s\n", p_metered);
				//printf("p_other: %s\n", p_other);
				//printf("q_metered: %s\n", q_metered);
				//printf("q_other: %s\n", q_other);
				//printf("amp_angle_metered: %s\n", amp_angle_metered);
				//printf("amp_angle_other: %s\n", amp_angle_other);
				//printf("amp_metered: %s\n", amp_metered);
				//printf("amp_other: %s\n", amp_other);
				//printf("ploss: %s\n", ploss);
				//printf("qloss: %s\n", qloss);
				//printf("stat: %s\n", stat);
				//printf("violate: %s\n", violate);
				//printf("exception: %s\n", exception);
                
				sqlite3_bind_text(stmt3, 1, branch_name, -1, SQLITE_STATIC);
                rc = sqlite3_step(stmt3);
                if (rc == SQLITE_ROW) {
                    // Retrieve the result from the first (and only) column
                    int exists = sqlite3_column_int(stmt3, 0);
                    if (exists == 1) {
                        if (season[0] == 's' || season[0] == 'S') {
                            sqlite3_bind_double(stmt4, 1, atof(rateA), -1, SQLITE_STATIC);
                            sqlite3_bind_double(stmt4, 2, atof(rateB), -1, SQLITE_STATIC);
                            sqlite3_bind_double(stmt4, 3, atof(rateC), -1, SQLITE_STATIC);
                            sqlite3_bind_text(stmt4, 4, branch_name, -1, SQLITE_STATIC);
                            rc = sqlite3_step(stmt4);
                            if ((rc != SQLITE_DONE) && (rc != SQLITE_ROW)) {
                                handle_error(db, "step error");
                                sqlite3_close(db);
                                exit(-1);
                            }

                            sqlite3_reset(stmt4);
                            sqlite3_clear_bindings(stmt4);
                        }
                        else {
                            sqlite3_bind_double(stmt5, 1, atof(rateA), -1, SQLITE_STATIC);
                            sqlite3_bind_double(stmt5, 2, atof(rateB), -1, SQLITE_STATIC);
                            sqlite3_bind_double(stmt5, 3, atof(rateC), -1, SQLITE_STATIC);
                            sqlite3_bind_text(stmt5, 4, branch_name, -1, SQLITE_STATIC);
                            rc = sqlite3_step(stmt5);
                            if ((rc != SQLITE_DONE) && (rc != SQLITE_ROW)) {
                                handle_error(db, "step error");
                                sqlite3_close(db);
                                exit(-1);
                            }

                            sqlite3_reset(stmt5);
                            sqlite3_clear_bindings(stmt5);
                        }

                    }
                    else {
                        sqlite3_bind_text(stmt, 1, branch_name, -1, SQLITE_STATIC);
                        sqlite3_bind_int64(stmt, 2, atoi(metered_bus_number), -1, SQLITE_STATIC);
                        sqlite3_bind_int64(stmt, 3, atoi(other_bus_number), -1, SQLITE_STATIC);
                        sqlite3_bind_text(stmt, 4, branch_id, -1, SQLITE_STATIC);
                        sqlite3_bind_double(stmt, 5, atof(voltage_base), -1, SQLITE_STATIC);
                        if (season[0] == 's' || season[0] == 'S') {
                            sqlite3_bind_double(stmt, 6, atof(rateA), -1, SQLITE_STATIC);
                            sqlite3_bind_double(stmt, 7, atof(rateB), -1, SQLITE_STATIC);
                            sqlite3_bind_double(stmt, 8, atof(rateC), -1, SQLITE_STATIC);
                        }
                        else {
                            sqlite3_bind_double(stmt, 9, atof(rateA), -1, SQLITE_STATIC);
                            sqlite3_bind_double(stmt, 10, atof(rateB), -1, SQLITE_STATIC);
                            sqlite3_bind_double(stmt, 11, atof(rateC), -1, SQLITE_STATIC);
                        }

                        rc = sqlite3_step(stmt);
                        if ((rc != SQLITE_DONE) && (rc != SQLITE_ROW)) {
                            handle_error(db, "step error");
                            sqlite3_close(db);
                            exit(-1);
                        }

                        sqlite3_reset(stmt);
                        sqlite3_clear_bindings(stmt);
                    }
                }
                else {
                    fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
                }

                sqlite3_reset(stmt3);
                sqlite3_clear_bindings(stmt3);

				sqlite3_bind_text(stmt2, 1, scenario, -1, SQLITE_STATIC);
				sqlite3_bind_text(stmt2, 2, contingency, -1, SQLITE_STATIC);
				sqlite3_bind_text(stmt2, 3, branch_name, -1, SQLITE_STATIC);
				sqlite3_bind_int(stmt2, 4, atoi(stat), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 5, atof(p_metered), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 6, atof(p_other), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 7, atof(q_metered), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 8, atof(q_other), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 9, atof(amp_angle_metered), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 10, atof(amp_angle_other), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 11, atof(amp_metered), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 12, atof(amp_other), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 13, atof(ploss), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 14, atof(qloss), -1, SQLITE_STATIC);
				sqlite3_bind_int(stmt2, 15, atoi(violate), -1, SQLITE_STATIC);
                if (exception == NULL) {
                    sqlite3_bind_int(stmt2, 16, 0, SQLITE_STATIC);
                } else {
                    sqlite3_bind_int(stmt2, 16, atoi(exception), -1, SQLITE_STATIC);
                }
				rc = sqlite3_step(stmt2);
                if ((rc != SQLITE_DONE) && (rc != SQLITE_ROW)) {
					handle_error(db, "step error");
					sqlite3_close(db);
					exit(-1);
                }
				sqlite3_reset(stmt2);
				sqlite3_clear_bindings(stmt2);


            }
			fclose(file);
			stmt = NULL;
			stmt2 = NULL;
			stmt3 = NULL;
			free(sql_str);
			free(sql_str2);
            free(sql_str3);
			free(line);
			sqlite3_finalize(stmt);
			sqlite3_finalize(stmt2);
            sqlite3_finalize(stmt3);
        }
    }
}
void populateTransformer2(sqlite3* db, char* path) {
    sqlite3_stmt* stmt = NULL;
	sqlite3_stmt* stmt2 = NULL;
    int rc = 0;
	DIR* d;
	struct dirent* dir;
	d = opendir(path);
	if (d == NULL) {
		printf("Couldn't open directory\n");
		sqlite3_close(db);
		exit(-1);
	}
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR) { // recursively traverse through directories
            char filePath[1024];
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
                continue;
            snprintf(filePath, sizeof(filePath), "%s/%s", path, dir->d_name);
            populateTransformer2(db, filePath);
        }
        if (strstr(dir->d_name, ".csv") != NULL) { // if it's a csv file then we process
            char* filename = strdup(dir->d_name);
            //char* filename = "}
			if (!filename) {
				handle_error(db, "strdup error");
				closedir(d);
				exit(-1);
			}
			printf("filename: %s\n", filename);
            size_t needed = snprintf(NULL, 0, "INSERT OR IGNORE INTO Transformer2 (`Xformer Name`, `Winding 1`, `Winding 2`, `Xfmr ID`, `MVA Base`, `Winding 1 nominal KV`, `Winding 2 nominal KV`, `RateA Winding 1`, `RateB Winding 1`, `RateC Winding 1`, `RateA Winding 2`, `RateB Winding 2`, `RateC Winding 2`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
			char* sql_str = malloc(needed);
            if (sql_str == NULL) {
				printf("malloc failed\n");
				exit(-1);
            }

            snprintf(sql_str, needed, "INSERT OR IGNORE INTO Transformer2 (`Xformer Name`, `Winding 1`, `Winding 2`, `Xfmr ID`, `MVA Base`, `Winding 1 nominal KV`, `Winding 2 nominal KV`, `RateA Winding 1`, `RateB Winding 1`, `RateC Winding 1`, `RateA Winding 2`, `RateB Winding 2`, `RateC Winding 2`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
			rc = sqlite3_prepare_v2(db, sql_str, -1, &stmt, NULL);
            if (rc != SQLITE_OK) {
				handle_error(db, "prepare error1");
				exit(-1);
            }

			char* contingency = getContingency(filename);
			printf("contingency: %s\n", contingency);
			char* scenario = getScenario(filename);

            size_t needed2 = snprintf(NULL, 0, "INSERT OR IGNORE INTO `Transformer2 Simulation Results` (`Scenario Name`, `Contingency Name`, `Xformer Name`, `stat`, `p_winding 1`, `p_winding 2`, `q_winding 1`, `q_winding 2`, `amp_winding1`, `amp_winding2`, `ploss`, `qloss`, `violate`, `exception`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
			char* sql_str2 = malloc(needed2);
            if (sql_str2 == NULL) {
				printf("malloc failed\n");
				exit(-1);
            }
            snprintf(sql_str2, needed2, "INSERT OR IGNORE INTO `Transformer2 Simulation Results` (`Scenario Name`, `Contingency Name`, `Xformer Name`, `stat`, `p_winding 1`, `p_winding 2`, `q_winding 1`, `q_winding 2`, `amp_winding1`, `amp_winding2`, `ploss`, `qloss`, `violate`, `exception`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
			rc = sqlite3_prepare_v2(db, sql_str2, -1, &stmt2, NULL);
			if (rc != SQLITE_OK) {
				handle_error(db, "prepare error2");
				exit(-1);
            }
			char* line = malloc(1024);
			char filePath[1024];
			snprintf(filePath, sizeof(filePath), ".\\Thermal2winding\\%s", dir->d_name);
			FILE* file = fopen(filePath, "r");
            if (file == NULL) {
				printf("Couldn't open file\n");
				sqlite3_close(db);
				exit(-1);
            }
			fgets(line, 1024, file); // gets rid of column name
			printf("line: %s\n", line);
            while (fgets(line, 1024, file) != NULL) {
				printf("line: %s\n", line);
				size_t line_size = snprintf(NULL, 0, line) + 2;
				char* space = malloc(line_size);
				space[0] = '$';
				space[1] = '\0';
                printf("lin1e: %s\n", space);
                strcat(space, line);
				printf("line: %s\n", space);
				char* xformer_name = strtok(space, ",");
				char* winding1 = strtok(NULL, ",");
				char* winding2 = strtok(NULL, ",");
				char* xfmr_id = strtok(NULL, ",");
				char* mva_base = strtok(NULL, ",");
				char* winding1_nominal_kv = strtok(NULL, ",");
				char* winding2_nominal_kv = strtok(NULL, ",");
				char* rateA_winding1 = strtok(NULL, ",");
				char* rateB_winding1 = strtok(NULL, ",");
				char* rateC_winding1 = strtok(NULL, ",");
				char* rateA_winding2 = strtok(NULL, ",");
				char* rateB_winding2 = strtok(NULL, ",");
				char* rateC_winding2 = strtok(NULL, ",");
				char* p_winding1 = strtok(NULL, ",");
				char* p_winding2 = strtok(NULL, ",");
				char* q_winding1 = strtok(NULL, ",");
				char* q_winding2 = strtok(NULL, ",");
				char* amp_winding1 = strtok(NULL, ",");
				char* amp_winding2 = strtok(NULL, ",");
				char* ploss = strtok(NULL, ",");
				char* qloss = strtok(NULL, ",");
				char* stat = strtok(NULL, ",");
				char* violate = strtok(NULL, ",");
				char* exception = strtok(NULL, ",");

				printf("xformer_name: %s\n", xformer_name);
                if (xformer_name[0] == '$') {
                    memset(xformer_name, 0, strlen(xformer_name));

                    strcat(xformer_name, winding1);
                    strcat(xformer_name, ":");
                    strcat(xformer_name, winding2);
                    strcat(xformer_name, ":");
					strcat(xformer_name, xfmr_id);
				}
				printf("winding1: %s\n", winding1);
				printf("winding2: %s\n", winding2);
				printf("xfmr_id: %s\n", xfmr_id);
				printf("mva_base: %s\n", mva_base);
				printf("winding1_nominal_kv: %s\n", winding1_nominal_kv);
				printf("winding2_nominal_kv: %s\n", winding2_nominal_kv);
				printf("rateA_winding1: %s\n", rateA_winding1);
				printf("rateB_winding1: %s\n", rateB_winding1);
				printf("rateC_winding1: %s\n", rateC_winding1);
				printf("rateA_winding2: %s\n", rateA_winding2);
				printf("rateB_winding2: %s\n", rateB_winding2);
				printf("rateC_winding2: %s\n", rateC_winding2);
				printf("p_winding1: %s\n", p_winding1);
				printf("p_winding2: %s\n", p_winding2);
				printf("q_winding1: %s\n", q_winding1);
				printf("q_winding2: %s\n", q_winding2);
				printf("amp_winding1: %s\n", amp_winding1);
				printf("amp_winding2: %s\n", amp_winding2);
				printf("ploss: %s\n", ploss);
				printf("qloss: %s\n", qloss);
				printf("stat: %s\n", stat);
				printf("violate: %s\n", violate);
				printf("exception: %s\n", exception);
                
				sqlite3_bind_text(stmt, 1, xformer_name, -1, SQLITE_STATIC);
				sqlite3_bind_int64(stmt, 2, atoi(winding1), -1, SQLITE_STATIC);
				sqlite3_bind_int64(stmt, 3, atoi(winding2), -1, SQLITE_STATIC);
				sqlite3_bind_text(stmt, 4, xfmr_id, -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt, 5, atof(mva_base), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt, 6, atof(winding1_nominal_kv), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt, 7, atof(winding2_nominal_kv), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt, 8, atof(rateA_winding1), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt, 9, atof(rateB_winding1), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt, 10, atof(rateC_winding1), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt, 11, atof(rateA_winding2), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt, 12, atof(rateB_winding2), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt, 13, atof(rateC_winding2), -1, SQLITE_STATIC);
				rc = sqlite3_step(stmt);
                if ((rc != SQLITE_DONE) && (rc != SQLITE_ROW)) {
                    handle_error(db, "step error");
                    sqlite3_close(db);
                    exit(-1);
                }
                sqlite3_reset(stmt);
                sqlite3_clear_bindings(stmt);

				sqlite3_bind_text(stmt2, 1, scenario, -1, SQLITE_STATIC);
				sqlite3_bind_text(stmt2, 2, contingency, -1, SQLITE_STATIC);
				sqlite3_bind_text(stmt2, 3, xformer_name, -1, SQLITE_STATIC);
				sqlite3_bind_int(stmt2, 4, atoi(stat), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 5, atof(p_winding1), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 6, atof(p_winding2), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 7, atof(q_winding1), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 8, atof(q_winding2), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 9, atof(amp_winding1), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 10, atof(amp_winding2), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 11, atof(ploss), -1, SQLITE_STATIC);
				sqlite3_bind_double(stmt2, 12, atof(qloss), -1, SQLITE_STATIC);
				if (violate == NULL) {
					sqlite3_bind_int(stmt2, 13, 0, SQLITE_STATIC);
				}
				else {
					sqlite3_bind_int(stmt2, 13, atoi(violate), -1, SQLITE_STATIC);
				}

				if (exception == NULL) {
					sqlite3_bind_int(stmt2, 14, 0, SQLITE_STATIC);
				}
				else {
					sqlite3_bind_int(stmt2, 14, atoi(exception), -1, SQLITE_STATIC);
				}
				rc = sqlite3_step(stmt2);
                if ((rc != SQLITE_DONE) && (rc != SQLITE_ROW)) {
					handle_error(db, "step error"); 
					sqlite3_close(db);
					exit(-1);
                }
				sqlite3_reset(stmt2);
				sqlite3_clear_bindings(stmt2);

            }
            fclose(file);
            stmt = NULL;
            stmt2 = NULL;
            free(sql_str);
            free(sql_str2);
            free(line);
            sqlite3_finalize(stmt);
            sqlite3_finalize(stmt2);
        }
    }
    closedir(d);
}
int main(int argc, char* argv[]) {
    char* file = "database2.db";
    sqlite3* db = NULL;
    int rc = 0;
    sqlite3_initialize();
    rc = sqlite3_open_v2(file, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    if (rc != SQLITE_OK) {
        handle_error(db, "Cannot open database");     
        exit(-1);
    }

    // initializing tables
  //  const char* queries[] = {
  //      "DROP TABLE IF EXISTS Scenarios;",
  //      "CREATE TABLE Scenarios (`Scenario Name` TEXT PRIMARY KEY, `Study` TEXT, Season TEXT, Year INT, Load FLOAT, Topology TEXT);",
  //      "DROP TABLE IF EXISTS Contingency;",
		//"CREATE TABLE Contingency (`Contingency Name` TEXT PRIMARY KEY, `NERC Category` TEXT);",
  //      "DROP TABLE IF EXISTS BUS;",
  //      "CREATE TABLE BUS (`Bus Number` INT PRIMARY KEY, `Bus Name` TEXT, Area INT, Zone INT, Owner INT, `Voltage Base` FLOAT, criteria_nlo FLOAT, criteria_nhi FLOAT, criteria_elo FLOAT, criteria_ehi FLOAT);",
  //      "DROP TABLE IF EXISTS `Bus Simulation Results`;",
  //      "CREATE TABLE `Bus Simulation Results` (`Scenario Name` TEXT, `Contingency Name` TEXT, `Bus Number` INT, stat INT, bus_pu FLOAT NOT NULL, bus_angle FLOAT NOT NULL, violate INT, exception INT, PRIMARY KEY (`Scenario Name`, `Contingency Name`, `Bus Number`), FOREIGN KEY (`Scenario Name`) REFERENCES Scenarios (`Scenario Name`) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (`Contingency Name`) REFERENCES Contingency (`Contingency Name`) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (`Bus Number`) REFERENCES BUS (`Bus Number`) ON DELETE CASCADE ON UPDATE CASCADE);",
		//"DROP TABLE IF EXISTS Branch;",
		//"CREATE TABLE Branch (`Branch Name` TEXT PRIMARY KEY, `Metered Bus Number` INT, `Other Bus Number` INT, `Bus ID` TEXT, `Voltage Base` FLOAT, `RateA sum` FLOAT, `RateB sum` FLOAT, `RateC sum` FLOAT, `RateA win` FLOAT, `RateB win` FLOAT, `RateC win` FLOAT);",
		//"DROP TABLE IF EXISTS `Branch Simulation Results`;",
		//"CREATE TABLE `Branch Simulation Results` (`Scenario Name` TEXT, `Contingency Name` TEXT, `Branch Name` TEXT, `stat` INT, `p_metered` FLOAT, `p_other` FLOAT, `q_metered` FLOAT, `q_other` FLOAT, `amp_angle_metered` FLOAT, `amp_angle_other` FLOAT, `amp_metered` FLOAT, `amp_other` FLOAT, `ploss` FLOAT, `qloss` FLOAT, `violate` INT, `exception` INT, PRIMARY KEY (`Scenario Name`, `Contingency Name`, `Branch Name`), FOREIGN KEY (`Scenario Name`) REFERENCES Scenarios (`Scenario Name`) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (`Contingency Name`) REFERENCES Contingency (`Contingency Name`) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (`Branch Name`) REFERENCES Branch (`Branch Name`) ON DELETE CASCADE ON UPDATE CASCADE);"
  //      "DROP TABLE IF EXISTS Transformer2;",
        // "CREATE TABLE Transformer2 (`Xformer Name` TEXT PRIMARY KEY, `Winding 1` INT, `Winding 2` INT, `Xfmr ID` TEXT, `MVA Base` FLOAT, `Winding 1 nominal KV` FLOAT, `Winding 2 nominal KV` FLOAT, `RateA Winding 1` FLOAT, `RateB Winding 1` FLOAT, `RateC Winding 1` FLOAT, `RateA Winding 2` FLOAT, `RateB Winding 2` FLOAT, `RateC Winding 2` FLOAT, `Tap Limit min` FLOAT, `Tap Limit max` FLOAT, `Tap Steps` INT);",
      //  "DROP TABLE IF EXISTS `Transformer2 Simulation Results`;",
     //   "CREATE TABLE `Transformer2 Simulation Results` (`Scenario Name` TEXT, `Contingency Name` TEXT, `Xformer Name` TEXT, `stat` INT, `p_winding 1` FLOAT, `p_winding 2` FLOAT, `q_winding 1` FLOAT, `q_winding 2` FLOAT, `amp_winding1` FLOAT, `amp_winding2` FLOAT, `Tap 1 Ratio` FLOAT, `Tap 2 Ratio` FLOAT, `Winding 1 Angle` FLOAT, `ploss` FLOAT, `qloss` FLOAT, `violate` INT, `exception` INT, PRIMARY KEY (`Scenario Name`, `Contingency Name`, `Xformer Name`), FOREIGN KEY (`Scenario Name`) REFERENCES Scenarios (`Scenario Name`) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (`Contingency Name`) REFERENCES Contingency (`Contingency Name`) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (`Xformer Name`) REFERENCES Transformer2 (`Xformer Name`) ON DELETE CASCADE ON UPDATE CASCADE);"
  // 
  // 
  // 
  // 
  //  };
    const char* queries[] = {
        "DROP TABLE IF EXISTS Scenarios;",
        "CREATE TABLE Scenarios (`Scenario Name` TEXT PRIMARY KEY, `Study` TEXT, Season TEXT, Year INT, Load FLOAT, Topology TEXT);",
        "DROP TABLE IF EXISTS Contingency;",
        "CREATE TABLE Contingency (`Contingency Name` TEXT PRIMARY KEY, `NERC Category` TEXT, `Date Last Modified` TEXT);",
            };
    sqlite3_stmt* stmt = NULL;
    for (int i = 0; i < 4; i++) {
		printf("i: %d\n", i);
        stmt = NULL;
        rc = sqlite3_prepare_v2(db, queries[i], -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            handle_error(db, "prepare error");
            exit(-1);
        }
        rc = sqlite3_step(stmt);
        while (rc != SQLITE_DONE) {
            handle_error(db, "statement error");
            exit(-1);
        }
        sqlite3_finalize(stmt);
    }

	//populateScenCont(db, ".");
	//populateBusTables(db, "./Voltage");
	//populateBranchTables(db, "./ThermalBranch");
	populateTransformer2(db, "./Thermal2winding");
    
	return 0;
}



