#include<stdio.h>
#include<string.h>
 
int main()
{
    char *str = "Host: localhost";
    char header1[20];
	char header2[20];
	
     sscanf(str, "%s:%s", header1, header2);
	printf("headers are: %s header 2: %s\n",header1,header2);	
 
    // signal to operating system program ran fine
    return 0;
}
