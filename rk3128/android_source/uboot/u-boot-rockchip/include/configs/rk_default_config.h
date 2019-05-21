#ifndef __RK_DEFAULT_CONFIG_H
#define __RK_DEFAULT_CONFIG_H


#define CONFIG_RKCHIP_RK3128

/* rk clock module */
#define CONFIG_RK_CLOCK

/* rk gpio module */
#define CONFIG_RK_GPIO

/* LCDC console */
#define CONFIG_LCD

/* rk display module */
#ifdef CONFIG_LCD

#define CONFIG_RK_FB
#define CONFIG_RK3036_FB
#ifndef CONFIG_PRODUCT_BOX
#define CONFIG_RK_PWM
#endif

#ifdef CONFIG_PRODUCT_BOX
#define CONFIG_RK_HDMI
#endif

#define CONFIG_LCD_LOGO
#define CONFIG_LCD_BMP_RLE8
#define CONFIG_CMD_BMP

/* CONFIG_COMPRESS_LOGO_RLE8 or CONFIG_COMPRESS_LOGO_RLE16 */
#undef CONFIG_COMPRESS_LOGO_RLE8
#undef CONFIG_COMPRESS_LOGO_RLE16

#define CONFIG_BMP_16BPP
#define CONFIG_BMP_24BPP
#define CONFIG_BMP_32BPP
#define LCD_BPP             LCD_COLOR16

#define CONFIG_LCD_MAX_WIDTH        4096
#define CONFIG_LCD_MAX_HEIGHT       2048


/*
 *  * select serial console configuration
 *   */
#define CONFIG_BAUDRATE         115200
/* valid baudrates */
#define CONFIG_SYS_BAUDRATE_TABLE   { 9600, 19200, 38400, 57600, 115200 }


/* input clock of PLL: has 24MHz input clock at rk30xx */
#define CONFIG_SYS_CLK_FREQ_CRYSTAL 24000000
#define CONFIG_SYS_CLK_FREQ     CONFIG_SYS_CLK_FREQ_CRYSTAL
#define CONFIG_SYS_HZ           1000    /* decrementer freq: 1 ms ticks */

/* rk lcd size at the end of ddr address */
#define CONFIG_RK_FB_DDREND

#ifdef CONFIG_RK_FB_DDREND
/* support load bmp files for kernel logo */
#define CONFIG_KERNEL_LOGO

/* rk lcd total size = fb size + kernel logo size */
#define CONFIG_RK_LCD_SIZE      SZ_32M
#define CONFIG_RK_FB_SIZE       SZ_16M
#endif

#define CONFIG_BRIGHTNESS_DIM       64

#undef CONFIG_UBOOT_CHARGE

#endif /* CONFIG_LCD */

/* rk power config */
#define CONFIG_POWER
#define CONFIG_POWER_RK

/* rk i2c module */
#define CONFIG_RK_I2C
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_I2C_SPEED            100000

#define CONFIG_SCREEN_ON_VOL_THRESD     0

#define RKCHIP_UNKNOWN      0

/* rockchip cpu mask */
#define ROCKCHIP_CPU_MASK   0xffff0000
#define ROCKCHIP_CPU_RK3026 0x30260000
#define ROCKCHIP_CPU_RK3036 0x30360000 /* audi-s */
#define ROCKCHIP_CPU_RK30XX 0x30660000
#define ROCKCHIP_CPU_RK30XXB    0x31680000
#define ROCKCHIP_CPU_RK312X 0x31260000 /* audi */
#define ROCKCHIP_CPU_RK3188 0x31880000
#define ROCKCHIP_CPU_RK319X 0x31900000
#define ROCKCHIP_CPU_RK3288 0x32880000

/* rockchip cpu type */
#define CONFIG_RK3036           (ROCKCHIP_CPU_RK3036 | 0x00)    /* rk3036 chip */
#define CONFIG_RK3066       (ROCKCHIP_CPU_RK30XX | 0x01)    /* rk3066 chip */
#define CONFIG_RK3126           (ROCKCHIP_CPU_RK312X | 0x00)    /* rk3126 chip */
#define CONFIG_RK3128           (ROCKCHIP_CPU_RK312X | 0x01)    /* rk3128 chip */
#define CONFIG_RK3168       (ROCKCHIP_CPU_RK30XXB | 0x01)   /* rk3168 chip */
#define CONFIG_RK3188       (ROCKCHIP_CPU_RK3188 | 0x00)    /* rk3188 chip */
#define CONFIG_RK3188_PLUS  (ROCKCHIP_CPU_RK3188 | 0x10)    /* rk3188 plus chip */
#define CONFIG_RK3288       (ROCKCHIP_CPU_RK3288 | 0x00)    /* rk3288 chip */


#endif
