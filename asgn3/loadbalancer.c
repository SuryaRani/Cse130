#include <err.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/time.h>

typedef struct bridge_t
{
    int client;
    int server;

} bridge;
typedef struct server_info_t
{
    int port;
    int fd;
    int clientfd;
    bool alive;
    int totalReq;
    int errors;
    pthread_t workId;
} ServerInfo;

typedef struct servers_t
{
    int numServ;
    ServerInfo *servs;
    //pthread_mutex_t mut;
} Servers;

Servers servers;
int priority = 0;

int q[1000];
ssize_t qSize;
int front = 0;
int tail = 0;

bool wait = false;
bool canStart = false;

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t healthMut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t healthCond = PTHREAD_COND_INITIALIZER;
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
    //sleep(1);
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
void bridge_loop(int first, int second)
{
    printf("DO i go into bridge\n");

    int sockfd1 = first;
    int sockfd2 = second;
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

void *work(void *obj)
{
    printf("HoW MANY TIMES I GET HERE\n");
    bridge *b_pointer = (bridge *)obj;
    bridge b = *b_pointer;
    printf("MAybe I ASSUME\n");

    pthread_mutex_lock(&mut);
    pthread_cond_wait(&cond, &mut);

    printf("THIS IS HERE I ASSUME\n");
    b.client = q[front];
    if (front == 999)
    {
        front = 0;
    }
    else
    {
        front++;
    }
    b.server = servers.servs[priority].fd;
    // i need to wait longer here just for the first time throuhg to be able to prioritize
    printf("THis is client: %d\n", b.client);
    printf("THis is server: %d\n", b.server);
    while (canStart == false)
    {
    }

    bridge_loop(b.client, b.server);

    pthread_mutex_unlock(&mut);
}

char *getHealth(int fd, int port)

{
    printf("Here 9\n");
    char healthMsg[100];
    sprintf(healthMsg, "GET /healthcheck HTTP/1.1\r\n\r\n");
    send(fd, healthMsg, sizeof(healthMsg) / sizeof(char), 0);
    char *recieved = malloc(1000);
    printf("Here 10\n");
    ssize_t bytes = recv(fd, recieved, 1000, 0);
    if (bytes == -1)
    {
        printf("ITS FAILING\n");
        return "FAIL";
    }
    recieved[bytes] = 0;
    printf("this is recieved: %s\n", recieved);
    printf("Here 11\n");
    return recieved;
}

void parseHealth(int i)
{
    printf("Here 8\n");
    char *healthMsg = getHealth(servers.servs[i].fd, servers.servs[i].port);
    printf("Here 12\n");
    if (strcmp(healthMsg, "FAIL") == 0)
    {
        servers.servs[i].errors = -1;
        servers.servs[i].totalReq = -1;
    }
    //this might fail because im not checking for format of health check implement this later
    char *tok = strtok(healthMsg, "\n");
    int err = -1;
    int tot = -1;
    tok = strtok(NULL, "\n");
    tok = strtok(NULL, "\n");
    tok = strtok(NULL, "\n");

    if (tok != NULL)
    {
        printf("TOK 1: %s\n", tok);
        sscanf(tok, "%d", &err);
        tok = strtok(NULL, "\n");

        if (tok != NULL)
        {
            printf("TOK 2: %s\n", tok);
            sscanf(tok, "%d", &tot);

            if (err == -1 || tot == -1)
            {
                servers.servs[i].alive = false;
            }

            printf("THIS IS ERRORS: %d\n", err);
            printf("THIS IS ToTAL: %d\n", tot);
            printf("Here 13\n");
            servers.servs[i].errors = err;
            printf("Here 14\n");

            printf("Here 15\n");
            servers.servs[i].totalReq = tot;
        }
    }

    printf("Here 16\n");
}
int prioritize()
{

    int least = __INT_MAX__;
    int itemNum = -1;
    int success = -1;
    for (int i = 0; i < servers.numServ; i++)
    {
        if (servers.servs[i].totalReq < least)
        {
            itemNum = i;
            least = servers.servs[i].totalReq;
            success = servers.servs[i].totalReq - servers.servs[i].errors;
        }
        else if (servers.servs[i].totalReq == least)
        {
            if ((servers.servs[i].totalReq - servers.servs[i].errors) < success)
            {
                itemNum = i;
                least = servers.servs[i].totalReq;
                success = servers.servs[i].totalReq - servers.servs[i].errors;
            }
        }
    }
    return itemNum;
}

void checkHealth()
{
    printf("Here 6\n");
    for (int i = 0; i < servers.numServ; i++)
    {

        parseHealth(i);
        printf("Here 7\n");
        // i might need to check if server is alive earlier than this maybe right when connecting
        if (servers.servs[i].errors == -1 && servers.servs[i].totalReq == -1)
        {
            servers.servs[i].alive = false;
        }
        else
        {
            servers.servs[i].alive = true;
        }
    }
}

void *timedHealth(void *obj)
{
    int numRequests = (int)obj;

    struct timespec ts;
    struct timeval now;
    while (true)
    {
        pthread_mutex_lock(&healthMut);
        memset(&ts, 0, sizeof(ts));

        gettimeofday(&now, NULL);
        ts.tv_sec = now.tv_sec + 5;
        printf("IM IN HEALTH\n");
        if (wait == false)
        {
            pthread_cond_wait(&healthCond, &healthMut);
        }
        else
        {
            pthread_cond_timedwait(&healthCond, &healthMut, &ts);
        }

        printf("IT HAS BEEN 5 seconds\n");

        checkHealth();
        priority = prioritize();
        canStart = true;
        printf("THIS IS PRITIORTY in health %d\n", priority);

        pthread_mutex_unlock(&healthMut);
    }
}
void serverInit(int counter, int ports[])
{
    servers.numServ = counter;
    servers.servs = malloc(servers.numServ * sizeof(ServerInfo));

    for (int i = 0; i < counter; i++)
    {
        servers.servs[i].port = ports[i];
        servers.servs[i].alive = false;
    }
}

int main(int argc, char **argv)
{
    int connfd, listenfd, acceptfd;
    int threads = 4;
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
    //int servers[counter];
    //int clients[counter];

    serverInit(counter, ports);
    pthread_t threadId;

    printf("NOT GETTInG HERE\n");
    //printf("servers: %d\n", servers.servs[priority].port);
    bridge bridge;
    bridge.client = -1;
    bridge.server = -1;

    //bridge.client = acceptfd;
    //bridge.server = servers.servs[priority].fd;

    //printf("THis is client before : %d\n", bridge.client);
    //printf("THis is server before: %d\n", bridge.server);
    int erro = 0;
    /*for (int i = 0; i < threads; i++)
    {
        printf("loop: %d\n", i);
        erro = pthread_create(&servers.servs[i].workId, NULL, work, &bridge);
        if (erro)
        {
            dprintf(STDERR_FILENO, "ERROR setting up thread\n");
            return EXIT_FAILURE;
        }

    //pthread_join(servers.servs[i].workId, NULL);
}*/
    printf("BEFORE LISTEN\n");
    if ((listenfd = server_listen(listenport)) < 0)
        err(1, "failed listening");

    pthread_create(&threadId, NULL, timedHealth, reqHealth);

    int count = 0;
    int target = 0;
    while (1)
    {
        for (int i = 0; i < counter; i++)
        {

            printf("HERE 1\n");
            connectport = ports[i];
            printf("HERE 2\n");

            if ((connfd = client_connect(connectport)) < 0)
            {

                servers.servs[i].alive = false;
                err(1, "failed connecting");
            }
            else
            {
                servers.servs[i].alive = true;
            }

            printf("HERE 3\n");

            servers.servs[i].fd = connfd;
            servers.servs[i].port = ports[i];
            printf("CONNECTED TO PORT: %d\n", ports[i]);
            printf("HERE 4\n");

            //clients[i] = acceptfd;

            // This is a sample on how to bridge connections.
            // Modify as needed.
        }

        if ((acceptfd = accept(listenfd, NULL, NULL)) < 0)
            err(1, "failed accepting");
        printf("CONNECTED TO cient: %d\n", acceptfd);
        q[tail] = acceptfd;
        if (tail == 999 && front == 0)
        {
            dprintf(STDERR_FILENO, "QUEUE FULL OF REQUESTs\n");
            return EXIT_FAILURE;
        }
        else if (tail == 999)
        {
            tail = 0;
        }
        else
        {
            tail++;
        }
        printf("THIS IS PRIOROTIY: %d\n", priority);
        printf("THIS IS SERVER PORT: %d\n", servers.servs[priority].port);
        //pthread_cond_signal(&cond);
        wait = true;
        //pthread_cond_signal(&healthCond);
        //checkHealth();
        //priority = prioritize();
        printf("HERE 5s\n");

        bridge_loop(acceptfd, servers.servs[priority].fd);
    }

    //checkHealth();
    //priority = prioritize();
}