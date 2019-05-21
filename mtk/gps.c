#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char **argv)
{
    int fd;
    int len;
    char buf[1024];

    fd = open("/dev/gps", O_RDWR);
    if(fd < 0)
    {
        printf("open gps failed\r\n");
        return -1;
    }

    printf("open gps success\r\n");

    memset(buf, 0, 1024);

    while(1)
    {
        len = read(fd, buf, 1024);
        printf("%s\r\n", buf);
    }

    return 0;
}
