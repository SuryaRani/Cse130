#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <unistd.h> // write
#include <string.h> // memset
#include <stdlib.h> // atoi

#define BUFFER_SIZE 512

void put(uint8_t *buffer, ssize_t length, int clientSock)
{
    printf("GOT INTO PUt\n");
    //dprintf(clientSock, "HTTP/1.1 201 CREATED\r\n");
}

void get(int clientSock)
{
    printf("GOT INTO GET\n");
    //dprintf(clientSock, "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n");
}

void head(int clientSock)
{
    printf("GOT INTO HEAD\n");
    //dprintf(clientSock, "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n");
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

    size_t conLen = 0;
    char func[4];
    char preLen[20];

    while (1)
    {
        printf("[+] server is waiting...\n");

        int client_sockd = accept(server_sockd, &client_addr, &client_addrlen);
        // Remember errors happen

        uint8_t buff[BUFFER_SIZE + 1];
        ssize_t bytes = recv(client_sockd, buff, BUFFER_SIZE, 0);
        buff[bytes] = 0; // null terminate
        printf("[+] received %ld bytes from client\n[+] response: ", bytes);
        sscanf(buff, "%s ", func);
        sscanf(buff, "%s %zu", preLen, &conLen);
        printf("THIS IS BUFF: %s\n", buff);
        write(STDOUT_FILENO, buff, bytes);
        printf("\n");
        printf("THIS IS THE FUNCTION: %s\n", func);

        if (strcmp(func, "HEAD") == 0)
        {
            head(client_sockd);
            dprintf(client_sockd, "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n");
        }
        else if (strcmp(func, "PUT") == 0)
        {
            if (strcmp("Content-Length:", preLen) != 0)
            {
                dprintf(client_sockd, "HTTP/1.1 400 BAD REQUEST\r\n\r\n");
            }
            else
            {
                put(buff, conLen, client_sockd);
            }
        }
        else if (strcmp(func, "GET") == 0)
        {
            get(client_sockd);
            dprintf(client_sockd, "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n");
        }
        else
        {
            dprintf(client_sockd, "HTTP/1.1 400 BAD REQUEST\r\n\r\n");
        }

        //printf("THIS IS THE Sentence before content length: %s\n", func);
    }
    return 0;
}
