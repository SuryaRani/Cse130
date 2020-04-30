#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <unistd.h> // write
#include <string.h> // memset
#include <stdlib.h> // atoi
#include <ctype.h>
#include <stdio.h>
#include <errno.h>

#define BUFFER_SIZE 4097

void put(uint8_t *buffer, int length, int clientSock, char *file)
{
    printf("GOT INTO PUt\n");
    printf("This is the file: %s", file);
    /*uint8_t readFile[length];
    if (buffer.length < 4097)
    {
        write
    }
    size_t rd = read(op, readFile, length);
    if (rd != 0)
    {
        if (rd == 10)
        {
            //need to figure out how to not print out again with the last buffer space
            int w = write(STDOUT_FILENO, rdBuf, 10);
            size_t keepRead = read(op, rdBuf, 10);
            while (keepRead == 10 && w == 10)
            {
                w = write(STDOUT_FILENO, rdBuf, 10);
                keepRead = read(op, rdBuf, 10);
            }
            if (keepRead < 10 && keepRead > 0)
            {
                w = write(STDOUT_FILENO, rdBuf, keepRead);
            }
            //printf("\n");
        }
        else
        {
            int w = write(STDOUT_FILENO, rdBuf, rd);
            if (w < 0)
            {
            }
        }
    }*/
    dprintf(clientSock, "HTTP/1.1 201 CREATED\r\nContent-Length: 0\r\n\r\n");
}

void get(int clientSock, char *file)
{
    printf("This is the file: %s", file);
    int op = open(file, O_RDONLY);
    uint8_t rd[1000];
    if (op >= 0)
    {
        printf("GOT INTO GET\n");
        size_t reading = read(op, rd, 1000);
        if (reading != 0)
        {
            if (reading == 1000)
            {
            }
            else
            {
                size_t sending = send(clientSock, rd, reading, 0);
            }
        }

        dprintf(clientSock, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", reading);
    }
    else
    {
        int err = errno;
        if (err == ENOENT)
        {
            dprintf(clientSock, "HTTP/1.1 404 File Not Found\r\n");
        }
        else if (err == EACCES)
        {
            dprintf(clientSock, "HTTP/1.1 403 File Forbidden\r\n");
        }
        else
        {
            dprintf(clientSock, "HTTP/1.1 500 Internal Server Error\r\n");
        }
    }
}

void head(int clientSock, char *file)
{
    printf("GOT INTO HEAD\n");
    dprintf(clientSock, "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n");
}

int parse(uint8_t *buffer)
{
    //return a number if i can get the content length
    // if not then
}

int main(int argc, char **argv)
{
    /*
        Create sockaddr_in with server information
    */
    char *port = "8080";
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t addrlen = sizeof(server_addr);

    /*
        Create server socket
    */
    int server_sockd = socket(AF_INET, SOCK_STREAM, 0);

    // Need to check if server_sockd < 0, meaning an error
    if (server_sockd < 0)
    {
        perror("socket");
    }

    /*
        Configure server socket
    */
    int enable = 1;

    /*
        This allows you to avoid: 'Bind: Address Already in Use' error
    */
    int ret = setsockopt(server_sockd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    /*
        Bind server address to socket that is open
    */
    ret = bind(server_sockd, (struct sockaddr *)&server_addr, addrlen);

    /*
        Listen for incoming connections
    */
    ret = listen(server_sockd, 5); // 5 should be enough, if not use SOMAXCONN

    if (ret < 0)
    {
        return 1;
    }

    /*
        Connecting with a client
    */
    struct sockaddr client_addr;
    socklen_t client_addrlen;

    char func[5];
    int conLen = 0;
    char preLen[20];
    char *token;
    char fileName[50];

    while (1)
    {
        printf("[+] server is waiting...\n");

        int client_sockd = accept(server_sockd, &client_addr, &client_addrlen);
        // Remember errors happen

        uint8_t buff[BUFFER_SIZE + 1];
        ssize_t bytes = recv(client_sockd, buff, BUFFER_SIZE, 0);
        buff[bytes] = 0; // null terminate

        //parsing algorithm
        token = strtok(buff, "\r\n");
        sscanf(token, "%s %s ", func, fileName);
        //checks if first character is a / or not
        if (fileName[0] == '/')
        {
            memmove(fileName, fileName + 1, strlen(fileName));
        }
        printf("This is filenme: %s\n", fileName);
        printf("also this is fiirst char: %c", fileName[0]);
        //check if filename violates any of the rules
        // first check the size make sure it is less than or equal to 27 characters

        if (strlen(fileName) > 27)
        {
            dprintf(client_sockd, "HTTP/1.1 400 BAD REQUEST\r\n\r\n");
            //printf("in filename siz\n");
            close(client_sockd);
        }

        //check if filename is alphanumeric or contains -, _
        for (int i = 0; i < strlen(fileName); i++)
        {
            if (!((isalpha(fileName[i])) != 0 || (isdigit(fileName[i]) != 0) || fileName[i] == '-' || fileName[i] == '_'))
            {
                dprintf(client_sockd, "HTTP/1.1 400 BAD REQUEST\r\n\r\n");
                //printf("IN Alpha num\n");
                close(client_sockd);
            }
        }
        //to move onto the next token
        token = strtok(NULL, "\r\n");
        char header1[40];
        char header2[40];
        // this tokenizes each part of the buffer by \r\n to seperate headers
        // make sure that each header follows the right conventions or else throw an erroor
        while (token != NULL)
        {
            // this is to check if it is giving content length in request
            if (sscanf(token, "Content-Length: %d", &conLen) > 0)
            {
                printf("This is content length: %d\n", conLen);
            }
            // this is checking if its a normal header and if it is we just ignore it
            else if (sscanf(token, "%s %s", header1, header2) == 2)
            {
                if (header1[strlen(header1) - 1] != ':')
                {
                    dprintf(client_sockd, "HTTP/1.1 400 BAD REQUEST\r\n\r\n");
                }
            }
            // if it contains another \r\n in the string that means its the end of the request and might have data after it
            // we must read the data after if it is a put
            else
            {
                dprintf(client_sockd, "HTTP/1.1 400 BAD REQUEST\r\n\r\n");
            }
            printf("Token: %s\n", token);
            token = strtok(NULL, "\r\n");
        }

        printf("[+] received %ld bytes from client\n[+] response: ", bytes);
        //sscanf(buff, "%s ", func);

        printf("THIS IS BUFF: %s\n", buff);
        write(STDOUT_FILENO, buff, bytes);
        printf("\n");
        printf("THIS IS THE FUNCTION: %s\n", func);
        //sscanf(buff, " %s %d", preLen, &conLen);

        if (strcmp(func, "HEAD") == 0)
        {
            head(client_sockd, fileName);
            close(client_sockd);
            //dprintf(client_sockd, "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n");
        }
        else if (strcmp(func, "PUT") == 0)
        {

            //there might be an error here since we are not checking if the words before the content length is exactly content length
            // i think well be fine because of the sscanf which has that string in the formating
            put(buff, conLen, client_sockd, fileName);
            close(client_sockd);
        }
        else if (strcmp(func, "GET") == 0)
        {
            get(client_sockd, fileName);
            close(client_sockd);
            //dprintf(client_sockd, "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n");
        }
        else
        {
            dprintf(client_sockd, "HTTP/1.1 400 BAD REQUEST\r\n\r\n");
            close(client_sockd);
        }
    }
    return 0;
}
