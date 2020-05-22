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
    int logFile;
    ssize_t *offset;
    //char *message;
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
int fails = 0;
int trials = 0;
int requests;

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

const char badMesg[] = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
const char notFoundMesg[] = "HTTP/1.1 404 File Not Found\r\nContent-Length: 0\r\n\r\n";
const char forbiddenMesg[] = "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n";
const char createdMesg[] = "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n";
const char internalErorrMesg[] = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";

char *put(long length, int clientSock, char *file, char *msg, int boolSend, char *extraMessage) //, char *buffer, char *maybeData)
{
    //printf("AM I STUCK IN PUT\n");
    printf("THIS IS FILE IN PUT: %s\n", file);

    //check if we have write permissions for file
    struct stat st;
    int result = stat(file, &st);
    //char msg[15000];
    //printf("STUCK 1\n");
    if (result == 0)
    {
        //printf("STUCK 2\n");
        if ((st.st_mode & S_IWUSR) == 0)
        {
            //printf("STUCK 3\n");
            send(clientSock, forbiddenMesg, strlen(forbiddenMesg), 0);
            fails++;
            sprintf(msg, "FAIL: PUT /%s HTTP/1.1 --- response 403\n========\n", file);
            return msg;
        }
        //printf("STUCK 4\n");
    }
    //open the file or create it if it doesnt exist
    if (strcmp(file, "healthcheck") == 0)
    {
        send(clientSock, forbiddenMesg, strlen(forbiddenMesg), 0);
        fails++;
        sprintf(msg, "FAIL: PUt /%s HTTP/1.1 --- response 403\n", file);
        return msg;
    }
    int op = open(file, O_WRONLY | O_CREAT | O_TRUNC);
    //printf("STUCK 5\n");
    //check if you have access to opening and writing to the file
    if (op < 0)
    {
        //printf("STUCK 6\n");
        send(clientSock, internalErorrMesg, strlen(internalErorrMesg), 0);
        fails++;
        //printf("STUCK 7\n");

        sprintf(msg, "FAIL: PUT /%s HTTP/1.1 --- response 500\n========\n", file);
        return msg;
    }
    else if (boolSend == 1)
    {
        write(op, extraMessage, strlen(extraMessage));
        send(clientSock, createdMesg, strlen(createdMesg), 0);
        char *sendMsg = malloc(1100);
        sprintf(sendMsg, "PUT /%s length %ld\n%s========\n", file, length, extraMessage);
        return sendMsg;
    }
    else
    {
        //printf("STUCK 8\n");
        //create a buffer to store the incoming data with the content length as the length of the buffer
        char fileRecieved[length];
        if (length == 0)
        {
            sprintf(msg, "PUT /%s length %ld\n========\n", file, length);
            return msg;
        }
        //printf("STUCK 9\n");

        printf("do i ever get here\n");
        //char *tok = strtok(buffer, "\r\n\r\n");
        //printf("TOK = %s\n", tok);
        //long data;
        long var = 0;
        //printf("STUCK herewajkdjf;l\n");
        //printf("THIS IS CLIENT SOCK: %d\n", clientSock);

        //printf("this is length: %ld\n", length);
        ssize_t r = recv(clientSock, fileRecieved, length, 0);
        //printf("this is fileReceieved: %s\n", fileRecieved);
        //printf("STUCK 10\n");
        var += r;
        ssize_t w = write(op, fileRecieved, r);
        //printf("STUCK 11\n");
        while (var != length)
        {
            //printf("STUCK IN PUT LOOP\n");
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
        //printf("SHOULD IT BE  A MSG: %s\nThis is the size: %X\n", fileRecieved, sizeof(fileRecieved) / sizeof(uint8_t));
        //printf("THIS HAS TO BE MSG\n")

        char *bigMsg = malloc(length + 150);

        sprintf(bigMsg, "PUT /%s length %ld\n%s========\n", file, length, fileRecieved);
        //printf("THIS IS MESSAGE MAYBE SEGFAULT: %s\n", msg);

        return bigMsg;
    }
}

char *get(int clientSock, char *file, char *msg, int log)
{
    printf("AM I STUCK IN Get\n");

    //check if we have read permissions for file
    struct stat sta;
    int result = stat(file, &sta);
    printf("STUCK 1\n");
    //char msg[15000];
    if (result == 0)
    {
        printf("STUCK 2\n");
        if ((sta.st_mode & S_IRUSR) == 0)
        {
            printf("STUCK 3\n");
            send(clientSock, forbiddenMesg, strlen(forbiddenMesg), 0);
            fails++;

            sprintf(msg, "FAIL: GET /%s HTTP/1.1 --- response 403\n========\n", file);
            return msg;
        }
    }
    printf("STUCK 4\n");
    //open the file to only read and then create a buffer to store the data in
    if (strcmp(file, "healthcheck") == 0 && log == 1)
    {
        int trialsLen = 0;
        int failsLen = 0;
        int healthLen = 0;
        int trialsDiv = trials;
        int failsDiv = fails;
        trials--;
        do
        {
            trialsLen++;
            trialsDiv /= 10;
        } while (trialsDiv != 0);
        do
        {
            failsLen++;
            failsDiv /= 10;

        } while (failsDiv != 0);
        healthLen = trialsLen + failsLen + 1;

        sprintf(msg, "HTTP/1.1 200 OK \r\nContent-Length: %d\r\n\r\n%d\n%d", healthLen, fails, trials);
        send(clientSock, msg, strlen(msg), 0);
        trials++;
        return msg;
    }
    else if (strcmp(file, "healthcheck") == 0 && log == -1)
    {
        send(clientSock, notFoundMesg, strlen(notFoundMesg), 0);
        fails++;
        sprintf(msg, "FAIL: GET /%s HTTP/1.1 --- response 404\n", file);
        return msg;
    }
    int op = open(file, O_RDONLY);
    uint8_t rd[10000];
    printf("STUCK 5\n");
    if (op >= 0)
    {
        printf("STUCK 6\n");
        //this is to find the file size so we can print the ok message before continually reading and writing
        struct stat st;
        stat(file, &st);
        printf("STUCK 7\n");
        //this is to get the file size to send the ok message if everything is ok
        size_t fileSize = st.st_size;
        printf("STUCK 8\n");
        size_t reading = read(op, rd, 10000);
        printf("STUCK 9\n");

        //send the response first then send the data after
        //create string to send
        char okMesg[10000];
        char dataRecv[fileSize];
        int counter = 0;
        printf("STUCK 10\n");
        sprintf(okMesg, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", fileSize);
        //dprintf(clientSock, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", fileSize);
        send(clientSock, okMesg, strlen(okMesg), 0);
        //check that it read some bytes so we can start sending them over to the client
        if (reading != 0)
        {
            printf("STUCK 11\n");
            //if the buffer is full then you want to send it and then keep reading and writing the data
            if (reading == 10000)
            {
                printf("STUCK 12\n");
                //write the data in the buffer first to the client then keep reading and writing in a loop
                //strcat(dataRecv, (unsigned char *)rd);
                for (int i = 0; i < reading; i++)
                {
                    dataRecv[counter] = rd[i];
                    counter++;
                }
                printf("STUCK 14\n");
                size_t w = send(clientSock, rd, reading, 0);
                printf("STUCK 15\n");
                size_t keepRead = read(op, rd, 10000);
                printf("STUCK 16\n");
                while (keepRead == 10000 && w == 10000)
                {
                    //printf("STUCK IN GET LOOP\n");
                    //strcat(dataRecv, rd);
                    for (int i = 0; i < keepRead; i++)
                    {
                        //printf("STUCK 17\n");
                        //printf("THIS IS DATA %d\n", rd[i]);
                        if (counter >= fileSize)
                        {
                            for (int i = 0; i < 1000; i++)
                            {

                                printf("UH OH\n");
                            }
                            return "UH OH";
                        }
                        dataRecv[counter] = rd[i];
                        counter++;
                    }
                    printf("STUCK 18\n");
                    w = send(clientSock, rd, keepRead, 0);
                    keepRead = read(op, rd, 10000);
                    //printf("IS this infinite loop prob not\n");
                }
                // once the buffer is no longer full which means that this should be the last time we send
                if (keepRead < 10000 && keepRead > 0)
                {
                    //printf("STUCK 19\n");
                    //strcat(dataRecv, rd);
                    for (int i = 0; i < keepRead; i++)
                    {
                        //printf("STUCK IN GET LOOP\n");
                        dataRecv[counter] = rd[i];
                        counter++;
                    }
                    w = send(clientSock, rd, keepRead, 0);
                }
            }
            else
            {
                //if the buffer is not full after the first read
                //strcat(dataRecv, rd);
                printf("STUCK 20\n");
                for (int i = 0; i < reading; i++)
                {
                    //printf("STUCK 21\n");
                    if (counter >= fileSize)
                    {
                        printf("THIS IS OUT OF BOUNDS CANT BE HERE\n");
                    }
                    dataRecv[counter] = rd[i];
                    counter++;
                }
                printf("STUCK 22\n");
                size_t sending = send(clientSock, rd, reading, 0);
                if (sending == 0)
                {
                }
            }
        }
        //printf("THis is data Recv: %s\n", dataRecv);
        //printf("THIS IS MESSAGE MAYBE SEGFAULT: %s\n", msg);
        //printf("OK I GET HERE\n");
        //printf("This is the length of the data: %ld\n", sizeof(dataRecv) / sizeof(uint8_t));
        //
        char *bigMsg = malloc(fileSize + 150);

        sprintf(bigMsg, "GET /%s length %ld\n%s========\n", file, fileSize, dataRecv);
        printf("DO I GO PAST SPRINTF\n");
        return bigMsg;
    }
    //this is all error checking to make sure that the file is found and not forbidden etc
    else
    {
        int err = errno;
        if (err == ENOENT)
        {
            send(clientSock, notFoundMesg, strlen(notFoundMesg), 0);
            fails++;
            sprintf(msg, "FAIL: GET /%s HTTP/1.1 --- response 404\n", file);
            return msg;
        }
        else if (err == EACCES)
        {
            send(clientSock, forbiddenMesg, strlen(forbiddenMesg), 0);
            fails++;
            sprintf(msg, "FAIL: GET /%s HTTP/1.1 --- response 403\n", file);
            return msg;
        }
        else
        {
            send(clientSock, internalErorrMesg, strlen(internalErorrMesg), 0);
            fails++;
            sprintf(msg, "FAIL: GET /%s HTTP/1.1 --- response 500\n", file);
            return msg;
        }
    }
}

char *head(int clientSock, char *file, char *msg)
{
    printf("AM I STUCK IN HEAD\n");
    //check if we have read permissions for file
    struct stat sta;
    int result = stat(file, &sta);
    //char msg[15000];
    if (result == 0)
    {
        if ((sta.st_mode & S_IRUSR) == 0)
        {
            send(clientSock, forbiddenMesg, strlen(forbiddenMesg), 0);
            sprintf(msg, "FAIL: HEAD /%s HTTP/1.1 --- response 403\n", file);
            fails++;

            return msg;
        }
    }
    if (strcmp(file, "healthcheck") == 0)
    {
        send(clientSock, forbiddenMesg, strlen(forbiddenMesg), 0);
        sprintf(msg, "FAIL: HEAD /%s HTTP/1.1 --- response 403\n", file);
        fails++;

        return msg;
    }
    //bascially same exact thing as put but just dont send data we just need the file size and to make sure that we can access the file
    int op = open(file, O_RDONLY);

    if (op >= 0)
    {
        struct stat st;
        stat(file, &st);
        size_t fileSize = st.st_size;

        dprintf(clientSock, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", fileSize);
        sprintf(msg, "HEAD /%s length %ld\n", file, fileSize);
        return msg;
    }
    else
    {
        int err = errno;
        if (err == ENOENT)
        {
            send(clientSock, notFoundMesg, strlen(notFoundMesg), 0);
            sprintf(msg, "FAIL: HEAD /%s HTTP/1.1 --- response 404\n", file);
            fails++;

            return msg;
        }
        else if (err == EACCES)
        {
            send(clientSock, forbiddenMesg, strlen(forbiddenMesg), 0);
            sprintf(msg, "FAIL: HEAD /%s HTTP/1.1 --- response 403\n", file);
            fails++;

            return msg;
        }
        else
        {
            send(clientSock, internalErorrMesg, strlen(internalErorrMesg), 0);
            sprintf(msg, "FAIL: HEAD /%s HTTP/1.1 --- response 500\n", file);
            fails++;

            return msg;
        }
    }
}
char *doServer(int client_sockd, char *msg, int log)
{
    char func[5];
    long conLen = 0;
    char *token;
    char *tokSave;
    char fileName[50];
    char serverVersion[20];
    //char msg[15000];
    char buff[BUFFER_SIZE + 1];
    char copyBuff[1000];
    ssize_t bytes = recv(client_sockd, buff, BUFFER_SIZE, 0);
    buff[bytes] = 0; // null terminate
    strcpy(copyBuff, buff);
    //printf("THiS IS COPY BUFF: %s\n", copyBuff);
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
    //token = strtok_rm(buff, "\r\n\r\n", &tokSave);
    token = strtok_r(buff, "\r\n", &tokSave);
    char bigTok[1000];
    strcpy(bigTok, token);
    //rintf("TOKEN HERE: %s\n", token);
    sscanf(token, "%s %s %s", func, fileName, serverVersion);
    //checks if first character is a / or not
    int boolSlash = 0;
    if (strcmp(serverVersion, "HTTP/1.1") != 0)
    {
        send(client_sockd, badMesg, strlen(badMesg), 0);
        close(client_sockd);
        sprintf(msg, "FAIL: /%s HTTP/1.1 --- response 400\n", fileName);
        fails++;
        return msg;
    }
    if (fileName[0] == '/')
    {
        memmove(fileName, fileName + 1, strlen(fileName));
        boolSlash = 1;
    }
    else
    {
        send(client_sockd, badMesg, strlen(badMesg), 0);
        close(client_sockd);
        sprintf(msg, "FAIL: /%s HTTP/1.1 --- response 400\n", fileName);
        fails++;
        return msg;
    }
    //check if filename violates any of the rules
    // first check the size make sure it is less than or equal to 27 characters

    //this might cause problems i might need to change back to 27
    if (strlen(fileName) > 27 && boolSlash == 0)
    {
        //i think i might have to change the response for errors to only have one \r\n instead of two and then close
        send(client_sockd, badMesg, strlen(badMesg), 0);
        //printf("in filename siz\n");
        close(client_sockd);
        sprintf(msg, "FAIL: /%s HTTP/1.1 --- response 400\n", fileName);
        fails++;
        return msg;
    }

    //check if filename is alphanumeric or contains -, _
    for (int i = 0; i < strlen(fileName); i++)
    {
        if (!((isalpha(fileName[i])) != 0 || (isdigit(fileName[i]) != 0) || fileName[i] == '-' || fileName[i] == '_'))
        {
            send(client_sockd, badMesg, strlen(badMesg), 0);
            //printf("IN Alpha num\n");
            close(client_sockd);
            sprintf(msg, "FAIL: /%s HTTP/1.1 --- response 400\n", fileName);
            fails++;
            return msg;
        }
    }
    //to move onto the next token
    token = strtok_r(NULL, "\r\n", &tokSave);
    strcpy(bigTok, token);
    char header1[40];
    char header2[40];
    int boolSendData = 0;
    char sendData[1000];
    int newBool = 0;

    // this tokenizes each part of the buffer by \r\n to seperate headers
    // make sure that each header follows the right conventions or else throw an error
    while (token != NULL)
    {
        // this is to check if it is giving content length in request
        if (sscanf(token, "Content-Length: %ld", &conLen) > 0)
        {
            boolSendData = 1;
        }
        // this is checking if its a normal header and if it is we just ignore it
        else if (sscanf(token, "%s %s", header1, header2) == 2)
        {
            if (header1[strlen(header1) - 1] != ':')
            {
                send(client_sockd, badMesg, strlen(badMesg), 0);
                close(client_sockd);
                sprintf(msg, "FAIL: /%s HTTP/1.1 --- response 400\n", fileName);
                fails++;
                return msg;
            }
        }
        // if it contains another \r\n in the string that means its the end of the request and might have data after it
        // we must read the data after if it is a put
        else if (boolSendData == 1)

        {
            strcpy(sendData, token);
            newBool = 1;
            break;
        }
        else
        {
            send(client_sockd, badMesg, strlen(badMesg), 0);
            close(client_sockd);
            sprintf(msg, "FAIL: /%s HTTP/1.1 --- response 400\n", fileName);
            fails++;
            return msg;
        }
        token = strtok_r(NULL, "\r\n", &tokSave);
    }

    printf("[+] received %ld bytes from client\n[+] response: ", bytes);
    //sscanf(buff, "%s ", func);

    write(STDOUT_FILENO, buff, bytes);

    // once we parse through the headers we use the arguments that we made and we can go into one of these three functions
    printf("THIS IS FILENAME: %s\n", fileName);
    if (strcmp(func, "HEAD") == 0)
    {
        return head(client_sockd, fileName, msg);
        close(client_sockd);
        //dprintf(client_sockd, "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n");
    }
    else if (strcmp(func, "PUT") == 0)
    {

        //there might be an error here since we are not checking if the words before the content length is exactly content length
        // i think well be fine because of the sscanf which has that string in the formating
        return put(conLen, client_sockd, fileName, msg, newBool, sendData); //, buffCopy, newTok);
        close(client_sockd);
    }
    else if (strcmp(func, "GET") == 0)
    {
        return get(client_sockd, fileName, msg, log);
        close(client_sockd);
    }
    else
    {
        send(client_sockd, badMesg, strlen(badMesg), 0);
        close(client_sockd);
        sprintf(msg, "FAIL: /%s HTTP/1.1 --- response 400\n", fileName);
        fails++;
        return msg;
    }
}
ssize_t findLen(char *msg)
{
    ssize_t len = 0;
    while (msg[len] != 0)
    {
        len++;
    }
    return len;
}
char *sendLog(char *msg, int file)
{
    //ssize_t len = findLen(msg);

    //printf("THIS IS The SIjZE OF MESSAGE with akdsfj %ld\n", len);
    //printf("THiS IS MESSAGE: %s\n", msg);
    return msg;
}

void makeHex(char *buff, char *mesg)
{
    int size = strlen(mesg);
    char smallBuff[20];
    int counter = 0;
    char count[16];
    //smallBuff[0] = '\0';
    for (int i = 0; i < size; i++)
    {
        snprintf(smallBuff, 19, " %02x", (unsigned char)mesg[i]);
        //printf("THIS IS SMALL BUFF: %s\n", smallBuff);
        if (i % 20 == 0 && i != 0)
        {
            strcat(buff, "\n");
            snprintf(count, 15, "%08d", counter);
            strcat(buff, count);
            counter += 20;
        }
        else if (i == 0)
        {
            snprintf(count, 15, "%08d", counter);
            counter += 20;
            strcat(buff, count);
        }
        strcat(buff, smallBuff);
        //printf("THIS IS BUFFER: %s\n", buff);
    }
    strcat(buff, " 0a");
    strcat(buff, "\n");
}

void *work(void *obj)
{
    workerThread *wrkr = (workerThread *)obj;
    printf("IN worker thread\n");
    int cSock;
    // i think they will all be writing to msg and it will mess it up you probably need to let each one have their own buffer
    char msg[20000];
    while (1)
    {
        pthread_mutex_lock(&mut);
        printf("Worker: [%d]\n", wrkr->id);
        printf("Requests:BEFORE WORK %d\n", requests);
        //if (requests == 0)
        //{
        printf("OR COME IN HERE\n");
        if (requests == 0)
        {
            pthread_cond_wait(&wrkr->cond, &mut);
        }
        //}
        if (requests > 0)
        {
            printf("I HAVE TO COME HERE RIGHT\n");
            wrkr->clientSock = q[front];
            printf("THIS IS MY CLIENT SOCK: %d\n", wrkr->clientSock);

            trials++;
            cSock = q[front];
            printf("THIS IS MY C SOCK: %d\n", cSock);
            if (front == 999)
            {
                front = 0;
            }
            else
            {
                front++;
            }
            //printf("DSKJFLASJIT MIGHT BE CuZ THIS\n");
            int log = 0;
            if (wrkr->logFile == -1)
            {
                log = -1;
            }
            else
            {
                log = 1;
            }
            char *mesg = doServer(cSock, msg, log);
            //printf("DO I GET HERE IT MIGHT BE CuZ THIS\n");

            char a[100];
            char b[16000];
            char c[20];
            char *firstPart = strtok(mesg, "\n");
            ssize_t sizeA = snprintf(a, 100, "%s\n", firstPart);

            //printf("THIS IS FIRST PART %s\n", firstPart);
            char *secondPart = strtok(NULL, "\n");
            //int counting = 0;
            //char buff[10];
            // char buff[16000];

            //printf("NOW THIS IS B: %s", b);

            ssize_t sizeB = (sizeof(secondPart) / sizeof(char));
            //printf("THiS IS LENGTH OF B: %ld\n", sizeB);
            ssize_t sizeC = snprintf(c, 20, "========\n");

            //printf("THIS IS Second PART %s\n", secondPart);
            //char *firstPart = strtok(msg, "\n");
            //printf("THIS IS FIRST PART %s\n", firstPart);
            ssize_t theCountSize = 0;
            if (sizeB / 60 == 0 && sizeB % 60 > 0)
            {
                theCountSize++;
            }
            else if (sizeB / 60 > 0 && sizeB % 60 > 0)
            {
                theCountSize = (sizeB / 60) + 1;
            }
            else
            {
                theCountSize = (sizeB / 60);
            }
            theCountSize *= 9;

            //sleep(5);
            wrkr->clientSock = -1;
            printf("done with request\n");
            if (secondPart != NULL)
            {
                makeHex(b, secondPart);
            }
            sizeB = strlen(b);
            ssize_t totalLen = sizeA + sizeC + sizeB;
            //pthread_cond_signal(&wrkr->cond);
            ssize_t oSet = *(wrkr->offset);
            *(wrkr->offset) += totalLen;
            //char printHex[150];
            //int hexCount = 0;
            //int count = 0;
            // char keepPrinting[150];

            //printf("WHY IS B CHANGED: %s", b);
            requests--;
            printf("Requests:AFTER SUB %d\n", requests);

            pthread_mutex_unlock(&mut);
            printf("SERVER [%d] is done\n", wrkr->id);
            if (wrkr->logFile != -1)
            {
                pwrite(wrkr->logFile, a, sizeA, oSet);
                oSet += sizeA;
                //printf("DO I asdfjkljasdlfjjaldsf IN HERE\n");

                if (secondPart != NULL)
                {
                    //printf("DO I GO IN HERE\n");

                    //printf("This is B: %s\n", b);

                    pwrite(wrkr->logFile, b, sizeB, oSet);
                    oSet += sizeB;
                }

                pwrite(wrkr->logFile, c, sizeC, oSet);
                oSet += sizeC;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    //this is to parse the command arguments and see what is being asked for
    char *port = NULL;
    int numThreads = 4;
    char *logFile = NULL;
    //
    printf("THis is first arg: %s\n", argv[0]);
    /*if (strcmp(argv[0], "./httpserver") != 0)
    {
        dprintf(STDERR_FILENO, "Include httpserver\n");
        return EXIT_FAILURE;
    }*/
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
    int fileName = -1;
    ssize_t poffset = 0;
    if (logFile != NULL)
    {
        fileName = open(logFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }
    //circularArray q;
    for (int i = 0; i < numThreads; i++)
    {
        workers[i].id = i;
        workers[i].clientSock = -1;
        workers[i].logFile = fileName;
        workers[i].cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
        workers[i].offset = &poffset;

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

    // we want the server to keep running until the user decides to shut it down
    int counter = 0;
    int target = 0;
    while (1)
    {
        printf("[+] server is waiting...\n");
        int client_sockd = accept(server_sockd, &client_addr, &client_addrlen);
        requests++;
        printf("Requests: AFter accept %d\n", requests);
        printf("clientSock: %d\n", client_sockd);

        target = counter % numThreads;
        //workers[target].clientSock = client_sockd;
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
        printf("I DEF SINGAL TO WAke\n");
        counter++;
    }
    return 0;
}
