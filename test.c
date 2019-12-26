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
    if(strstr(filename, "profile.html")){
        strcat(header_buff, "Set-Cookie: name=Mihai; Max-Age=60000; Path=/\r\n");
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

void parsingPath(int new_socket,char* path){

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
        response_generator(new_socket, "home.html");

    }else if(strstr(path, "profiles/")){
        //printf("Profiles page\n");
        char* profile = strstr(path, "profiles/") + 9;
        //printf("%s\n", profile);
         
        if((strstr(path, "profiles/img/") == 0) && (strstr(path, "profiles/css/")== 0)){
            //TODO: personalised profile page
            response_generator(new_socket, "profile.html");

        }
        else{
            response_generator(new_socket, profile);
        }
        profile[0] = '\0';
    
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
        char responce[MAX_LEN] = {0};
        char path[100] = {0};
        char description[1024] = {0};
        char imageURL[1024] = {0};
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
            if(strstr(buffer, "GET") != 0){
                //printf("GET REQ\n\n\n");
                parsingPath(new_socket, path);
            }else{
                //printf("POST REQ\n\n\n");
                //char imageBuff[1000000] = {0};
                char* pch = NULL;
                

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
               
                parsingPath(new_socket, path);
            }
                //response_generator (new_socket, path);
        }
        buffer[0] = '\0';
        responce[0] = '\0';
        path[0] = '\0';
        description[0] = '\0';
        imageURL[0] = '\0';
        //printf("Terminated %d -> %d \n\n", orderOfTh, threadSocket[orderOfTh]);     
    }
}


