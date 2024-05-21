#include "sqlite-amalgamation-3450300/sqlite3.h"
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

int main(int argc, char* argv[]) {
    printf("beginning");
    getchar();
    return 0;
    
    char* file = "database.db"; /* default to temp db */
    sqlite3 *db = NULL;
    int rc = 0;

    sqlite3_initialize();
    /* create a statement from an SQL string */
    sqlite3_stmt* stmt = NULL;
    char sql_str[] = ".mode csv .import C:\\Users\\ading\\Downloads\\Project\\contacts.csv contacts";
    sqlite3_prepare_v2(db, sql_str, strlen(sql_str), &stmt, NULL);


    rc = sqlite3_open_v2(file, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        printf("not ok");
        exit(-1);
    }
    printf("ok");
    /*  perform database operations  */

    sqlite3_close(db);
    sqlite3_shutdown();


    /*_Use_decl_annotations_ int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) */
    
}