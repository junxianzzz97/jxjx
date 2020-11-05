#include    <stdio.h>
#include    <string.h>
#include    <stdlib.h>
#include    <errno.h>
#include    <netdb.h>
#include    <unistd.h>

#include    <arpa/inet.h>
#include    <sys/wait.h>
#include    <fcntl.h>
#include    <sys/sendfile.h>

#include    <sys/types.h>
#include    <sys/socket.h> //socket creat
#include    <netinet/in.h> //client server

char web[8000]="HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html><head><title>Network_prog_HW1</title></head>\r\n"
"<body><center>upload image<br>\r\n"
"<img src=\"image.jpeg\" width=\"200px\"><p>\r\n"
"<input type=\"file\" name=\"upload\"><p>\r\n"
"<input type=\"submit\" value=\"submit\">\r\n"
"</center></body></html>\r\n";

static void sigchld_handler(int sig) //kill zombie
{
    int ret;
    if(sig==SIGCHLD){
        waitpid(0,&ret,WNOHANG);
    }
}

void print(int fdsock, int k)
{
    socklen_t addrlen;
    struct sockaddr_in addr;
    switch(k){
        case 1:
            printf("\n\tServer listen address: ");
            break;
        case 2:
            printf("Connected server address: ");
            break;
    }
    getsockname(fdsock, (struct sockaddr *) &addr, &addrlen);//get addr name
    printf("%s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}

void Process(int clientSock)
{
    char filename[1024] = "DB/";
    char filetype[128];
    char buf[68888];
    char temp_filename[1024];
    char temp_index[128];
    char *temp;
    char *temp1;
    int fdimg;
    char *line;
    int len,count=0;
    FILE *fptr;
    int name_index=0;

    memset(buf, 0, sizeof(buf));
    read(clientSock, buf, sizeof(buf));

    if(strncmp(buf, "Get /image,jpeg", 16)==0){
        fdimg = open("image.jpeg", O_RDONLY);
        sendfile(clientSock, fdimg, NULL, 3000000);
        close(fdimg);
    }
    else if(strncmp(buf, "POST /", 6)==0){
        temp = strstr(buf, "filename");
        if(temp!=NULL){
            len = strlen("filename");
            temp = temp + len + 2;
            count = 3;
            while(*temp!='\"'){
                filename[count++]=*temp;
                temp++;
            }
            ++temp;
            filename[count]='\0';
            if(count>3){
                printf("receive filename:%s\n", filename);
                while(!(*(temp-4)=='\r' && *(temp-3)=='\n' && *(temp-2)=='\r' && *(temp-1)=='\n')) ++temp;

                temp1=buf+strlen(buf)-3;
                while(*temp1!='\r') --temp1;
                --temp1;
                if(access(filename, 0)<0){
                    fptr = fopen(filename, "w");
                    fwrite(temp, 1, temp1-temp+1, fptr);
                    fclose(fptr);
                }
                else{
                    name_index = 1;
                    line = strtok(filename, ".");
                    strcpy(filename, line);
                    line = strtok(NULL, ".");
                    strcpy(filetype, line);
                    while(1){
                        strcpy(temp_filename, filename);
                        sprintf(temp_index, "%d", name_index);
                        strcat(temp_filename, "_");
                        strcat(temp_filename, temp_index);
                        strcat(temp_filename, ".");
                        strcat(temp_filename, filetype);
                        if(access(temp_filename, 0)<0){
                            fptr = fopen(temp_filename,"w");
                            fwrite(temp, 1, temp1-temp+1, fptr);
                            fclose(fptr);
                            break;
                        }
                        ++name_index;
                    }
                }
            }
        }
        else printf("Not Found\n");
        write(clientSock, web, sizeof(web));
    }
    else
        write(clientSock, web, sizeof(web));
}

int main(void)
{
    int serverSock;
    struct sockaddr_in serverAddr;
    struct sigaction sa;
    int clientSock;
    int pass=1;

    serverSock = socket(AF_INET, SOCK_STREAM,0); //Socket created (domain(IPv4),type,protocol)
    if(serverSock<0){                            //type(Bit stream transmission),p(connect with kernel)
        fprintf(stderr, "Error\n");
        exit(EXIT_FAILURE);
    }

    bzero(&serverAddr, sizeof(serverAddr)); //initial serverAddr bits=0
    serverAddr.sin_family = AF_INET; //IPv4
    serverAddr.sin_port = htons(1117); //Host to Network Short int(port Num) 16bits
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); //Host to Network long int(IP addr) 32bits

    if(setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR,&pass, sizeof(int))==-1){ //no let server shutdown
        fprintf(stderr, "Error: setsockopt\n");  //relocated new socket with same ip
        exit(EXIT_FAILURE);
    }
    if(bind(serverSock, (struct sockaddr *) &serverAddr, sizeof(serverAddr))<0){
        fprintf(stderr, "Error: bind\n");    //which port to in
        exit(EXIT_FAILURE);
    }

    int Listen = listen(serverSock, 10);
    if(Listen < 0){ //how many person can connect this server
        fprintf(stderr, "Error: The server is not listening.\n");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = sigchld_handler; //kill zombie
    sigemptyset(&sa.sa_mask); //initial and clear
    sa.sa_flags = SA_RESTART; //while system interrupted can continue
    if(sigaction(SIGCHLD, &sa, NULL)==-1){
        fprintf(stderr, "Error: sigaction\n");
        exit(EXIT_FAILURE);
    }

    print(Listen, 1);
    while(1){
        clientSock = accept(serverSock, NULL, NULL);
        pid_t child = fork();
        if(child==0){
            close(Listen);
            print(clientSock, 2);
            Process(clientSock);
            close(clientSock);
            printf("connected socket is closing...\n");
            exit(0);
        }
        close (clientSock);
    }
    return 0;
}
