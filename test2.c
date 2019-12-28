#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

int callback(void *, int, char **, char **);


int main(void) {
    
    sqlite3 *db;
    char *err_msg = 0;
    
    int rc = sqlite3_open("ceva.db", &db);
    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", 
                sqlite3_errmsg(db));
        sqlite3_close(db);
        
        return 1;
    }
    
    char *sql = "SELECT user_id AS id, nume FROM users";
    char testString[1000] = {0};
        
    rc = sqlite3_exec(db, sql, callback, testString, &err_msg);
    
    //printf("%s\n", HTML_PROFILE("Mihai", "ceva"));

    if (rc != SQLITE_OK ) {
        
        fprintf(stderr, "Failed to select data\n");
        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);
        sqlite3_close(db);
        
        return 1;
    } 
    
    sqlite3_close(db);
    
    return 0;
}

int callback(void *NotUsed, int argc, char **argv, 
                    char **azColName) {
    
    char *test = (char*) NotUsed;
    
    for (int i = 0; i < argc; i++) {

        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    strcat(test, argv[1]);
    
    
    printf("\n");
    
    return 0;
}