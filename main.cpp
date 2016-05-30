//
//  main.cpp
//  socketwebserver
//
//  Created by hyz on 16/3/12.
//  Copyright © 2016年 hyz. All rights reserved.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>
#include <regex.h>
#include <dirent.h>
#include <time.h>

#define MYPORT "3490"
#define BACKLOG 10  // limitation number for the queue
#define MAXLEN 1024

//int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
// node is the host name to connect to, service is a number or a name of service(http), hints para points to a struct addrinfo that are already filled out

// int socket(int domain, int type, int protocal);
// domain is PF_INET or PF_INET6
// type is SOCK_STREAM or SOCK_DGRAM, protocal set to 0 to auto choose the proper protocal for given type
// actually AF_INET == PF_INET
// AF -> address family  PF -> protocal family


// int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
// sockfd is the socket descriptor return by the socket() function, my_addr is a pointer to a sturct sockaddr contains info about your addr(port and ip), addrlen refer to the length in bytes of that addr.

// int connect(int sockfd, struct sockaddr *serv_addr, int addrlen);
// serv_addr contain the destination info like ip and port

void sigchld_handler(int s) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    
    errno = saved_errno;
}

int regxMatch(char *ori, char *matcher) {
    regex_t regex;
    int reti;
    char msgbuf[100];
    
    reti = regcomp(&regex, matcher, 0);
    if (reti) {
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }
    
    reti = regexec(&regex, ori, 0, NULL, 0);
    
    if (!reti) {
        // match the result!
        return 1;
    }
    else if (reti == REG_NOMATCH) {
        // not match
        return 0;
    }
    else {
        regerror(reti, &regex, msgbuf, sizeof(msgbuf));
        fprintf(stderr, "Regex match failed: %s\n", msgbuf);
        return -1;
    }
    
    regfree(&regex);
}

// 权限设置 1 - Yes  0 - Not Match  -1 - Regular expression match error
int authorityBound(char *filePath) {
     regxMatch(filePath, "Desktop/TestFolder");   
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// parse a file into a string
char* fileToString(char *filename) {
    FILE *fp = fopen(filename, "r");
    int size = 0;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);
    char *out = (char*)malloc(size + 1);
    char tmp[size + 1];
    int i = 0;
    char a; 
    while ((a = fgetc(fp)) != EOF) {
        tmp[i++] = a;
    }
    fclose(fp);
    tmp[size] = '\0';
    strcpy(out, tmp);
    return out;
}

void sendImg(int socket, char *filepath) {
    FILE *fp = fopen(filepath, "rb");
    int size = 0;
    fseek(fp, 0, SEEK_END); 
    size = ftell(fp);
    rewind(fp);
    int rc = 0;
    char *res = (char*)malloc(size + 1);
    char *head = "HTTP/1.1 200 OK\nContent-Type: image/jpeg\n";
    char *content_length = (char *)malloc(MAXLEN);
    sprintf(content_length, "%s%b", "Content-Length:", size); 
    printf(content_length);
    char *out = (char*)malloc(strlen(content_length) + strlen(head) + 2);
    sprintf(out, "%s%s%s", head, content_length, "\n\n");
    send(socket, out, strlen(out), 0);
    while((rc = fread(res, sizeof(unsigned char), size, fp)) != 0) {
        send(socket, res, size, 0);
    }
 
}   


void sendFile(int socket, char *filepath) {
    FILE *fp = fopen(filepath, "r");
    int fileSize = 0;
    char *line = NULL;
    size_t len = 0;
    ssize_t read = 0;

    printf("%s\n", filepath);
    if (fp == 0) {
       // not found situation here 
        char header[] = "HTTP/1.1 404 NOT FOUND\nContent-Type: text/html\n\n<HTML><HEAD><TITLE>Not Found lukas</TITLE></HEAD><BODY>Not Found</BODY></HTML>\r\n";
        send(socket, header, sizeof(header), 0);
    } else {
        if (regxMatch(filepath, "jpg")){	    
            sendImg(socket, filepath); 
        } else {
            char header[] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";
            send(socket, header, sizeof(header), 0);
            char *buf = fileToString(filepath);         // put the file content into a string
            send(socket, buf, strlen(buf), 0);
        }
    }
}

// read Directory
DIR* getFileStrcture(char *filePath) {
    int len;
    struct dirent *pDirent;
    DIR *pDir;

    pDir = opendir(filePath);// open the directory
    
    if (pDir == NULL) {
        printf("Cannot open the directory");
    }

    return pDir;
}

char * linkProducer(char *mm) {
    char header[] = "/Users/owner/Desktop/TestFolder/";
    char *res = (char*)malloc(strlen(mm) * 2 + 16 + sizeof(header) + 22);
    sprintf(res, "%s%s%s%s%s%s", "<tr width='500px' height='30px' style='background-color: #ee9f12'><td><a style= 'color: #f3f1f5;text-decoration:none; font-size=14px ' href=\"", header,mm, "\">",mm,"</a></td></tr>");
    return res;
}

void sendMainPage(int socket) {
    char *head_part =  "<html><head><title>Home Page</title></head><body style='background-color: #0f405b'><h1 align='center' style='color: #ffffff;padding-top: 50px'>This is Home Page</h1><h2 style='color: #ffffff;padding-top: 50px; padding-left:300px'>Available File</h2><table width='450px' style='text-align: center;padding-left: 300px; padding-top: 30px'>";

    send(socket, head_part, strlen(head_part), 0);

    struct dirent *pDirent;
    
    DIR *pDir = getFileStrcture("/Users/owner/Desktop/TestFolder");

    char *hide = ".";
    while((pDirent = readdir(pDir)) != NULL) {
        if ((strstr(pDirent->d_name, hide) - pDirent->d_name) == 0) {
            continue; 
        }
        char *res = linkProducer(pDirent->d_name);
        send(socket, res, strlen(res), 0);
    }    
    send(socket, "</table></html>\n</body>",23,0);   
}

// record log info
void Save_Loginfo(char *s, char *method, char *filepath, char *httpversion){
    
    FILE *fp=fopen("/Users/owner/Desktop/TestFolder/logInfo.txt","a");
    
    time_t t = time(0); 
    
    char tmp[64]; 
    
    strftime(tmp, sizeof(tmp), "%Y/%m/%d %X %A",localtime(&t)); 
    fprintf(fp,"| Date: %s\n", tmp);
    fprintf(fp,"| Server: Got connection from %s\n",s);
    fprintf(fp,"| Http Version : %s\n",httpversion);
    fprintf(fp,"| File Path : %s\n",filepath);
    fprintf(fp,"| Method : %s\n",method);
    fprintf(fp,"-----------------------------------------------------\n");    
    fclose(fp);
    return;
}

int main(int argc, char *argv[]) {
    struct sockaddr_storage their_addr; // store the incoming address
    socklen_t sin_size;
    struct addrinfo hints, *servinfo, *p;
    int sockfd, new_fd;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv; // status code
    char recbuf[2049];
    char *method;
    char *filepath;
    char *httpversion;
    char *sendContent;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // my own IP
    
    if((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n",gai_strerror(rv));
        return 1;
    }
    
    for (p = servinfo;p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server socket");
            continue;
        }
        
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        
        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        
        break;
    }
    
    freeaddrinfo(servinfo);   
    
    if ((listen(sockfd, BACKLOG)) == -1) {
        perror("listen");
        exit(1);
    }
    
    // this function is used to clean socket on ports
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    printf("server: waiting for connections... \n");
    
    while (1) {
        sin_size = sizeof(their_addr);
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr), s, sizeof(s));
            printf("-----------------------------------------------------\n");
            printf("| Server: got connection from %s\n", s);

            if (!fork()) {
                close(sockfd);
                recv(new_fd, recbuf, sizeof(recbuf), 0);
                // split the receing string
                method = strtok(recbuf, " ");  
                
                filepath = strtok(NULL, " "); 
                
                httpversion = strtok(NULL, " ");    // version of the http

                printf("| Method: %s\n| FilePath: %s\n| HTTPVERSION: %s\n", method, filepath, httpversion);
                printf("-----------------------------------------------------\n");
            
            Save_Loginfo(s, method, filepath, httpversion);

            if (strcmp(filepath, "/") == 0){
                sendMainPage(new_fd);  
            }else if(authorityBound(filepath) != 1) {
                send(new_fd, "Sorry, invalid Path [Users/owner/Desktop/TestFolder].." , 54, 0);
            }
            else {
                // send file 
                sendFile(new_fd, filepath);
            }
                
            close(new_fd);
            exit(0);
        }
        
        close(new_fd);
    }
    
    return 0;
}
