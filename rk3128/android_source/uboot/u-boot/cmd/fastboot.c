/*
 * Copyright 2008 - 2009 Windriver, <www.windriver.com>
 * Author: Tom Rix <Tom.Rix@windriver.com>
 *
 * (C) Copyright 2014 Linaro, Ltd.
 * Rob Herring <robh@kernel.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <command.h>
#include <console.h>
#include <g_dnl.h>
#include <usb.h>
#include <blk.h>

#include "decompress_ext4.h"

#if 0
#define FASTBOOT_PARTS_DEFAULT      \
            "flash=mmc,0:uboot:2nd:0x200,0x78000;" \
            "flash=mmc,0:2ndboot:2nd:0x200,0x4000;" \
            "flash=mmc,0:bootloader:boot:0x8000,0x70000;"   \
            "flash=mmc,0:boot:ext4:0x00100000,0x04000000;"      \
            "flash=mmc,0:system:ext4:0x04100000,0x28E00000;"    \
            "flash=mmc,0:cache:ext4:0x2CF00000,0x21000000;"     \
            "flash=mmc,0:misc:emmc:0x4E000000,0x00800000;"      \
            "flash=mmc,0:recovery:emmc:0x4E900000,0x01600000;"  \
            "flash=mmc,0:userdata:ext4:0x50000000,0x0;"
#else

#if 0
#define FASTBOOT_PARTS_DEFAULT      \
            "flash=mmc,0:uboot:2nd:0x100000,0x400000;" \
"flash=mmc,0:boot:ext4:0x500000,0x4000000;"      \
"flash=mmc,0:system:ext4:0x4500000,0x400000;"
#endif

#define FASTBOOT_PARTS_DEFAULT      \
    "flash=mmc,0:miniloader:2nd:0,0x400000;" \
    "flash=mmc,0:parmter:2nd:0x400000,0x400000;" \
    "flash=mmc,0:uboot:2nd:0x800000,0x800000;" \
    "flash=mmc,0:boot:ext4:0x1000000,0x4000000;" \
    "flash=mmc,0:system:ext4:0x5000000,0x4000000;"

#endif

//static const char *const f_parts_default = FASTBOOT_PARTS_DEFAULT;
char *f_parts_default = FASTBOOT_PARTS_DEFAULT;

#define FASTBOOT_MMC_MAX        3
#define FASTBOOT_EEPROM_MAX     1
#define FASTBOOT_NAND_MAX       1
#define FASTBOOT_MEM_MAX        1

#define FASTBOOT_DEV_PART_MAX   (16)                /* each device max partition max num */

/* device types */
#define FASTBOOT_DEV_EEPROM     (1<<0)  /*  name "eeprom" */
#define FASTBOOT_DEV_NAND       (1<<1)  /*  name "nand" */
#define FASTBOOT_DEV_MMC        (1<<2)  /*  name "mmc" */
#define FASTBOOT_DEV_MEM        (1<<3)  /*  name "mem" */

/* filesystem types */
#define FASTBOOT_FS_2NDBOOT     (1<<0)  /*  name "boot" <- bootable */
#define FASTBOOT_FS_BOOT        (1<<1)  /*  name "boot" <- bootable */
#define FASTBOOT_FS_RAW         (1<<2)  /*  name "raw" */
#define FASTBOOT_FS_FAT         (1<<4)  /*  name "fat" */
#define FASTBOOT_FS_EXT4        (1<<5)  /*  name "ext4" */
#define FASTBOOT_FS_UBI         (1<<6)  /*  name "ubi" */
#define FASTBOOT_FS_UBIFS       (1<<7)  /*  name "ubifs" */
#define FASTBOOT_FS_RAW_PART    (1<<8)  /*  name "emmc" */
#define FASTBOOT_FS_FACTORY     (1<<9)  /*  name "factory" */

#define FASTBOOT_FS_MASK        (FASTBOOT_FS_EXT4 | FASTBOOT_FS_FAT | FASTBOOT_FS_UBI | FASTBOOT_FS_UBIFS | FASTBOOT_FS_RAW_PART)

#define TCLK_TICK_HZ            (1000000)

/* Use 65 instead of 64
    * null gets dropped
     * strcpy's need the extra byte */
#define RESP_SIZE               (65)

struct fastboot_device;
struct fastboot_part {
    char partition[32];
    int dev_type;
    int dev_no;
    uint64_t start;     //内存地址，不是块号
    uint64_t length;
    unsigned int fs_type;
    unsigned int flags;
    struct fastboot_device *fd; 
    struct list_head link;
};

struct fastboot_device {
    char *device;
    int dev_max;
    unsigned int dev_type;
    unsigned int part_type;
    unsigned int fs_support;
    uint64_t parts[FASTBOOT_DEV_PART_MAX][2];   /* 0: start, 1: length */
    struct list_head link;
    int (*write_part)(struct fastboot_part *fpart, void *buf, uint64_t length);
    int (*capacity)(struct fastboot_device *fd, int devno, uint64_t *length);
    int (*create_part)(int dev, uint64_t (*parts)[2], int count);
};

struct fastboot_fs_type {
    char *name;
    unsigned int fs_type;
};

/* support fs type */
static struct fastboot_fs_type f_part_fs[] = {
    { "2nd"         , FASTBOOT_FS_2NDBOOT   },
    { "boot"        , FASTBOOT_FS_BOOT      },
    { "factory"     , FASTBOOT_FS_FACTORY   },
    { "raw"         , FASTBOOT_FS_RAW       },
    { "fat"         , FASTBOOT_FS_FAT       },
    { "ext4"        , FASTBOOT_FS_EXT4      },
    { "emmc"        , FASTBOOT_FS_RAW_PART  },
    { "nand"        , FASTBOOT_FS_RAW_PART  },
    { "ubi"         , FASTBOOT_FS_UBI       },
    { "ubifs"       , FASTBOOT_FS_UBIFS     },
};

/* Reserved partition names
 *
 *  NOTE :
 *      Each command must be ended with ";"
 *
 *  partmap :
 *          flash= <device>,<devno> : <partition> : <fs type> : <start>,<length> ;
 *      EX> flash= nand,0:bootloader:boot:0x300000,0x400000;
 *
 *  env :
 *          <command name> = " command arguments ";
 *      EX> bootcmd = "tftp 42000000 uImage";
 *
 *  cmd :
 *          " command arguments ";
 *      EX> "tftp 42000000 uImage";
 *
 */

static const char *f_reserve_part[] = {
    [0] = "partmap",            /* fastboot partition */
    [1] = "mem",                /* download only */
    [2] = "env",                /* u-boot environment setting */
    [3] = "cmd",                /* command run */
};

#define CONFIG_MEM_LOAD_ADDR                0x60800800
#define CFG_MEM_PHY_SYSTEM_SIZE             0x80000000

#define CFG_FASTBOOT_TRANSFER_BUFFER        CONFIG_MEM_LOAD_ADDR
#define CFG_FASTBOOT_TRANSFER_BUFFER_SIZE   (CFG_MEM_PHY_SYSTEM_SIZE - CFG_FASTBOOT_TRANSFER_BUFFER)

struct cmd_fastboot_interface
{
    /* This function is called when a buffer has been
       recieved from the client app.
       The buffer is a supplied by the board layer and must be unmodified.
       The buffer_size is how much data is passed in.
       Returns 0 on success
       Returns 1 on failure

       Set by cmd_fastboot  */
    int (*rx_handler)(const unsigned char *buffer,
            unsigned int buffer_size);

    /* This function is called when an exception has
       occurred in the device code and the state
       off fastboot needs to be reset

       Set by cmd_fastboot */
    void (*reset_handler)(void);

    /* A getvar string for the product name
       It can have a maximum of 60 characters

       Set by board */
    char *product_name;

    /* A getvar string for the serial number
       It can have a maximum of 60 characters

       Set by board */
    char *serial_no;

    /* Nand block size
       Supports the write option WRITE_NEXT_GOOD_BLOCK

       Set by board */
    unsigned int nand_block_size;

    /* Transfer buffer, for handling flash updates
       Should be multiple of the nand_block_size
       Care should be take so it does not overrun bootloader memory
       Controlled by the configure variable CFG_FASTBOOT_TRANSFER_BUFFER

       Set by board */
    unsigned char *transfer_buffer;

    /* How big is the transfer buffer
       Controlled by the configure variable
       CFG_FASTBOOT_TRANSFER_BUFFER_SIZE

       Set by board */
    unsigned int transfer_buffer_size;

};

typedef struct cmd_fastboot_interface f_cmd_inf;

//static f_cmd_inf f_interface = {
//    .rx_handler = NULL,
//    .reset_handler = NULL,
//    .product_name = NULL,
//    .serial_no = NULL,
//    .transfer_buffer = (unsigned char *)CFG_FASTBOOT_TRANSFER_BUFFER,
//    .transfer_buffer_size = CFG_FASTBOOT_TRANSFER_BUFFER_SIZE,
//};

extern int mmc_get_part_table(struct blk_desc *desc, uint64_t (*parts)[2], int *count);
extern ulong mmc_bwrite(struct udevice *dev, lbaint_t start, lbaint_t blkcnt, const void *src);

static int get_parts_from_lists(struct fastboot_part *fpart, uint64_t (*parts)[2], int *count);

static inline void print_response(char *response, char *string)
{
    if (response)
        sprintf(response, "%s", string);
    printf("%s\n", string);
}

static inline void sort_string(char *p, int len) 
{
    int i, j;
    for (i = 0, j = 0; len > i; i++) {
        if (0x20 != p[i] && 0x09 != p[i] && 0x0A != p[i])
            p[j++] = p[i];
    }    
    p[j] = 0; 
}

static inline void parse_comment(const char *str, const char **ret)
{
    const char *p = str, *r;

    do {
        if (!(r = strchr(p, '#')))
            break;
        r++;

        if (!(p = strchr(r, '\n'))) {
            printf("---- not end comments '#' ----\n");
            break;
        }
        p++;
    } while (1);

    /* for next */
    *ret = p;
}

static inline int parse_string(const char *s, const char *e, char *b, int len)
{
    int l, a = 0;

    do { while (0x20 == *s || 0x09 == *s || 0x0a == *s) { s++; } } while(0);

    if (0x20 == *(e-1) || 0x09 == *(e-1))
        do { e--; while (0x20 == *e || 0x09 == *e) { e--; }; a = 1; } while(0);

    l = (e - s + a);
    if (l > len) {
        printf("-- Not enough buffer %d for string len %d [%s] --\n", len, l, s);
        return -1;
    }

    strncpy(b, s, l);
    b[l] = 0;

    return l;
}

static int mmc_check_part_table(struct blk_desc *desc, struct fastboot_part *fpart)
{
    uint64_t parts[FASTBOOT_DEV_PART_MAX][2] = { {0,0}, };
    int i = 0, num = 0; 
    int ret = 1; 

    if (0 > mmc_get_part_table(desc, parts, &num))
        return -1;

    for (i = 0; num > i; i++) {
        if (parts[i][0] == fpart->start &&
                parts[i][1] == fpart->length)
            return 0;
        /* when last partition set value is zero,
           set avaliable length */
        if ((num-1) == i && 
                parts[i][0] == fpart->start &&
                0 == fpart->length) {
            fpart->length = parts[i][1];
            ret = 0; 
            break;
        }    
    }    
    return ret; 
}

int mmc_make_parts(int dev, uint64_t (*parts)[2], int count)
{
    char cmd[1024];
    int i = 0, l = 0, p = 0;

    l = sprintf(cmd, "fdisk %d %d:", dev, count);
    p = l;
    for (i= 0; count > i; i++) {
        l = sprintf(&cmd[p], " 0x%llx:0x%llx", parts[i][0], parts[i][1]);
        p += l;
    }

    if (p >= sizeof(cmd)) {
        printf("** %s: cmd stack overflow : stack %d, cmd %d **\n",
                __func__, sizeof(cmd), p);
        while(1);
    }

    cmd[p] = 0;
    printf("%s\n", cmd);

    /* "fdisk <dev no> [part table counts] <start:length> <start:length> ...\n" */
    return run_command(cmd, 0);
}


static void part_dev_print(struct fastboot_device *fd)
{
    struct fastboot_part *fp;
    struct list_head *entry, *n;

    printf("Device: %s\n", fd->device);
    struct list_head *head = &fd->link;
    if (!list_empty(head)) {
        list_for_each_safe(entry, n, head) {
            fp = list_entry(entry, struct fastboot_part, link);
            printf(" %s.%d: %s, %s : 0x%llx, 0x%llx\n",
                    fd->device, fp->dev_no, fp->partition,
                    FASTBOOT_FS_MASK&fp->fs_type?"fs":"img",
                    fp->start, fp->length);
        }
    }
}

static int mmc_part_write(struct fastboot_part *fpart, void *buf, uint64_t length)
{
    struct blk_desc *desc;
    struct fastboot_device *fd = fpart->fd;
    int dev = fpart->dev_no;
    lbaint_t blk, cnt;
    int blk_size = 512;
    char cmd[32];
    int ret = 0;

    sprintf(cmd, "mmc dev %d", dev);

    debug("** mmc.%d partition %s (%s)**\n",
            dev, fpart->partition, fpart->fs_type&FASTBOOT_FS_EXT4?"FS":"Image");

    /* set mmc devicee */
    if (0 > blk_get_device_by_str("mmc", simple_itoa(dev), &desc)) {
        if (0 > run_command(cmd, 0))
            return -1;
        if (0 > run_command("mmc rescan", 0))
            return -1;
    }

    if (0 > run_command(cmd, 0))    /* mmc device */
        return -1;

    if (0 > blk_get_device_by_str("mmc", simple_itoa(dev), &desc))
        return -1;

    if (fpart->fs_type == FASTBOOT_FS_2NDBOOT ||
            fpart->fs_type == FASTBOOT_FS_BOOT) {
        char args[64];
        int l = 0, p = 0;

        if (fpart->fs_type == FASTBOOT_FS_2NDBOOT)
            p = sprintf(args, "update_mmc %d 2ndboot", dev);
        else
            p = sprintf(args, "update_mmc %d boot", dev);

        l = sprintf(&args[p], " %p 0x%llx 0x%llx", buf, fpart->start, length);
        p += l;
        args[p] = 0;

        return run_command(args, 0); /* update_mmc [dev no] <type> 'mem' 'addr' 'length' [load addr] */
    }

    if (fpart->fs_type & FASTBOOT_FS_MASK) {

        ret = mmc_check_part_table(desc, fpart);
        if (0 > ret)
            return -1;

        if (ret) {  /* new partition */
            uint64_t parts[FASTBOOT_DEV_PART_MAX][2] = { {0,0}, };
            int num;

            printf("Warn  : [%s] make new partitions ....\n", fpart->partition);
            part_dev_print(fpart->fd);

            mdelay(1000);
            printf("--->get_parts_from_lists\r\n");
            get_parts_from_lists(fpart, parts, &num);
            ret = mmc_make_parts(dev, parts, num);
            mdelay(1000);
            if (0 > ret) {
                printf("** Fail make partition : %s.%d %s**\n",
                        fd->device, dev, fpart->partition);
                return -1;
            }
        }

        if (mmc_check_part_table(desc, fpart))
            return -1;
    }

    if ((fpart->fs_type & FASTBOOT_FS_EXT4) )
    {
        if(0 == check_compress_ext4((char*)buf, fpart->length)) 
        {
            debug("write compressed ext4 ...\n");
            return write_compressed_ext4((char*)buf, fpart->start/blk_size);
            //return write_compressed_ext4((char*)buf, fpart->start);
        }
    }

    blk = fpart->start/blk_size ;
    cnt = (length/blk_size) + ((length & (blk_size-1)) ? 1 : 0);

    printf("write image to 0x%llx(0x%x), 0x%llx(0x%x)\n",
            fpart->start, (unsigned int)blk, length, (unsigned int)blk);
    
    ret = mmc_bwrite(desc->bdev, blk, cnt, buf);

    return (0 > ret ? ret : 0);
}

static int get_parts_from_lists(struct fastboot_part *fpart, uint64_t (*parts)[2], int *count)
{
    struct fastboot_part *fp = fpart;
    struct fastboot_device *fd = fpart->fd;
    struct list_head *head = &fd->link;
    struct list_head *entry, *n;
    int dev = fpart->dev_no;
    int i = 0;

    if (!parts || !count) {
        printf("-- No partition input params --\n");
        return -1;
    }
    *count = 0;

    if (list_empty(head))
        return 0;

    list_for_each_safe(entry, n, head) {
        fp = list_entry(entry, struct fastboot_part, link);
        if ((FASTBOOT_FS_MASK & fp->fs_type) &&
                (dev == fp->dev_no)) {
            parts[i][0] = fp->start;
            parts[i][1] = fp->length;
            i++;
            debug("%s.%d = %s\n", fd->device, dev, fp->partition);
        }
    }

    *count = i; /* set part count */
    return 0;
}

static int mmc_part_capacity(struct fastboot_device *fd, int devno, uint64_t *length)
{
    struct blk_desc *desc;
    char cmd[32];

    debug("** mmc.%d capacity **\n", devno);

    /* set mmc devicee */
    if (0 > blk_get_device_by_str("mmc", simple_itoa(devno), &desc)) {
        sprintf(cmd, "mmc dev %d", devno);
        if (0 > run_command(cmd, 0))
            return -1;
        if (0 > run_command("mmc rescan", 0))
            return -1;
    }

    if (0 > blk_get_device_by_str("mmc", simple_itoa(devno), &desc))
        return -1;

    *length = (uint64_t)desc->lba * (uint64_t)desc->blksz;

    debug("%u*%u = %llu\n", (uint)desc->lba, (uint)desc->blksz, *length);

    return 0;
}

static int parse_part_address(const char *parts, const char **ret,
        struct fastboot_device **fdev, struct fastboot_part *fpart)
{
    const char *p, *id;
    char str[64] = { 0, };
    int id_len;

    if (ret)
        *ret = NULL;

    id = p = parts;
    if (!(p = strchr(id, ';')) && !(p = strchr(id, '\n'))) {
        printf("no <; or '\n'> identifier\n");
        return -1;
    }
    id_len = p - id;

    /* for next */
    p++, *ret = p;

    if (!(p = strchr(id, ','))) {
        printf("no <start> identifier\n");
        return -1;
    }

    parse_string(id, p, str, sizeof(str));
    fpart->start = simple_strtoull(str, NULL, 16);
    debug("start : 0x%llx\n", fpart->start);

    p++;
    parse_string(p, p+id_len, str, sizeof(str));    /* dev no*/
    fpart->length = simple_strtoull(str, NULL, 16);
    debug("length: 0x%llx\n", fpart->length);

    return 0;
}

static struct fastboot_device f_devices[] = {
    {
        .device     = "eeprom",
        .dev_max    = FASTBOOT_EEPROM_MAX,
        .dev_type   = FASTBOOT_DEV_EEPROM,
        .fs_support = (FASTBOOT_FS_2NDBOOT | FASTBOOT_FS_BOOT | FASTBOOT_FS_RAW),
#ifdef CONFIG_CMD_EEPROM
        .write_part = eeprom_part_write,
#endif//CONFIG_CMD_EEPROM
    },
#if defined(CONFIG_NAND_MTD)
    {
        .device     = "nand",
        .dev_max    = FASTBOOT_NAND_MAX,
        .dev_type   = FASTBOOT_DEV_NAND,
        .fs_support = (FASTBOOT_FS_2NDBOOT | FASTBOOT_FS_BOOT | FASTBOOT_FS_RAW | FASTBOOT_FS_UBI),
#ifdef CONFIG_CMD_NAND
        .write_part = nand_part_write,
#endif//CONFIG_CMD_NAND
    },
#else /* CONFIG_NAND_FTL */
    {
        .device     = "nand",
        .dev_max    = FASTBOOT_NAND_MAX,
        .dev_type   = FASTBOOT_DEV_NAND,
        .fs_support = (FASTBOOT_FS_2NDBOOT | FASTBOOT_FS_BOOT | FASTBOOT_FS_RAW | FASTBOOT_FS_EXT4 |
                FASTBOOT_FS_RAW_PART | FASTBOOT_FS_FACTORY),
#ifdef CONFIG_CMD_NAND
        .write_part = nand_part_write,
        .capacity = nand_part_capacity,
        .create_part = nand_make_parts
#endif//CONFIG_CMD_NAND
    },

#endif//CONFIG_NAND_FTL
    {
        .device     = "mmc",
        .dev_max    = FASTBOOT_MMC_MAX,
        .dev_type   = FASTBOOT_DEV_MMC,
        .part_type  = PART_TYPE_DOS,
        .fs_support = (FASTBOOT_FS_2NDBOOT | FASTBOOT_FS_BOOT | FASTBOOT_FS_RAW |
                FASTBOOT_FS_FAT | FASTBOOT_FS_EXT4 | FASTBOOT_FS_RAW_PART),
#ifdef CONFIG_CMD_MMC
        .write_part = mmc_part_write,
        .capacity = mmc_part_capacity,
        .create_part = mmc_make_parts,
#endif
    },
};

#define FASTBOOT_DEV_SIZE   ARRAY_SIZE(f_devices)

typedef int (parse_fnc_t) (const char *parts, const char **ret,
        struct fastboot_device **fdev, struct fastboot_part *fpart);

int part_lists_check(const char *part)
{
    struct fastboot_device *fd = f_devices;
    struct fastboot_part *fp; 
    struct list_head *entry, *n;
    int i =0;

    for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
        struct list_head *head = &fd->link;
        if (list_empty(head))
            continue;
        list_for_each_safe(entry, n, head) {
            fp = list_entry(entry, struct fastboot_part, link);
            if (!strcmp(fp->partition, part)) {
                return 0;
            }    
        }    
    }    
    return -1;
}

static int parse_part_device(const char *parts, const char **ret,
        struct fastboot_device **fdev, struct fastboot_part *fpart)
{
    struct fastboot_device *fd = *fdev;
    const char *p, *id, *c;
    char str[32];
    int i = 0, id_len;

    if (ret)
        *ret = NULL;

    id = p = parts;
    if (!(p = strchr(id, ':'))) {
        printf("no <dev-id> identifier\n");
        return 1;
    }    
    id_len = p - id;

    /* for next */
    p++, *ret = p; 

    c = strchr(id, ',');
    parse_string(id, c, str, sizeof(str));

    for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
        if (strcmp(fd->device, str) == 0) { 
            /* add to device */
            list_add_tail(&fpart->link, &fd->link);
            *fdev = fd;

            /* dev no */
            debug("device: %s", fd->device);
            if (!(p = strchr(id, ','))) {
                printf("no <dev-no> identifier\n");
                return -1;
            }    
            p++; 
            parse_string(p, p+id_len, str, sizeof(str));    /* dev no*/
            /* dev no */
            fpart->dev_no = simple_strtoul(str, NULL, 10); 
            if (fpart->dev_no >= fd->dev_max) {
                printf("** Over dev-no max %s.%d : %d **\n",
                        fd->device, fd->dev_max-1, fpart->dev_no);
                return -1;
            }    

            debug(".%d\n", fpart->dev_no);
            fpart->fd = fd;
            return 0;
        }    
    }    

    /* to delete */
    fd = *fdev;
    strcpy(fpart->partition, "unknown");
    list_add_tail(&fpart->link, &fd->link);

    printf("** Can't device parse : %s **\n", parts);
    return -1;
}

static int parse_part_fs(const char *parts, const char **ret,
        struct fastboot_device **fdev, struct fastboot_part *fpart)
{
    struct fastboot_device *fd = *fdev;
    struct fastboot_fs_type *fs = f_part_fs;
    const char *p, *id;
    char str[16] = { 0, };
    int i = 0;

    if (ret)
        *ret = NULL;

    id = p = parts;
    if (!(p = strchr(id, ':'))) {
        printf("no <dev-id> identifier\n");
        return -1;
    }

    /* for next */
    p++, *ret = p;
    p--; parse_string(id, p, str, sizeof(str));

    for (; ARRAY_SIZE(f_part_fs) > i; i++, fs++) {
        if (strcmp(fs->name, str) == 0) {
            if (!(fd->fs_support & fs->fs_type)) {
                printf("** '%s' not support '%s' fs **\n", fd->device, fs->name);
                return -1;
            }

            fpart->fs_type = fs->fs_type;
            debug("fs    : %s\n", fs->name);
            return 0;
        }
    }

    printf("** Can't fs parse : %s **\n", str);
    return -1;
}

static int parse_part_partition(const char *parts, const char **ret,
        struct fastboot_device **fdev, struct fastboot_part *fpart)
{
    struct fastboot_device *fd = f_devices;
    struct fastboot_part *fp;
    struct list_head *entry, *n;
    const char *p, *id;
    char str[32] = { 0, };
    int i = 0;

    if (ret)
        *ret = NULL;

    id = p = parts;
    if (!(p = strchr(id, ':'))) {
        printf("no <name> identifier\n");
        return -1;
    }

    /* for next */
    p++, *ret = p;
    p--; parse_string(id, p, str, sizeof(str));

    for (i = 0; ARRAY_SIZE(f_reserve_part) > i; i++) {
        const char *r =f_reserve_part[i];
        if (!strcmp(r, str)) {
            printf("** Reserved partition name : %s  **\n", str);
            return -1;
        }
    }

    /* check partition */
    for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
        struct list_head *head = &fd->link;
        if (list_empty(head))
            continue;
        list_for_each_safe(entry, n, head) {
            fp = list_entry(entry, struct fastboot_part, link);
            if (!strcmp(fp->partition, str)) {
                printf("** Exist partition : %s -> %s **\n",
                        fd->device, fp->partition);
                strcpy(fpart->partition, str);
                fpart->partition[strlen(str)] = 0;
                return -1;
            }
        }
    }

    strcpy(fpart->partition, str);
    fpart->partition[strlen(str)] = 0;
    debug("part  : %s\n", fpart->partition);

    return 0;
}

parse_fnc_t *parse_part_seqs[] = {
    parse_part_device,
    parse_part_partition,
    parse_part_fs,
    parse_part_address,
    0,  /* end */
};

static int parse_part_head(const char *parts, const char **ret)
{
    const char *p = parts;
    int len = strlen("flash=");

    debug("\n");
    if (!(p = strstr(p, "flash=")))
        return -1;

    *ret = p + len;
    return 0;
}

static inline void part_lists_init(int init)
{
    struct fastboot_device *fd = f_devices;
    struct fastboot_part *fp;

    struct list_head *entry, *n;
    int i = 0;

    for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
        struct list_head *head = &fd->link;
        //struct list_head *head = &fd->list;

        if (init) {
            INIT_LIST_HEAD(head);
            memset(fd->parts, 0x0, sizeof(FASTBOOT_DEV_PART_MAX*2));
            continue;
        }

        if (list_empty(head))
            continue;

        //debug("delete [%s]:", fd->gpt_part_info.name);
        list_for_each_safe(entry, n, head) {
            fp = list_entry(entry, struct fastboot_part, link);
            //fp = list_entry(entry, struct disk_part, list);
            debug("%s ", fp->partition);
            list_del(entry);
            free(fp);
        }
        debug("\n");
        INIT_LIST_HEAD(head);
    }
}


static int part_lists_make(const char *ptable_str, int ptable_str_len)
{
    struct fastboot_device *fd = f_devices;
    struct fastboot_part *fp;
    parse_fnc_t **p_fnc_ptr;
    const char *p = ptable_str;
    int len = ptable_str_len;
    int err = -1;

    debug("\n---part_lists_make ---\n");
    part_lists_init(0);

    parse_comment(p, &p);
    sort_string((char*)p, len);

    /* new parts table */
    while (1) {
        fd = f_devices;
        fp = malloc(sizeof(*fp));

        if (!fp) {
            printf("** Can't malloc fastboot part table entry **\n");
            err = -1;
            break;
        }

        if (parse_part_head(p, &p)) {
            if (err)
                printf("-- unknown parts head: [%s]\n", p);
            break;
        }

        for (p_fnc_ptr = parse_part_seqs; *p_fnc_ptr; ++p_fnc_ptr) {
            if ((*p_fnc_ptr)(p, &p, &fd, fp) != 0) {
                err = -1;
                goto fail_parse;
            }
        }
        err = 0;
    }

fail_parse:
    if (err)
        part_lists_init(0);

    return err;
}

static void part_lists_print(void)
{
    struct fastboot_device *fd = f_devices;
    struct fastboot_part *fp;
    struct list_head *entry, *n;
    int i;

    printf("\nFastboot Partitions:\n");
    for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
        struct list_head *head = &fd->link;
        if (list_empty(head))
            continue;
        list_for_each_safe(entry, n, head) {
            fp = list_entry(entry, struct fastboot_part, link);
            printf(" %s.%d: %s, %s : 0x%llx, 0x%llx\n",
                    fd->device, fp->dev_no, fp->partition,
                    FASTBOOT_FS_MASK&fp->fs_type?"fs":"img", fp->start, fp->length);
        }
    }

    printf("Support fstype :");
    for (i = 0; ARRAY_SIZE(f_part_fs) > i; i++)
        printf(" %s ", f_part_fs[i].name);
    printf("\n");

    printf("Reserved part  :");
    for (i = 0; ARRAY_SIZE(f_reserve_part) > i; i++)
        printf(" %s ", f_reserve_part[i]);
    printf("\n");
}

/*
 *
 *  fboot_cmd_flash: 将从内存地址addr处开始的内容一共download_bytes个字节
 *  写入mmc中的cmd分区上。
 *
 */
int fboot_cmd_flash(const char *cmd, unsigned char *addr, unsigned int download_bytes)
{
    int i;
    struct fastboot_device *fd = f_devices;
    struct fastboot_part *fp;
    struct list_head *entry, *n; 

    for(i=0; i < FASTBOOT_DEV_SIZE; i++, fd++)
    {   
        struct list_head *head = &fd->link;
        if(list_empty(head))
            continue;

        list_for_each_safe(entry, n, head)
        {   
            fp = list_entry(entry, struct fastboot_part, link);
            if(!strcmp(fp->partition, cmd))
            {   
                //执行mmc写入操作
                //if(((uint64_t)download_bytes > fp->length) && (fp->length != 0)) 
                //{   
                //    printf("--->FAIL image too large for partition\r\n");
                //    printf("--->download_bytes= %d\r\n", download_bytes);
                //    printf("--->fp->length = %d\r\n", fp->length);

                //    return -1;
                //}   

                if((fd->write_part) && (fd->dev_type != FASTBOOT_DEV_MEM))
                {   
                    if(fd->write_part(fp, addr, download_bytes) < 0)
                    {   
                        printf("FAIL to flash partition\r\n");
                        return -1;
                    }   
                }   
            }   
        }   
    } 

    return 0;
}

static int do_fastboot(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int controller_index;
	char *usb_controller;
	int ret;

//add by duanshuai start
    static int init_parts = 0;
    int err;
    const char *p;
    int i;
    struct fastboot_device *fd = f_devices;
    struct fastboot_part *fp;
    struct list_head *entry, *n;
    //char resp[RESP_SIZE] = "OKAY";
    const char *cmd = "boot";
//add by duanshuai end

	if (argc < 2)
		return CMD_RET_USAGE;

	usb_controller = argv[1];
	controller_index = simple_strtoul(usb_controller, NULL, 0);

	ret = board_usb_init(controller_index, USB_INIT_DEVICE);
	if (ret) {
		error("USB init failed: %d", ret);
		return CMD_RET_FAILURE;
	}

	g_dnl_clear_detach();
	ret = g_dnl_register("usb_dnl_fastboot");
	if (ret)
		return ret;

	if (!g_dnl_board_usb_cable_connected()) {
		puts("\rUSB cable not detected.\n" \
		     "Command exit.\n");
		ret = CMD_RET_FAILURE;
		goto exit;
	}
//add by duanshuai start
    if(!init_parts)
    {
        p = FASTBOOT_PARTS_DEFAULT;
        part_lists_init(1);
        err = part_lists_make(p, strlen(p));
        if(err < 0)
            return err;

        init_parts = 1;

        part_lists_print();

        //new add start
        for (i = 0; FASTBOOT_DEV_SIZE > i; i++, fd++) {
            struct list_head *head = &fd->link;
            if (list_empty(head))
                continue;

            list_for_each_safe(entry, n, head) {
                fp = list_entry(entry, struct fastboot_part, link);
                if (!strcmp(fp->partition, cmd)) {

                    /*if ((fst->down_bytes > fp->length) && (fp->length != 0)) {
                      print_response(resp, "FAIL image too large for partition");
                      goto err_flash;
                      } */   
                    int dev = fp->dev_no;
                    struct fastboot_device *fd_tmp = fp->fd;
                    struct blk_desc *desc;
                    char cmd_tmp[32];

                    sprintf(cmd_tmp, "mmc dev %d", dev);

                    debug("** mmc.%d partition %s (%s)**\n",
                            dev, fp->partition, fp->fs_type&FASTBOOT_FS_EXT4?"FS":"Image");

                    /* set mmc devicee */
                    if (0 > blk_get_device_by_str("mmc", simple_itoa(dev), &desc)) {
                        if (0 > run_command(cmd_tmp, 0))
                            return -1;
                        if (0 > run_command("mmc rescan", 0))
                            return -1;
                    }

                    if (0 > run_command(cmd_tmp, 0))    /* mmc device */
                        return -1;

                    if (0 > blk_get_device_by_str("mmc", simple_itoa(dev), &desc))
                        return -1;

                    if (fp->fs_type & FASTBOOT_FS_MASK) {

                        ret = mmc_check_part_table(desc, fp);
                        if (0 > ret)
                            return -1;

                        if (ret) {  /* new partition */
                            uint64_t parts[FASTBOOT_DEV_PART_MAX][2] = { {0,0}, };
                            int num;

                            printf("Warn  : [%s] make new partitions ....\n", fp->partition);
                            part_dev_print(fp->fd);

                            mdelay(1000);
                            printf("--->get_parts_from_lists\r\n");
                            get_parts_from_lists(fp, parts, &num);
                            ret = mmc_make_parts(dev, parts, num);
                            mdelay(1000);
                            if (0 > ret) {
                                printf("** Fail make partition : %s.%d %s**\n",
                                        fd_tmp->device, dev, fp->partition);
                                return -1;
                            }
                        }

                        if (mmc_check_part_table(desc, fp))
                            return -1;
                    }

                    goto done_flash;
                }    
            }    
        }    
        //new add end
    }
//add by duanshuai end
done_flash:

    printf("goto while\r\n");

	while (1) {
		if (g_dnl_detach())
			break;
		if (ctrlc())
			break;
		usb_gadget_handle_interrupts(controller_index);
	}

	ret = CMD_RET_SUCCESS;

exit:
	g_dnl_unregister();
	g_dnl_clear_detach();
	board_usb_cleanup(controller_index, USB_INIT_DEVICE);

	return ret;
}

U_BOOT_CMD(
	fastboot, 2, 1, do_fastboot,
	"use USB Fastboot protocol",
	"<USB_controller>\n"
	"    - run as a fastboot usb device"
);
