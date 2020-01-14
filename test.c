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

int checkUserExistance(char* user){
    sqlite3 *db;
    char *err_msg = 0;
    
    int rc = sqlite3_open("ceva.db", &db);
    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", 
                sqlite3_errmsg(db));
        sqlite3_close(db);
        
        pthread_exit("O crepat la baza de date");
    }

    char sqlQ[200] = {0};
    char userReturned[128] = {0};
    sprintf(sqlQ, "select token from users where token = '%s'", user);

    rc = sqlite3_exec(db, sqlQ, callbackLogin, userReturned, &err_msg);

    sqlite3_close(db);
    return strlen(userReturned);     

}

int checkMessagesExistance(int userFrom, int userTo){
    sqlite3 *db;
    char *err_msg = 0;
    
    int rc = sqlite3_open("ceva.db", &db);
    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", 
                sqlite3_errmsg(db));
        sqlite3_close(db);
        
        pthread_exit("O crepat la baza de date");
    }

    char sqlQ[200] = {0};
    char userReturned[128] = {0};
    sprintf(sqlQ, "select id_mesaj from mesaj where (id_from = %d and id_touser = %d) or (id_from = %d and id_touser = %d)", userFrom, userTo, userTo, userFrom);

    rc = sqlite3_exec(db, sqlQ, callbackProfilePosts, userReturned, &err_msg);


    sqlite3_close(db);
    return strlen(userReturned);
}

int checkIfPostedByUser(int idPost, int idUser){
    sqlite3 *db;
    char *err_msg = 0;
    
    int rc = sqlite3_open("ceva.db", &db);
    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", 
                sqlite3_errmsg(db));
        sqlite3_close(db);
        
        pthread_exit("O crepat la baza de date");
    }

    char sqlQ[200] = {0};
    char userReturned[128] = {0};
    sprintf(sqlQ, "select id_postare from postare where id_postare = %d and user_id = %d", idPost, idUser);

    rc = sqlite3_exec(db, sqlQ, callbackProfilePosts, userReturned, &err_msg);
    sqlite3_close(db);

    return strlen(userReturned);
}

int checkIfUserLikedThisPost(int idPost, int idUser){
    sqlite3 *db;
    char *err_msg = 0;
    
    int rc = sqlite3_open("ceva.db", &db);
    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", 
                sqlite3_errmsg(db));
        sqlite3_close(db);
        
        pthread_exit("O crepat la baza de date");
    }

    char sqlQ[200] = {0};
    char userReturned[128] = {0};
    sprintf(sqlQ, "select id_postare from like where id_postare = %d and id_user = %d", idPost, idUser);

    rc = sqlite3_exec(db, sqlQ, callbackProfilePosts, userReturned, &err_msg);
    sqlite3_close(db);

    return strlen(userReturned);
}

int countNumberOfLikesOfPost(int idPost){
    sqlite3 *db;
    char *err_msg = 0;
    
    int rc = sqlite3_open("ceva.db", &db);
    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", 
                sqlite3_errmsg(db));
        sqlite3_close(db);
        
        pthread_exit("O crepat la baza de date");
    }

    char sqlQ[200] = {0};
    char userReturned[128] = {0};
    sprintf(sqlQ, "select COUNT(*) from like where id_postare = %d", idPost);

    rc = sqlite3_exec(db, sqlQ, callbackProfilePosts, userReturned, &err_msg);
    sqlite3_close(db);

    return atoi(userReturned);
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

        //feed for non existent user

        strcat(head, "<!DOCTYPE html> <html> <head> <meta content=\"text/html;charset=utf-8\" http-equiv=\"Content-Type\"> <meta content=\"utf-8\" http-equiv=\"encoding\"> <title>SocialBear</title> <link rel=\"stylesheet\" href=\"css/reset.css\" /> <link rel=\"stylesheet\" href=\"css/home.css\" /> <link rel=\"stylesheet\" href=\"css/post.css\" /> <link rel=\"stylesheet\" href=\"css/widget.css\" /> <link rel=\"stylesheet\" href=\"css/menu.css\" /> <link rel=\"stylesheet\" href=\"css/chat.css\" /> </head> <body> <header> <img src=\"img/header/menu-button.png\" class=\"menu_img\"/> <a href=\"/\"> <img src=\"img/header/SocialBear.png\" class=\"logo\"/> </a> <input id=\"searchBar\" type=\"search\" placeholder=\"Search\" /> <img src=\"img/header/musica-searcher.png\" class=\"nav_s\"/> </header> <div class=\"menu\"> <a href = \"/login\"> <div class=\"menu_element\"> <img src=\"img/header/ellipsis.png\" class=\"element_image\" /> <h2>Login</h2> </div> </a> </div> <div class=\"feed\"> <div class=\"posts\"> <br/><br/>");
        
        char sqlQuerryForPosts[1000] = {0};
        strcat(sqlQuerryForPosts, "select nume, prenume, grup_id, profile_img, posted_date, img_source, description, u.token, id_postare from users u natural join postare p where p.grup_id = 0 ORDER BY p.posted_date DESC LIMIT 100");
        
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
        int i = 0;
        while (pch2 != NULL){
            strcat(postsReturned[i], pch2);         
            pch2 = strtok(NULL, "~");
            ++i;
        }

        for(int j = 0; j < i; j++){
            char singularPost[2050] = {0};
            char* postTab = NULL;
            char tokensForPost[9][1025] = {0};
            postTab = strtok(postsReturned[j], "|");
            int k = 0;
            while (postTab != NULL){
                strcat(tokensForPost[k], postTab);         
                postTab = strtok(NULL, "|");
                k++;
            }
           
            sprintf(singularPost, " <div id=\"%s\" class=\"post\"> <img class=\"profile_pic\" src=\"%s\" /> <a href=\"/profiles/%s\" >%s %s</a> <font>%s</font> <hr/>%s<img src=\"%s\" class=\"post_image\" /> <hr/> </div>",
            tokensForPost[8], tokensForPost[3], tokensForPost[7], tokensForPost[1], tokensForPost[0], tokensForPost[4], tokensForPost[6], tokensForPost[5]);
            strcat(posts, singularPost);

            singularPost[0] = '\0';
        }
        
        strcat(bottom, "</div> </div> <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js\"> </script> <script> $(window).on('load', function() { $(\"img\").each(function(){ var image = $(this); if(this.naturalWidth == 0 || image.readyState == 'uninitialized'){ $(image).unbind(\"error\").hide(); } }); }); $('#searchBar').on('keypress',function(e) { if(e.which == 13) { window.location.href = \"/search/\" + $('#searchBar').val(); } });</script> </body> </html>");
        
        sprintf(file_buff, "%s%s%s", head,posts,bottom);
        
        long resp_size = strlen(file_buff);

        sprintf(responce, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n%s", resp_size, file_buff);

        sqlite3_close(db);
        resp = responce;
        return resp;


    }else{

        //Feed for existing user

        sprintf(head, "<!DOCTYPE html> <html> <head> <meta content=\"text/html;charset=utf-8\" http-equiv=\"Content-Type\"> <meta content=\"utf-8\" http-equiv=\"encoding\"> <title>SocialBear</title> <link rel=\"stylesheet\" href=\"css/reset.css\" /> <link rel=\"stylesheet\" href=\"css/profile.css\" /> <link rel=\"stylesheet\" href=\"css/home.css\" /> <link rel=\"stylesheet\" href=\"css/post.css\" /> <link rel=\"stylesheet\" href=\"css/widget.css\" /> <link rel=\"stylesheet\" href=\"css/menu.css\" /> <link rel=\"stylesheet\" href=\"css/chat.css\" /> </head> <body> <header> <img src=\"img/header/menu-button.png\" class=\"menu_img\"/> <a href=\"/\"> <img src=\"img/header/SocialBear.png\" class=\"logo\"/> </a> <input id=\"searchBar\" type=\"search\" placeholder=\"Search\" /> <a href=\"/messenger\" target=\"_blank\"> <img src=\"img/header/conversation-speech-bubbles-.png\" id=\"chatBubble\" class=\"nav\"/> </a> <img src=\"img/header/musica-searcher.png\" class=\"nav_s\"/> </header> <div class=\"menu\"> <a href = \"/profiles/%s\"> <div class=\"menu_element\"> <img src=\"img/header/history-clock-button.png\" class=\"element_image\" /> <h2>My Profile</h2> </div> </a> <a href = \"/edit/%s\"> <div class=\"menu_element\"> <img src=\"img/header/settings-cogwheel-button.png\" class=\"element_image\" /> <h2>Edit Profile</h2> </div> </a> <div id=\"logoutButton\" class=\"menu_element\"> <img src=\"img/header/ellipsis.png\" class=\"element_image\" /> <h2>Logout</h2> </div> </div> <div class=\"feed\"> <div class=\"posts\"> <br/><br/> <div class=\"post post_form\" style=\"padding:0;\"> <div id=\"description\" contenteditable=\"true\">Description</div> <div id=\"imageURL\" contenteditable=\"true\">Image URL</div> <div contenteditable=\"false\"> <input type=\"checkbox\" value=\"1\" name=\"r1\" id=\"r1\" checked=\"checked\"/> <label class=\"whatever\" for=\"r1\">Private</label> </div> <button id=\"post_button\" class=\"post_form_submit\"></button> </div>", cookie, cookie);
        
        char sqlQuerryForPosts[1000] = {0};
        sprintf(sqlQuerryForPosts, "select nume, prenume, grup_id, profile_img, posted_date, img_source, description, u.token, id_postare from users u natural join postare p where p.user_id = %d or (EXISTS (select * from prieteni f where f.id_friend = u.user_id and f.id_user = %d) and p.grup_id = 1) or (p.posted_date > datetime('now','-2 days') and p.grup_id = 0 and p.user_id <> %d) order by p.posted_date desc LIMIT 100;", atoi(cookie), atoi(cookie), atoi(cookie));
        
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
            char singularPost[2480] = {0};
            char* postTab = NULL;
            char tokensForPost[9][1025] = {0};
            postTab = strtok(postsReturned[j], "|");
            int k = 0;
            while (postTab != NULL){
                strcat(tokensForPost[k], postTab);         
                postTab = strtok(NULL, "|");
                k++;
            }
           
            sprintf(singularPost, " <div id=\"%s\" class=\"post\"> <img class=\"profile_pic\" src=\"%s\" /> <a href=\"/profiles/%s\" >%s %s</a> <font>%s</font> <hr/>%s<img src=\"%s\" class=\"post_image\" /> <hr/>",
            tokensForPost[8], tokensForPost[3], tokensForPost[7], tokensForPost[1], tokensForPost[0], tokensForPost[4], tokensForPost[6], tokensForPost[5]);

            if(checkIfPostedByUser(atoi(tokensForPost[8]), atoi(cookie))){
                strcat(singularPost, "<button class=\"button delete left\"></button>");
            }

            if(checkIfUserLikedThisPost(atoi(tokensForPost[8]), atoi(cookie))){
                sprintf(singularPost,"%s <button class=\"button right lk ac_like\"> %d </button>", singularPost, countNumberOfLikesOfPost(atoi(tokensForPost[8])));
            }else{
                sprintf(singularPost,"%s <button class=\"button right lk like\"> %d </button>", singularPost, countNumberOfLikesOfPost(atoi(tokensForPost[8])));
            }

            strcat(singularPost, "</div>");
            strcat(posts, singularPost);

            singularPost[0] = '\0';
        }

        
        if((100 - i) > 0){
            char sqlQuerryForPosts2[1000] = {0};
            sprintf(sqlQuerryForPosts2, "select nume, prenume, grup_id, profile_img, posted_date, img_source, description, u.token, id_postare from users u natural join postare p where p.grup_id = 0 and p.posted_date < datetime('now','-2 days') AND p.user_id <> %d order by p.posted_date desc LIMIT 100-%d;", atoi(cookie), i);

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
                char tokensForPost[9][1025] = {0};
                postTab = strtok(postsReturned2[j], "|");
                int k = 0;
                while (postTab != NULL){
                    strcat(tokensForPost[k], postTab);         
                    postTab = strtok(NULL, "|");
                    k++;
                }
            
                sprintf(singularPost, " <div id=\"%s\" class=\"post\"> <img class=\"profile_pic\" src=\"%s\" /> <a href=\"/profiles/%s\" >%s %s</a> <font>%s</font> <hr/>%s<img src=\"%s\" class=\"post_image\" /> <hr/>",
                tokensForPost[8], tokensForPost[3], tokensForPost[7], tokensForPost[1], tokensForPost[0], tokensForPost[4], tokensForPost[6], tokensForPost[5]);
                
                if(checkIfUserLikedThisPost(atoi(tokensForPost[8]), atoi(cookie))){
                sprintf(singularPost,"%s <button class=\"button right lk ac_like\"> %d </button>", singularPost, countNumberOfLikesOfPost(atoi(tokensForPost[8])));
                }else{
                    sprintf(singularPost,"%s <button class=\"button right lk like\"> %d </button>", singularPost, countNumberOfLikesOfPost(atoi(tokensForPost[8])));
                }
                
                strcat(singularPost, "</div>");

                strcat(posts, singularPost);

                singularPost[0] = '\0';
            }
        }
        
        strcat(bottom, "</div> </div> <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js\"></script> <script>$(\".button.right.lk\").click( function(e){ var thisButton = $(this); $.ajax({ type: 'POST', dataType: \"html\", url: '/like', data: {postId: thisButton.parent().attr('id')}, success: function(data) { if(data == \"success\"){ if (thisButton.hasClass('like')) { console.log('like'); thisButton.removeClass(\"like\").addClass(\"ac_like\"); thisButton.html((parseInt(thisButton.html()) + 1).toString()); }else if (thisButton.hasClass('ac_like')) { console.log('dislike'); thisButton.removeClass(\"ac_like\").addClass(\"like\"); thisButton.html((parseInt(thisButton.html()) - 1).toString()); } } else{ alert(\"Could not like post\"); } } }); }); $(\".button.delete.left\").click(function(e){ if (confirm('Are you sure you want to delete this post?')) { $.ajax({ type: 'DELETE', dataType: \"html\", url: '/deletePost', data: {id: $(this).parent().attr('id')}, success: function(data) { if(data == \"success\"){ location.reload(); } else{ alert(\"Could not delete post\"); } } }); } });  $('#searchBar').on('keypress',function(e) { if(e.which == 13) { window.location.href = \"/search/\" + $('#searchBar').val(); } }); $(window).on('load', function() { $(\"img\").each(function(){ var image = $(this); if(this.naturalWidth == 0 || image.readyState == 'uninitialized'){ $(image).unbind(\"error\").hide(); } }); }); $(document).ready(function() { $('#description').click(function(e){ if($('#description').text() == \"Description\") $('#description').text(\"\"); }); $('#imageURL').click(function(e){ if($('#imageURL').text() == \"Image URL\") $('#imageURL').text(\"\"); }); }); $(\"#logoutButton\").click(function(e){ document.cookie = \"token= ; expires = Thu, 01 Jan 1970 00:00:00 GMT\"; window.location.href = \"/\"; }); $(document).ready(function() { $('#post_button').click(function(e) { e.preventDefault(); if($('#description').text() == \"\" || $('#description').text() == \"Description\"){ alert(\"Description can not be null\"); }else if(!$('#imageURL').text().includes(\"http://\") && !$('#imageURL').text().includes(\"https://\") && !($('#imageURL').text() == \"Image URL\") && !($('#imageURL').text() == \"\")){ alert(\"Invalid image URL\"); }else{ var imgURL; if($('#imageURL').text() == \"Image URL\" || $('#imageURL').text() == \"\"){ imgURL = \"https://cdn.pixabay.com/photo/2018/01/1/23/12/nature-3082832__340.jpg\"; }else{ imgURL = $('#imageURL').text(); } $.ajax({ type: 'POST', dataType: \"text\", url: '/postare', data: {description: $('#description').text(), imageUrl: imgURL, private: $('#r1').prop('checked') ? '1' : '0'}, success: function(data) { if(data == \"success\"){ window.location.href = \"/\"; }else{ alert(\"There was an error while posting your post\"); } } }); } }); }); </script> </body> </html>");
        
        sprintf(file_buff, "%s%s%s", head,posts,bottom);
        
        long resp_size = strlen(file_buff);

        sprintf(responce, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n%s", resp_size, file_buff);

        resp = responce;

        sqlite3_close(db);
        return resp;
    }
    
}

//Edit profile page
 char* personalizedEditPageMaker(char* edit,char* cookie){

        char file_buff[MAX_LEN] = {0};
        char head[5000] = {0};
        char bottom [5000] = {0};
        char responce[MAX_LEN] = {0};
        char formInfo[5000] = {0};
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
        sprintf(sqlQuerry, "SELECT nume, prenume, profile_img, cover_url FROM users WHERE token = '%s'", edit);

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

        sprintf(head, "<!DOCTYPE html> <html> <head> <meta content=\"text/html;charset=utf-8\" http-equiv=\"Content-Type\"> <meta content=\"utf-8\" http-equiv=\"encoding\"> <title>%s's profile</title> <meta id=\"meta\" name=\"viewport\" content=\"width=device-width; initial-scale=1.0\" /> <meta id=\"meta\" name=\"viewport\" content=\"width=device-width; initial-scale=1.0\" /> <link rel=\"stylesheet\" href=\"css/reset.css\" /> <link rel=\"stylesheet\" href=\"css/home.css\" /> <link rel=\"stylesheet\" href=\"css/profile.css\" /> <link rel=\"stylesheet\" href=\"css/post.css\" /> <link rel=\"stylesheet\" href=\"css/widget.css\" /> <link rel=\"stylesheet\" href=\"css/menu.css\" /> <link rel=\"stylesheet\" href=\"css/chat.css\" /> </head> <body> <header> <img src=\"img/header/menu-button.png\" class=\"menu_img\"/> <a href=\"/\"> <img src=\"img/header/SocialBear.png\" class=\"logo\"/> </a> <a href=\"/messenger\" target=\"_blank\"> <img src=\"img/header/conversation-speech-bubbles-.png\" id=\"chatBubble\" class=\"nav\"/> </a> </header> <!--Cover img--> <div class=\"profile\" style=\"background-image:url(%s);\"> <div class=\"sub_profile\"> <center> <!--profile pic--> <img src=\"%s\" class=\"profile_pic\" /><br/> <!--Name--> <h2>%s %s</h2><br/> ", tokens[0],tokens[3],tokens[2],tokens[1],tokens[0]);
        strcat(head, "</center> </div> </div> <div class=\"feed\"> <div class=\"posts\">");
        
        sprintf(formInfo, "<div class=\"post post_form\" style=\"padding:0;\"> <div contenteditable=\"false\">New First Name <div id=\"firstName\" contenteditable=\"true\">%s</div></div> <div contenteditable=\"false\">New Last Name <div id=\"lastName\" contenteditable=\"true\">%s</div></div> <div contenteditable=\"false\">Cover Image <div id=\"coverURL\" contenteditable=\"true\">%s</div></div> <div contenteditable=\"false\">Profile Picture <div id=\"profileURL\" contenteditable=\"true\">%s</div></div> <button id=\"updateProfileButton\" class=\"post_form_submit\"></button> </div>", tokens[1], tokens[0], tokens[3], tokens[2]);

        strcat(bottom, "</div> </div> <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js\"></script>");
        strcat(bottom, "<script>if(document.cookie == \"\" || document.cookie != (\"token=\" + location.href.substr(location.href.lastIndexOf('/') + 1))){ window.location.href = \"/\"; } $(document).ready(function() { $('#updateProfileButton').click(function(e) { e.preventDefault(); if( !$('#profileURL').text().includes(\"http://\") && !$('#profileURL').text().includes(\"https://\") && !$('#profileURL').text().includes(\"img/profile/1.png\") && !$('#coverURL').text().includes(\"https://\") && !$('#coverURL').text().includes(\"http://\") && !$('#coverURL').text().includes(\"img/cover/Wallpaper-Macbook.jpg\")){ alert(\"Invalid image URL\"); }else{ $.ajax({ type: 'PUT', dataType: \"text\", url: '/updateProfile', data: {firstName: $('#firstName').text(), lastName: $('#lastName').text(), coverURL: $('#coverURL').text(), profileURL: $('#profileURL').text()}, success: function(data) { if(data == \"success\"){ location.reload(); }else{ console.log(data); alert(\"There was an error while updating your profile\"); } } }); } }); }); </script>");
        strcat(bottom, "</body> </html>");

        sprintf(file_buff, "%s%s%s", head, formInfo, bottom);
        
        long resp_size = strlen(file_buff);

        sprintf(responce, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n%s", resp_size, file_buff);

        
        sqlite3_close(db);
        
        resp = responce;
        return resp;

} 

char* personalizedSearchPageMaker(char* search,char* cookie){
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
        printf("Search for non existend user\n");
        strcat(head, "<!DOCTYPE html> <html> <head> <meta content=\"text/html;charset=utf-8\" http-equiv=\"Content-Type\"> <meta content=\"utf-8\" http-equiv=\"encoding\"> <title>SocialBear</title> <link rel=\"stylesheet\" href=\"css/reset.css\" /> <link rel=\"stylesheet\" href=\"css/home.css\" /> <link rel=\"stylesheet\" href=\"css/post.css\" /> <link rel=\"stylesheet\" href=\"css/widget.css\" /> <link rel=\"stylesheet\" href=\"css/menu.css\" /> <link rel=\"stylesheet\" href=\"css/chat.css\" /> </head> <body> <header> <img src=\"img/header/menu-button.png\" class=\"menu_img\"/> <a href=\"/\"> <img src=\"img/header/SocialBear.png\" class=\"logo\"/> </a> <input id=\"searchBar\" type=\"search\" placeholder=\"Search\" />  <img src=\"img/header/musica-searcher.png\" class=\"nav_s\"/> </header> <div class=\"menu\"> <a href = \"/login\"> <div class=\"menu_element\"> <img src=\"img/header/ellipsis.png\" class=\"element_image\" /> <h2>Login</h2> </div> </a> </div> <div class=\"feed\"> <div class=\"posts\"> <br/><br/>");
        
        char sqlQuerryForPosts[1000] = {0};
        sprintf(sqlQuerryForPosts, "select nume, prenume, grup_id, profile_img, posted_date, img_source, description, u.token, id_postare from users u natural join postare p where p.grup_id = 0 AND p.description LIKE '%%%s%%' ORDER BY p.posted_date DESC LIMIT 100", search);
        
        char returnedPosts[MAX_LEN] = {0};
        rc = sqlite3_exec(db, sqlQuerryForPosts, callbackProfilePosts, returnedPosts, &err_msg); 


        char sqlQuerryForUsers[1000] = {0};
        sprintf(sqlQuerryForUsers, "select nume, prenume, profile_img, token from users where nume LIKE '%%%s%%' or prenume LIKE '%%%s%%'", search, search);
        
        char returnedUsers[MAX_LEN] = {0};
        int rc2 = sqlite3_exec(db, sqlQuerryForUsers, callbackProfilePosts, returnedUsers, &err_msg); 

        

        if (rc != SQLITE_OK ) {
            
            fprintf(stderr, "Failed to select data\n");
            fprintf(stderr, "SQL error: %s\n", err_msg);

            sqlite3_free(err_msg);
            sqlite3_close(db);
            
            pthread_exit("O crepat la baza de date");
        }

        if (rc2 != SQLITE_OK ) {
            
            fprintf(stderr, "Failed to select data\n");
            fprintf(stderr, "SQL error: %s\n", err_msg);

            sqlite3_free(err_msg);
            sqlite3_close(db);
            
            pthread_exit("O crepat la baza de date");
        }
        sqlite3_close(db);


        char* pch3 = NULL;
        char usersReturned[105][2050] = {0};
        pch3 = strtok(returnedUsers, "~");
        int index = 0;
        while (pch3 != NULL){
            strcat(usersReturned[index], pch3);         
            pch3 = strtok(NULL, "~");
            ++index;
        }

        for(int j = 0; j < index; j++){
            char singularPost[2050] = {0};
            char* postTab = NULL;
            char tokensForPost[4][1025] = {0};
            postTab = strtok(usersReturned[j], "|");
            int k = 0;
            while (postTab != NULL){
                strcat(tokensForPost[k], postTab);         
                postTab = strtok(NULL, "|");
                k++;
            }
           
            sprintf(singularPost, " <div class=\"post\"> <img class=\"profile_pic\" src=\"%s\" /> <a href=\"/profiles/%s\" >%s %s</a><hr/> </div>",
            tokensForPost[2], tokensForPost[3], tokensForPost[1], tokensForPost[0]);
            strcat(posts, singularPost);

            singularPost[0] = '\0';
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
            char tokensForPost[9][1025] = {0};
            postTab = strtok(postsReturned[j], "|");
            int k = 0;
            while (postTab != NULL){
                strcat(tokensForPost[k], postTab);         
                postTab = strtok(NULL, "|");
                k++;
            }
           
            sprintf(singularPost, " <div id=\"%s\" class=\"post\"> <img class=\"profile_pic\" src=\"%s\" /> <a href=\"/profiles/%s\" >%s %s</a> <font>%s</font> <hr/>%s<img src=\"%s\" class=\"post_image\" /> <hr/> </div>",
            tokensForPost[8], tokensForPost[3], tokensForPost[7], tokensForPost[1], tokensForPost[0], tokensForPost[4], tokensForPost[6], tokensForPost[5]);
            strcat(posts, singularPost);

            singularPost[0] = '\0';
        }
        
        strcat(bottom, "</div> </div> <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js\"> </script> <script>$('#searchBar').on('keypress',function(e) { if(e.which == 13) { window.location.href = \"/search/\" + $('#searchBar').val(); } }); $(window).on('load', function() { $(\"img\").each(function(){ var image = $(this); if(this.naturalWidth == 0 || image.readyState == 'uninitialized'){ $(image).unbind(\"error\").hide(); } }); }); </script> </body> </html>");
        
        sprintf(file_buff, "%s%s%s", head,posts,bottom);
        
        long resp_size = strlen(file_buff);

        sprintf(responce, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n%s", resp_size, file_buff);

        resp = responce;
        return resp;


    }else{

        //Search for existing user

        sprintf(head, "<!DOCTYPE html> <html> <head> <meta content=\"text/html;charset=utf-8\" http-equiv=\"Content-Type\"> <meta content=\"utf-8\" http-equiv=\"encoding\"> <title>SocialBear</title> <link rel=\"stylesheet\" href=\"css/reset.css\" /> <link rel=\"stylesheet\" href=\"css/profile.css\" /> <link rel=\"stylesheet\" href=\"css/home.css\" /> <link rel=\"stylesheet\" href=\"css/post.css\" /> <link rel=\"stylesheet\" href=\"css/widget.css\" /> <link rel=\"stylesheet\" href=\"css/menu.css\" /> <link rel=\"stylesheet\" href=\"css/chat.css\" /> </head> <body> <header> <img src=\"img/header/menu-button.png\" class=\"menu_img\"/> <a href=\"/\"> <img src=\"img/header/SocialBear.png\" class=\"logo\"/> </a> <input id=\"searchBar\" type=\"search\" placeholder=\"Search\" /> <a href=\"/messenger\" target=\"_blank\"> <img src=\"img/header/conversation-speech-bubbles-.png\" id=\"chatBubble\" class=\"nav\"/> </a> <img src=\"img/header/musica-searcher.png\" class=\"nav_s\"/> </header> <div class=\"menu\"> <a href = \"/profiles/%s\"> <div class=\"menu_element\"> <img src=\"img/header/history-clock-button.png\" class=\"element_image\" /> <h2>My Profile</h2> </div> </a> <a href = \"/edit/%s\"> <div class=\"menu_element\"> <img src=\"img/header/settings-cogwheel-button.png\" class=\"element_image\" /> <h2>Edit Profile</h2> </div> </a> <div id=\"logoutButton\" class=\"menu_element\"> <img src=\"img/header/ellipsis.png\" class=\"element_image\" /> <h2>Logout</h2> </div> </div> <div class=\"feed\"> <div class=\"posts\"> <br/><br/>", cookie, cookie);
        
        char sqlQuerryForPosts[1000] = {0};
        sprintf(sqlQuerryForPosts, "select nume, prenume, grup_id, profile_img, posted_date, img_source, description, u.token, id_postare from users u natural join postare p where p.description LIKE '%%%s%%' AND (p.user_id = %d or (EXISTS (select * from prieteni f where f.id_friend = u.user_id and f.id_user = %d) and p.grup_id = 1) or (p.posted_date > datetime('now','-2 days') and p.grup_id = 0 and p.user_id <> %d)) order by p.posted_date desc LIMIT 100;",search, atoi(cookie), atoi(cookie), atoi(cookie));
        

        char returnedPosts[MAX_LEN] = {0};
        rc = sqlite3_exec(db, sqlQuerryForPosts, callbackProfilePosts, returnedPosts, &err_msg);

        char sqlQuerryForUsers[1000] = {0};
        sprintf(sqlQuerryForUsers, "select nume, prenume, profile_img, token from users where nume LIKE '%%%s%%' or prenume LIKE '%%%s%%'", search, search);
        
        char returnedUsers[MAX_LEN] = {0};
        int rc2 = sqlite3_exec(db, sqlQuerryForUsers, callbackProfilePosts, returnedUsers, &err_msg); 

        

        if (rc != SQLITE_OK ) {
            
            fprintf(stderr, "Failed to select data\n");
            fprintf(stderr, "SQL error: %s\n", err_msg);

            sqlite3_free(err_msg);
            sqlite3_close(db);
            
            pthread_exit("O crepat la baza de date");
        }

        if (rc2 != SQLITE_OK ) {
            
            fprintf(stderr, "Failed to select data\n");
            fprintf(stderr, "SQL error: %s\n", err_msg);

            sqlite3_free(err_msg);
            sqlite3_close(db);
            
            pthread_exit("O crepat la baza de date");
        }
        


        char* pch3 = NULL;
        char usersReturned[105][2050] = {0};
        pch3 = strtok(returnedUsers, "~");
        int index = 0;
        while (pch3 != NULL){
            strcat(usersReturned[index], pch3);         
            pch3 = strtok(NULL, "~");
            ++index;
        }

        for(int j = 0; j < index; j++){
            char singularPost[2050] = {0};
            char* postTab = NULL;
            char tokensForPost[4][1025] = {0};
            postTab = strtok(usersReturned[j], "|");
            int k = 0;
            while (postTab != NULL){
                strcat(tokensForPost[k], postTab);         
                postTab = strtok(NULL, "|");
                k++;
            }
           
            sprintf(singularPost, " <div class=\"post\"> <img class=\"profile_pic\" src=\"%s\" /> <a href=\"/profiles/%s\" >%s %s</a><hr/> </div>",
            tokensForPost[2], tokensForPost[3], tokensForPost[1], tokensForPost[0]);
            strcat(posts, singularPost);

            singularPost[0] = '\0';
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
            char tokensForPost[9][1025] = {0};
            postTab = strtok(postsReturned[j], "|");
            int k = 0;
            while (postTab != NULL){
                strcat(tokensForPost[k], postTab);         
                postTab = strtok(NULL, "|");
                k++;
            }
           
            sprintf(singularPost, " <div id=\"%s\" class=\"post\"> <img class=\"profile_pic\" src=\"%s\" /> <a href=\"/profiles/%s\" >%s %s</a> <font>%s</font> <hr/>%s<img src=\"%s\" class=\"post_image\" /> <hr/>",
            tokensForPost[8], tokensForPost[3], tokensForPost[7], tokensForPost[1], tokensForPost[0], tokensForPost[4], tokensForPost[6], tokensForPost[5]);
            
            if(checkIfPostedByUser(atoi(tokensForPost[8]), atoi(cookie))){
                strcat(singularPost, "<button class=\"button delete left\"></button>");
            }

            if(checkIfUserLikedThisPost(atoi(tokensForPost[8]), atoi(cookie))){
                sprintf(singularPost,"%s <button class=\"button right lk ac_like\"> %d </button>", singularPost, countNumberOfLikesOfPost(atoi(tokensForPost[8])));
            }else{
                sprintf(singularPost,"%s <button class=\"button right lk like\"> %d </button>", singularPost, countNumberOfLikesOfPost(atoi(tokensForPost[8])));
            }
                
            strcat(singularPost, "</div>");
            
            strcat(posts, singularPost);
            singularPost[0] = '\0';
        }

        
        if((100 - i) > 0){
            char sqlQuerryForPosts2[1000] = {0};
            sprintf(sqlQuerryForPosts2, "select nume, prenume, grup_id, profile_img, posted_date, img_source, description, u.token, id_postare from users u natural join postare p where p.description LIKE '%%%s%%' AND (p.grup_id = 0 and p.posted_date < datetime('now','-2 days') AND p.user_id <> %d) order by p.posted_date desc LIMIT 100-%d;", search, atoi(cookie), i);

            printf("%s\n", sqlQuerryForPosts2);
            
            char returnedPosts2[MAX_LEN] = {0};
            rc = sqlite3_exec(db, sqlQuerryForPosts2, callbackProfilePosts, returnedPosts2, &err_msg);

            printf("%s\n", returnedPosts2);

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
                char tokensForPost[9][1025] = {0};
                postTab = strtok(postsReturned2[j], "|");
                int k = 0;
                while (postTab != NULL){
                    strcat(tokensForPost[k], postTab);         
                    postTab = strtok(NULL, "|");
                    k++;
                }
            
                sprintf(singularPost, " <div id=\"%s\" class=\"post\"> <img class=\"profile_pic\" src=\"%s\" /> <a href=\"/profiles/%s\" >%s %s</a> <font>%s</font> <hr/>%s<img src=\"%s\" class=\"post_image\" /> <hr/>",
                tokensForPost[8], tokensForPost[3], tokensForPost[7], tokensForPost[1], tokensForPost[0], tokensForPost[4], tokensForPost[6], tokensForPost[5]);
                
                if(checkIfUserLikedThisPost(atoi(tokensForPost[8]), atoi(cookie))){
                sprintf(singularPost,"%s <button class=\"button right lk ac_like\"> %d </button>", singularPost, countNumberOfLikesOfPost(atoi(tokensForPost[8])));
                }else{
                    sprintf(singularPost,"%s <button class=\"button right lk like\"> %d </button>", singularPost, countNumberOfLikesOfPost(atoi(tokensForPost[8])));
                }
                
                strcat(singularPost, "</div>");
                
                strcat(posts, singularPost);

                singularPost[0] = '\0';
            }
        }
        
        strcat(bottom, "</div> </div> <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js\"></script> <script>$(\".button.right.lk\").click( function(e){ var thisButton = $(this); $.ajax({ type: 'POST', dataType: \"html\", url: '/like', data: {postId: thisButton.parent().attr('id')}, success: function(data) { if(data == \"success\"){ if (thisButton.hasClass('like')) { console.log('like'); thisButton.removeClass(\"like\").addClass(\"ac_like\"); thisButton.html((parseInt(thisButton.html()) + 1).toString()); }else if (thisButton.hasClass('ac_like')) { console.log('dislike'); thisButton.removeClass(\"ac_like\").addClass(\"like\"); thisButton.html((parseInt(thisButton.html()) - 1).toString()); } } else{ alert(\"Could not like post\"); } } }); }); $(\".button.delete.left\").click(function(e){ if (confirm('Are you sure you want to delete this post?')) { $.ajax({ type: 'DELETE', dataType: \"html\", url: '/deletePost', data: {id: $(this).parent().attr('id')}, success: function(data) { if(data == \"success\"){ location.reload(); } else{ alert(\"Could not delete post\"); } } }); } }); $('#searchBar').on('keypress',function(e) { if(e.which == 13) { window.location.href = \"/search/\" + $('#searchBar').val(); } }); $(window).on('load', function() { $(\"img\").each(function(){ var image = $(this); if(this.naturalWidth == 0 || image.readyState == 'uninitialized'){ $(image).unbind(\"error\").hide(); } }); }); $(\"#logoutButton\").click(function(e){ document.cookie = \"token= ; expires = Thu, 01 Jan 1970 00:00:00 GMT; path=/\"; location.reload(); }); </script> </body> </html>");
        
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
            sprintf(sqlQuerryForPosts, "select nume, prenume, grup_id, profile_img, posted_date, img_source, description, id_postare from users u natural join postare p where u.token = '%s' AND p.grup_id = 0 ORDER BY p.posted_date DESC LIMIT 100", profile);
        }else{
            sprintf(sqlQuerryForPosts, "select nume, prenume, grup_id, profile_img, posted_date, img_source, description, id_postare from users u natural join postare p where u.token = '%s' AND p.grup_id = 0 UNION select nume, prenume, grup_id, profile_img, posted_date, img_source, description, id_postare from users u natural join postare p where u.token = '%s' AND p.grup_id = 1 AND (EXISTS(SELECT * FROM prieteni WHERE (id_user = %d and id_friend = %d)) OR p.user_id = %d) ORDER BY p.posted_date DESC LIMIT 100", profile, profile, atoi(cookie), atoi(profile), atoi(cookie));

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
            char tokensForPost[8][1025] = {0};
            postTab = strtok(postsReturned[j], "|");
            int k = 0;
            while (postTab != NULL){
                strcat(tokensForPost[k], postTab);         
                postTab = strtok(NULL, "|");
                k++;
            }
           
            sprintf(singularPost, " <div id=\"%s\" class=\"post\"> <img class=\"profile_pic\" src=\"%s\" /> <a href=\"#\" >%s %s</a> <font>%s</font> <hr/>%s<img src=\"%s\" class=\"post_image\" /> <hr/>",
            tokensForPost[7], tokensForPost[3], tokensForPost[1], tokensForPost[0], tokensForPost[4], tokensForPost[6], tokensForPost[5]);

            if(cookie != NULL && checkIfPostedByUser(atoi(tokensForPost[7]), atoi(cookie))){
                strcat(singularPost, "<button class=\"button delete left\"></button>");
            }

            if(cookie != NULL){
                if(checkIfUserLikedThisPost(atoi(tokensForPost[7]), atoi(cookie))){
                sprintf(singularPost,"%s <button class=\"button right lk ac_like\"> %d </button>", singularPost, countNumberOfLikesOfPost(atoi(tokensForPost[8])));
                }else{
                    sprintf(singularPost,"%s <button class=\"button right lk like\"> %d </button>", singularPost, countNumberOfLikesOfPost(atoi(tokensForPost[8])));
                }
            }

            strcat(singularPost, "</div>");

            strcat(posts, singularPost);

            singularPost[0] = '\0';
        }
        
        if(cookie != NULL)
            sprintf(head, "<!DOCTYPE html> <html> <head> <meta content=\"text/html;charset=utf-8\" http-equiv=\"Content-Type\"> <meta content=\"utf-8\" http-equiv=\"encoding\"> <title>%s's profile</title> <meta id=\"meta\" name=\"viewport\" content=\"width=device-width; initial-scale=1.0\" /> <meta id=\"meta\" name=\"viewport\" content=\"width=device-width; initial-scale=1.0\" /> <link rel=\"stylesheet\" href=\"css/reset.css\" /> <link rel=\"stylesheet\" href=\"css/home.css\" /> <link rel=\"stylesheet\" href=\"css/profile.css\" /> <link rel=\"stylesheet\" href=\"css/post.css\" /> <link rel=\"stylesheet\" href=\"css/widget.css\" /> <link rel=\"stylesheet\" href=\"css/menu.css\" /> <link rel=\"stylesheet\" href=\"css/chat.css\" /> </head> <body> <header> <img src=\"img/header/menu-button.png\" class=\"menu_img\"/> <a href=\"/\"> <img src=\"img/header/SocialBear.png\" class=\"logo\"/> </a> <a href=\"/messenger\" target=\"_blank\"> <img src=\"img/header/conversation-speech-bubbles-.png\" id=\"chatBubble\" class=\"nav\"/> </a> </header> <div class=\"menu\"> <a href = \"/profiles/%s\"> <div class=\"menu_element\"> <img src=\"img/header/history-clock-button.png\" class=\"element_image\" /> <h2>My Profile</h2> </div> </a> <a href = \"/edit/%s\"> <div class=\"menu_element\"> <img src=\"img/header/settings-cogwheel-button.png\" class=\"element_image\" /> <h2>Edit Profile</h2> </div> </a> <div id = \"logoutButton\" class=\"menu_element\"> <img src=\"img/header/ellipsis.png\" class=\"element_image\" /> <h2>Logout</h2> </div> </div> <!--Cover img--> <div class=\"profile\" style=\"background-image:url(%s);\"> <div class=\"sub_profile\"> <center> <!--profile pic--> <img src=\"%s\" class=\"profile_pic\" /><br/> <!--Name--> <h2>%s %s</h2><br/> ", tokens[0], cookie, cookie, tokens[3],tokens[2],tokens[1],tokens[0]);
        else{
            sprintf(head, "<!DOCTYPE html> <html> <head> <meta content=\"text/html;charset=utf-8\" http-equiv=\"Content-Type\"> <meta content=\"utf-8\" http-equiv=\"encoding\"> <title>%s's profile</title> <meta id=\"meta\" name=\"viewport\" content=\"width=device-width; initial-scale=1.0\" /> <meta id=\"meta\" name=\"viewport\" content=\"width=device-width; initial-scale=1.0\" /> <link rel=\"stylesheet\" href=\"css/reset.css\" /> <link rel=\"stylesheet\" href=\"css/home.css\" /> <link rel=\"stylesheet\" href=\"css/profile.css\" /> <link rel=\"stylesheet\" href=\"css/post.css\" /> <link rel=\"stylesheet\" href=\"css/widget.css\" /> <link rel=\"stylesheet\" href=\"css/menu.css\" /> <link rel=\"stylesheet\" href=\"css/chat.css\" /> </head> <body> <header> <img src=\"img/header/menu-button.png\" class=\"menu_img\"/> <a href=\"/\"> <img src=\"img/header/SocialBear.png\" class=\"logo\"/> </a> </header> <div class=\"menu\"> <a href = \"/login\"> <div class=\"menu_element\"> <img src=\"img/header/ellipsis.png\" class=\"element_image\" /> <h2>Login</h2> </div> </a> </div> <!--Cover img--> <div class=\"profile\" style=\"background-image:url(%s);\"> <div class=\"sub_profile\"> <center> <!--profile pic--> <img src=\"%s\" class=\"profile_pic\" /><br/> <!--Name--> <h2>%s %s</h2><br/> ", tokens[0],tokens[3],tokens[2],tokens[1],tokens[0]);
        }

        if(cookie != NULL && atoi(cookie) != atoi(profile)){
            char sqlFriend[100] = {0};
            char sqlRespFriend[100] = {0};
            sprintf(sqlFriend, "SELECT * FROM prieteni where id_user = %d and id_friend = %d;", atoi(cookie), atoi(profile));

            rc = sqlite3_exec(db, sqlFriend, callbackFriend, sqlRespFriend, &err_msg);
            if(strlen(sqlRespFriend) > 0){
                strcat(head, "<button id = \"followButton\" class=\"btn_follow\">Unfollow</button> <button id = \"messageButton\" >Message</button>");
            }else{
                strcat(head, "<button id = \"followButton\" class=\"btn_follow\">Follow</button> <button id = \"messageButton\">Message</button>");
            }
        }
        strcat(head, "</center> </div> </div> <div class=\"feed\"> <div class=\"posts\">");
        

        if(cookie != NULL && atoi(cookie) == atoi(profile)){
            strcat(head, "<div class=\"post post_form\" style=\"padding:0;\"> <div id=\"description\" contenteditable=\"true\">Description</div> <div id=\"imageURL\" contenteditable=\"true\">Image URL</div> <div contenteditable=\"false\"> <input type=\"checkbox\" value=\"1\" name=\"r1\" id=\"r1\" checked=\"checked\"/> <label class=\"whatever\" for=\"r1\">Private</label> </div> <button id=\"post_button\" class=\"post_form_submit\"></button> </div>");
        }
       
        //sprintf(posts)

        strcat(bottom, "</div> </div> <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js\"></script>");
        
        if(cookie != NULL && atoi(cookie) == atoi(profile)){
            strcat(bottom, "<script> $(\".button.right.lk\").click( function(e){ var thisButton = $(this); $.ajax({ type: 'POST', dataType: \"html\", url: '/like', data: {postId: thisButton.parent().attr('id')}, success: function(data) { if(data == \"success\"){ if (thisButton.hasClass('like')) { console.log('like'); thisButton.removeClass(\"like\").addClass(\"ac_like\"); thisButton.html((parseInt(thisButton.html()) + 1).toString()); }else if (thisButton.hasClass('ac_like')) { console.log('dislike'); thisButton.removeClass(\"ac_like\").addClass(\"like\"); thisButton.html((parseInt(thisButton.html()) - 1).toString()); } } else{ alert(\"Could not like post\"); } } }); }); $(\".button.delete.left\").click(function(e){ if (confirm('Are you sure you want to delete this post?')) { $.ajax({ type: 'DELETE', dataType: \"html\", url: '/deletePost', data: {id: $(this).parent().attr('id')}, success: function(data) { if(data == \"success\"){ location.reload(); } else{ alert(\"Could not delete post\"); } } }); } }); $(window).on('load', function() { $(\"img\").each(function(){ var image = $(this); if(this.naturalWidth == 0 || image.readyState == 'uninitialized'){ $(image).unbind(\"error\").hide(); } }); }); $(\"#logoutButton\").click(function(e){console.log(document.cookie); document.cookie = \"token= ; expires = Thu, 01 Jan 1970 00:00:00 GMT; path=/\"; location.reload();}); $(document).ready(function() { $('#description').click(function(e){ if($('#description').text() == \"Description\") $('#description').text(\"\"); }); $('#imageURL').click(function(e){ if($('#imageURL').text() == \"Image URL\") $('#imageURL').text(\"\"); }); }); $(document).ready(function() { $('#post_button').click(function(e) { e.preventDefault(); if($('#description').text() == \"\" || $('#description').text() == \"Description\"){ alert(\"Description can not be null\"); }else if(!$('#imageURL').text().includes(\"http://\") && !$('#imageURL').text().includes(\"https://\") && !($('#imageURL').text() == \"Image URL\") && !($('#imageURL').text() == \"\")){ alert(\"Invalid image URL\"); }else{ var imgURL; if($('#imageURL').text() == \"Image URL\" || $('#imageURL').text() == \"\"){ imgURL = \"https://cdn.pixabay.com/photo/2018/01/1/23/12/nature-3082832__340.jpg\"; }else{ imgURL = $('#imageURL').text(); } $.ajax({ type: 'POST', dataType: \"text\", url: '/postare', data: {description: $('#description').text(), imageUrl: imgURL, private: $('#r1').prop('checked') ? '1' : '0'}, success: function(data) { if(data == \"success\"){ window.location.href = \"/\"; }else{ alert(\"There was an error while posting your post\"); } } }); } }); }); </script>");
        }else if(cookie != NULL){
            strcat(bottom, "<script> $(\".button.right.lk\").click( function(e){ var thisButton = $(this); $.ajax({ type: 'POST', dataType: \"html\", url: '/like', data: {postId: thisButton.parent().attr('id')}, success: function(data) { if(data == \"success\"){ if (thisButton.hasClass('like')) { console.log('like'); thisButton.removeClass(\"like\").addClass(\"ac_like\"); thisButton.html((parseInt(thisButton.html()) + 1).toString()); }else if (thisButton.hasClass('ac_like')) { console.log('dislike'); thisButton.removeClass(\"ac_like\").addClass(\"like\"); thisButton.html((parseInt(thisButton.html()) - 1).toString()); } } else{ alert(\"Could not like post\"); } } }); }); $(window).on('load', function() { $(\"img\").each(function(){ var image = $(this); if(this.naturalWidth == 0 || image.readyState == 'uninitialized'){ $(image).unbind(\"error\").hide(); } }); }); $('#messageButton').click(function(e) { $.ajax({ type: 'POST', dataType: \"html\", url: '/postMsg', data: {message: '~HelloBear', idTo: location.href.substr(location.href.lastIndexOf('/') + 1)}, success: function(data) { if(data == \"success\"){ window.open('http://127.0.0.1:8080/messenger', '_blank'); } else{ alert(\"Could not send messege\"); } } }); }); $(\"#logoutButton\").click(function(e){console.log(document.cookie); document.cookie = \"token= ; expires = Thu, 01 Jan 1970 00:00:00 GMT; path=/\"; location.reload();}); $(document).ready(function() { $('#followButton').click(function(e) { e.preventDefault(); $.ajax({ type: 'POST', dataType: \"text\", url: '/prieteni', data: {userId: location.href.substr(location.href.lastIndexOf('/') + 1)}, success: function(data) { if(data == \"success\"){ location.reload(); }else{ alert(\"Could not establish friendship\"); } } }); }); });</script>");
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



char* ajaxMessagePageMaker(char* cookie){

    char file_buff[MAX_LEN] = {0};
    char people[5000] = {0};
    char messages[MAX_LEN] = {0};
    char responce[MAX_LEN] = {0};
    char *resp;

    char returnedUsers[MAX_LEN] = {0};

    sqlite3 *db;
        char *err_msg = 0;
        
        int rc = sqlite3_open("ceva.db", &db);
        
        if (rc != SQLITE_OK) {
            
            fprintf(stderr, "Cannot open database: %s\n", 
                    sqlite3_errmsg(db));
            sqlite3_close(db);
            
            pthread_exit("O crepat la baza de date");
        }

    char sqlQuerryForUsers[2000] = {0} ;
    
    sprintf(sqlQuerryForUsers, "select user_id, nume, prenume, token, profile_img from users where user_id <> %d and exists (select * from mesaj where (id_from = user_id and id_touser = %d) or (id_from = %d and id_touser = user_id)) LIMIT 100", atoi(cookie), atoi(cookie), atoi(cookie));
    
    rc = sqlite3_exec(db, sqlQuerryForUsers, callbackProfilePosts, returnedUsers, &err_msg); 

    //printf("%s\n", returnedUsers);
    
    char* pch2 = NULL;
    char usersReturned[105][2050] = {0};
    pch2 = strtok(returnedUsers, "~");
    int i = 0;
    while (pch2 != NULL){
        strcat(usersReturned[i], pch2);         
        pch2 = strtok(NULL, "~");
        ++i;
    }

    char userIdsInOrder[200][5] = {0};
    char tokensInOrder[200][128] = {0};
    strcat(people, "<div class=\"left\" style=\"overflow-y: scroll; overflow-x: hidden;\"> <ul class=\"people\">");
    
    for(int j = 0; j < i; j++){
        char singularUser[2050] = {0};
        char* userTab = NULL;
        char tokensForUser[5][1025] = {0};
        userTab = strtok(usersReturned[j], "|");
        int k = 0;
        while (userTab != NULL){
            strcat(tokensForUser[k], userTab);         
            userTab = strtok(NULL, "|");
            k++;
        }
        
        strcat(userIdsInOrder[j], tokensForUser[0]);
        strcat(tokensInOrder[j], tokensForUser[3]);

        sprintf(singularUser, "<li class=\"person\" data-chat=\"%s\"> <img src=\"%s\" alt=\"%s%s\" /> <span class=\"name\">%s %s</span> </li>",
        tokensForUser[3], tokensForUser[4], tokensForUser[2], tokensForUser[1], tokensForUser[2], tokensForUser[1]);
        strcat(people, singularUser);

        singularUser[0] = '\0';
    }

        strcat(people, "</ul></div>");

        strcat(messages, "<div class=\"right\"> <div class=\"top\"><span>Name: <span id=\"topName\" class=\"name\">Bear</span></span></div>");
        

        for(int j = 0; j < i; j++){
            

            char sqlQuerryForMessages[200] = {0};
            char messagesFromAUser[3000] = {0};
            messagesFromAUser[0] = '\0';
            sqlQuerryForMessages[0] = '\0';
            sprintf(sqlQuerryForMessages, "select mesaj, id_from, id_touser from mesaj where (id_from = %d and id_touser = %s) or (id_from = %s and id_touser = %d) ORDER BY receive_date LIMIT 100", atoi(cookie), userIdsInOrder[j], userIdsInOrder[j], atoi(cookie));

            //printf("%s\n\n", sqlQuerryForMessages);

            rc = sqlite3_exec(db, sqlQuerryForMessages, callbackProfilePosts, messagesFromAUser, &err_msg);
            
            
            
            //printf("%s\n", messagesFromAUser);

            char* pchM = NULL;
            pchM = strtok(messagesFromAUser, "~");

            char messagesReturned[105][256] = {0};

            int k = 0; 
            while(pchM != NULL){
                strcat(messagesReturned[k], pchM);
                pchM = strtok(NULL, "~");
                ++k;
            }


            sprintf(messages, "%s <div class=\"chat\" data-chat=\"%s\"> <div id = \"%s\" style=\"overflow-y: scroll;\">", messages, tokensInOrder[j], tokensInOrder[j]);
            
            for(int l = 0; l < k; l++){
                char* messTab = NULL;
                char tokensForMss[3][300] = {0};
                messTab = strtok(messagesReturned[l], "|");
                int x = 0;
                while (messTab != NULL){
                    strcat(tokensForMss[x], messTab);
                    messTab = strtok(NULL, "|");
                    x++;
                }

                
                if(atoi(tokensForMss[1]) == atoi(cookie)){
                    sprintf(messages, "%s <div class=\"bubble me\"> %s </div>", messages, tokensForMss[0]);
                }else if(atoi(tokensForMss[2]) == atoi(cookie)){
                    sprintf(messages, "%s <div class=\"bubble you\"> %s </div>", messages, tokensForMss[0]);
                }

                tokensForMss[0][0] = '\0';
                tokensForMss[1][0] = '\0';
                tokensForMss[2][0] = '\0';

                
            }

            strcat(messages, "</div></div>");
        }

        strcat(messages, "<div class=\"write\"> <a href=\"javascript:;\" class=\"write-link attach\"></a> <input id=\"textBoxInput\" type=\"text\" /> </div> </div>");

        sprintf(file_buff,"%s%s", people, messages);

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
    
    char header_buff [4096];
    char file_buff [MAX_LEN];
    char filesize[40];

    header_buff[0] = '\0';
    file_buff[0] = '\0';
    filesize[0] = '\0';

        

    if (filename == NULL) {
        strcpy (header_buff, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nContent-Type: text/html\r\n");
    }

    FILE *fp;
    fp = fopen (filename, "r");

    if (fp == NULL) {
        printf ("fp is null or filename = 404\n");
        response_generator(conn_fd, "404.html");
        return;       
    }
    else if (fp != NULL) {
        if(strcmp(filename, "404.html") == 0){
            strcpy (header_buff, "HTTP/1.1 404 Not Found\r\nContent-Length: ");
        }else{
            strcpy (header_buff, "HTTP/1.1 200 OK\r\nContent-Length: ");
        }
        /* content-length: */
        strcat (header_buff, filesize);
        strcat (header_buff, "\r\n");
        /* content-type: */
        strcat (header_buff, find_content_type (filename));
        //printf ("%s\n", find_content_type (filename));
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

        // calculating the size of the file
    fseek(fp, 0L, SEEK_END);  
    long int res = ftell(fp);
    sprintf (filesize, "%ld", res); // put the file size of buffer, so we can add it to the response header
    rewind(fp);

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
                strcat(loginID, pch + 8);
                URIdecode(loginID, decodedLoginID);
            }else if(strstr(pch, "loginPass=") != 0){
                strcat(password, pch + 10);
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
            strcat(responce, "HTTP/1.1 200 OK\r\nContent-Length: 7\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nfailure");
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
                strcat(registerPreName, pch + 15);
                //URIdecode(loginID, decodedLoginID);
            }else if(strstr(pch, "registerName=") != 0){
                strcat(registerName, pch + 13);
            }else if(strstr(pch, "registerEmail=") != 0){
                strcat(registerEmail, pch + 14);
                URIdecode(registerEmail, decodedLoginEmail);
            }else if(strstr(pch, "registerToken=") != 0){
                strcat(registerID, pch + 14);
            }else if(strstr(pch, "registerPassword=") != 0){
                strcat(password, pch + 17);
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
                strcat(responce, "HTTP/1.1 200 OK\r\nContent-Length: 5\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nemail");
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
        char responce[1024] = {0};

        printf("\n\n req head: %s\n", requestHead);

        char* pch = NULL;
        pch = strtok(requestHead, "&");
       
        while (pch != NULL)
        {
            if(strstr(pch, "description=") != 0){
                strcat(description, pch + 12);
                URIdecode(description, decodedDescription);
            }else if(strstr(pch, "imageUrl=") != 0){
                strcat(imgURL, pch + 9);
                URIdecode(imgURL, decodedImgURL);
            }else if(strstr(pch, "private=") != 0){
                strcat(private, pch + 8);
            }
            pch = strtok(NULL, "&");
        }

         printf("A picat inaite time declaration\n");
        
        time_t t = time(NULL);
        struct tm tm;
        localtime_r(&t, &tm);
       
       printf("A picat dupa time declaration\n");

        char sqlInsert[1000] = {0};
        sprintf(sqlInsert, "INSERT INTO POSTARE(user_id, grup_id, posted_date, img_source, description) VALUES (%d, %c, '%d-%02d-%02d %02d:%02d:%02d', '%s', '%s');",
        atoi(cookiezy), private[0], tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, decodedImgURL, decodedDescription);

        printf("A picat dupa time use\n");

        rc = sqlite3_exec(db, sqlInsert, 0, 0, &err_msg);
    

        if (rc != SQLITE_OK ) {
            strcat(responce, "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nfail");
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
                strcat(userId, pch + 7);
            }
            pch = strtok(NULL, "&");
        }


        sprintf(sqlInsert, "INSERT INTO prieteni(id_user, id_friend) values (%d, %d);", atoi(cookiezy), atoi(userId));
        rc = sqlite3_exec(db, sqlInsert, 0, 0, &err_msg);
        

        if (rc != SQLITE_OK ) {
            if(strstr(err_msg, "UNIQUE")){
                char sqlDelete[1000] = {0};
                sprintf(sqlDelete, "DELETE FROM prieteni WHERE id_user=%d AND id_friend=%d;", atoi(cookiezy), atoi(userId));
                int rc2;
                char* err_msg2 = 0;
                rc2 = sqlite3_exec(db, sqlDelete, 0, 0, &err_msg2);

                if(rc2 != SQLITE_OK){
                    sqlite3_free(err_msg);
                    sqlite3_close(db);
                    strcat(responce, "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nfail");
                }else{
                    strcat(responce, "HTTP/1.1 200 OK\r\nContent-Length: 7\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nsuccess");
                }
            }else{
                strcat(responce, "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nfail");
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
    }else if(strcmp(requestPage, "postMsg") == 0){
        
        char message[256] = {0};
        char decodedMessege[256] = {0};
        char idTo[128] = {0};
        char responce[1024] = {0};
        char sqlInsertMessage[1000] = {0};

        
        printf("%s\n", requestHead);
        char* pch = NULL;
        pch = strtok(requestHead, "&");

        while (pch != NULL)
        {
            if(strstr(pch, "message=") != 0){
                strcat(message, pch + 8);
                URIdecode(message, decodedMessege);
            }else if(strstr(pch, "idTo=") != 0){
                strcat(idTo, pch + 5);
            }
            pch = strtok(NULL, "&");
        }

        

        
        if(!checkMessagesExistance(atoi(cookiezy), atoi(idTo)) || !(message[0] == '~')){
            time_t t = time(NULL);
            struct tm tm;
            localtime_r(&t, &tm);

            printf("%s\n", "Un mesaj dupa ce a trecut ce a trecut de time");

            sprintf(sqlInsertMessage, "INSERT INTO mesaj(mesaj, id_from, id_togrup, id_touser, receive_date) values ('%s', %d, 0, %d, '%d-%02d-%02d %02d:%02d:%02d');",
            decodedMessege, atoi(cookiezy), atoi(idTo), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

            rc = sqlite3_exec(db, sqlInsertMessage, 0, 0, &err_msg);
        
            printf("%s\n", "Un mesaj dupa ce a trecut de insert");

            if (rc != SQLITE_OK ) {
                strcat(responce, "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nfail");
                printf("%s\n", err_msg);
                sqlite3_free(err_msg);
                sqlite3_close(db);
            }else{
                strcat(responce, "HTTP/1.1 200 OK\r\nContent-Length: 7\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nsuccess");
            }
            
            printf("%s (%s)->(%s) \n", decodedMessege, cookiezy, idTo);
        }else{
            strcat(responce, "HTTP/1.1 200 OK\r\nContent-Length: 7\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nsuccess");
        }

        int writeError;
        if((writeError = write (conn_fd, responce, strlen(responce))) < 0){
            pthread_exit("crepat");
        }
        
        message[0] = '\0';
        decodedMessege[0] = '\0';
        idTo[0] = '\0';
        responce[0] = '\0';
        sqlInsertMessage[0] = '\0';
        
    }else if(strcmp(requestPage, "like") == 0){
        
        char sqlInsert[1000] = {0};
        char postId[128] = {0};
        char responce[1024] = {0};

        
         char* pch = NULL;
        pch = strtok(requestHead, "&");

        while (pch != NULL)
        {
            if(strstr(pch, "postId=") != 0){
                strcat(postId, pch + 7);
            }
            pch = strtok(NULL, "&");
        }

        //printf("%s\n", postId);

        
        sprintf(sqlInsert, "INSERT INTO like(id_user, id_postare) values (%d, %d);", atoi(cookiezy), atoi(postId));
        rc = sqlite3_exec(db, sqlInsert, 0, 0, &err_msg);
        

        if (rc != SQLITE_OK ) {
            if(strstr(err_msg, "UNIQUE")){
                char sqlDelete[1000] = {0};
                sprintf(sqlDelete, "DELETE FROM like WHERE id_user=%d AND id_postare=%d;", atoi(cookiezy), atoi(postId));
                int rc2;
                char* err_msg2 = 0;
                rc2 = sqlite3_exec(db, sqlDelete, 0, 0, &err_msg2);

                if(rc2 != SQLITE_OK){
                    sqlite3_free(err_msg);
                    sqlite3_close(db);
                    strcat(responce, "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nfail");
                }else{
                    strcat(responce, "HTTP/1.1 200 OK\r\nContent-Length: 7\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nsuccess");
                }
            }else{
                strcat(responce, "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nfail");
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
        postId[0] = '\0';
        responce[0] = '\0';
    }
    
}

void put_response_generator(int conn_fd, char* requestPage, char* requestHead, char* cookiezy){
    printf("put\n");
    printf("%s\n", requestHead);

    sqlite3 *db;
    char *err_msg = 0;
    
    int rc = sqlite3_open("ceva.db", &db);
    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", 
                sqlite3_errmsg(db));
        sqlite3_close(db);
        
        pthread_exit("Database Error");
    }

    if(strcmp(requestPage, "updateProfile") == 0){
        char updateName[128] = {0};
        char updatePreName[128] = {0};
        char updateImageURL[1024] = {0};
        char decodedImageURL[1024] = {0};
        char updateCoverURL[1024] = {0};
        char decodedCoverURL[1024] = {0};

        
        char responce[1024] = {0};

       

        char* pch = NULL;
        pch = strtok(requestHead, "&");

        while (pch != NULL)
        {
            if(strstr(pch, "firstName=") != 0){
                strcat(updatePreName, pch + 10);
            }else if(strstr(pch, "lastName=") != 0){
                strcat(updateName, pch + 9);
            }else if(strstr(pch, "coverURL=") != 0){
                strcat(updateCoverURL, pch + 9);
                URIdecode(updateCoverURL, decodedCoverURL);
            }else if(strstr(pch, "profileURL=") != 0){
                strcat(updateImageURL, pch + 11);
                URIdecode(updateImageURL, decodedImageURL);
            }
            pch = strtok(NULL, "&");

        }

        char sqlUpdate[1000] = {0};

        sprintf(sqlUpdate, "UPDATE users SET nume='%s', prenume='%s', profile_img='%s', cover_url='%s' WHERE user_id=%d", updateName, updatePreName, decodedImageURL, decodedCoverURL, atoi(cookiezy));
        
        printf("%s\n", sqlUpdate);
        
        rc = sqlite3_exec(db, sqlUpdate, 0, 0, &err_msg);

        if (rc != SQLITE_OK ) {
            strcat(responce, "HTTP/1.1 200 OK\r\nContent-Length: 4\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nfail");
            printf("%s\n%s\n", sqlUpdate, err_msg);
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

        updateName[0] = '\0';
        updatePreName[0] = '\0';
        updateImageURL[0] = '\0';
        decodedImageURL[0] = '\0';
        updateCoverURL[0] = '\0';
        decodedCoverURL[0] = '\0';
    }
}

void delete_response_generator(int conn_fd, char* requestPage, char* requestHead, char* cookiezy){
    sqlite3 *db;
    char *err_msg = 0;
    
    int rc = sqlite3_open("ceva.db", &db);
    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Cannot open database: %s\n", 
                sqlite3_errmsg(db));
        sqlite3_close(db);
        
        pthread_exit("Database Error");
    }

    if(strcmp(requestPage, "deletePost") == 0){
        //printf("%s\n%s\n", requestPage, requestHead);
        char id[10] = {0};
        char sqlDelete[128] = {0};
        char responce[1024] = {0};

        strcat(id, requestHead+3);

        sprintf(sqlDelete, "DELETE FROM postare where id_postare = %s", id);

        int rc;
        rc = sqlite3_exec(db, sqlDelete, 0, 0, &err_msg);

        if (rc != SQLITE_OK ) {
            strcat(responce, "HTTP/1.1 404 Not Found\r\nContent-Length: 4\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nfail");
            printf("%s\n%s\n", sqlDelete, err_msg);
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
        id[0] = '\0';
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

    //printf("%s(%s)\n", path, cookie);

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
            if(checkUserExistance(profile)){
                strcat(file_buff, personalizedProfilePageMaker(profile, cookie));
                write (new_socket, file_buff, strlen(file_buff));
            }else{
                response_generator(new_socket, "404user.html");
            }
        }else{
            //printf("%s\n", path);
            response_generator(new_socket, profile);
        }
        profile[0] = '\0';
    
    }else if(strstr(path, "search/")){
        //printf("Profiles page\n");
        char* search = strstr(path, "search/") + 7;
        //
         
        if((strstr(path, "search/img/") == 0) && (strstr(path, "search/css/")== 0) && (strstr(path, "search/js/")== 0)){
            //TODO: personalised profile 
            strcat(file_buff, personalizedSearchPageMaker(search, cookie));
            
            write (new_socket, file_buff, strlen(file_buff));
        }
        else{
            response_generator(new_socket, search);
        }
        search[0] = '\0';
    
    }else if(strstr(path, "edit/")){
        //printf("Profiles page\n");
        char* edit = strstr(path, "edit/") + 5;
        //
         
        if((strstr(path, "edit/img/") == 0) && (strstr(path, "edit/css/")== 0) && (strstr(path, "edit/js/")== 0)){
           
            //TODO: personalised profile 
            strcat(file_buff, personalizedEditPageMaker(edit, cookie));
           
            write (new_socket, file_buff, strlen(file_buff));
        }
        else{
            response_generator(new_socket, edit);
        }
        edit[0] = '\0';
    
    }else if(strstr(path, "messenger")){
        
        char* edit = strstr(path, "messenger") + 9;
         
        if((strstr(path, "messenger/img/") == 0) && (strstr(path, "messenger/css/")== 0) && (strstr(path, "messenger/js/")== 0)){
           
            //TODO: personalised profile
            if(cookie != NULL){
                response_generator(new_socket, "chat.html");
            }else{
                response_generator(new_socket, "403.html");
            }
        }
        else{
            response_generator(new_socket, edit);
        }
        edit[0] = '\0';
    
    }else if(strstr(path, "AJAXmsg")){
         //printf("%s\n", path);
            //TODO: personalised profile 
        strcat(file_buff, ajaxMessagePageMaker(cookie));
           
        write (new_socket, file_buff, strlen(file_buff));
    
    }else if(strcmp(path, "login") == 0){
        response_generator(new_socket, "login.html");
    
    }else{
        if(strstr(path, "js/chat.js")){
            response_generator (new_socket, "js/chat.js");
        }else{
            if((strstr(path, ".jpg") == 0) && 
            (strstr(path, ".png")== 0) && 
            (strstr(path, ".css")== 0) && 
            (strstr(path, ".js") == 0) &&
            (strstr(path, ".woff2") == 0)&&
            (strstr(path, ".gif") == 0) &&
            (strstr(path, ".html") == 0)
            ){
                response_generator (new_socket, "404.html");
            }else{
                response_generator (new_socket, path);
            }
        }
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
            }else if(strstr(buffer, "PUT ") != 0){
                put_response_generator(new_socket, path, strstr(bufferCopy, "\r\n\r\n") + 4, cookie);
            }else if(strstr(buffer, "DELETE")){
                delete_response_generator(new_socket, path, strstr(bufferCopy, "\r\n\r\n") + 4, cookie);
            }
                //response_generator (new_socket, path);
        }

        buffer[0] = '\0';
        path[0] = '\0';

        //printf("Terminated %d -> %d \n\n", orderOfTh, threadSocket[orderOfTh]);     
    }

}
