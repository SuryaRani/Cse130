#include <stdio.h>
#include <fcntl.h>
#include <err.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//this will echo back the stdin when there are no arguments or the "-" argument
void echo()
{
    // const char *fle = STDIN_FILENO;
    //int op = open(fle, O_RDONLY);
    char rdBuf[10];
    size_t rd = read(STDIN_FILENO, rdBuf, 10);
    if (rd != 0)
    {
        if (rd == 10)
        {
            int w = write(STDOUT_FILENO, rdBuf, 10);
            size_t keepRead = read(STDIN_FILENO, rdBuf, 10);
            while (keepRead == 10 && w == 10)
            {
                w = write(STDOUT_FILENO, rdBuf, 10);
                keepRead = read(STDIN_FILENO, rdBuf, 10);
            }
            if (keepRead < 10 && keepRead > 0)
            {
                w = write(STDOUT_FILENO, rdBuf, keepRead);
            }
        }
        else
        {
            int w = write(STDOUT_FILENO, rdBuf, rd);
            if (w < 0)
            {
                warn("%s", STDIN_FILENO);
            }
        }
        echo();
    }
    // i think there is an error here closing stdin check it out
    close(STDIN_FILENO);
}

//this is for all other files to print out what is in the file
void printNorm(const char *fle)
{
    int op = open(fle, O_RDONLY);
    if (op < 0)
    {
        warn("%s", fle);
        return;
    }
    char rdBuf[10];
    size_t rd = read(op, rdBuf, 10);
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
    }
    close(op);
}

//takes in the files and decides which printer to use either echo or printNorm
int main(int argc, const char *argv[])
{
    // for no arguments
    if (argc == 1)
    {
        echo();
    }

    //for any amount of arguments
    int numArg = argc;
    numArg--;

    while (numArg > 0)
    {

        const char *file = argv[numArg];
        const char *dash = "-";
        if (strcmp(file, dash))
        {
            printNorm(file);
        }
        else
        {
            echo();
        }
        numArg--;
    }
}
