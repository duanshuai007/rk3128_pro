#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>

int main(int argc, char **argv)
{
    int fd;

    /* 打开fb设备文件 */
    fd = open("/dev/fb0", O_RDWR);
    if (-1 == fd)
    {
        perror("open fb");
        return -1;
    }

    /* 获取fix屏幕信息:获取命令为FBIOGET_FSCREENINFO */
    struct fb_fix_screeninfo fixInfo;

    if (ioctl(fd, FBIOGET_FSCREENINFO, &fixInfo) == -1)
    {
        perror("get fscreeninfo");
        close(fd);
        return -2;
    }
    /* 打印fix信息 */
    //printf("id = %s\n", fixInfo.id); /* 厂商id信息 */
    //printf("line length = %d\n", fixInfo.line_length); 
                                    /* 这里获取的是一行像素所需空间
                                     * 该空间大小是出厂时就固定的了
                                     * 厂商会对一行像素字节进行对齐*/

    /* 获取var屏幕的信息:获取命令为FBIOGET_VSCREENINFO */
    struct fb_var_screeninfo varInfo;

    if (ioctl(fd, FBIOGET_VSCREENINFO, &varInfo) == -1)
    {
        perror("get var screen failed\n");
        close(fd);
        return -3;
    }
    /* 打印var信息 */
    printf("xres=%d,yres=%d\r\n", varInfo.xres, varInfo.yres);
    //printf("bits_per_pixel = %d\n", varInfo.bits_per_pixel);
    //printf("red: offset = %d, length = %d\n", \
                    varInfo.red.offset, varInfo.red.length);
    //printf("green: offset = %d, length = %d\n", \
                    varInfo.green.offset, varInfo.green.length);
    //printf("blue: offset = %d, length = %d\n", \
                    varInfo.blue.offset, varInfo.blue.length);
    //printf("transp: offset = %d, length = %d\n", \
                    varInfo.transp.offset, varInfo.transp.length);

    close(fd);
    return 0;
}
