/* servTCPConcTh2.c - Exemplu de server TCP concurent care deserveste clientii
   prin crearea unui thread pentru fiecare client.
   Asteapta un numar de la clienti si intoarce clientilor numarul incrementat.
	Intoarce corect identificatorul din program al thread-ului.
  
   
   Autor: Lenuta Alboaie  <adria@infoiasi.ro> (c)2009
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <stdint.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sqlite3.h>
#include <time.h>

/* portul folosit */
#define PORT 8080
#define MAX_LEN 1048000


/* codul de eroare returnat de anumite apeluri */
extern int errno;

int filesOpen = 0;
int requests = 0;

typedef struct thData{
	int idThread; //id-ul thread-ului tinut in evidenta de acest program
	int cl; //descriptorul intors de accept
}thData;

//pthread_mutex_t lock;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void raspunde(void *);

//SQL callbacks
int callbackProfilePage(void *returnString, int argc, char **argv, char **azColName);
int callbackProfilePosts(void *returnString, int argc, char **argv, char **azColName);
int callbackLogin(void *returnString, int argc, char **argv, char **azColName);
int callbackFriend(void *returnString, int argc, char **argv, char **azColName);

int URIdecode (char *str, char *copy) {
        int len = strlen(str), i, j = 0;
        char hex[3] = {0};
 
        for (i = 0; i < len; i++) {
                if (str[i] == '%' && i < len-2) {
                        i++;
                        strncpy(hex, &str[i++], 2);
                        copy[j] = strtol(hex, NULL, 16);
                } else if (str[i] == '+') copy[j] = ' ';
                else copy[j] = str[i];
                j++;
        }
        copy[j] = '\0';
 
        return j;
}

void parse(char* line, char* actualPath)
{
    
    /* Find out where everything is */
    const char *start_of_path = strchr(line, '/') + 1;
    const char *start_of_query = strchr(start_of_path, ' ');
   

    

    /* Get the right amount of memory */
    char path[start_of_query - start_of_path];

    /* Copy the strings into our memory */
    strncpy(path, start_of_path,  start_of_query - start_of_path);

    /* Null terminators (because strncpy does not provide them) */
    path[sizeof(path)] = 0; 
    strcpy(actualPath, path);
}

char* personalizedFeedPageMaker(char *cookie){
        
        char file_buff[MAX_LEN] = {0};
        char head[5000] = {0};
        char posts[MAX_LEN] = {0};
        char bottom [5000] = {0};
        char responce[MAX_LEN] = {0};
        char *resp;


        sqlite3 *db;
        char *err_msg = 0;
        
        int rc = sqlite3_open("ceva.db", &db);
        
        if (rc != SQLITE_OK) {
            
            fprintf(stderr, "Cannot open database: %s\n", 
                    sqlite3_errmsg(db));
            sqlite3_close(db);
            
            pthread_exit("O crepat la baza de date");
        }

    if(cookie == NULL){
        printf("Feed for non existend user\n");
        strcat(head, "<!DOCTYPE html> <html> <head> <meta content=\"text/html;charset=utf-8\" http-equiv=\"Content-Type\"> <meta content=\"utf-8\" http-equiv=\"encoding\"> <title>SocialBear</title> <link rel=\"stylesheet\" href=\"css/reset.css\" /> <link rel=\"stylesheet\" href=\"css/home.css\" /> <link rel=\"stylesheet\" href=\"css/post.css\" /> <link rel=\"stylesheet\" href=\"css/widget.css\" /> <link rel=\"stylesheet\" href=\"css/menu.css\" /> <link rel=\"stylesheet\" href=\"css/chat.css\" /> </head> <body> <header> <img src=\"img/header/menu-button.png\" class=\"menu_img\"/> <a href=\"/\"> <img src=\"img/header/SocialBear.png\" class=\"logo\"/> </a> <input type=\"search\" placeholder=\"Search\" /> <img src=\"img/header/user-shape.png\" id=\"profileBubble\" class=\"nav\"/> <img src=\"img/header/conversation-speech-bubbles-.png\" id=\"chatBubble\" class=\"nav\"/> <img src=\"img/header/musica-searcher.png\" class=\"nav_s\"/> </header> <div class=\"menu\"> <a href = \"/login\"> <div class=\"menu_element\"> <img src=\"img/header/ellipsis.png\" class=\"element_image\" /> <h2>Login</h2> </div> </a> </div> <div class=\"feed\"> <div class=\"posts\"> <br/><br/>");
        
        char sqlQuerryForPosts[1000] = {0};
        strcat(sqlQuerryForPosts, "select nume, prenume, grup_id, profile_img, posted_date, img_source, description, u.token from users u natural join postare p where p.grup_id = 0 ORDER BY p.posted_date DESC LIMIT 100");
        
        char returnedPosts[MAX_LEN] = {0};
        rc = sqlite3_exec(db, sqlQuerryForPosts, callbackProfilePosts, returnedPosts, &err_msg); 

        

        if (rc != SQLITE_OK ) {
            
            fprintf(stderr, "Failed to select data\n");
            fprintf(stderr, "SQL error: %s\n", err_msg);

            sqlite3_free(err_msg);
            sqlite3_close(db);
            
            pthread_exit("O crepat la baza de date");
        }
        sqlite3_close(db);

        char* pch2 = NULL;
        char postsReturned[105][2050] = {0};
        pch2 = strtok(returnedPosts, "~");
        int i = 0;
        while (pch2 != NULL){
            strcat(postsReturned[i], pch2);         
            pch2 = strtok(NULL, "~");
            ++i;
        }

        for(int j = 0; j < i; j++){
            char singularPost[2050] = {0};
            char* postTab = NULL;
            char tokensForPost[8][1025] = {0};
            postTab = strtok(postsReturned[j], "|");
            int k = 0;
            while (postTab != NULL){
                strcat(tokensForPost[k], postTab);         
                postTab = strtok(NULL, "|");
                k++;
            }
           
            sprintf(singularPost, " <div class=\"post\"> <img class=\"profile_pic\" src=\"%s\" /> <a href=\"/profiles/%s\" >%s %s</a> <font>%s</font> <hr/>%s<img src=\"%s\" class=\"post_image\" /> <hr/> </div>",
            tokensForPost[3], tokensForPost[7], tokensForPost[1], tokensForPost[0], tokensForPost[4], tokensForPost[6], tokensForPost[5]);
            strcat(posts, singularPost);

            singularPost[0] = '\0';
        }
        
        strcat(bottom, "</div> </div> <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js\"> </script> <script> $(window).on('load', function() { $(\"img\").each(function(){ var image = $(this); if(this.naturalWidth == 0 || image.readyState == 'uninitialized'){ $(image).unbind(\"error\").hide(); } }); }); </script> </body> </html>");
        
        sprintf(file_buff, "%s%s%s", head,posts,bottom);
        
        long resp_size = strlen(file_buff);

        sprintf(responce, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n%s", resp_size, file_buff);

        resp = responce;
        return resp;


    }else{

        //Feed for existing user

        sprintf(head, "<!DOCTYPE html> <html> <head> <meta content=\"text/html;charset=utf-8\" http-equiv=\"Content-Type\"> <meta content=\"utf-8\" http-equiv=\"encoding\"> <title>SocialBear</title> <link rel=\"stylesheet\" href=\"css/reset.css\" /> <link rel=\"stylesheet\" href=\"css/profile.css\" /> <link rel=\"stylesheet\" href=\"css/home.css\" /> <link rel=\"stylesheet\" href=\"css/post.css\" /> <link rel=\"stylesheet\" href=\"css/widget.css\" /> <link rel=\"stylesheet\" href=\"css/menu.css\" /> <link rel=\"stylesheet\" href=\"css/chat.css\" /> </head> <body> <header> <img src=\"img/header/menu-button.png\" class=\"menu_img\"/> <a href=\"/\"> <img src=\"img/header/SocialBear.png\" class=\"logo\"/> </a> <input type=\"search\" placeholder=\"Search\" /> <img src=\"img/header/user-shape.png\" id=\"profileBubble\" class=\"nav\"/> <img src=\"img/header/conversation-speech-bubbles-.png\" id=\"chatBubble\" class=\"nav\"/> <img src=\"img/header/musica-searcher.png\" class=\"nav_s\"/> </header> <div class=\"menu\"> <a href = \"/profiles/%s\"> <div class=\"menu_element\"> <img src=\"img/header/history-clock-button.png\" class=\"element_image\" /> <h2>My Profile</h2> </div> </a> <div class=\"menu_element\"> <img src=\"img/header/settings-cogwheel-button.png\" class=\"element_image\" /> <h2>Edit Profile</h2> </div> <div id=\"logoutButton\" class=\"menu_element\"> <img src=\"img/header/ellipsis.png\" class=\"element_image\" /> <h2>Logout</h2> </div> </div> <div class=\"feed\"> <div class=\"posts\"> <br/><br/> <div class=\"post post_form\" style=\"padding:0;\"> <div id=\"description\" contenteditable=\"true\">Description</div> <div id=\"imageURL\" contenteditable=\"true\">Image URL</div> <div contenteditable=\"false\"> <input type=\"checkbox\" value=\"1\" name=\"r1\" id=\"r1\" checked=\"checked\"/> <label class=\"whatever\" for=\"r1\">Private</label> </div> <button id=\"post_button\" class=\"post_form_submit\"></button> </div>", cookie);
        
        char sqlQuerryForPosts[1000] = {0};
        sprintf(sqlQuerryForPosts, "select nume, prenume, grup_id, profile_img, posted_date, img_source, description, u.token from users u natural join postare p where p.user_id = %c or (EXISTS (select * from prieteni f where f.id_friend = u.user_id and f.id_user = %c) and p.grup_id = 1) or (p.posted_date > datetime('now','-2 days') and p.grup_id = 0 and p.user_id <> %c) order by p.posted_date desc LIMIT 100;", cookie[0], cookie[0], cookie[0]);
        
        char returnedPosts[MAX_LEN] = {0};
        rc = sqlite3_exec(db, sqlQuerryForPosts, callbackProfilePosts, returnedPosts, &err_msg);

        

        if (rc != SQLITE_OK ) {
            
            fprintf(stderr, "Failed to select data\n");
            fprintf(stderr, "SQL error: %s\n", err_msg);

            sqlite3_free(err_msg);
            sqlite3_close(db);
            
            //TODO: ERROR MESSAGE id db not working
        }
        

        char* pch2 = NULL;
        char postsReturned[105][2050] = {0};
        pch2 = strtok(returnedPosts, "~");
        int i = 0;
        while (pch2 != NULL){
            strcat(postsReturned[i], pch2);         
            pch2 = strtok(NULL, "~");
            ++i;
        }

        for(int j = 0; j < i; j++){
            char singularPost[2050] = {0};
            char* postTab = NULL;
            char tokensForPost[8][1025] = {0};
            postTab = strtok(postsReturned[j], "|");
            int k = 0;
            while (postTab != NULL){
                strcat(tokensForPost[k], postTab);         
                postTab = strtok(NULL, "|");
                k++;
            }
           
            sprintf(singularPost, " <div class=\"post\"> <img class=\"profile_pic\" src=\"%s\" /> <a href=\"/profiles/%s\" >%s %s</a> <font>%s</font> <hr/>%s<img src=\"%s\" class=\"post_image\" /> <hr/> </div>",
            tokensForPost[3], tokensForPost[7], tokensForPost[1], tokensForPost[0], tokensForPost[4], tokensForPost[6], tokensForPost[5]);
            strcat(posts, singularPost);

            singularPost[0] = '\0';
        }

        
        if((100 - i) > 0){
            char sqlQuerryForPosts2[1000] = {0};
            sprintf(sqlQuerryForPosts2, "select nume, prenume, grup_id, profile_img, posted_date, img_source, description, u.token from users u natural join postare p where p.grup_id = 0 and p.posted_date < datetime('now','-2 days') AND p.user_id <> %c order by p.posted_date desc LIMIT 100-%d;", cookie[0], i);

            char returnedPosts2[MAX_LEN] = {0};
            rc = sqlite3_exec(db, sqlQuerryForPosts2, callbackProfilePosts, returnedPosts2, &err_msg);

            char* pch3 = NULL;
            char postsReturned2[105][2050] = {0};
            pch3 = strtok(returnedPosts2, "~");
            int ii = 0;
            while (pch3 != NULL){
                strcat(postsReturned2[ii], pch3);         
                pch3 = strtok(NULL, "~");
                ++ii;
            }

            for(int j = 0; j < ii; j++){
                char singularPost[2050] = {0};
                char* postTab = NULL;
                char tokensForPost[8][1025] = {0};
                postTab = strtok(postsReturned2[j], "|");
                int k = 0;
                while (postTab != NULL){
                    strcat(tokensForPost[k], postTab);         
                    postTab = strtok(NULL, "|");
                    k++;
                }
            
                sprintf(singularPost, " <div class=\"post\"> <img class=\"profile_pic\" src=\"%s\" /> <a href=\"/profiles/%s\" >%s %s</a> <font>%s</font> <hr/>%s<img src=\"%s\" class=\"post_image\" /> <hr/> </div>",
                tokensForPost[3], tokensForPost[7], tokensForPost[1], tokensForPost[0], tokensForPost[4], tokensForPost[6], tokensForPost[5]);
                strcat(posts, singularPost);

                singularPost[0] = '\0';
            }
        }
        
        strcat(bottom, "</div> </div> <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js\"></script> <script> $(window).on('load', function() { $(\"img\").each(function(){ var image = $(this); if(this.naturalWidth == 0 || image.readyState == 'uninitialized'){ $(image).unbind(\"error\").hide(); } }); }); $(document).ready(function() { $('#description').click(function(e){ if($('#description').text() == \"Description\") $('#description').text(\"\"); }); $('#imageURL').click(function(e){ if($('#imageURL').text() == \"Image URL\") $('#imageURL').text(\"\"); }); }); $(\"#logoutButton\").click(function(e){ document.cookie = \"token= ; expires = Thu, 01 Jan 1970 00:00:00 GMT\"; window.location.href = \"/\"; }); $(document).ready(function() { $('#post_button').click(function(e) { e.preventDefault(); if($('#description').text() == \"\" || $('#description').text() == \"Description\"){ alert(\"Description can not be null\"); }else if(!$('#imageURL').text().includes(\"http://\") && !$('#imageURL').text().includes(\"https://\") && !($('#imageURL').text() == \"Image URL\") && !($('#imageURL').text() == \"\")){ alert(\"Invalid image URL\"); }else{ var imgURL; if($('#imageURL').text() == \"Image URL\" || $('#imageURL').text() == \"\"){ imgURL = \"https://cdn.pixabay.com/photo/2018/01/1/23/12/nature-3082832__340.jpg\"; }else{ imgURL = $('#imageURL').text(); } $.ajax({ type: 'POST', dataType: \"text\", url: '/postare', data: {description: $('#description').text(), imageUrl: imgURL, private: $('#r1').prop('checked') ? '1' : '0'}, success: function(data) { if(data == \"success\"){ window.location.href = \"/\"; }else{ alert(\"There was an error while posting your post\"); } } }); } }); }); </script> </body> </html>");
        
        sprintf(file_buff, "%s%s%s", head,posts,bottom);
        
        long resp_size = strlen(file_buff);

        sprintf(responce, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n%s", resp_size, file_buff);

        resp = responce;

        sqlite3_close(db);
        return resp;
    }
    
}

//PROFILE PAGE TAG
char* personalizedProfilePageMaker(char *profile, char *cookie){
        char file_buff[MAX_LEN] = {0};
        char head[5000] = {0};
        char posts[MAX_LEN] = {0};
        char bottom [15000] = {0};
        char responce[MAX_LEN] = {0};
        char *resp;

        sqlite3 *db;
        char *err_msg = 0;
        
        int rc = sqlite3_open("ceva.db", &db);
        
        if (rc != SQLITE_OK) {
            
            fprintf(stderr, "Cannot open database: %s\n", 
                    sqlite3_errmsg(db));
            sqlite3_close(db);
            
            pthread_exit("O crepat la baza de date");
        }

        
        
        char sqlQuerry[1000] = {0};
        sprintf(sqlQuerry, "SELECT nume, prenume, profile_img, cover_url FROM users WHERE token = '%s'", profile);

        //printf("Nu crapa pana aici \n");

        char returnedString[100] = {0};
        rc = sqlite3_exec(db, sqlQuerry, callbackProfilePage, returnedString, &err_msg);

       

        char* pch = NULL;
        char tokens[4][128] = {0};
        pch = strtok(returnedString, "|");
        int i = 0;
        while (pch != NULL){
            strcat(tokens[i], pch);         
            pch = strtok(NULL, "|");
            ++i;
        }
        if (rc != SQLITE_OK ) {
            
            fprintf(stderr, "Failed to select data\n");
            fprintf(stderr, "SQL error: %s\n", err_msg);

            sqlite3_free(err_msg);
            sqlite3_close(db);
            
            pthread_exit("O crepat la baza de date");
        }

        
        char sqlQuerryForPosts[2000] = {0};
        if(cookie == NULL){
            sprintf(sqlQuerryForPosts, "select nume, prenume, grup_id, profile_img, posted_date, img_source, description from users u natural join postare p where u.token = '%s' AND p.grup_id = 0 ORDER BY p.posted_date DESC LIMIT 100", profile);
        }else{
            sprintf(sqlQuerryForPosts, "select nume, prenume, grup_id, profile_img, posted_date, img_source, description from users u natural join postare p where u.token = '%s' AND p.grup_id = 0 UNION select nume, prenume, grup_id, profile_img, posted_date, img_source, description from users u natural join postare p where u.token = '%s' AND p.grup_id = 1 AND (EXISTS(SELECT * FROM prieteni WHERE (id_user = %c and id_friend = %c)) OR p.user_id = %c) ORDER BY p.posted_date DESC LIMIT 100", profile, profile, cookie[0], profile[0], cookie[0]);

        }
        char returnedPosts[MAX_LEN] = {0};
        rc = sqlite3_exec(db, sqlQuerryForPosts, callbackProfilePosts, returnedPosts, &err_msg); 

        

        if (rc != SQLITE_OK ) {
            
            fprintf(stderr, "Failed to select data\n");
            fprintf(stderr, "SQL error: %s\n", err_msg);

            sqlite3_free(err_msg);
            sqlite3_close(db);
            
            pthread_exit("O crepat la baza de date");
        }
        

        char* pch2 = NULL;
        char postsReturned[105][2050] = {0};
        pch2 = strtok(returnedPosts, "~");
        i = 0;
        while (pch2 != NULL){
            strcat(postsReturned[i], pch2);         
            pch2 = strtok(NULL, "~");
            ++i;
        }
        
        for(int j = 0; j < i; j++){
            char singularPost[2050] = {0};
            char* postTab = NULL;
            char tokensForPost[7][1025] = {0};
            postTab = strtok(postsReturned[j], "|");
            int k = 0;
            while (postTab != NULL){
                strcat(tokensForPost[k], postTab);         
                postTab = strtok(NULL, "|");
                k++;
            }
           
            sprintf(singularPost, " <div class=\"post\"> <img class=\"profile_pic\" src=\"%s\" /> <a href=\"#\" >%s %s</a> <font>%s</font> <hr/>%s<img src=\"%s\" class=\"post_image\" /> <hr/> </div>",
            tokensForPost[3], tokensForPost[1], tokensForPost[0], tokensForPost[4], tokensForPost[6], tokensForPost[5]);
            strcat(posts, singularPost);

            singularPost[0] = '\0';
        }
        
        if(cookie != NULL)
            sprintf(head, "<!DOCTYPE html> <html> <head> <meta content=\"text/html;charset=utf-8\" http-equiv=\"Content-Type\"> <meta content=\"utf-8\" http-equiv=\"encoding\"> <title>%s's profile</title> <meta id=\"meta\" name=\"viewport\" content=\"width=device-width; initial-scale=1.0\" /> <meta id=\"meta\" name=\"viewport\" content=\"width=device-width; initial-scale=1.0\" /> <link rel=\"stylesheet\" href=\"css/reset.css\" /> <link rel=\"stylesheet\" href=\"css/home.css\" /> <link rel=\"stylesheet\" href=\"css/profile.css\" /> <link rel=\"stylesheet\" href=\"css/post.css\" /> <link rel=\"stylesheet\" href=\"css/widget.css\" /> <link rel=\"stylesheet\" href=\"css/menu.css\" /> <link rel=\"stylesheet\" href=\"css/chat.css\" /> </head> <body> <header> <img src=\"img/header/menu-button.png\" class=\"menu_img\"/> <a href=\"/\"> <img src=\"img/header/SocialBear.png\" class=\"logo\"/> </a> <input type=\"search\" placeholder=\"Search\" /> <img src=\"img/header/conversation-speech-bubbles-.png\" class=\"nav\"/> </header> <div class=\"menu\"> <a href = \"/profiles/%s\"> <div class=\"menu_element\"> <img src=\"img/header/history-clock-button.png\" class=\"element_image\" /> <h2>My Profile</h2> </div> </a> <div class=\"menu_element\"> <img src=\"img/header/settings-cogwheel-button.png\" class=\"element_image\" /> <h2>Edit Profile</h2> </div> <div id = \"logoutButton\" class=\"menu_element\"> <img src=\"img/header/ellipsis.png\" class=\"element_image\" /> <h2>Logout</h2> </div> </div> <!--Cover img--> <div class=\"profile\" style=\"background-image:url(%s);\"> <div class=\"sub_profile\"> <center> <!--profile pic--> <img src=\"%s\" class=\"profile_pic\" /><br/> <!--Name--> <h2>%s %s</h2><br/> ", tokens[0], cookie, tokens[3],tokens[2],tokens[1],tokens[0]);
        else{
            sprintf(head, "<!DOCTYPE html> <html> <head> <meta content=\"text/html;charset=utf-8\" http-equiv=\"Content-Type\"> <meta content=\"utf-8\" http-equiv=\"encoding\"> <title>%s's profile</title> <meta id=\"meta\" name=\"viewport\" content=\"width=device-width; initial-scale=1.0\" /> <meta id=\"meta\" name=\"viewport\" content=\"width=device-width; initial-scale=1.0\" /> <link rel=\"stylesheet\" href=\"css/reset.css\" /> <link rel=\"stylesheet\" href=\"css/home.css\" /> <link rel=\"stylesheet\" href=\"css/profile.css\" /> <link rel=\"stylesheet\" href=\"css/post.css\" /> <link rel=\"stylesheet\" href=\"css/widget.css\" /> <link rel=\"stylesheet\" href=\"css/menu.css\" /> <link rel=\"stylesheet\" href=\"css/chat.css\" /> </head> <body> <header> <img src=\"img/header/menu-button.png\" class=\"menu_img\"/> <a href=\"/\"> <img src=\"img/header/SocialBear.png\" class=\"logo\"/> </a> <input type=\"search\" placeholder=\"Search\" /> <img src=\"img/header/conversation-speech-bubbles-.png\" class=\"nav\"/> </header> <div class=\"menu\"> <a href = \"/login\"> <div class=\"menu_element\"> <img src=\"img/header/ellipsis.png\" class=\"element_image\" /> <h2>Login</h2> </div> </a> </div> <!--Cover img--> <div class=\"profile\" style=\"background-image:url(%s);\"> <div class=\"sub_profile\"> <center> <!--profile pic--> <img src=\"%s\" class=\"profile_pic\" /><br/> <!--Name--> <h2>%s %s</h2><br/> ", tokens[0],tokens[3],tokens[2],tokens[1],tokens[0]);
        }

        if(cookie != NULL && cookie[0] != profile[0]){
            char sqlFriend[100] = {0};
            char sqlRespFriend[100] = {0};
            sprintf(sqlFriend, "SELECT * FROM prieteni where id_user = %c and id_friend = %c;", cookie[0], profile[0]);

            rc = sqlite3_exec(db, sqlFriend, callbackFriend, sqlRespFriend, &err_msg);
            if(strlen(sqlRespFriend) > 0){
                strcat(head, "<button id = \"followButton\" class=\"btn_follow\">Unfollow</button> <button>Message</button>");
            }else{
                strcat(head, "<button id = \"followButton\" class=\"btn_follow\">Follow</button> <button>Message</button>");
            }
        }
        strcat(head, "</center> </div> </div> <div class=\"feed\"> <div class=\"posts\">");
        

        if(cookie != NULL && cookie[0] == profile[0]){
            strcat(head, "<div class=\"post post_form\" style=\"padding:0;\"> <div id=\"description\" contenteditable=\"true\">Description</div> <div id=\"imageURL\" contenteditable=\"true\">Image URL</div> <div contenteditable=\"false\"> <input type=\"checkbox\" value=\"1\" name=\"r1\" id=\"r1\" checked=\"checked\"/> <label class=\"whatever\" for=\"r1\">Private</label> </div> <button id=\"post_button\" class=\"post_form_submit\"></button> </div>");
        }
       
        //sprintf(posts)

        strcat(bottom, "</div> </div> <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js\"></script>");
        
        if(cookie != NULL && cookie[0] == profile[0]){
            strcat(bottom, "<script> $(window).on('load', function() { $(\"img\").each(function(){ var image = $(this); if(this.naturalWidth == 0 || image.readyState == 'uninitialized'){ $(image).unbind(\"error\").hide(); } }); }); $(\"#logoutButton\").click(function(e){console.log(document.cookie); document.cookie = \"token= ; expires = Thu, 01 Jan 1970 00:00:00 GMT; path=/\"; location.reload();}); $(document).ready(function() { $('#description').click(function(e){ if($('#description').text() == \"Description\") $('#description').text(\"\"); }); $('#imageURL').click(function(e){ if($('#imageURL').text() == \"Image URL\") $('#imageURL').text(\"\"); }); }); $(document).ready(function() { $('#post_button').click(function(e) { e.preventDefault(); if($('#description').text() == \"\" || $('#description').text() == \"Description\"){ alert(\"Description can not be null\"); }else if(!$('#imageURL').text().includes(\"http://\") && !$('#imageURL').text().includes(\"https://\") && !($('#imageURL').text() == \"Image URL\") && !($('#imageURL').text() == \"\")){ alert(\"Invalid image URL\"); }else{ var imgURL; if($('#imageURL').text() == \"Image URL\" || $('#imageURL').text() == \"\"){ imgURL = \"https://cdn.pixabay.com/photo/2018/01/1/23/12/nature-3082832__340.jpg\"; }else{ imgURL = $('#imageURL').text(); } $.ajax({ type: 'POST', dataType: \"text\", url: '/postare', data: {description: $('#description').text(), imageUrl: imgURL, private: $('#r1').prop('checked') ? '1' : '0'}, success: function(data) { if(data == \"success\"){ window.location.href = \"/\"; }else{ alert(\"There was an error while posting your post\"); } } }); } }); }); </script>");
        }else if(cookie != NULL){
            strcat(bottom, "<script> $(window).on('load', function() { $(\"img\").each(function(){ var image = $(this); if(this.naturalWidth == 0 || image.readyState == 'uninitialized'){ $(image).unbind(\"error\").hide(); } }); }); $(\"#logoutButton\").click(function(e){console.log(document.cookie); document.cookie = \"token= ; expires = Thu, 01 Jan 1970 00:00:00 GMT; path=/\"; location.reload();}); $(document).ready(function() { $('#followButton').click(function(e) { e.preventDefault(); $.ajax({ type: 'POST', dataType: \"text\", url: '/prieteni', data: {userId: location.href.substr(location.href.lastIndexOf('/') + 1)}, success: function(data) { if(data == \"success\"){ location.reload(); }else{ alert(\"Could not establish friendship\"); } } }); }); });</script>");
        }else{
            strcat(bottom, "<script> $(window).on('load', function() { $(\"img\").each(function(){ var image = $(this); if(this.naturalWidth == 0 || image.readyState == 'uninitialized'){ $(image).unbind(\"error\").hide(); } }); }); </script>");
        }


        strcat(bottom, "</body> </html>");
        
        sprintf(file_buff, "%s%s%s", head,posts,bottom);
        
        long resp_size = strlen(file_buff);

        sprintf(responce, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n%s", resp_size, file_buff);
        
        resp = responce;
        sqlite3_close(db);
        return resp;
}

int callbackProfilePage(void *returnString, int argc, char **argv, char **azColName) {
    
    char *stringToBeReturned = (char*) returnString;

    for (int i = 0; i < argc; i++) {
            strcat(stringToBeReturned, argv[i]);
            strcat(stringToBeReturned, "|");
    }
    stringToBeReturned[strlen(stringToBeReturned)-1] = '\0';
    

    return 0;
}

int callbackProfilePosts(void *returnString, int argc, char **argv, char **azColName) {
    
    char *stringToBeReturned = (char*) returnString;

    for (int i = 0; i < argc; i++) {
            strcat(stringToBeReturned, argv[i]);
            strcat(stringToBeReturned, "|");
    }
    stringToBeReturned[strlen(stringToBeReturned)-1] = '~';
    

    return 0;
}

int callbackLogin(void *returnString, int argc, char **argv, char **azColName) {
    
    char *stringToBeReturned = (char*) returnString;

    for (int i = 0; i < argc; i++) {
        //printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
            if(strcmp(azColName[i], "token") == 0){
                strcat(stringToBeReturned, argv[i]);
            }
    }
    

    return 0;
}

int callbackFriend(void *returnString, int argc, char **argv, char **azColName) {
    
    char *stringToBeReturned = (char*) returnString;

    for (int i = 0; i < argc; i++) {
            if(strcmp(azColName[i], "id_friend") == 0){
                strcat(stringToBeReturned, argv[i]);
            }
    }
    

    return 0;
}

char *find_content_type (char *filename) {
    char *p;  // pointer to the type found
    int i;
    char buf1[500]; // used to store the extension of the file
    char buf2[500];
    buf1[0] = '\0';
    buf2[0] = '\0'; 
    p = (char *)malloc(30);
    strcpy(buf1, filename);
    //printf("name of file requested: %s \n", buf1);

    /* find the extension: */
    for (i = 0; i<strlen(buf1); i++) {
        if ( buf1[i] == '.' ) {
            strcpy(buf2, &buf1[i]);
        }
    }
    /* find the type: */
    if ( strcmp(buf2, ".html") == 0 || strcmp (buf2, ".hml") == 0) {
        strcpy (buf2, "Content-Type: text/html \r\n");
    }

    else if ( strcmp(buf2, ".txt") == 0) {
        strcpy (buf2, "Content-Type: text/plain \r\n");
    }

    else if ( strcmp(buf2, ".jpg") == 0 || strcmp (buf2, ".jpeg") == 0) {
        strcpy (buf2, "Content-Type: image/jpeg \r\n");
    }

    else if ( strcmp(buf2, ".ico") == 0 || strcmp (buf2, ".jpeg") == 0) {
        strcpy (buf2, "Content-Type: image/webp \r\n");
    }

    else if ( strcmp(buf2, ".png") == 0) {
        strcpy (buf2, "Content-Type: image/png \r\n");
    }

    else if ( strcmp(buf2, ".gif") == 0) {
        strcpy (buf2, "Content-Type: image/gif \r\n");
    }
    else if ( strcmp(buf2, ".css") == 0) {
        strcpy (buf2, "Content-Type: text/css \r\n");
    }
    else if ( strcmp(buf2, ".js") == 0) {
        strcpy (buf2, "Content-Type: text/javascript \r\n");
    }
    else if ( strcmp(buf2, ".php") == 0) {
        strcpy (buf2, "Content-Type: text/x-php \r\n");
    }
    else {
        strcpy (buf2, "Content-Type: application/octet-stream \r\n");
    }

     p = buf2;
    //printf ("content-type: %s\n", p);
    //return "Content-type: image/jpeg\r\n";
    return p;
}

void response_generator (int conn_fd, char *filename) {

    /* vars needed for finding the length of the file */
    //struct stat filestat;
    
    char header_buff [2048];
    char file_buff [MAX_LEN];
    char filesize[20];

    header_buff[0] = '\0';
    file_buff[0] = '\0';
    filesize[0] = '\0';

     

    if (filename == NULL) {
        strcpy (header_buff, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nContent-Type: text/html\r\n");
    }

    //pthread_mutex_lock(&lock);
    FILE *fp;
    fp = fopen (filename, "r");
    //printf("Open File: %d\n", fp);
    fseek(fp, 0L, SEEK_END); 
  
    // calculating the size of the file 
    long int res = ftell(fp);
    sprintf (filesize, "%ld", res); // put the file size of buffer, so we can add it to the response header
    rewind(fp);
    filesOpen++;
    if (fp == NULL) {
    //printf("Problema aici: %d (%s) Dupa <%d> fisiere deschise\n", fp, filename, filesOpen);
    printf ("fp is null or filename = 404\n");
        strcpy (header_buff, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nContent-Type: text/plain\r\n\r\n ");       
    }

    else if (fp != NULL) {
        strcpy (header_buff, "HTTP/1.1 200 OK\r\nContent-Length: ");
        /* content-length: */
        strcat (header_buff, filesize);
        strcat (header_buff, "\r\n");
        /* content-type: */
        strcat (header_buff, find_content_type (filename));
        //printf ("%s\n", find_content_type (filename));
    }

    else {
      strcpy (header_buff, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\nContent-Type: text/html\r\n");   
    }        
    strcat (header_buff, "Connection: close\r\n\r\n");
    int writeError;

    
    if((writeError = write (conn_fd, header_buff, strlen(header_buff))) < 0){
        //printf("%s %d -> %d\n\n ", "O Crepat aici la write(1) wa", orderOfTh, threadSocket[orderOfTh]);
        //close(conn_fd);
        conn_fd = 0;
        //threadSocket[orderOfTh] = 0;
        pthread_exit("crepat");
    }
    //printf("%s\n\n", header_buff);

    fread (file_buff, sizeof(char), res + 1, fp);
    
    int y = fclose(fp);
    //printf("Closed File: %d(%d)\n", fp, y);
    //pthread_mutex_unlock(&lock);

    if((writeError = write (conn_fd, file_buff, res)) < 0){
        //printf("%s %d -> %d\n\n ", "O Crepat aici la write(2) wa", conn_fd, threadSocket[orderOfTh]);
        //close(conn_fd);
        //conn_fd = 0;
        //threadSocket[orderOfTh] = 0;
        pthread_exit("crepat");
    }
    
}

void post_response_generator(int conn_fd, char* requestPage, char* requestHead, char* cookiezy){
    
    sqlite3 *db;
    char *err_msg = 0;
    
    int rc = sqlite3_open("ceva.db", &db);
    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", 
                sqlite3_errmsg(db));
        sqlite3_close(db);
        
        pthread_exit("Database Error");
    }
    
    if(strcmp(requestPage, "login") == 0){
        char loginID[128] = {0};
        char decodedLoginID[128] = {0};
        char password[128] = {0};
        char responce[1024] = {0};

       

        char* pch = NULL;
        pch = strtok(requestHead, "&");

        while (pch != NULL)
        {
            if(strstr(pch, "loginId=") != 0){
                strcpy(loginID, pch + 8);
                URIdecode(loginID, decodedLoginID);
            }else if(strstr(pch, "loginPass=") != 0){
                strcpy(password, pch + 10);
            }
            pch = strtok(NULL, "&");
        }
        
        char sql[300] = {0};
        sprintf(sql, "SELECT token FROM users WHERE email='%s' AND password = '%s'", decodedLoginID, password);
        char token[128] = {0};
        
        rc = sqlite3_exec(db, sql, callbackLogin, token, &err_msg);
    

        if (rc != SQLITE_OK ) {
            
            fprintf(stderr, "Failed to select data\n");
            fprintf(stderr, "SQL error: %s\n", err_msg);

            sqlite3_free(err_msg);
            sqlite3_close(db);
            
            pthread_exit("Database Error");
        } 
        
        sqlite3_close(db);

        if(strlen(token) == 0){
            strcpy(responce, "HTTP/1.1 200 OK\r\nContent-Length: 7\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nfailure");
        }else{
            sprintf(responce, "HTTP/1.1 200 OK\r\nContent-Length: 7\r\nContent-Type: text/plain\r\nSet-Cookie: token=%s; Max-Age=86400\r\nConnection: close\r\n\r\nsuccess", token);
        }

        int writeError;

        if((writeError = write (conn_fd, responce, strlen(responce))) < 0){
            pthread_exit("crepat");
        }

        responce[0] = '\0';
        loginID[0] = '\0';
        password[0] = '\0';
        sql[0] = '\0';
        token[0] = '\0';

    }else if(strcmp(requestPage, "register") == 0){

        char registerName[128] = {0};
        char registerPreName[128] = {0};
        char registerEmail[128] = {0};
        char registerID[128] = {0};
        char decodedLoginEmail[128] = {0};
        char password[128] = {0};
        char responce[1024] = {0};

       

        char* pch = NULL;
        pch = strtok(requestHead, "&");

        while (pch != NULL)
        {
            if(strstr(pch, "registePreName=") != 0){
                strcpy(registerPreName, pch + 15);
                //URIdecode(loginID, decodedLoginID);
            }else if(strstr(pch, "registerName=") != 0){
                strcpy(registerName, pch + 13);
            }else if(strstr(pch, "registerEmail=") != 0){
                strcpy(registerEmail, pch + 14);
                URIdecode(registerEmail, decodedLoginEmail);
            }else if(strstr(pch, "registerToken=") != 0){
                strcpy(registerID, pch + 14);
            }else if(strstr(pch, "registerPassword=") != 0){
                strcpy(password, pch + 17);
            }
            pch = strtok(NULL, "&");
        }
        char testString[1000] = {0};
        int i = 0;
        do{
            i++;
            testString[0] = '\0';
            char sql[1000] = {0};
            sprintf(sql, "SELECT token from users where user_id = %d", i);
            rc = sqlite3_exec(db, sql, callbackLogin, testString, &err_msg);
        }while(strlen(testString) != 0);

        char sqlInsert[1000] = {0};
        sprintf(sqlInsert, "INSERT INTO USERS(user_id ,nume, prenume, id_auth, password, email, token, profile_img, admin_right, cover_url) VALUES (%d, '%s', '%s', '%s', '%s', '%s', '%d%s', 'img/profile/1.png', 'no', 'img/cover/Wallpaper-Macbook.jpg')",
                i, registerName, registerPreName, registerID, password, decodedLoginEmail, i, registerID);
        
        rc = sqlite3_exec(db, sqlInsert, 0, 0, &err_msg);
    

        if (rc != SQLITE_OK ) {
            
            if(strstr(err_msg, "UNIQUE") != 0){
                strcpy(responce, "HTTP/1.1 200 OK\r\nContent-Length: 5\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nemail");
            }else{
                sprintf(responce, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n%s", strlen(err_msg), err_msg);
            }

            sqlite3_free(err_msg);
            sqlite3_close(db);
        }else{
            sprintf(responce, "HTTP/1.1 200 OK\r\nContent-Length: 7\r\nContent-Type: text/plain\r\nSet-Cookie: token=%d%s; Max-Age=86400\r\nConnection: close\r\n\r\nsuccess", i, registerID);
        }
        
        int writeError;
        if((writeError = write (conn_fd, responce, strlen(responce))) < 0){
            pthread_exit("crepat");
        }

        sqlite3_close(db);
        registerName[0] = '\0';
        registerPreName[0] = '\0';
        registerEmail[0] = '\0';
        registerID[0] = '\0';
        decodedLoginEmail[0] = '\0';
        password[0] = '\0';
        responce[0] = '\0';
    }else if(strcmp(requestPage, "postare") == 0){
        
        char description[1024] = {0};
        char decodedDescription[1024] = {0};
        char imgURL[1024] ={0};
        char decodedImgURL[1024] = {0};
        char private[3] = {0};
        char responce[MAX_LEN] = {0};

       // printf("\n\n req head: %s\n", requestHead);

        char* pch = NULL;
        pch = strtok(requestHead, "&");
       
        while (pch != NULL)
        {
            if(strstr(pch, "description=") != 0){
                strcpy(description, pch + 12);
                URIdecode(description, decodedDescription);
            }else if(strstr(pch, "imageUrl=") != 0){
                strcpy(imgURL, pch + 9);
                URIdecode(imgURL, decodedImgURL);
            }else if(strstr(pch, "private=") != 0){
                strcpy(private, pch + 8);
            }
            pch = strtok(NULL, "&");
        }
        
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
       

        char sqlInsert[1000] = {0};
        sprintf(sqlInsert, "INSERT INTO POSTARE(user_id, grup_id, posted_date, img_source, description) VALUES (%c, %c, '%d-%02d-%02d %02d:%02d:%02d', '%s', '%s')",
        cookiezy[0], private[0], tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, decodedImgURL, decodedDescription);

        rc = sqlite3_exec(db, sqlInsert, 0, 0, &err_msg);
    

        if (rc != SQLITE_OK ) {
            strcpy(responce, "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nfail");
            sqlite3_free(err_msg);
            sqlite3_close(db);
        }else{
            strcat(responce, "HTTP/1.1 200 OK\r\nContent-Length: 7\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nsuccess");
        }

        int writeError;
        if((writeError = write (conn_fd, responce, strlen(responce))) < 0){
            pthread_exit("crepat");
        }

        sqlite3_close(db);
        description[0] = '\0';
        decodedDescription[0] = '\0';
        imgURL[0] = '\0';
        decodedImgURL[0] = '\0';
        private[0] = '\0';
        responce[0] = '\0';
    }
    else if(strcmp(requestPage, "prieteni") == 0){
        
        char sqlInsert[1000] = {0};
        char userId[128] = {0};
        char responce[1024] = {0};


        char* pch = NULL;
        pch = strtok(requestHead, "&");

        while (pch != NULL)
        {
            if(strstr(pch, "userId=") != 0){
                strcpy(userId, pch + 7);
            }
            pch = strtok(NULL, "&");
        }

        printf("%c\n", userId[0]);

        sprintf(sqlInsert, "INSERT INTO prieteni(id_user, id_friend) values (%c, %c);", cookiezy[0], userId[0]);
        rc = sqlite3_exec(db, sqlInsert, 0, 0, &err_msg);
        

        if (rc != SQLITE_OK ) {
            if(strstr(err_msg, "UNIQUE")){
                char sqlDelete[1000] = {0};
                sprintf(sqlDelete, "DELETE FROM prieteni WHERE id_user=%c AND id_friend=%c;", cookiezy[0], userId[0]);
                int rc2;
                char* err_msg2 = 0;
                rc2 = sqlite3_exec(db, sqlDelete, 0, 0, &err_msg2);

                if(rc2 != SQLITE_OK){
                    sqlite3_free(err_msg);
                    sqlite3_close(db);
                    strcpy(responce, "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nfail");
                }else{
                    strcat(responce, "HTTP/1.1 200 OK\r\nContent-Length: 7\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nsuccess");
                }
            }else{
                strcpy(responce, "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nfail");
            }
            sqlite3_free(err_msg);
            sqlite3_close(db);
        }else{
            strcat(responce, "HTTP/1.1 200 OK\r\nContent-Length: 7\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nsuccess");
        }

        int writeError;
        if((writeError = write (conn_fd, responce, strlen(responce))) < 0){
            pthread_exit("crepat");
        }

        sqlite3_close(db);
        userId[0] = '\0';
        responce[0] = '\0';
    }
    
}

int main ()
{
  struct sockaddr_in server;	// structura folosita de server
  struct sockaddr_in from;	
  int nr;		//mesajul primit de trimis la client 
  int sd;		//descriptorul de socket 
  int pid;
  pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
	int i=0;
  

  /* crearea unui socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server]Eroare la socket().\n");
      return errno;
    }
  /* utilizarea optiunii SO_REUSEADDR */
  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  
  /* pregatirea structurilor de date */
  bzero (&server, sizeof (server));
  bzero (&from, sizeof (from));
  
  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;	
  /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl (INADDR_ANY);
  /* utilizam un port utilizator */
    server.sin_port = htons (PORT);
  
  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server]Eroare la bind().\n");
      return errno;
    }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen (sd, 2) == -1)
    {
      perror ("[server]Eroare la listen().\n");
      return errno;
    }

  /* servim in mod concurent clientii...folosind thread-uri */
  while (1)
    {
      if(i > 500) i = 0; /*refolosirea threadurilor*/
      int client; /*descriptorul de socket pentru client*/

      thData * td; //parametru functia executata de thread     
      int length = sizeof (from);

      fflush (stdout);

      if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
    	{
	      perror ("[server]Eroare la accept().\n");
	      continue;
	    }
        /* s-a realizat conexiunea, se astepta mesajul */
	
    
	td=(struct thData*)malloc(sizeof(struct thData));	
	td->idThread=i++;
	td->cl=client;

    /* creem threadul care se va ocupa de aceast client */
	pthread_create(&th[i], NULL, &treat, td);	      
			
	}//while
};				
static void *treat(void * arg)
{		
        /*dereferentierea clientului*/
		struct thData tdL; 
		tdL= *((struct thData*)arg);

		fflush (stdout);		 

        /*se realizaza citirea parsarea si trimiterea raspunsului*/
		raspunde((struct thData*)arg); 

		/* am terminat cu acest client, inchidem conexiunea */
		close (tdL.cl);
		return(NULL);	
  		
};

void parsingPath(int new_socket, char* path, char* cookie){

    char header_buff [2048];
    char file_buff [MAX_LEN];
    char filesize[20];

    header_buff[0] = '\0';
    file_buff[0] = '\0';
    filesize[0] = '\0';

    //printf("%s\n", path);

    //if GET REQUEST
    if(strcmp(path, "") == 0){
        //printf("Home\n");
        //TODO: personalised home page
        strcat(file_buff, personalizedFeedPageMaker(cookie));
            
        write (new_socket, file_buff, strlen(file_buff));

    }else if(strstr(path, "profiles/")){
        //printf("Profiles page\n");
        char* profile = strstr(path, "profiles/") + 9;
        //
         
        if((strstr(path, "profiles/img/") == 0) && (strstr(path, "profiles/css/")== 0) && (strstr(path, "profiles/js/")== 0)){
            //TODO: personalised profile 
            strcat(file_buff, personalizedProfilePageMaker(profile, cookie));
            
            write (new_socket, file_buff, strlen(file_buff));
        }
        else{
            response_generator(new_socket, profile);
        }
        profile[0] = '\0';
    
    }else if(strcmp(path, "login") == 0){
        response_generator(new_socket, "login.html");
    
    }else{
        response_generator (new_socket, path);
    }
}

void raspunde(void *arg)
{
    int nr, i=0;
	struct thData tdL; 
	tdL= *((struct thData*)arg);

    
    //printf("Adress: %d\n\n", order);
        int new_socket = tdL.cl;
        //printf("Adress: %d\n", &orderOfTh);
        //printf("%d\n", orderOfTh);

        //int new_socket = threadSocket[orderOfTh];

        char buffer[5000] = {0};
        char bufferCopy[5000] = {0};
        char path[100] = {0};
        long valread;


        if((valread = read(new_socket , buffer, MAX_LEN)) < 0){
            //printf("%s %d -> %d\n\n ", "Error reading from client...", orderOfTh, threadSocket[orderOfTh]);
            //close(new_socket);
            new_socket = 0;
            //threadSocket[orderOfTh] = 0;
            pthread_exit("crepat");
        }else{
        
        if(strlen(buffer) > 10){
            //printf("%s\n\n", buffer);
            parse(buffer, path);

            strcpy(bufferCopy, buffer);
            bufferCopy[strlen(bufferCopy)] = '\0';    

            char cookie[128] = {0};

                char* pch = NULL;
                pch = strtok(buffer, "\r\n");
                while (pch != NULL){  
                    if(strstr(pch, "Cookie: ") != 0){
                        strcat(cookie, strstr(pch, "token=") + 6);
                    }
                    pch = strtok(NULL, "\r\n");
                }
            if(strstr(buffer, "GET ") != 0){
                //GET REQUEST
                if(strlen(cookie) > 1){
                    parsingPath(new_socket, path, cookie);
                }else{
                    parsingPath(new_socket, path, NULL);
                }
            }else if(strstr(buffer, "POST ") != 0){
                post_response_generator(new_socket, path, strstr(bufferCopy, "\r\n\r\n") + 4, cookie);
            }
                //response_generator (new_socket, path);
        }

        buffer[0] = '\0';
        path[0] = '\0';

        //printf("Terminated %d -> %d \n\n", orderOfTh, threadSocket[orderOfTh]);     
    }

}
