#include <stdlib.h> 
#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX 100
#define PORT 8002 
#define SA struct sockaddr

void itoa(char *buf, int number)
{
    if (number == 0) {
        buf[0] = '0';
        return;
    }

    char temp[10] = { 0 };
    int i = 0;

    while (number > 0) {
        temp[i++] = '0' + number % 10;
        number /= 10;
    }

    //reverse the number
    for (int i = 0, j = strlen(temp) - 1; j >= 0; i++, j--) {
        buf[i] = temp[j];
    }
}

long int findSize(const char *file_name) {
    int size = -1;
    FILE *fp = fopen(file_name, "rb");
    if (fp != NULL) {
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        rewind(fp);
    }
    fclose(fp);
    return size;
}

void downloadFile(int sockfd, const char *file_name) { 
    char buff[1025] = { 0 };     
    FILE *fp;

    int size = findSize(file_name);
    if (size < 0) {
        size = 0;
    }
    itoa(buff, size);
    printf("Size: %d\n", size);
    printf("Buff: %s\n", buff);
    send(sockfd, buff, strlen(buff), 0);
    bzero(buff, sizeof(buff));

    fp = fopen(file_name, "rb");
    if (!fp) {
        printf("Error Opening File..\n");
        return ;
    }

    char msg[10000] = { 0 };
    while (fgets(buff, sizeof(buff), fp)) {
        strcat(msg, buff);
        bzero(buff, sizeof(buff));
    }
    send(sockfd, msg, strlen(msg), 0);
    fclose(fp);  
    
    printf("File Sent !!! \n");
    
}

void uploadFile(int sockfd, char *file_name) { 
    char buff[1024];
    char *data;
    int size;

    recv(sockfd, buff, sizeof(buff), 0);
    size = atoi(buff);
    memset(buff, 0, 1024);

    recv(sockfd, buff, sizeof(buff), 0);
    data = strdup(buff);
    memset(buff, 0, 1024);

    FILE *fp = fopen(file_name, "w");
    if( fp == NULL ){
        printf("Error Creating File..\n");
        return;
    }
    fprintf(fp, "%s", data);
    free(data);
    fclose (fp);
    printf("File Upload successfully !!! \n");
}

void listFiles(int sockfd)
{
    char buffer[1024] = { 0 };
    char msg[10000] = { 0 };

    system("ls > temp");

    int size = findSize("temp");
    if (size < 0) {
        size = 0;
    }
    itoa(buffer, size);

    printf("Size: %d\n", size);
    printf("Buff: %s\n", buffer);

    send(sockfd, buffer, sizeof(buffer), 0);
    bzero(buffer, sizeof(buffer));

    FILE *fp = fopen("temp", "r");
    if (!fp) {
        printf("Error opening file.\n");
    }
    while (fgets(buffer, sizeof(buffer), fp)) {
        strcat(msg, buffer);
        bzero(buffer, sizeof(buffer));
    }
    printf("Msg: %s\n", msg);
    system("wc -c temp");
    fclose(fp);
    send(sockfd, msg, strlen(msg), 0);
    system("echo Content; cat temp");
    unlink("temp");
}

void command_handler(int sockfd, char *cmd)
{
    char *inst;
    char *filename;

    inst = strstr(cmd, "get");
    if (inst && (inst - cmd == 0)) {
        filename = cmd + 3;
        downloadFile(sockfd, filename);
    } else {
        inst = strstr(cmd, "put");
        if (inst && (inst - cmd == 0)) {
            filename = cmd + 3;
            uploadFile(sockfd, filename);
        } else {
            if (strcmp(cmd, "ls") == 0) {
                listFiles(sockfd);
            } else {
                printf("Error handling commands.\n");
                printf("Last command: [%s]\n", cmd);
                return;
            }
        }
    }
}

int main() { 
    int sockfd; 
    struct sockaddr_in servaddr, cli; 
    socklen_t len = sizeof(cli);

    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd < 0) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } else
        printf("Socket successfully created..\n"); 
        
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 

    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) < 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 


    if (listen(sockfd, 5) < 0) { 
        printf("Error listening\n");
    }

    while (1) {
        int clientfd = accept(sockfd, (SA*)&cli, &len);
        if (clientfd < 0) {
            printf("Error accepting.\n");
        }

        while (1) {
            char command[1024] = { 0 };
            int len = recv(clientfd, command, sizeof(command), 0);
            if (len <= 0) {
                printf("client disconnected.\n");
                close(clientfd);
                break;
            }
            command_handler(clientfd, command);
        }
    }
}
