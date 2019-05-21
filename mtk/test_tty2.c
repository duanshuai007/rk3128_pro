#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/serial.h>

int main(int argc, char **argv)
{
    int fd;
    unsigned int i;

    struct serial_struct ss;


    fd = open("/dev/ttyAMA1", O_RDWR | O_NOCTTY);
    if(fd < 0)
    {
        perror("open");
        return -1;
    }

    if(ioctl(fd, TIOCGSERIAL, &ss) < 0)
    {
        printf("TIOCGSERIAL error\r\n");
    }

    while(1)
    {
        write(fd, "hello world!\r\n", sizeof("hello world!\r\n"));
        sleep(1);
    }

    return 0;
}
