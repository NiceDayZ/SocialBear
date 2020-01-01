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
int callbackProfilePage(void *returnString, int argc, char **argv, char **azColName);
int callbackProfilePosts(void *returnString, int argc, char **argv, char **azColName);

void parse(char* line, char* actualPath)
{
    
    /* Find out where everything is */
    const char *start_of_path = strchr(line, '/') + 1;
    const char *start_of_query = strchr(start_of_path, ' ');
   

    //printf("\n\n\n\n\n\n\n\n\n%s", line);

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
        strcat(head, "<html> <head> <title>SocialBear</title> <link rel=\"stylesheet\" href=\"css/reset.css\" /> <link rel=\"stylesheet\" href=\"css/home.css\" /> <link rel=\"stylesheet\" href=\"css/post.css\" /> <link rel=\"stylesheet\" href=\"css/widget.css\" /> <link rel=\"stylesheet\" href=\"css/menu.css\" /> <link rel=\"stylesheet\" href=\"css/chat.css\" /> </head> <body> <header> <img src=\"img/header/menu-button.png\" class=\"menu_img\"/> <img src=\"img/header/SocialBear.png\" class=\"logo\"/> <input type=\"search\" placeholder=\"Search\" /> <img src=\"img/header/user-shape.png\" class=\"nav\"/> <img src=\"img/header/notifications-button.png\" class=\"nav\"/> <img src=\"img/header/conversation-speech-bubbles-.png\" class=\"nav\"/> <img src=\"img/header/burn-button.png\" class=\"nav\"/> <img src=\"img/header/musica-searcher.png\" class=\"nav_s\"/> </header> <div class=\"menu\"> <div class=\"menu_element\"> <img src=\"img/header/history-clock-button.png\" class=\"element_image\" /> <h2>My Profile</h2> </div> <div class=\"menu_element\"> <img src=\"img/header/settings-cogwheel-button.png\" class=\"element_image\" /> <h2>Edit Profile</h2> </div> <div class=\"menu_element\"> <img src=\"img/header/ellipsis.png\" class=\"element_image\" /> <h2>Login</h2> </div> </div> <div class=\"feed\"> <div class=\"posts\"> <br/><br/>");
        
        char sqlQuerryForPosts[1000] = {0};
        strcat(sqlQuerryForPosts, "select nume, prenume, grup_id, profile_img, posted_date, img_source, description from users u natural join postare p where p.grup_id = 0 ORDER BY p.posted_date DESC");
        
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
            char tokensForPost[7][1025] = {0};
            postTab = strtok(postsReturned[j], "|");
            int k = 0;
            while (postTab != NULL){
                strcat(tokensForPost[k], postTab);         
                postTab = strtok(NULL, "|");
                k++;
            }
           
            sprintf(singularPost, " <div class=\"post\"> <img class=\"profile_pic\" src=\"%s\" /> <a href=\"#\" >%s %s</a> <font>%s</font> <hr/>%s<img src=\"%s\" class=\"post_image\" /> <hr/> </div>",
            tokensForPost[3], tokensForPost[0], tokensForPost[1], tokensForPost[4], tokensForPost[6], tokensForPost[5]);
            strcat(posts, singularPost);

            singularPost[0] = '\0';
        }
        
        strcat(bottom, "</div> </div> <div class=\"chat\"> <div class=\"chat_element\"> <img src=\"img/profile/4.jpg\" class=\"element_image\" /> <h2>Full Name</h2> </div> </div> <script src=\"js/jquery.js\"></script> </body> </html>");
        
        sprintf(file_buff, "%s%s%s", head,posts,bottom);
        
        long resp_size = strlen(file_buff);

        sprintf(responce, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n%s", resp_size, file_buff);

        resp = responce;
        return resp;


    }else{
        printf("Feed for %s\n", cookie);
        return "Deocamdata atat\r\n";
    }
    
}

char* personalizedProfilePageMaker(char *profile, char *cookie){
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

        
        char sqlQuerryForPosts[1000] = {0};
        sprintf(sqlQuerryForPosts, "select nume, prenume, grup_id, profile_img, posted_date, img_source, description from users u natural join postare p where u.token = '%s' ORDER BY p.posted_date DESC", profile);
        
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
            tokensForPost[3], tokensForPost[0], tokensForPost[1], tokensForPost[4], tokensForPost[6], tokensForPost[5]);
            strcat(posts, singularPost);

            singularPost[0] = '\0';
        }

        sprintf(head, "<html> <head> <title>%s's profile</title> <meta id=\"meta\" name=\"viewport\" content=\"width=device-width; initial-scale=1.0\" /> <meta id=\"meta\" name=\"viewport\" content=\"width=device-width; initial-scale=1.0\" /> <link rel=\"stylesheet\" href=\"css/reset.css\" /> <link rel=\"stylesheet\" href=\"css/home.css\" /> <link rel=\"stylesheet\" href=\"css/profile.css\" /> <link rel=\"stylesheet\" href=\"css/post.css\" /> <link rel=\"stylesheet\" href=\"css/widget.css\" /> <link rel=\"stylesheet\" href=\"css/menu.css\" /> <link rel=\"stylesheet\" href=\"css/chat.css\" /> </head> <body> <header> <img src=\"img/header/menu-button.png\" class=\"menu_img\"/> <img src=\"img/header/SocialBear.png\" class=\"logo\"/> <input type=\"search\" placeholder=\"Search\" /> <img src=\"img/header/conversation-speech-bubbles-.png\" class=\"nav\"/> </header> <div class=\"menu\"> <div class=\"menu_element\"> <img src=\"img/header/history-clock-button.png\" class=\"element_image\" /> <h2>My Profile</h2> </div> <div class=\"menu_element\"> <img src=\"img/header/settings-cogwheel-button.png\" class=\"element_image\" /> <h2>Edit Profile</h2> </div> <div class=\"menu_element\"> <img src=\"img/header/ellipsis.png\" class=\"element_image\" /> <h2>Admin Page</h2> </div> </div> <!--Cover img--> <div class=\"profile\" style=\"background-image:url(%s);\"> <div class=\"sub_profile\"> <center> <!--profile pic--> <img src=\"%s\" class=\"profile_pic\" /><br/> <!--Name--> <h2>%s %s</h2><br/> <button class=\"btn_follow\">Follow</button> <button>Message</button> </center> </div> </div> <div class=\"feed\"> <div class=\"posts\"> <div class=\"post post_form\" style=\"padding:0;\"> <div contenteditable=\"true\"> Write Something </div> <div contenteditable=\"true\"> Image URL </div> <button class=\"post_form_submit\"></button> </div>",
        tokens[0],tokens[3],tokens[2],tokens[0],tokens[1]);

       
        //sprintf(posts)

        strcat(bottom, "<div class=\"chat\"> <div class=\"chat_element\"> <img src=\"img/profile/4.jpg\" class=\"element_image\" /> <h2>Full Name</h2> </div> </div> <script src=\"js/jquery.js\"></script> </body> </html>");
        
        sprintf(file_buff, "%s%s%s", head,posts,bottom);
        
        long resp_size = strlen(file_buff);

        sprintf(responce, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n%s", resp_size, file_buff);

        resp = responce;
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
        printf("CEVAAAA\n\n");
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
        strcpy (header_buff, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nContent-Type: text/plain\r\n");       
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

     /* if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    } */

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
            strcat(file_buff, personalizedProfilePageMaker(profile, NULL));
            
            write (new_socket, file_buff, strlen(file_buff));
        }
        else{
            response_generator(new_socket, profile);
        }
        profile[0] = '\0';
    
    }else if(strcmp(path, "login") == 0){
        
        response_generator(new_socket, "login.html");
    
    }else{
       // printf("Orice altceva\n");
       // printf("%s\n", path);
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
            parse(buffer, path);
            if(strstr(buffer, "GET ") != 0){
                //GET REQUEST
                //TODO: ADD cookies
                parsingPath(new_socket, path, NULL);
            }else if(strstr(buffer, "POST ") != 0){
                //POST REQUEST
                /* char* pch = NULL;
                pch = strtok(buffer, "\r\n");

                while (pch != NULL)
                {
                    if(strstr(pch, "text_desc=") != 0){
                        strcpy(description, pch + 10);
                        printf("%s\n", description);
                    }else if(strstr(pch, "image_url=") != 0){
                        strcpy(imageURL, pch + 10);
                        printf("%s\n", imageURL);
                    }
                    pch = strtok(NULL, "\r\n");
                }
               //TODO add session cookie
                parsingPath(new_socket, path, NULL); */
                printf("%s\n\n\n", buffer);
                response_generator(new_socket, "home.html");

            }
                //response_generator (new_socket, path);
        }

        buffer[0] = '\0';
        path[0] = '\0';

        //printf("Terminated %d -> %d \n\n", orderOfTh, threadSocket[orderOfTh]);     
    }

}
