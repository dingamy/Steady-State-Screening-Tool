#include "sqlite-amalgamation-3450300/sqlite3.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <io.h>
#include <time.h>
char* VOLTAGE_FOLDER = "./VoltageEEEE";
char* THERMALBRANCH_FOLDER = "./ThermalBranchHHHh";
char* THERMAL2_FOLDER = "./thermal2winding";

void handle_error(sqlite3* db, const char* msg) {
    fprintf(stderr, "%s: %s\n", msg, sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(-1);
}
char* getContingency(char* path) {
	char* pathCopy = strdup(path);
    char* contingency = strtok(pathCopy, "@");
    contingency = strtok(NULL, "@");
    contingency = strtok(contingency, "-");
    if (strcmp(contingency, "System") == 0) { // edge case
        contingency = "INTACT";
    }
    if (contingency[0] == 'P') { // if category exists, then it starts with P followed by 1 or 2 digits
        strtok(contingency, "#");
        contingency = strtok(NULL, "");
    }
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
	return season;
}
char* getDate(char* path) {
    struct stat filestat;
    stat(path, &filestat);
    char* dt[100];
    strftime(dt, sizeof(dt), "%Y-%m-%d %H:%M", localtime(&filestat.st_mtime));
    return dt;
}
void prepareStatement(sqlite3* db, const char* sql_str, sqlite3_stmt** stmt) {
	int rc = sqlite3_prepare_v2(db, sql_str, -1, stmt, NULL);
	if (rc != SQLITE_OK) {
		handle_error(db, "prepare error");
	}
}
int stepStatement(sqlite3* db, sqlite3_stmt** stmt) {
	int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
		handle_error(db, "step error");
	}
    return rc;
}

void processBusFile(sqlite3* db, FILE* file, sqlite3_stmt* statements[], char* scenario, char* contingency, char* date) {
    int rc = 0;
    char line[1024];
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

     
        sqlite3_bind_int64(statements[0], 1, atoi(bus_number), -1, SQLITE_STATIC);
        sqlite3_bind_text(statements[0], 2, bus_name, -1, SQLITE_STATIC);
        sqlite3_bind_int(statements[0], 3, atoi(area), -1, SQLITE_STATIC);
        sqlite3_bind_int(statements[0], 4, atoi(zone), -1, SQLITE_STATIC);
        sqlite3_bind_int(statements[0], 5, atoi(owner), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[0], 6, atof(voltage_base), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[0], 7, atof(criteria_nlo), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[0], 8, atof(criteria_nhi), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[0], 9, atof(criteria_elo), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[0], 10, atof(criteria_ehi), -1, SQLITE_STATIC);
        stepStatement(db, statements[0]);
        
        sqlite3_bind_text(statements[1], 1, scenario, -1, SQLITE_STATIC);
        sqlite3_bind_text(statements[1], 2, contingency, -1, SQLITE_STATIC);
        sqlite3_bind_int64(statements[1], 3, atoi(bus_number), -1, SQLITE_STATIC);
        sqlite3_bind_int(statements[1], 4, atoi(stat), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 5, atof(bus_pu), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 6, atof(bus_angle), -1, SQLITE_STATIC);
        sqlite3_bind_int(statements[1], 7, (strncmp(violate, "Fail", 4) == 0 || atoi(violate) == 1) ? 1 : 0);
        sqlite3_bind_int(statements[1], 8, exception ? atoi(exception) : 0);
        sqlite3_bind_text(statements[1], 9, date, -1, SQLITE_STATIC);
        stepStatement(db, statements[1]);

        for (int x = 0; x < 2; x++) {
            sqlite3_reset(statements[x]);
            sqlite3_clear_bindings(statements[x]);
        }
    }
}
void processBranchFile(sqlite3* db, FILE* file, sqlite3_stmt* statements[], char* scenario, char* contingency, char* season, char* date) {
    int rc = 0;
    char line[1024];
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

        sqlite3_bind_text(statements[2], 1, branch_name, -1, SQLITE_STATIC);
        stepStatement(db, statements[2]);

        // Retrieve the result from the first (and only) column
        int exists = sqlite3_column_int(statements[2], 0);
        if (exists == 1) { // branch already exists 
            if (season[0] == 's' || season[0] == 'S') { // summer
                sqlite3_bind_double(statements[3], 1, atof(rateA), -1, SQLITE_STATIC);
                sqlite3_bind_double(statements[3], 2, atof(rateB), -1, SQLITE_STATIC);
                sqlite3_bind_double(statements[3], 3, atof(rateC), -1, SQLITE_STATIC);
                sqlite3_bind_text(statements[3], 4, branch_name, -1, SQLITE_STATIC);
                stepStatement(db, statements[3]);
            }
            else { // winter
                sqlite3_bind_double(statements[4], 1, atof(rateA), -1, SQLITE_STATIC);
                sqlite3_bind_double(statements[4], 2, atof(rateB), -1, SQLITE_STATIC);
                sqlite3_bind_double(statements[4], 3, atof(rateC), -1, SQLITE_STATIC);
                sqlite3_bind_text(statements[4], 4, branch_name, -1, SQLITE_STATIC);
                stepStatement(db, statements[4]);
            }
        }
        else { // branch doesn't exist
            sqlite3_bind_text(statements[0], 1, branch_name, -1, SQLITE_STATIC);
            sqlite3_bind_int64(statements[0], 2, atoi(metered_bus_number), -1, SQLITE_STATIC);
            sqlite3_bind_int64(statements[0], 3, atoi(other_bus_number), -1, SQLITE_STATIC);
            sqlite3_bind_text(statements[0], 4, branch_id, -1, SQLITE_STATIC);
            sqlite3_bind_double(statements[0], 5, atof(voltage_base), -1, SQLITE_STATIC);

            if (season[0] == 's' || season[0] == 'S') {
                sqlite3_bind_double(statements[0], 6, atof(rateA), -1, SQLITE_STATIC);
                sqlite3_bind_double(statements[0], 7, atof(rateB), -1, SQLITE_STATIC);
                sqlite3_bind_double(statements[0], 8, atof(rateC), -1, SQLITE_STATIC);
            }
            else {
                sqlite3_bind_double(statements[0], 9, atof(rateA), -1, SQLITE_STATIC);
                sqlite3_bind_double(statements[0], 10, atof(rateB), -1, SQLITE_STATIC);
                sqlite3_bind_double(statements[0], 11, atof(rateC), -1, SQLITE_STATIC);
            }
            stepStatement(db, statements[0]);
        }

        sqlite3_bind_text(statements[1], 1, scenario, -1, SQLITE_STATIC);
        sqlite3_bind_text(statements[1], 2, contingency, -1, SQLITE_STATIC);
        sqlite3_bind_text(statements[1], 3, branch_name, -1, SQLITE_STATIC);
        sqlite3_bind_int(statements[1], 4, atoi(stat), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 5, atof(p_metered), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 6, atof(p_other), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 7, atof(q_metered), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 8, atof(q_other), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 9, atof(amp_angle_metered), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 10, atof(amp_angle_other), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 11, atof(amp_metered), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 12, atof(amp_other), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 13, atof(ploss), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 14, atof(qloss), -1, SQLITE_STATIC);
        sqlite3_bind_int(statements[1], 15, (strncmp(violate, "Fail", 4) == 0 || atoi(violate) == 1) ? 1 : 0);
        sqlite3_bind_int(statements[1], 16, exception ? atoi(exception) : 0);
        sqlite3_bind_text(statements[1], 17, date, -1, SQLITE_STATIC);

        stepStatement(db, statements[1]);

		for (int i = 0; i < 5; i++) {
			sqlite3_reset(statements[i]);
            sqlite3_clear_bindings(statements[i]);
        }
    }
}
void processT2File(sqlite3* db, FILE* file, sqlite3_stmt* statements[], char* scenario, char* contingency, char* date) {
	char line[1024];
    int rc = 0;
    while (fgets(line, 1024, file) != NULL) {
        size_t line_size = snprintf(NULL, 0, line) + 2;
        char* space = malloc(line_size);
        space[0] = '$';
        space[1] = '\0';
        strcat(space, line);
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

        if (strcmp(xformer_name, "$") == 0) {
            memset(xformer_name, 0, strlen(xformer_name));
            strcat(xformer_name, winding1);
            strcat(xformer_name, ":");
            strcat(xformer_name, winding2);
            strcat(xformer_name, ":");
            strcat(xformer_name, xfmr_id);
        }
        else {
			for (int i = 1; i < strlen(xformer_name); i++) {
                xformer_name[i - 1] = xformer_name[i];
			}
			xformer_name[strlen(xformer_name) - 1] = '\0';
        }

        sqlite3_bind_text(statements[0], 1, xformer_name, -1, SQLITE_STATIC);
        sqlite3_bind_int64(statements[0], 2, atoi(winding1), -1, SQLITE_STATIC);
        sqlite3_bind_int64(statements[0], 3, atoi(winding2), -1, SQLITE_STATIC);
        sqlite3_bind_text(statements[0], 4, xfmr_id, -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[0], 5, atof(mva_base), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[0], 6, atof(winding1_nominal_kv), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[0], 7, atof(winding2_nominal_kv), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[0], 8, atof(rateA_winding1), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[0], 9, atof(rateB_winding1), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[0], 10, atof(rateC_winding1), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[0], 11, atof(rateA_winding2), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[0], 12, atof(rateB_winding2), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[0], 13, atof(rateC_winding2), -1, SQLITE_STATIC);
		stepStatement(db, statements[0]);
        

        sqlite3_bind_text(statements[1], 1, scenario, -1, SQLITE_STATIC);
        sqlite3_bind_text(statements[1], 2, contingency, -1, SQLITE_STATIC);
        sqlite3_bind_text(statements[1], 3, xformer_name, -1, SQLITE_STATIC);
        sqlite3_bind_int(statements[1], 4, atoi(stat), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 5, atof(p_winding1), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 6, atof(p_winding2), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 7, atof(q_winding1), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 8, atof(q_winding2), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 9, atof(amp_winding1), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 10, atof(amp_winding2), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 11, atof(ploss), -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[1], 12, atof(qloss), -1, SQLITE_STATIC);
        sqlite3_bind_int(statements[1], 13, strncmp(violate, "Fail", 4) == 0 || atoi(violate) == 1 ? 1 : 0);
        sqlite3_bind_int(statements[1], 14, exception ? atoi(exception) : 0);
        sqlite3_bind_text(statements[1], 15, date, -1, SQLITE_STATIC);
        stepStatement(db, statements[1]);
 
		for (int y = 0; y < 2; y++) {
			sqlite3_reset(statements[y]);
			sqlite3_clear_bindings(statements[y]);
		}
    }
}
void checkForUpdates(sqlite3* db, FILE* file, sqlite3_stmt* statements[], char* scenario, char* contingency, char* season, char* date, int type) {
    sqlite3_bind_text(statements[0], 1, scenario, -1, SQLITE_STATIC);
    sqlite3_bind_text(statements[0], 2, contingency, -1, SQLITE_STATIC);
    sqlite3_bind_text(statements[0], 3, date, -1, SQLITE_STATIC);
	printf("current file date: %s\n", date);
	int rc = stepStatement(db, statements[0]);
    
	const unsigned char* extracted_date = sqlite3_column_text(statements[0], 0);
    printf("date: %s\n", extracted_date);
    if (extracted_date != NULL && strcmp(extracted_date, date) == 0) {
		printf("No updates\n");
    }
    else {
		printf("updates found or new file found\n");
        sqlite3_stmt* stmt = NULL;
        sqlite3_stmt* stmt2 = NULL;
        sqlite3_stmt* stmt3 = NULL;
        sqlite3_stmt* stmt4 = NULL;
        sqlite3_stmt* stmt5 = NULL;

        char* table_name;
        switch (type) {
            case 0:
				table_name = "Bus";
				break;
			case 1:
				table_name = "Branch";
				break;
            case 2:
				table_name = "Transformer2";
				break;
        }

        size_t needed = snprintf(NULL, 0, "DELETE FROM `%s Simulation Results` WHERE `Scenario Name` = '%s' and `Contingency Name` ='%s';", table_name, scenario, contingency) + 1;
        char* sql_str = malloc(needed);
        snprintf(sql_str, needed, "DELETE FROM `%s Simulation Results` WHERE `Scenario Name` = '%s' and `Contingency Name` ='%s';", table_name, scenario, contingency);
        prepareStatement(db, sql_str, &stmt);
		stepStatement(db, stmt);
		sqlite3_finalize(stmt);
        stmt = NULL;
        
        char* sql_str2;
        switch (type) {
		    case 0: 
                sql_str = "INSERT OR IGNORE INTO BUS (`Bus Number`, `Bus Name`, `Area`, `Zone`, `Owner`, `Voltage Base`, `criteria_nlo`, `criteria_nhi`, `criteria_elo`, `criteria_ehi`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
                prepareStatement(db, sql_str, &stmt);
                sql_str2 = "INSERT OR IGNORE INTO `Bus Simulation Results` (`Scenario Name`, `Contingency Name`, `Bus Number`, `stat`, `bus_pu`, `bus_angle`, `violate`, `exception`, `Date Last Modified`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
                prepareStatement(db, sql_str2, &stmt2);
                sqlite3_stmt* statements1[] = { stmt, stmt2 };
                processBusFile(db, file, statements1, scenario, contingency, date);
                break;
            case 1:
                sql_str = "INSERT OR IGNORE INTO Branch (`Branch Name`, `Metered Bus Number`, `Other Bus Number`, `Branch ID`, `Voltage Base`, `RateA sum`, `RateB sum`, `RateC sum`, `RateA win`, `RateB win`, `RateC win`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
                prepareStatement(db, sql_str, &stmt);
                sql_str2 = "INSERT OR IGNORE INTO `Branch Simulation Results` (`Scenario Name`, `Contingency Name`, `Branch Name`, `stat`, `p_metered`, `p_other`, `q_metered`, `q_other`, `amp_angle_metered`, `amp_angle_other`, `amp_metered`, `amp_other`, `ploss`, `qloss`, `violate`, `exception`, `Date Last Modified`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
                prepareStatement(db, sql_str2, &stmt2);
                char* sql_str3 = "SELECT exists(SELECT * FROM `Branch` WHERE `Branch Name` = ?);";
                prepareStatement(db, sql_str3, &stmt3);
                char* sql_str4 = "UPDATE Branch SET `RateA sum` = ?, `RateB sum` = ?, `RateC sum` = ? WHERE `Branch Name` = ?;";
                prepareStatement(db, sql_str4, &stmt4);
                char* sql_str5 = "UPDATE Branch SET `RateA win` = ?, `RateB win` = ?, `RateC win` = ? WHERE `Branch Name` = ?;";
                prepareStatement(db, sql_str5, &stmt5);
                sqlite3_stmt* statements2[] = { stmt, stmt2, stmt3, stmt4, stmt5 };
                processBranchFile(db, file, statements2, scenario, contingency, season, date);
				sqlite3_finalize(stmt3);
				sqlite3_finalize(stmt4);
				sqlite3_finalize(stmt5);
                break;
			case 2:
                sql_str = "INSERT OR IGNORE INTO Transformer2 (`Xformer Name`, `Winding 1`, `Winding 2`, `Xfmr ID`, `MVA Base`, `Winding 1 nominal KV`, `Winding 2 nominal KV`, `RateA Winding 1`, `RateB Winding 1`, `RateC Winding 1`, `RateA Winding 2`, `RateB Winding 2`, `RateC Winding 2`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
                prepareStatement(db, sql_str, &stmt);
                sql_str2 = "INSERT OR IGNORE INTO `Transformer2 Simulation Results` (`Scenario Name`, `Contingency Name`, `Xformer Name`, `stat`, `p_winding 1`, `p_winding 2`, `q_winding 1`, `q_winding 2`, `amp_winding 1`, `amp_winding 2`, `ploss`, `qloss`, `violate`, `exception`, `Date Last Modified`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
                prepareStatement(db, sql_str2, &stmt2);
                sqlite3_stmt* statements3[] = { stmt, stmt2 };
                processT2File(db, file, statements3, scenario, contingency, date);
                break;
        }

		sqlite3_finalize(stmt);
		sqlite3_finalize(stmt2);
    }

	sqlite3_reset(statements[0]);
	sqlite3_clear_bindings(statements[0]);
}

void processFile(sqlite3* db, char*path, char* filename, sqlite3_stmt* statements[], int type) {
    printf("processing file: %s\n", filename);
    char* contingency = getContingency(filename);
    char* scenario = getScenario(filename);
	char* season = getSeason(filename);
    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s\\%s", path, filename);
	char* date = getDate(filePath);

    FILE* file = fopen(filePath, "r");
    if (file == NULL) {
        printf("Couldn't open file\n");
        sqlite3_close(db);
        exit(-1);
    }
    char line[1024];
    fgets(line, 1024, file); // gets rid of column name
    printf("LINE: %s\n", line);
    if (type == 0) { // 0 is for bus table
        printf("bus table\n");
		processBusFile(db, file, statements, scenario, contingency, date);
    }
	else if (type == 1) { // branch table
        printf("branch table\n");
        processBranchFile(db, file, statements, scenario, contingency, season, date);
    }
	else if (type == 2) { // transformer2 table
		printf("t2 table\n");
		processT2File(db, file, statements, scenario, contingency, date);
	}
    else if (type > 9) { // checks for updated files
		printf("check for updated files\n");
        checkForUpdates(db, file, statements, scenario, contingency, season, date, type % 10);
    }
    fclose(file);
}
void traverseDirectory(sqlite3* db, char*path, sqlite3_stmt* statements[], int type) {
    DIR* d;
    d = opendir(path);
    if (d == NULL) {
        printf("Couldn't open directory\n");
        sqlite3_close(db);
        exit(-1);
    }
    struct dirent* dir;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR) { // recursively traverse through directories
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
                continue;
            char filePath[1024];
            snprintf(filePath, sizeof(filePath), "%s/%s", path, dir->d_name);
            traverseDirectory(db, filePath, statements, type);
        }
        if (strstr(dir->d_name, ".csv") != NULL) { // if it's a csv file then we process
            char* filename = strdup(dir->d_name);
            if (!filename) {
                handle_error(db, "strdup error");
                closedir(d);
                exit(-1);
            }
            processFile(db, path, filename, statements, type);
        }
    }
    closedir(d);
}
void populateScenCont(sqlite3* db, const char* path) {
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
            // parsing the filename to get column data
            char* category = NULL;
            char* contingency;
            contingency = strtok(filename, "@");
            contingency = strtok(NULL, "@");
            contingency = strtok(contingency, "-");
            char* scenario = strdup(filename);
            if (!scenario) {
                free(filename);
                handle_error(db, "strdup error");
            }
            scenario = strtok(scenario, "@");

            char* study = strdup(scenario);
            if (!study) {
                free(filename);
                free(scenario);
                handle_error(db, "strdup error");
            }
            study = strtok(study, "-");
            char* season = strtok(NULL, "-");
            char* topology = strtok(NULL, "");
            char* year = strdup(season);
            if (!filename) {
                free(filename);
                free(scenario);
                free(study);
                handle_error(db, "strdup error");
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

            if (strcmp(contingency, "System") == 0) { // edge case
                contingency = "INTACT";
                category = "basecase";
            }

            if (contingency[0] == 'P') { // if category exists, then it starts with P followed by 1 or 2 digits
                category = strtok(contingency, "#");
                contingency = strtok(NULL, "");
            }

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
                exit(-1);
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
                free(filename);
                free(scenario);
                free(study);
                free(sql_str);
                handle_error(db, "prepare error");
            }

            rc = sqlite3_step(stmt);
            while (rc != SQLITE_DONE) {
                free(filename);
                free(scenario);
                free(study);
                free(sql_str);
                handle_error(db, "statement error");
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
                free(filename);
                free(scenario);
                free(study);
                free(sql_str);
                handle_error(db, "prepare error");
            }
            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                free(filename);
                free(scenario);
                free(study);
                free(sql_str);
                handle_error(db, "statement error");
            }

            sqlite3_finalize(stmt);
            stmt = NULL;
            free(filename);
            free(scenario);
            free(study);
            free(sql_str);

        }
    }
}
void populateBusTables(sqlite3* db, char* path) {
    sqlite3_stmt* stmt = NULL;
    sqlite3_stmt* stmt2 = NULL;

    char* sql_str = "INSERT OR IGNORE INTO BUS (`Bus Number`, `Bus Name`, `Area`, `Zone`, `Owner`, `Voltage Base`, `criteria_nlo`, `criteria_nhi`, `criteria_elo`, `criteria_ehi`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    prepareStatement(db, sql_str, &stmt);
    char* sql_str2 = "INSERT OR IGNORE INTO `Bus Simulation Results` (`Scenario Name`, `Contingency Name`, `Bus Number`, `stat`, `bus_pu`, `bus_angle`, `violate`, `exception`, `Date Last Modified`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    prepareStatement(db, sql_str2, &stmt2);

    sqlite3_stmt* statements[] = { stmt, stmt2 };
	traverseDirectory(db, path, statements, 0);

    for (int i = 0; i < 2; i++) {
        sqlite3_finalize(statements[i]);
    }
}
void populateBranchTables(sqlite3* db, char* path) {
    sqlite3_stmt* stmt = NULL;
    sqlite3_stmt* stmt2 = NULL;
    sqlite3_stmt* stmt3 = NULL;
    sqlite3_stmt* stmt4 = NULL;
    sqlite3_stmt* stmt5 = NULL;

    char* sql_str = "INSERT OR IGNORE INTO Branch (`Branch Name`, `Metered Bus Number`, `Other Bus Number`, `Branch ID`, `Voltage Base`, `RateA sum`, `RateB sum`, `RateC sum`, `RateA win`, `RateB win`, `RateC win`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    prepareStatement(db, sql_str, &stmt);
    char* sql_str2 = "INSERT OR IGNORE INTO `Branch Simulation Results` (`Scenario Name`, `Contingency Name`, `Branch Name`, `stat`, `p_metered`, `p_other`, `q_metered`, `q_other`, `amp_angle_metered`, `amp_angle_other`, `amp_metered`, `amp_other`, `ploss`, `qloss`, `violate`, `exception`, `Date Last Modified`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    prepareStatement(db, sql_str2, &stmt2);
    char* sql_str3 = "SELECT exists(SELECT * FROM `Branch` WHERE `Branch Name` = ?);";
    prepareStatement(db, sql_str3, &stmt3);
    char* sql_str4 = "UPDATE Branch SET `RateA sum` = ?, `RateB sum` = ?, `RateC sum` = ? WHERE `Branch Name` = ?;";
    prepareStatement(db, sql_str4, &stmt4);
    char* sql_str5 = "UPDATE Branch SET `RateA win` = ?, `RateB win` = ?, `RateC win` = ? WHERE `Branch Name` = ?;";
    prepareStatement(db, sql_str5, &stmt5);

	sqlite3_stmt* statements[] = { stmt, stmt2, stmt3, stmt4, stmt5 };
	traverseDirectory(db, path, statements, 1);

	for (int i = 0; i < 5; i++) {
		sqlite3_finalize(statements[i]);
	}
}
void populateTransformer2Tables(sqlite3* db, char* path)   {
    sqlite3_stmt* stmt = NULL;
    sqlite3_stmt* stmt2 = NULL;
    char* sql_str = "INSERT OR IGNORE INTO Transformer2 (`Xformer Name`, `Winding 1`, `Winding 2`, `Xfmr ID`, `MVA Base`, `Winding 1 nominal KV`, `Winding 2 nominal KV`, `RateA Winding 1`, `RateB Winding 1`, `RateC Winding 1`, `RateA Winding 2`, `RateB Winding 2`, `RateC Winding 2`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    prepareStatement(db, sql_str, &stmt);
    char* sql_str2 = "INSERT OR IGNORE INTO `Transformer2 Simulation Results` (`Scenario Name`, `Contingency Name`, `Xformer Name`, `stat`, `p_winding 1`, `p_winding 2`, `q_winding 1`, `q_winding 2`, `amp_winding 1`, `amp_winding 2`, `ploss`, `qloss`, `violate`, `exception`, `Date Last Modified`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    prepareStatement(db, sql_str2, &stmt2);
    sqlite3_stmt* statements[] = { stmt, stmt2 };
    traverseDirectory(db, path, statements, 2);
    for (int i = 0; i < 2; i++) {
        sqlite3_finalize(statements[i]);
    }
}
void repopulateTables(sqlite3* db) {
    const char* queries[] = {
        "DROP TABLE IF EXISTS Scenarios;",
        "CREATE TABLE Scenarios (`Scenario Name` TEXT PRIMARY KEY, `Study` TEXT, Season TEXT, Year INT, Load FLOAT, Topology TEXT);",
        "DROP TABLE IF EXISTS Contingency;",
        "CREATE TABLE Contingency (`Contingency Name` TEXT PRIMARY KEY, `NERC Category` TEXT, `Date Last Modified` TEXT);",
        "DROP TABLE IF EXISTS BUS;",
        "CREATE TABLE BUS (`Bus Number` INT PRIMARY KEY, `Bus Name` TEXT, Area INT, Zone INT, Owner INT, `Voltage Base` FLOAT, criteria_nlo FLOAT, criteria_nhi FLOAT, criteria_elo FLOAT, criteria_ehi FLOAT);",
        "DROP TABLE IF EXISTS `Bus Simulation Results`;",
        "CREATE TABLE `Bus Simulation Results` (`Scenario Name` TEXT, `Contingency Name` TEXT, `Bus Number` INT, stat INT, bus_pu FLOAT NOT NULL, bus_angle FLOAT NOT NULL, violate INT, exception INT, `Date Last Modified` TEXT, PRIMARY KEY (`Scenario Name`, `Contingency Name`, `Bus Number`), FOREIGN KEY (`Scenario Name`) REFERENCES Scenarios (`Scenario Name`) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (`Contingency Name`) REFERENCES Contingency (`Contingency Name`) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (`Bus Number`) REFERENCES BUS (`Bus Number`) ON DELETE CASCADE ON UPDATE CASCADE);",
        "DROP TABLE IF EXISTS Branch;",
        "CREATE TABLE Branch (`Branch Name` TEXT PRIMARY KEY, `Metered Bus Number` INT, `Other Bus Number` INT, `Branch ID` TEXT, `Voltage Base` FLOAT, `RateA sum` FLOAT, `RateB sum` FLOAT, `RateC sum` FLOAT, `RateA win` FLOAT, `RateB win` FLOAT, `RateC win` FLOAT);",
        "DROP TABLE IF EXISTS `Branch Simulation Results`;",
        "CREATE TABLE `Branch Simulation Results` (`Scenario Name` TEXT, `Contingency Name` TEXT, `Branch Name` TEXT, `stat` INT, `p_metered` FLOAT, `p_other` FLOAT, `q_metered` FLOAT, `q_other` FLOAT, `amp_angle_metered` FLOAT, `amp_angle_other` FLOAT, `amp_metered` FLOAT, `amp_other` FLOAT, `ploss` FLOAT, `qloss` FLOAT, `violate` INT, `exception` INT, `Date Last Modified` TEXT, PRIMARY KEY (`Scenario Name`, `Contingency Name`, `Branch Name`), FOREIGN KEY (`Scenario Name`) REFERENCES Scenarios (`Scenario Name`) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (`Contingency Name`) REFERENCES Contingency (`Contingency Name`) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (`Branch Name`) REFERENCES Branch (`Branch Name`) ON DELETE CASCADE ON UPDATE CASCADE);",
        "DROP TABLE IF EXISTS `Transformer2`;",
        "CREATE TABLE Transformer2 (`Xformer Name` TEXT PRIMARY KEY, `Winding 1` INT, `Winding 2` INT, `Xfmr ID` TEXT, `MVA Base` FLOAT, `Winding 1 nominal KV` FLOAT, `Winding 2 nominal KV` FLOAT, `RateA Winding 1` FLOAT, `RateB Winding 1` FLOAT, `RateC Winding 1` FLOAT, `RateA Winding 2` FLOAT, `RateB Winding 2` FLOAT, `RateC Winding 2` FLOAT, `Tap Limit min` FLOAT, `Tap Limit max` FLOAT, `Tap Steps` INT);",
        "DROP TABLE IF EXISTS `Transformer2 Simulation Results`;",
        "CREATE TABLE `Transformer2 Simulation Results` (`Scenario Name` TEXT, `Contingency Name` TEXT, `Xformer Name` TEXT, `stat` INT, `p_winding 1` FLOAT, `p_winding 2` FLOAT, `q_winding 1` FLOAT, `q_winding 2` FLOAT, `amp_winding 1` FLOAT, `amp_winding 2` FLOAT, `Tap 1 Ratio` FLOAT, `Tap 2 Ratio` FLOAT, `Winding 1 Angle` FLOAT, `ploss` FLOAT, `qloss` FLOAT, `violate` INT, `exception` INT, `Date Last Modified` TEXT, PRIMARY KEY (`Scenario Name`, `Contingency Name`, `Xformer Name`), FOREIGN KEY (`Scenario Name`) REFERENCES Scenarios (`Scenario Name`) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (`Contingency Name`) REFERENCES Contingency (`Contingency Name`) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (`Xformer Name`) REFERENCES Transformer2 (`Xformer Name`) ON DELETE CASCADE ON UPDATE CASCADE);"
    };
    sqlite3_stmt* stmt = NULL;
    for (int i = 0; i < 16; i++) {
        stmt = NULL;
        prepareStatement(db, queries[i], &stmt);
		stepStatement(db, stmt);
        sqlite3_finalize(stmt);
    }
    
    populateScenCont(db, ".");
    populateBusTables(db, VOLTAGE_FOLDER);
    populateBranchTables(db, THERMALBRANCH_FOLDER);
    populateTransformer2Tables(db, THERMAL2_FOLDER);
}
void updateTables(sqlite3* db) {
    sqlite3_stmt* stmt = NULL;
	char* sql_str = "SELECT `Date Last Modified` FROM `Bus Simulation Results` WHERE `Scenario Name` = ? and `Contingency Name` = ?;";
	prepareStatement(db, sql_str, &stmt);
    sqlite3_stmt* statements[] = { stmt };
    traverseDirectory(db, VOLTAGE_FOLDER, statements, 10);
	sqlite3_finalize(stmt);
	stmt = NULL;
    printf("DONE CHECKING BUS\n");

    sql_str = "SELECT `Date Last Modified` FROM `Branch Simulation Results` WHERE `Scenario Name` = ? and `Contingency Name` = ?;";
    prepareStatement(db, sql_str, &stmt);
    statements[0] = stmt;
    traverseDirectory(db, THERMALBRANCH_FOLDER, statements, 11);
    sqlite3_finalize(stmt);
    stmt = NULL;
    printf("DONE CHECKING BRANCH \n");

    sql_str = "SELECT `Date Last Modified` FROM `Transformer2 Simulation Results` WHERE `Scenario Name` = ? and `Contingency Name` = ?;";
    prepareStatement(db, sql_str, &stmt);
    statements[0] = stmt;
    traverseDirectory(db, THERMAL2_FOLDER, statements, 12);
    sqlite3_finalize(stmt);
    stmt = NULL;
	printf("DONE CHECKING T2\n");
}
int main(int argc, char* argv[]) {
    char* file = "database2.db";
    sqlite3* db = NULL;
    int rc = 0;
    sqlite3_initialize();
    rc = sqlite3_open_v2(file, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    if (rc != SQLITE_OK) {
        handle_error(db, "Cannot open database");     
    }
    //repopulateTables(db);
    updateTables(db);
	sqlite3_close(db);
	return 0;
}
