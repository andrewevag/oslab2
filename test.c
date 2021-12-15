#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>




int main(){

    int fd = open("/dev/lunix0-batt", 0);
    if(fd < 0)
    {
        printf("failed to open it = %d\n", fd);
        exit(EXIT_FAILURE);
    }


    char buf[BUFSIZ];
    int rv = read(fd, buf, 10);
    if(rv < 0)
    {
        printf("read returned = %d\n", rv);
    }
    else
    {
        buf[rv] = '\0';
        printf("read buf = %s\n", buf);
    }

    return 0;
}