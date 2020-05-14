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
#include <pthread.h>

#define BUFFER_SIZE 4096

/*typedef struct ciruclar_aray_t
{
    int q[1000];
    ssize_t qSize;
    int head;
    int tail;
} circularArray;*/

typedef struct workerThreadArgs
{
    int id;
    pthread_t workId;
    int clientSock;
    pthread_cond_t cond;
    //circularArray queue;
} workerThread;

/*typedef struct mainThreadArgs
{
    int fd;
    ssize_t numThreads;
} mainThread;*/
int q[1000];
ssize_t qSize;
int front;
int tail;

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

const char badMesg[] = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
const char notFoundMesg[] = "HTTP/1.1 404 File Not Found\r\nContent-Length: 0\r\n\r\n";
const char forbiddenMesg[] = "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n";
const char createdMesg[] = "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n";
const char internalErorrMesg[] = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";

void put(long length, int clientSock, char *file) //, char *buffer, char *maybeData)
{
    //check if we have write permissions for file
    struct stat st;
    int result = stat(file, &st);
    if (result == 0)
    {
        if ((st.st_mode & S_IWUSR) == 0)
        {
            send(clientSock, forbiddenMesg, strlen(forbiddenMesg), 0);

            return;
        }
    }
    //open the file or create it if it doesnt exist
    int op = open(file, O_WRONLY | O_CREAT | O_TRUNC);
    //check if you have access to opening and writing to the file
    if (op < 0)
    {
        send(clientSock, internalErorrMesg, strlen(internalErorrMesg), 0);
    }
    else
    {
        //create a buffer to store the incoming data with the content length as the length of the buffer
        uint8_t fileRecieved[length];
        //size_t w = 0;
        /*if (maybeData != NULL)
        {
            printf("in here");
            write(op, fileRecieved, strlen(maybeData));
            recv(clientSock, fileRecieved, length - strlen(maybeData), 0);
            write(op, fileRecieved, length - strlen(maybeData));
        }
        else
        {*/
        printf("do i ever get here");
        //char *tok = strtok(buffer, "\r\n\r\n");
        //printf("TOK = %s\n", tok);
        //long data;
        long var = 0;
        ssize_t r = recv(clientSock, fileRecieved, length, 0);
        var += r;
        ssize_t w = write(op, fileRecieved, r);
        while (var != length)
        {
            r = recv(clientSock, fileRecieved, length, 0);
            var += r;
            w = write(op, fileRecieved, r);
        }
        if (w == 0)
        {
        }
        //printf("THIS IS RECIEVE BYTES: %ld", r);

        //write the data to the file from the buffer and print a created code
        //ssize_t w = write(op, fileRecieved, length);

        //}
        send(clientSock, createdMesg, strlen(createdMesg), 0);
    }
}

void get(int clientSock, char *file)
{
    //check if we have read permissions for file
    struct stat sta;
    int result = stat(file, &sta);
    if (result == 0)
    {
        if ((sta.st_mode & S_IRUSR) == 0)
        {
            send(clientSock, forbiddenMesg, strlen(forbiddenMesg), 0);

            return;
        }
    }
    //open the file to only read and then create a buffer to store the data in
    int op = open(file, O_RDONLY);
    uint8_t rd[1000];
    if (op >= 0)
    {
        //this is to find the file size so we can print the ok message before continually reading and writing
        struct stat st;
        stat(file, &st);
        //this is to get the file size to send the ok message if everything is ok
        size_t fileSize = st.st_size;
        size_t reading = read(op, rd, 1000);
        //send the response first then send the data after
        //create string to send
        char okMesg[1000];
        sprintf(okMesg, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", fileSize);
        //dprintf(clientSock, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", fileSize);
        send(clientSock, okMesg, strlen(okMesg), 0);
        //check that it read some bytes so we can start sending them over to the client
        if (reading != 0)
        {
            //if the buffer is full then you want to send it and then keep reading and writing the data
            if (reading == 1000)
            {
                //write the data in the buffer first to the client then keep reading and writing in a loop
                size_t w = send(clientSock, rd, reading, 0);
                size_t keepRead = read(op, rd, 1000);
                while (keepRead == 1000 && w == 1000)
                {
                    w = send(clientSock, rd, keepRead, 0);
                    keepRead = read(op, rd, 1000);
                    printf("IS this infinite loop prob not\n");
                }
                // once the buffer is no longer full which means that this should be the last time we send
                if (keepRead < 1000 && keepRead > 0)
                {
                    w = send(clientSock, rd, keepRead, 0);
                }
            }
            else
            {
                //if the buffer is not full after the first read
                size_t sending = send(clientSock, rd, reading, 0);
                if (sending == 0)
                {
                }
            }
        }
    }
    //this is all error checking to make sure that the file is found and not forbidden etc
    else
    {
        int err = errno;
        if (err == ENOENT)
        {
            send(clientSock, notFoundMesg, strlen(notFoundMesg), 0);
        }
        else if (err == EACCES)
        {
            send(clientSock, forbiddenMesg, strlen(forbiddenMesg), 0);
        }
        else
        {
            send(clientSock, internalErorrMesg, strlen(internalErorrMesg), 0);
        }
    }
}

void head(int clientSock, char *file)
{
    //check if we have read permissions for file
    struct stat sta;
    int result = stat(file, &sta);
    if (result == 0)
    {
        if ((sta.st_mode & S_IRUSR) == 0)
        {
            send(clientSock, forbiddenMesg, strlen(forbiddenMesg), 0);

            return;
        }
    }
    //bascially same exact thing as put but just dont send data we just need the file size and to make sure that we can access the file
    int op = open(file, O_RDONLY);

    if (op >= 0)
    {
        struct stat st;
        stat(file, &st);
        size_t fileSize = st.st_size;

        dprintf(clientSock, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", fileSize);
    }
    else
    {
        int err = errno;
        if (err == ENOENT)
        {
            send(clientSock, notFoundMesg, strlen(notFoundMesg), 0);
        }
        else if (err == EACCES)
        {
            send(clientSock, forbiddenMesg, strlen(forbiddenMesg), 0);
        }
        else
        {
            send(clientSock, internalErorrMesg, strlen(internalErorrMesg), 0);
        }
    }
}
void doServer(int client_sockd)
{
    char func[5];
    long conLen = 0;
    char *token;
    char fileName[50];
    char serverVersion[20];

    char buff[BUFFER_SIZE + 1];
    ssize_t bytes = recv(client_sockd, buff, BUFFER_SIZE, 0);
    buff[bytes] = 0; // null terminate
    //creating a copy of the request to be able to check if their is data at the end that i need to read to be able to put
    //char buffCopy[BUFFER_SIZE + 1];
    //strcpy(buffCopy, buff);
    //char *truncBuff = strtok(buff, "\r\n\r\n");
    //new comment
    //char *newTok = strtok(buffCopy, "\r\n\r\n");
    //newTok = strtok(NULL, "\r\n\r\n");
    //printf("This is new tok: %s\n", newTok);
    //parsing algorithm
    //first we want to delimit each part of the request by the \r\n to get each header, this will give us the first header
    // the first header contains the function name and the filename which we need
    token = strtok(buff, "\r\n");
    sscanf(token, "%s %s %s", func, fileName, serverVersion);
    //checks if first character is a / or not
    if (strcmp(serverVersion, "HTTP/1.1") != 0)
    {
        send(client_sockd, badMesg, strlen(badMesg), 0);
        close(client_sockd);
    }
    if (fileName[0] == '/')
    {
        memmove(fileName, fileName + 1, strlen(fileName));
    }
    else
    {
        send(client_sockd, badMesg, strlen(badMesg), 0);
        close(client_sockd);
    }
    //check if filename violates any of the rules
    // first check the size make sure it is less than or equal to 27 characters

    if (strlen(fileName) > 27)
    {
        //i think i might have to change the response for errors to only have one \r\n instead of two and then close
        send(client_sockd, badMesg, strlen(badMesg), 0);
        //printf("in filename siz\n");
        close(client_sockd);
    }

    //check if filename is alphanumeric or contains -, _
    for (int i = 0; i < strlen(fileName); i++)
    {
        if (!((isalpha(fileName[i])) != 0 || (isdigit(fileName[i]) != 0) || fileName[i] == '-' || fileName[i] == '_'))
        {
            send(client_sockd, badMesg, strlen(badMesg), 0);
            //printf("IN Alpha num\n");
            close(client_sockd);
            break;
        }
    }
    //to move onto the next token
    token = strtok(NULL, "\r\n");
    char header1[40];
    char header2[40];
    // this tokenizes each part of the buffer by \r\n to seperate headers
    // make sure that each header follows the right conventions or else throw an error
    while (token != NULL)
    {
        // this is to check if it is giving content length in request
        if (sscanf(token, "Content-Length: %ld", &conLen) > 0)
        {
        }
        // this is checking if its a normal header and if it is we just ignore it
        else if (sscanf(token, "%s %s", header1, header2) == 2)
        {
            if (header1[strlen(header1) - 1] != ':')
            {
                send(client_sockd, badMesg, strlen(badMesg), 0);
                close(client_sockd);
            }
        }
        // if it contains another \r\n in the string that means its the end of the request and might have data after it
        // we must read the data after if it is a put
        else
        {
            send(client_sockd, badMesg, strlen(badMesg), 0);
            close(client_sockd);
        }
        token = strtok(NULL, "\r\n");
    }

    printf("[+] received %ld bytes from client\n[+] response: ", bytes);
    //sscanf(buff, "%s ", func);

    write(STDOUT_FILENO, buff, bytes);

    // once we parse through the headers we use the arguments that we made and we can go into one of these three functions
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
        put(conLen, client_sockd, fileName); //, buffCopy, newTok);
        close(client_sockd);
    }
    else if (strcmp(func, "GET") == 0)
    {
        get(client_sockd, fileName);
        close(client_sockd);
    }
    else
    {
        send(client_sockd, badMesg, strlen(badMesg), 0);
        close(client_sockd);
    }
}
void *work(void *obj)
{
    workerThread *wrkr = (workerThread *)obj;
    printf("IN worker thread\n");
    int cSock;
    while (1)
    {
        pthread_mutex_lock(&mut);
        while (wrkr->clientSock < 0)
        {
            pthread_cond_wait(&wrkr->cond, &mut);
        }
        cSock = q[front];
        if (front == 999)
        {
            front = 0;
        }
        else
        {
            front++;
        }
        doServer(cSock);

        //sleep(5);
        wrkr->clientSock = -1;
        printf("done with request\n");
        //pthread_cond_signal(&wrkr->cond);
        pthread_mutex_unlock(&mut);
    }
}

/*void *delegate(void *obj)
{
    printf("In main thread\n");
    while (true)
    {
        pthread_mutex_lock(mut);
        while (dataReady)
        {
            pthread_cond_wait(&cond, &mut);
        }
        //do stuff in here
        int clientSock = accept(server_sockd, &client_addr, &client_addrlen);

        if (clientSock >= 0)
        {
            q[tail] = clientSock;
            if (tail == 999)
            {
                tail = 0;
            }
            else
            {
                tail++;
            }
        }
        dataReady = 1;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mut);
    }
}*/

int main(int argc, char *argv[])
{
    //this is to parse the command arguments and see what is being asked for
    char *port = NULL;
    int numThreads = 4;
    char *logFile = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (atoi(argv[i]) != 0)
        {
            if (strcmp(argv[i - 1], "-N") == 0)
            {
                numThreads = atoi(argv[i]);
            }
            else
            {
                port = argv[i];
            }
        }
        if (strcmp(argv[i], "-l") == 0)
        {
            if (i < argc - 1)
            {
                logFile = argv[i + 1];
            }
            else
            {
                dprintf(STDERR_FILENO, "Include log file\n");
                return EXIT_FAILURE;
            }
        }
    }
    if (port == NULL)
    {
        dprintf(STDERR_FILENO, "Include port number\n");
        return EXIT_FAILURE;
    }
    if (numThreads <= 0)
    {
        dprintf(STDERR_FILENO, "Increase thread amount\n");
        return EXIT_FAILURE;
    }

    //char *port = argv[1];

    if (atoi(port) <= 1024)
    {
        dprintf(STDERR_FILENO, "Port number must be above 1024\n");
        return EXIT_FAILURE;
    }
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
    ret = listen(server_sockd, SOMAXCONN); // 5 should be enough, if not use SOMAXCONN

    if (ret < 0)
    {
        return 1;
    }

    //creating the threads
    workerThread workers[numThreads];
    int err = 0;
    //circularArray q;
    for (int i = 0; i < numThreads; i++)
    {
        workers[i].id = i;
        workers[i].clientSock = -1;
        workers[i].cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
        //workers[i].queue = &q;

        err = pthread_create(&workers[i].workId, NULL, &work, &workers[i]);
        if (err)
        {
            dprintf(STDERR_FILENO, "ERROR setting up thread\n");
            return EXIT_FAILURE;
        }
    }
    //mainThread mThread;
    //pthread_create(&threads[numThreads], NULL, &delegate, &mThread);

    /*
        Connecting with a client
    */
    struct sockaddr client_addr;
    socklen_t client_addrlen;

    //these are to take in the arguments from the request that the client sends us like function name and conten length etc
    /* char func[5];
    long conLen = 0;
    char *token;
    char fileName[50];
    char serverVersion[20];*/

    // we want the server to keep running until the user decides to shut it down
    int counter = 0;
    int target = 0;
    while (1)
    {
        printf("[+] server is waiting...\n");
        int client_sockd = accept(server_sockd, &client_addr, &client_addrlen);

        target = counter % numThreads;
        workers[target].clientSock = client_sockd;
        q[tail] = client_sockd;
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
        pthread_cond_signal(&workers[target].cond);
        counter++;

        /*while ((client_sockd = ) >= 0)
        {
            q[tail] = client_sockd;
            if (tail == 999 && head == 0)
            {
                dprintf(STDERR_FILENO, "queue is full\n");
            }
            else if (tail == 999)
            {
                tail = 0;
                qSize = 999 - head;
            }
            else
            {
                tail++;
                qSize++;
            }
        }*/

        // Remember errors happen

        /*char buff[BUFFER_SIZE + 1];
        ssize_t bytes = recv(client_sockd, buff, BUFFER_SIZE, 0);
        buff[bytes] = 0; // null terminate
        //creating a copy of the request to be able to check if their is data at the end that i need to read to be able to put
        //char buffCopy[BUFFER_SIZE + 1];
        //strcpy(buffCopy, buff);
        //char *truncBuff = strtok(buff, "\r\n\r\n");
        //new comment
        //char *newTok = strtok(buffCopy, "\r\n\r\n");
        //newTok = strtok(NULL, "\r\n\r\n");
        //printf("This is new tok: %s\n", newTok);
        //parsing algorithm
        //first we want to delimit each part of the request by the \r\n to get each header, this will give us the first header
        // the first header contains the function name and the filename which we need
        token = strtok(buff, "\r\n");
        sscanf(token, "%s %s %s", func, fileName, serverVersion);
        //checks if first character is a / or not
        if (strcmp(serverVersion, "HTTP/1.1") != 0)
        {
            send(client_sockd, badMesg, strlen(badMesg), 0);
            close(client_sockd);
        }
        if (fileName[0] == '/')
        {
            memmove(fileName, fileName + 1, strlen(fileName));
        }
        else
        {
            send(client_sockd, badMesg, strlen(badMesg), 0);
            close(client_sockd);
        }
        //check if filename violates any of the rules
        // first check the size make sure it is less than or equal to 27 characters

        if (strlen(fileName) > 27)
        {
            //i think i might have to change the response for errors to only have one \r\n instead of two and then close
            send(client_sockd, badMesg, strlen(badMesg), 0);
            //printf("in filename siz\n");
            close(client_sockd);
        }

        //check if filename is alphanumeric or contains -, _
        for (int i = 0; i < strlen(fileName); i++)
        {
            if (!((isalpha(fileName[i])) != 0 || (isdigit(fileName[i]) != 0) || fileName[i] == '-' || fileName[i] == '_'))
            {
                send(client_sockd, badMesg, strlen(badMesg), 0);
                //printf("IN Alpha num\n");
                close(client_sockd);
                break;
            }
        }
        //to move onto the next token
        token = strtok(NULL, "\r\n");
        char header1[40];
        char header2[40];
        // this tokenizes each part of the buffer by \r\n to seperate headers
        // make sure that each header follows the right conventions or else throw an error
        while (token != NULL)
        {
            // this is to check if it is giving content length in request
            if (sscanf(token, "Content-Length: %ld", &conLen) > 0)
            {
            }
            // this is checking if its a normal header and if it is we just ignore it
            else if (sscanf(token, "%s %s", header1, header2) == 2)
            {
                if (header1[strlen(header1) - 1] != ':')
                {
                    send(client_sockd, badMesg, strlen(badMesg), 0);
                    close(client_sockd);
                }
            }
            // if it contains another \r\n in the string that means its the end of the request and might have data after it
            // we must read the data after if it is a put
            else
            {
                send(client_sockd, badMesg, strlen(badMesg), 0);
                close(client_sockd);
            }
            token = strtok(NULL, "\r\n");
        }

        printf("[+] received %ld bytes from client\n[+] response: ", bytes);
        //sscanf(buff, "%s ", func);

        write(STDOUT_FILENO, buff, bytes);

        // once we parse through the headers we use the arguments that we made and we can go into one of these three functions
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
            put(conLen, client_sockd, fileName); //, buffCopy, newTok);
            close(client_sockd);
        }
        else if (strcmp(func, "GET") == 0)
        {
            get(client_sockd, fileName);
            close(client_sockd);
        }
        else
        {
            send(client_sockd, badMesg, strlen(badMesg), 0);
            close(client_sockd);
        }*/
    }
    return 0;
}
