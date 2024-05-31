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
	char* contingency = malloc(sizeof(char) * strlen(path) + 1);
	char* pathCopy = strdup(path);
    strtok(pathCopy, "@");
    strtok(NULL, "@");
    contingency = strtok(contingency, "-");
    if (strcmp(contingency, "System") == 0) { // edge case
        contingency = "INTACT";
    }
    if (contingency[0] == 'P') { // if category exists, then it starts with P followed by 1 or 2 digits
        strtok(contingency, "#");
        contingency = strtok(NULL, "");
    }
	free(pathCopy); 
    return contingency;
}
char* getScenario(char* path) {
    char* pathCopy = strdup(path);
    char* scenario = strdup(pathCopy);
    scenario = strtok(scenario, "@");
	return scenario;
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
    const char* queries[] = {
        "DROP TABLE IF EXISTS Scenarios;",
        "CREATE TABLE Scenarios (`Scenario Name` TEXT PRIMARY KEY, `Study` TEXT, Season TEXT, Year INT, Load FLOAT, Topology TEXT);",
        "DROP TABLE IF EXISTS Contingency;",
		"CREATE TABLE Contingency (`Contingency Name` TEXT PRIMARY KEY, `NERC Category` TEXT);",
		"DROP TABLE IF EXISTS BUS;",
        "CREATE TABLE BUS (`Bus Number` INT, `Bus Name` TEXT, Area INT, Zone INT, Owner INT, `Voltage Base` FLOAT, criteria_nlo FLOAT, criteria_nhi FLOAT, criteria_elo FLOAT, criteria_ehi FLOAT);",
		"DROP TABLE IF EXISTS `Bus Simulation Results`;",
        "CREATE TABLE `Bus Simulation Results` (`Scenario Name` TEXT, `Contingency Name` TEXT, `Bus Number` INT, stat INT, bus_pu FLOAT NOT NULL, bus_angle FLOAT NOT NULL, violate INT, exception INT, PRIMARY KEY (`Scenario Name`, `Contingency Name`, `Bus Number`), FOREIGN KEY (`Scenario Name`) REFERENCES Scenarios (`Scenario Name`) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (`Contingency Name`) REFERENCES Contingency (`Contingency Name`) ON DELETE CASCADE ON UPDATE CASCADE, FOREIGN KEY (`Bus Number`) REFERENCES BUS (`Bus Number`) ON DELETE CASCADE ON UPDATE CASCADE);"
    };
    sqlite3_stmt* stmt = NULL;
    for (int i = 0; i < 8; i++) {
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
    // populating scenario and contingency tables with data
	populateScenCont(db, ".");
    printf("count: %d\n", COUNT);

	//populateBusBusSim(db, "./Voltage");
    
	return 0;
}

