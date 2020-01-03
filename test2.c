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
    
    char *sql = "INSERT INTO USERS(user_id ,nume, prenume, id_auth, password, email, token, profile_img, admin_right, cover_url) VALUES (5,'Oare', 'Cineva', 'cineva234', 'cineva213', 'ceva@yahoo.com', '5cineva234', 'img/profile/1.png', 'no', 'https://cdn.pixabay.com/photo/2018/01/12/10/19/fantasy-3077928__340.jpg');SELECT user_id AS id, nume FROM users WHERE id = 3";
    char *sqlPostare = "INSERT INTO POSTARE(user_id, grup_id, posted_date, img_source, description) VALUES (1, 0, '2019-01-01 10:00:00', 'https://cdn.pixabay.com/photo/2018/01/12/10/19/fantasy-3077928__340.jpg', 'Lorem ipsum dolor sit amed, dolorem ceva oricum asta e doar un test')";
    char testString[1000] = {0};

    rc = sqlite3_exec(db, sqlPostare, 0, 0, &err_msg);
    

    if (rc != SQLITE_OK ) {
        
        fprintf(stderr, "Failed to insert data\n");
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

        //printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
        strcat(test, argv[i]);
    }

    
    
    
    //printf("\n");
    
    return 0;
}