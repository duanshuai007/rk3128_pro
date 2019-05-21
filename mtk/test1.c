#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define COMBO_IOC_MAGIC             'h'
#define COMBO_IOCTL_GET_CHIP_ID     _IOR(COMBO_IOC_MAGIC, 0, int)

int main(int argc, char **argv)
{
    int fd;
    long id;
    
    fd = open("/dev/hifsdiod", O_RDWR | O_NOCTTY);
    if(fd < 0)
    {
        printf("hif-sdio open failed\r\n");
        return -1;
    }

    printf("hif-sdio open success\r\n");

    while(1)
    {
        id = ioctl(fd, COMBO_IOCTL_GET_CHIP_ID, NULL);
        printf("hif-sdio id: %08x\r\n", id);
            
        sleep(1);

    }

    return 0;
}
