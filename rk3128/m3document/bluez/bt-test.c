#include<stdio.h>      /*标准输入输出定义*/  
#include<stdlib.h>     /*标准函数库定义*/  
#include<unistd.h>     /*Unix 标准函数定义*/  
#include<sys/types.h>   
#include<sys/stat.h>     
#include<fcntl.h>      /*文件控制定义*/  
#include<termios.h>    /*PPSIX 终端控制定义*/  
#include<errno.h>      /*错误号定义*/  
#include<string.h>

//宏定义  
#define FALSE  -1  
#define TRUE   0  

#define HCI_COMMAND_PKT 0x01
#define CC_MIN_SIZE     7
#define CMD_SUCCESS     0x00
#define BCM43XX_CLOCK_48    1
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

static int read_hci_event(int fd, unsigned char* buf, int size)
{
    int remain, r;
    int count = 0; 

    if (size <= 0)
        return -1;

    /* The first byte identifies the packet type. For HCI event packets, it
     *      * should be 0x04, so we read until we get to the 0x04. */
    while (1) {
        r = read(fd, buf, 1);
        if (r <= 0)
            return -1;
        if (buf[0] == 0x04)
            break;
    }    
    count++;

    /* The next two bytes are the event code and parameter total length. */
    while (count < 3) { 
        r = read(fd, buf + count, 3 - count);
        if (r <= 0)
            return -1;
        count += r;
    }    

    /* Now we read the parameters. */
    if (buf[2] < (size - 3))
        remain = buf[2];
    else 
        remain = size - 3; 

    while ((count - 3) < remain) {
        r = read(fd, buf + count, remain - (count - 3)); 
        if (r <= 0)
            return -1;
        count += r;
    }    

    return count;
} 

static int bcm43xx_reset(int fd)
{
    unsigned char cmd[] = { HCI_COMMAND_PKT, 0x03, 0x0C, 0x00 };
    unsigned char resp[CC_MIN_SIZE];

    if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
        fprintf(stderr, "Failed to write reset command\n");
        return -1;
    }

    if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
        fprintf(stderr, "Failed to reset chip, invalid HCI event\n");
        return -1;
    }

    if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
        fprintf(stderr, "Failed to reset chip, command failure\n");
        return -1;
    }

    return 0;
}

static int bcm43xx_read_local_name(int fd, char *name, size_t size)
{
    unsigned char cmd[] = { HCI_COMMAND_PKT, 0x14, 0x0C, 0x00 };
    unsigned char *resp;
    unsigned int name_len;

    resp = malloc(size + CC_MIN_SIZE);
    if (!resp)
        return -1; 

    tcflush(fd, TCIOFLUSH);

    if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
        fprintf(stderr, "Failed to write read local name command\n");
        goto fail;
    }   

    if (read_hci_event(fd, resp, size) < CC_MIN_SIZE) {
        fprintf(stderr, "Failed to read local name, invalid HCI event\n");
        goto fail;
    }   

    if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
        fprintf(stderr, "Failed to read local name, command failure\n");
        goto fail;
    }   

    name_len = resp[2] - 1;

    strncpy(name, (char *) &resp[7], MIN(name_len, size));
    name[size - 1] = 0;

    printf("name:%s\r\n", name);

    free(resp);
    return 0;

fail:
    free(resp);
    return -1; 
}

static int bcm43xx_set_clock(int fd, unsigned char clock)
{
    unsigned char cmd[] = { HCI_COMMAND_PKT, 0x45, 0xfc, 0x01, 0x00 };
    unsigned char resp[CC_MIN_SIZE];

    printf("Set Controller clock (%d)\n", clock);

    cmd[4] = clock;

    tcflush(fd, TCIOFLUSH);

    if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
        fprintf(stderr, "Failed to write update clock command\n");
        return -1;
    }

    if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
        fprintf(stderr, "Failed to update clock, invalid HCI event\n");
        return -1;
    }

    if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
        fprintf(stderr, "Failed to update clock, command failure\n");
        return -1;
    }

    return 0;
}

static inline unsigned int tty_get_speed(int speed)
{
    switch (speed) {
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        case 230400:
            return B230400;
        case 460800:
            return B460800;
        case 500000:
            return B500000;
        case 576000:
            return B576000;
        case 921600:
            return B921600;
        case 1000000:
            return B1000000;
        case 1152000:
            return B1152000;
        case 1500000:
            return B1500000;
        case 2000000:
            return B2000000;
#ifdef B2500000
        case 2500000:
            return B2500000;
#endif
#ifdef B3000000
        case 3000000:
            return B3000000;
#endif
#ifdef B3500000
        case 3500000:
            return B3500000;
#endif
#ifdef B3710000
        case 3710000:
            return B3710000;
#endif
#ifdef B4000000
        case 4000000:
            return B4000000;
#endif
    }   

    return 0;
}

int set_speed(int fd, struct termios *ti, int speed)
{
    if (cfsetospeed(ti, tty_get_speed(speed)) < 0)
        return -errno;

    if (cfsetispeed(ti, tty_get_speed(speed)) < 0)
        return -errno;

    if (tcsetattr(fd, TCSANOW, ti) < 0)
        return -errno;

    return 0;
}

static int bcm43xx_set_speed(int fd, struct termios *ti, uint32_t speed)
{
    unsigned char cmd[] =
    { HCI_COMMAND_PKT, 0x18, 0xfc, 0x06, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00 };
    unsigned char resp[CC_MIN_SIZE];

    if (speed > 3000000 && bcm43xx_set_clock(fd, BCM43XX_CLOCK_48))
        return -1;

    printf("Set Controller UART speed to %d bit/s\n", speed);

    cmd[6] = (uint8_t) (speed);
    cmd[7] = (uint8_t) (speed >> 8);
    cmd[8] = (uint8_t) (speed >> 16);
    cmd[9] = (uint8_t) (speed >> 24);

    tcflush(fd, TCIOFLUSH);

    if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
        fprintf(stderr, "Failed to write update baudrate command\n");
        return -1;
    }

    if (read_hci_event(fd, resp, sizeof(resp)) < CC_MIN_SIZE) {
        fprintf(stderr, "Failed to update baudrate, invalid HCI event\n");
        return -1;
    }

    if (resp[4] != cmd[1] || resp[5] != cmd[2] || resp[6] != CMD_SUCCESS) {
        fprintf(stderr, "Failed to update baudrate, command failure\n");
        return -1;
    }

    usleep(200000);

    if (set_speed(fd, ti, speed) < 0) {
        perror("Can't set host baud rate");
        return -1; 
    }     

    return 0;
}

int main(int argc, char **argv)  
{  
    int fd;                            //文件描述符  
    int err;                           //返回调用函数的状态  
    int len;                          
    int i;  
    char rcv_buf[100];         
    uint32_t baud;
    char chip_name[20];
    struct termios ti;

    if(argc != 2)  
    {  
        printf("Usage:\r\n");  
        return FALSE;  
    }  

    fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY);
    if (fd < 0) { 
        perror("Can't open serial port");
        return -1;
    }    

    tcflush(fd, TCIOFLUSH);

    if (tcgetattr(fd, &ti) < 0) { 
        perror("Can't get port settings");
        return -1;
    }    

    cfmakeraw(&ti);

    //ti.c_cflag |= CLOCAL;
    //if (u->flags & FLOW_CTL)
    //    ti.c_cflag |= CRTSCTS;
    //else 
        ti.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &ti) < 0) { 
        perror("Can't set port settings");
        return -1;
    }    

    /* Set initial baudrate */
    if (set_speed(fd, &ti, 115200) < 0) { 
        perror("Can't set initial baud rate");
        return -1;
    }    

    tcflush(fd, TCIOFLUSH);

    //测试程序开始
    printf("===Start===\r\n");
    if(bcm43xx_reset(fd))
    {
        printf("reset cmd failed\r\n");
        return -1;
    }
    
    if (bcm43xx_read_local_name(fd, chip_name, sizeof(chip_name)))
    {
        printf("read local name failed\r\n");
        return -1;
    }

    baud = atoi(argv[1]);
    printf("baud:%d\r\n", baud);

    if (bcm43xx_set_speed(fd, &ti, baud))
    {
        return -1;
    }

    printf("second read local name\r\n");

    if (bcm43xx_read_local_name(fd, chip_name, sizeof(chip_name)))
    {
        printf("read local name failed\r\n");
        return -1;
    }

    close(fd);
    printf("===End===");

    return 0;
}  
