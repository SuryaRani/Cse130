#include <err.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * client_connect takes a port number and establishes a connection as a client.
 * connectport: port number of server to connect to
 * returns: valid socket if successful, -1 otherwise
 */
int client_connect(uint16_t connectport)
{
    int connfd;
    struct sockaddr_in servaddr;

    connfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connfd < 0)
        return -1;
    memset(&servaddr, 0, sizeof servaddr);

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(connectport);

    /* For this assignment the IP address can be fixed */
    inet_pton(AF_INET, "127.0.0.1", &(servaddr.sin_addr));

    if (connect(connfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        return -1;
    return connfd;
}

/*
 * server_listen takes a port number and creates a socket to listen on 
 * that port.
 * port: the port number to receive connections
 * returns: valid socket if successful, -1 otherwise
 */
int server_listen(int port)
{
    int listenfd;
    int enable = 1;
    struct sockaddr_in servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
        return -1;
    memset(&servaddr, 0, sizeof servaddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
        return -1;
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof servaddr) < 0)
        return -1;
    if (listen(listenfd, 500) < 0)
        return -1;
    return listenfd;
}

/*
 * bridge_connections send up to 100 bytes from fromfd to tofd
 * fromfd, tofd: valid sockets
 * returns: number of bytes sent, 0 if connection closed, -1 on error
 */
int bridge_connections(int fromfd, int tofd)
{
    char recvline[100];
    int n = recv(fromfd, recvline, 100, 0);
    if (n < 0)
    {
        printf("connection error receiving\n");
        return -1;
    }
    else if (n == 0)
    {
        printf("receiving connection ended\n");
        return 0;
    }
    recvline[n] = '\0';
    printf("%s", recvline);
    sleep(1);
    n = send(tofd, recvline, n, 0);
    if (n < 0)
    {
        printf("connection error sending\n");
        return -1;
    }
    else if (n == 0)
    {
        printf("sending connection ended\n");
        return 0;
    }
    return n;
}

/*
 * bridge_loop forwards all messages between both sockets until the connection
 * is interrupted. It also prints a message if both channels are idle.
 * sockfd1, sockfd2: valid sockets
 */
void bridge_loop(int sockfd1, int sockfd2)
{
    fd_set set;
    struct timeval timeout;

    int fromfd, tofd;
    while (1)
    {
        // set for select usage must be initialized before each select call
        // set manages which file descriptors are being watched
        FD_ZERO(&set);
        FD_SET(sockfd1, &set);
        FD_SET(sockfd2, &set);

        // same for timeout
        // max time waiting, 5 seconds, 0 microseconds
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        // select return the number of file descriptors ready for reading in set
        switch (select(FD_SETSIZE, &set, NULL, NULL, &timeout))
        {
        case -1:
            printf("error during select, exiting\n");
            return;
        case 0:
            printf("both channels are idle, waiting again\n");
            continue;
        default:
            if (FD_ISSET(sockfd1, &set))
            {
                fromfd = sockfd1;
                tofd = sockfd2;
            }
            else if (FD_ISSET(sockfd2, &set))
            {
                fromfd = sockfd2;
                tofd = sockfd1;
            }
            else
            {
                printf("this should be unreachable\n");
                return;
            }
        }
        if (bridge_connections(fromfd, tofd) <= 0)
            return;
    }
}

char *getHealth(int fd)
{
    char healthMsg[100];
    sprintf(healthMsg, "GET /healthcheck HTTP/1.1\r\nHost: localhost:%d\r\nUser-Agent: curl/7.58.0\r\nAccept:*/*\r\n\r\n", fd);
    send(fd, healthMsg, sizeof(healthMsg) / sizeof(char), 0);
    char *recieved = malloc(1000);
    ssize_t bytes = recv(fd, recieved, 1000, 0);
    if (bytes == -1)
    {
        return "FAIL";
    }
    recieved[bytes] = 0;
    return recieved;
}

void parseHealth(int err[], int req[], int count, int fd)
{
    char *healthMsg = getHealth(fd);
    if (strcmp(healthMsg, "FAIL") == 0)
    {
        err[count] = -1;
        req[count] = -1;
    }
    //this might fail because im not checking for format of health check implement this later
    char *tok = strtok(healthMsg, "\r\n");
    err[count] = atoi(tok);
    tok = strtok(NULL, "\r\n");
    req[count] = atoi(tok);
}
int prioritize(int err[], int req[], int health[])
{
    int least = __INT_MAX__;
    int itemNum = -1;
    int success = -1;
    for (int i = 0; i < sizeof(req[i]) / sizeof(int); i++)
    {
        if (req[i] < least)
        {
            itemNum = i;
            least = req[i];
            success = req[i] - err[i];
        }
        else if (req[i] == least)
        {
            if ((req[i] - err[i]) < success)
            {
                itemNum = i;
                least = req[i];
                success = req[i] - err[i];
            }
        }
    }
    return itemNum;
}

int checkHealth(int health[], int serv[])
{
    int err[1000];
    int req[1000];
    for (int i = 0; i < sizeof(serv[i]) / sizeof(int); i++)
    {
        parseHealth(err, req, i, serv[i]);
        if (err[i] == -1 && req[i] == -1)
        {
            health[i] = -1;
        }
        else
        {
            health[i] = 1;
        }
    }
    int priority = -1;
    priority = prioritize(err, req, health);
    return priority;
}

int main(int argc, char **argv)
{
    int connfd, listenfd, acceptfd;
    int threads = -1;
    int reqHealth = -1;
    int ports[argc - 2];
    int counter = 0;
    uint16_t connectport, listenport;

    if (argc < 3)
    {
        printf("missing arguments: usage %s port_to_connect port_to_listen", argv[0]);
        return 1;
    }
    //this is to parse through the arguments
    listenport = atoi(argv[1]);
    for (int i = 2; i < argc; i++)
    {
        if (atoi(argv[i]) != 0)
        {
            if (strcmp(argv[i - 1], "-R") == 0)
            {
                reqHealth = atoi(argv[i]);
            }
            else if (strcmp(argv[i - 1], "-N") == 0)
            {
                threads = atoi(argv[i]);
            }
            else
            {
                if (atoi(argv[i]) < 1024)
                {

                    dprintf(STDERR_FILENO, "Port number must be above 1024\n");
                    return EXIT_FAILURE;
                }
                ports[counter] = atoi(argv[i]);
                counter++;
            }
        }
    }
    if (counter == 0)
    {
        dprintf(STDERR_FILENO, "Must include at least one server port to connect to\n");
        return EXIT_FAILURE;
    }

    // Remember to validate return values
    // You can fail tests for not validating
    int servers[counter];
    //int clients[counter];
    int health[counter];
    for (int i = 0; i < counter; i++)
    {

        connectport = ports[i];

        if ((connfd = client_connect(connectport)) < 0)
            err(1, "failed connecting");
        if ((listenfd = server_listen(listenport)) < 0)
            err(1, "failed listening");
        if ((acceptfd = accept(listenfd, NULL, NULL)) < 0)
            err(1, "failed accepting");

        servers[i] = connfd;
        //clients[i] = acceptfd;

        // This is a sample on how to bridge connections.
        // Modify as needed.
    }
    int priority = 0;
    priority = checkHealth(health, servers);

    bridge_loop(acceptfd, servers[priority]);
}