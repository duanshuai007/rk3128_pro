在uboot中添加ext4分区功能，达到可以用fastboot对分区进行烧写的效果。
###目标uboot版本：2018-05

```
git clone git://git.denx.de/u-boot-rockchip.git
使用master分支进行编译。

```	

##1.修改Makefile

编译uboot需要使用对应版本的编译工具gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabihf.tar

下载地址:[http://releases.linaro.org/components/toolchain/binaries/latest/arm-linux-gnueabihf/](http://releases.linaro.org/components/toolchain/binaries/latest/arm-linux-gnueabihf/)

```
ARCH            ?= arm
CROSS_COMPILE   ?= gcc-linaro-7.2.1/bin/arm-linux-gnueabihf-

PS：这里的CROSS_COMPILE仅做示例，需要根据自己的编译工具安装目录进行填写。
```

##2.修改drivers/usb/gadget/f_fastboot.c文件
```
在static void cb_getvar(struct usb_ep *ep, struct usb_request *req)
函数外添加extern int part_lists_check(const char *part);
在函数内的最后一个else之前添加

    else if( !strncmp(cmd, "partition-type:", strlen("partition-type:"))) 
    {
        char *tmp = (char *)cmd;
        tmp += strlen("partition-type:");
        if(part_lists_check(tmp))
        {
            strcpy(response, "FAIL bad partition");
            goto done_getvar;
        }
        printf("\r\nREADY : ");
        printf("%s\r\n", cmd+strlen("partition-type:"));
    }
    
并在函数末尾的fastboot_tx_write_str(response);之前添加跳转标志done_getvar：.

修改后的效果：
    //add by duanshuai start
    else if( !strncmp(cmd, "partition-type:", strlen("partition-type:"))) 
    {
        char *tmp = (char *)cmd;
        tmp += strlen("partition-type:");
        if(part_lists_check(tmp))
        {
            strcpy(response, "FAIL bad partition");
            goto done_getvar;
        }
        printf("\r\nREADY : ");
        printf("%s\r\n", cmd+strlen("partition-type:"));
    }
    //add by duanshuai end
    else {

    char *envstr;

    envstr = malloc(strlen("fastboot.") + strlen(cmd) + 1);
    if (!envstr) {
      fastboot_tx_write_str("FAILmalloc error");
      return;
    }

    sprintf(envstr, "fastboot.%s", cmd);
    s = env_get(envstr);
    if (s) {
      strncat(response, s, chars_left);
    } else {
      printf("WARNING: unknown variable: %s\n", cmd);
      strcpy(response, "FAILVariable not implemented");
    }

    free(envstr);
  }

done_getvar:
    
  fastboot_tx_write_str(response);
}
```

##3.修改common/fb_mmc.c文件
```
添加extern int fboot_cmd_flash(const char *cmd, unsigned char *addr, unsigned int download_bytes);
在函数void fb_mmc_flash_write(const char *cmd, void *download_buffer,unsigned int download_bytes)

  struct blk_desc *dev_desc;
  disk_partition_t info;
//添加内容开始
    printf("--->%s, cmd=%s\r\n", __func__, cmd);
    printf("--->download_buffer=%08x\r\n", download_buffer);
    printf("--->download_bytes=%08x\r\n", download_bytes);
//添加内容结束
      dev_desc = blk_get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
      if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
        pr_err("invalid mmc device\n");
        fastboot_fail("invalid mmc device");
        return;
      }

#if CONFIG_IS_ENABLED(DOS_PARTITION)
  if (strcmp(cmd, CONFIG_FASTBOOT_MBR_NAME) == 0) {
    printf("%s: updating MBR\n", __func__);
    if (is_valid_dos_buf(download_buffer)) {
      printf("%s: invalid MBR - refusing to write to flash\n",
             __func__);
      fastboot_fail("invalid MBR partition");
      return;
    }
    if (write_mbr_partition(dev_desc, download_buffer)) {
      printf("%s: writing MBR partition failed\n", __func__);
      fastboot_fail("writing MBR partition failed");
      return;
    }
    printf("........ success\n");
    fastboot_okay("");
    return;
  }
    //添加内容开始
    else{
       fboot_cmd_flash(cmd, download_buffer, download_bytes);
    	printf("........ success\n");
    	fastboot_okay("");
       return;
    }
    //添加内容结束
#endif
```

##4.在cmd文件夹内新增四个文件
###4.1 cmd\_mmc\_update.c
```
    /*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Misc boot support
 */
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <mmc.h>
#include "decompress_ext4.h"
#include <asm/sections.h>
#include <blk.h>

#define MMC_BLOCK_SIZE    (512)

struct boot_dev_eeprom {
    char    addr_step;
    char    resv0[3];
    unsigned int resv1;
    unsigned int crc32;
};

struct boot_dev_mmc {
    char port_no;
    char resv0[3];
    char rese1;
    unsigned int crc32;     /* not use : s5p4418 */
};

union boot_dev_data {
    struct boot_dev_eeprom spirom;
    struct boot_dev_mmc mmc;
};


#define U32 unsigned int

#define SIGNATURE_ID ((((U32)'N')<< 0) | (((U32)'S')<< 8) | (((U32)'I')<<16) | (((U32)'H')<<24))

struct boot_dev_head {
    unsigned int vector[8];         // 0x000 ~ 0x01C
    unsigned int vector_rel[8];     // 0x020 ~ 0x03C
    unsigned int dev_addr;          // 0x040

    /* boot device info */
    unsigned int  load_size;        // 0x044
    unsigned int  load_addr;        // 0x048
    unsigned int  jump_addr;        // 0x04C
    union boot_dev_data bdi;        // 0x050~0x058

    unsigned int  resv_pll[4];              // 0x05C ~ 0x068
    unsigned int  resv_pll_spread[2];       // 0x06C ~ 0x070
    unsigned int  resv_dvo[5];              // 0x074 ~ 0x084
    char resv_ddr[36];              // 0x088 ~ 0x0A8
    unsigned int  resv_axi_b[32];       // 0x0AC ~ 0x128
    unsigned int  resv_axi_d[32];       // 0x12C ~ 0x1A8
    unsigned int  resv_stub[(0x1F8-0x1A8)/4];   // 0x1AC ~ 0x1F8
    unsigned int  signature;            // 0x1FC    "NSIH"
};

extern ulong mmc_bwrite(struct udevice *desc, lbaint_t start, lbaint_t blkcnt, const void *src);
extern int mmc_get_part_table(struct blk_desc *desc, uint64_t (*parts)[2], int *part_num);

/* update_mmc [dev no] <type> 'mem' 'addr' 'length' [load addr] */
int do_update_mmc(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
  struct blk_desc *desc;
  uint64_t dst_addr = 0, mem_len = 0;
  unsigned int mem_addr = 0;
  unsigned char *p;
  char cmd[32];
  lbaint_t blk, cnt;
  int ret, dev;

  if (6 > argc)
    goto usage;

  ret = blk_get_device_by_str("mmc", argv[1], &desc);
  if (0 > ret) {
    printf ("** Not find device mmc.%s **\n", argv[1]);
    return 1;
  }

  dev = simple_strtoul (argv[1], NULL, 10);
  sprintf(cmd, "mmc dev %d", dev);
  if (0 > run_command(cmd, 0))  /* mmc device */
    return -1;

  if (0 != strcmp(argv[2], "2ndboot")  &&
    0 != strcmp(argv[2], "boot") &&
    0 != strcmp(argv[2], "raw")  &&
    0 != strcmp(argv[2], "part"))
    goto usage;

  mem_addr = simple_strtoul (argv[3], NULL, 16);
  dst_addr = simple_strtoull(argv[4], NULL, 16);
  mem_len  = simple_strtoull(argv[5], NULL, 16);

  p   = (unsigned char *)((ulong)mem_addr);
  blk = (dst_addr/MMC_BLOCK_SIZE);
  cnt = (mem_len/MMC_BLOCK_SIZE) + ((mem_len & (MMC_BLOCK_SIZE-1)) ? 1 : 0);

  flush_dcache_all();

  if (! strcmp(argv[2], "2ndboot")) {
    struct boot_dev_head *bh = (struct boot_dev_head *)((ulong)mem_addr);
    struct boot_dev_mmc  *bd = (struct boot_dev_mmc *)&bh->bdi;

    bd->port_no = dev; /* set u-boot device port num */
    printf("head boot dev  = %d\n", bd->port_no);

    goto do_write;
  }

  if (! strcmp(argv[2], "boot")) {
    struct boot_dev_head head;
    struct boot_dev_head *bh = &head;
    struct boot_dev_mmc *bd = (struct boot_dev_mmc *)&bh->bdi;
    int len = sizeof(head);
    unsigned int load = CONFIG_SYS_TEXT_BASE;

    if (argc == 7)
      load = simple_strtoul (argv[6], NULL, 16);

    memset((void*)&head, 0x00, len);

    bh->load_addr = (unsigned int)load;
    bh->jump_addr = bh->load_addr;
    bh->load_size = (unsigned int)mem_len;
    bh->signature = SIGNATURE_ID;
    bd->port_no   = dev;

    printf("head boot dev  = %d\n", bd->port_no);
    printf("head load addr = 0x%08x\n", bh->load_addr);
    printf("head load size = 0x%08x\n", bh->load_size);
    printf("head gignature = 0x%08x\n", bh->signature);

    p -= len;
    memcpy(p, bh, len);

    mem_len += MMC_BLOCK_SIZE;
    cnt = (mem_len/MMC_BLOCK_SIZE) + ((mem_len & (MMC_BLOCK_SIZE-1)) ? 1 : 0);

    goto do_write;
  }

  if (strcmp(argv[2], "part") == 0) {
    uint64_t parts[4][2] = { {0,0}, };
    uint64_t part_len = 0;
    int partno = (int)dst_addr;
    int num = 0;

    if (0 > mmc_get_part_table(desc, parts, &num))
      return 1;

    if (partno > num || 1 > partno)  {
      printf ("** Invalid mmc.%d partition number %d (1 ~ %d) **\n",
        dev, partno, num);
      return 1;
    }

    dst_addr = parts[partno-1][0];  /* set write addr from part table */
    part_len = parts[partno-1][1];
    blk = (dst_addr/MMC_BLOCK_SIZE);

    if (0 == check_compress_ext4((char*)p, part_len)) {
      printf("update mmc.%d compressed ext4 = 0x%llx(%d) ~ 0x%llx(%d): ",
        dev, dst_addr, (unsigned int)blk, mem_len, (unsigned int)cnt);

      ret = write_compressed_ext4((char*)p, blk);
      printf("%s\n", ret?"Fail":"Done");
      return 1;
    }
    goto do_write;
  }

do_write:
  if (! blk) {
    printf("-- Fail: start %d block(0x%llx) is in MBR zone (0x200) --\n", (int)blk, dst_addr);
    return -1;
  }

  printf("update mmc.%d type %s = 0x%llx(0x%x) ~ 0x%llx(0x%x): ",
    dev, argv[2], dst_addr, (unsigned int)blk, mem_len, (unsigned int)cnt);

  ret = mmc_bwrite(desc->bdev, blk, cnt, (void const*)p);

  printf("%s\n", ret?"Done":"Fail");
  return ret;

usage:
  cmd_usage(cmdtp);
  return 1;
}

U_BOOT_CMD(
  update_mmc, CONFIG_SYS_MAXARGS, 1,  do_update_mmc,
  "update mmc data\n",
  "<dev no> <type> <mem> <addr> <length>\n"
  "    - type :  2ndboot | boot | raw | part \n\n"
  "update_mmc <dev no> boot 'mem' 'addr' 'length' [load addr]\n"
  "    - update  data 'length' add boot header(512) on 'mem' to device addr, \n"
  "      and set jump addr with 'load addr'\n"
  "      if no [load addr], set jump addr default u-boot _TEXT_BASE_\n\n"
  "update_mmc <dev no> raw 'mem' 'addr' 'length'\n"
  "    - update data 'length' on 'mem' to device addr.\n\n"
  "update_mmc <dev no> part 'mem' 'part no' 'length'\n"
  "    - update partition image 'length' on 'mem' to mmc 'part no'.\n\n"
  "Note.\n"
  "    - All numeric parameters are assumed to be hex.\n"
);

```
###4.2  cmd\_mmcparts.c
```
/*
 * (C) Copyright 2009 Nexell Co.,
 * jung hyun kim<jhkim@nexell.co.kr>
 *
 * Configuation settings for the Nexell board.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <mmc.h>

/*
#define debug printf
*/

#define MMC_BLOCK_SIZE      (512)
#define MAX_PART_TABLE      (8)
#define DOS_EBR_BLOCK       (0x100000/MMC_BLOCK_SIZE)

#define DOS_PART_DISKSIG_OFFSET   0x1b8
#define DOS_PART_TBL_OFFSET     0x1be
#define DOS_PART_MAGIC_OFFSET   0x1fe
#define DOS_PBR_FSTYPE_OFFSET   0x36
#define DOS_PBR32_FSTYPE_OFFSET   0x52
#define DOS_PBR_MEDIA_TYPE_OFFSET 0x15
#define DOS_MBR 0
#define DOS_PBR 1

typedef struct dos_partition {
  unsigned char boot_ind;   /* 0x80 - active      */
  unsigned char head;   /* starting head      */
  unsigned char sector;   /* starting sector      */
  unsigned char cyl;    /* starting cylinder      */
  unsigned char sys_ind;    /* What partition type      */
  unsigned char end_head;   /* end head       */
  unsigned char end_sector; /* end sector       */
  unsigned char end_cyl;    /* end cylinder       */
  unsigned char start4[4];  /* starting sector counting from 0  */
  unsigned char size4[4];   /* nr of sectors in partition   */
} dos_partition_t;

/* Convert char[4] in little endian format to the host format integer
 */
static inline void int_to_le32(unsigned char *le32, unsigned int blocks)
{
  le32[3] = (blocks >> 24) & 0xff;
  le32[2] = (blocks >> 16) & 0xff;
  le32[1] = (blocks >>  8) & 0xff;
  le32[0] = (blocks >>  0) & 0xff;
}

static inline int le32_to_int(unsigned char *le32)
{
    return ((le32[3] << 24) +
      (le32[2] << 16) +
      (le32[1] << 8) +
       le32[0]
     );
}

static inline int is_extended(int part_type)
{
    return (part_type == 0x5 ||
        part_type == 0xf ||
        part_type == 0x85);
}

static inline void part_mmc_chs(dos_partition_t *pt, lbaint_t lba_start, lbaint_t lba_size)
{
  int head = 1, sector = 1, cys = 0;
  int end_head = 254, end_sector = 63, end_cys = 1023;

  /* default CHS */
  pt->head = (unsigned char)head;
  pt->sector = (unsigned char)(sector + ((cys & 0x00000300) >> 2) );
  pt->cyl = (unsigned char)(cys & 0x000000FF);
  pt->end_head = (unsigned char)end_head;
  pt->end_sector = (unsigned char)(end_sector + ((end_cys & 0x00000300) >> 2) );
  pt->end_cyl = (unsigned char)(end_cys & 0x000000FF);

  int_to_le32(pt->start4, lba_start);
  int_to_le32(pt->size4, lba_size);
}

static int mmc_get_part_table_extended(struct blk_desc *desc,
        lbaint_t lba_start, lbaint_t relative, uint64_t (*parts)[2], int *part_num)
{
  unsigned char buffer[512] = { 0, };
  dos_partition_t *pt;
  lbaint_t lba_s, lba_l;
  int i = 0;

  debug("--- LBA S= 0x%llx : 0x%llx ---\n", (uint64_t)lba_start*desc->blksz, (uint64_t)relative*desc->blksz);

  if (0 > blk_dread(desc, lba_start, 1, (void *)buffer)) {
    printf ("** Error read mmc.%d partition info **\n", desc->devnum);
    return -1;
  }

  if (buffer[DOS_PART_MAGIC_OFFSET] != 0x55 ||
    buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xAA) {
    printf ("** Not find partition magic number **\n");
    return 0;
  }

  /* Print all primary/logical partitions */
  pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
  for (i = 0; 4 > i; i++, pt++) {

    lba_s = le32_to_int(pt->start4);
    lba_l = le32_to_int(pt->size4);

    if (is_extended(pt->sys_ind)) {
      debug("Extd p=\t0x%llx \t~ \t0x%llx (%ld:%ld) <%d>\n",
        (uint64_t)lba_s*desc->blksz, (uint64_t)lba_l*desc->blksz, lba_s, lba_l, i);
      continue;
    }

    if (lba_s && lba_l) {
      parts[*part_num][0] = (uint64_t)(lba_start + lba_s)*desc->blksz;
      parts[*part_num][1] = (uint64_t)lba_l*desc->blksz;
      debug("part.%d=\t0x%llx \t~ \t0x%llx (%ld:%ld) <%d>\n",
        *part_num, parts[*part_num][0], parts[*part_num][1], lba_s, lba_l, i);
      *part_num += 1;
    }
  }

  /* Follows the extended partitions */
  pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
  for (i = 0; 4 > i; i++, pt++) {
    if (is_extended(pt->sys_ind)) {
      lba_s = le32_to_int(pt->start4) + relative;
      mmc_get_part_table_extended(desc,
        lba_s,                /* next read block */
        lba_start == 0 ? lba_s : relative,  /* base offset, first offst */
        parts, part_num);
    }
  }
  return 0;
}

int mmc_get_part_table(struct blk_desc *desc, uint64_t (*parts)[2], int *part_num)
{
  int partcnt = 0;
  int ret = mmc_get_part_table_extended(desc, 0, 0, parts, &partcnt);
  if (0 > ret)
    return ret;

  *part_num = partcnt;
  return 0;
}

int mmc_make_part_table_extended(struct blk_desc *desc, lbaint_t lba_start,
          lbaint_t relative, uint64_t (*parts)[2], int part_num)
{
  unsigned char buffer[512] = { 0, };
  dos_partition_t *pt;
  lbaint_t lba_s = 0, lba_l = 0, last_lba = lba_start;
  int i = 0, ret = 0;

  debug("--- LBA S= 0x%llx : 0x%llx ---\n",
    (uint64_t)lba_start*desc->blksz, (uint64_t)relative*desc->blksz);

  memset(buffer, 0x0, sizeof(buffer));
  buffer[DOS_PART_MAGIC_OFFSET] = 0x55;
  buffer[DOS_PART_MAGIC_OFFSET + 1] = 0xAA;

  pt = (dos_partition_t *)(buffer + DOS_PART_TBL_OFFSET);
  for (i = 0; 2 > i && part_num > i; i++, pt++) {
    lba_s = (lbaint_t)(parts[i][0] / desc->blksz);
    lba_l = (lbaint_t)(parts[i][1] / desc->blksz);

    if (0 == lba_s) {
      printf("-- Fail: invalid part.%d start 0x%llx --\n", i, parts[i][0]);
      return -1;
    }

    if (last_lba > lba_s) {
      printf ("** Overlapped primary part table 0x%llx previous 0x%llx (EBR) **\n",
        (uint64_t)(lba_s)*desc->blksz, (uint64_t)(last_lba)*desc->blksz);
      return -1;
    }

    if (0 == lba_l)
      lba_l = desc->lba - last_lba - DOS_EBR_BLOCK;

    if (lba_l > (desc->lba - last_lba)) {
      printf("-- Fail: part %d invalid length 0x%llx, avaliable (0x%llx)(EBR) --\n",
        i, (uint64_t)(lba_l)*desc->blksz, (uint64_t)(desc->lba-last_lba)*desc->blksz);
      return -1;
    }

    pt->boot_ind = 0x00;
    pt->sys_ind  = 0x83;
    if (i == 1) {
      pt->sys_ind = 0x05;
      lba_s -= relative + DOS_EBR_BLOCK;
      lba_l += DOS_EBR_BLOCK;
    } else {
      lba_s -= lba_start;
    }

    debug("%s=\t0x%llx \t~ \t0x%llx \n",
      i == 0 ? "Prim p" : "Extd p",
      i == 0 ? (uint64_t)(lba_s + lba_start)*desc->blksz : (uint64_t)(lba_s)*desc->blksz,
      (uint64_t)(lba_l)*desc->blksz);

    part_mmc_chs(pt, lba_s, lba_l);
    last_lba = lba_s + lba_l;
  }

  ret = blk_dwrite(desc, lba_start, 1, (const void *)buffer);
  if (0 > ret)
    return ret;

  if (part_num)
    return mmc_make_part_table_extended(desc,
        lba_s + relative, relative, &parts[1], part_num-1);

  return ret;
}

int mmc_make_part_table(struct blk_desc *desc,
        uint64_t (*parts)[2], int part_num, unsigned int part_type)
{
  unsigned char buffer[MMC_BLOCK_SIZE];
  dos_partition_t *pt;
  uint64_t lba_s, lba_l, last_lba = 0;
  uint64_t avalible = desc->lba;
  int part_tables = part_num, part_EBR = 0;
  int i = 0, ret = 0;

    printf("=-->mmc_make_part_table\r\n");

  debug("--- Create mmc.%d partitions %d ---\n", desc->devnum, part_num);
  if (part_type != PART_TYPE_DOS) {
    printf ("** Support only DOS PARTITION **\n");
    return -1;
  }

  if (1 > part_num || part_num > MAX_PART_TABLE) {
    printf ("** Can't make partition tables %d (1 ~ %d) **\n", part_num, MAX_PART_TABLE);
    return -1;
  }

  debug("Total = %lld * %d :0x%llx (%d.%d G) \n", avalible,
    (int)desc->blksz, (uint64_t)(avalible)*desc->blksz,
    (int)((avalible*desc->blksz)/(1024*1024*1024)),
    (int)((avalible*desc->blksz)%(1024*1024*1024)));

  memset(buffer, 0x0, sizeof(buffer));
  buffer[DOS_PART_MAGIC_OFFSET] = 0x55;
  buffer[DOS_PART_MAGIC_OFFSET + 1] = 0xAA;

  if (part_num > 4) {
    part_tables = 3;
    part_EBR = 1;
  }

  pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
  last_lba = (int)(parts[0][0] / (uint64_t)desc->blksz);

  for (i = 0; part_tables > i; i++, pt++) {
    lba_s = (lbaint_t)(parts[i][0] / desc->blksz);
    lba_l = parts[i][1] ? (lbaint_t)(parts[i][1] / desc->blksz) : (desc->lba - last_lba);

    if (0 == lba_s) {
      printf("-- Fail: invalid part.%d start 0x%llx --\n", i, parts[i][0]);
      return -1;
    }

    if (last_lba > lba_s) {
      printf ("** Overlapped primary part table 0x%llx previous 0x%llx **\n",
        (uint64_t)(lba_s)*desc->blksz, (uint64_t)(last_lba)*desc->blksz);
      return -1;
    }

    if (lba_l > (desc->lba - last_lba)) {
      printf("-- Fail: part %d invalid length 0x%llx, avaliable (0x%llx) --\n",
        i, (uint64_t)(lba_l)*desc->blksz, (uint64_t)(desc->lba-last_lba)*desc->blksz);
      return -1;
    }

    /* Linux partition */
    pt->boot_ind = 0x00;
    pt->sys_ind  = 0x83;
    part_mmc_chs(pt, lba_s, lba_l);
    last_lba = lba_s + lba_l;

    debug("part.%d=\t0x%llx \t~ \t0x%llx \n",
      i, (uint64_t)(lba_s)*desc->blksz, (uint64_t)(lba_l)*desc->blksz);
  }

  if (part_EBR) {
    lba_s = (lbaint_t)(parts[3][0] / desc->blksz) - DOS_EBR_BLOCK;
    lba_l = (lbaint_t)(desc->lba - last_lba);

    if (last_lba > lba_s) {
      printf ("** Overlapped extended part table 0x%llx (with offset -0x%llx) previous 0x%llx **\n",
        (uint64_t)(lba_s)*desc->blksz, (uint64_t)DOS_EBR_BLOCK*desc->blksz,
        (uint64_t)(last_lba)*desc->blksz);
      return -1;
    }

    /* Extended partition */
    pt->boot_ind = 0x00;
    pt->sys_ind  = 0x05;
    part_mmc_chs(pt, lba_s, lba_l);

  }

  ret = blk_dwrite(desc, 0, 1, (const void *)buffer);
  if (0 > ret)
    return ret;

  if (part_EBR)
    return mmc_make_part_table_extended(desc, lba_s, lba_s, &parts[part_tables], part_num - part_tables);

  return ret;

}

/*
 * cmd : fdisk 0 n, s1:size, s2:size, s3:size, s4:size
 */
static int do_fdisk(cmd_tbl_t *cmdtp, int flag, int argc, char* const argv[])
{
  struct blk_desc *desc;
  int i = 0, ret;

  if (2 > argc)
    return CMD_RET_USAGE;

  ret = blk_get_device_by_str("mmc", argv[1], &desc);
  if (ret < 0) {
    printf ("** Not find device mmc.%s **\n", argv[1]);
    return 1;
  }

  if (2 == argc) {
    part_print(desc);
    dev_print(desc);
  } else {
    uint64_t parts[MAX_PART_TABLE][2];
    int count = 1;

    count = simple_strtoul(argv[2], NULL, 10);
    if (1 > count || count > MAX_PART_TABLE) {
      printf ("** Invalid partition table count %d (1 ~ %d) **\n", count, MAX_PART_TABLE);
      return -1;
    }

    for (i = 0; count > i; i++) {
      const char *p = argv[i+3];
      parts[i][0] = simple_strtoull(p, NULL, 16); /* start */
      if (!(p = strchr(p, ':'))) {
        printf("no <0x%llx:length> identifier\n", parts[i][0]);
        return -1;
      }
      p++;
      parts[i][1] = simple_strtoull(p, NULL, 16);   /* end */
      debug("part[%d] 0x%llx:0x%llx\n", i, parts[i][0], parts[i][1]);
    }

    mmc_make_part_table(desc, parts, count, PART_TYPE_DOS);
  }
  return 0;
}

U_BOOT_CMD(
  fdisk, 16, 1, do_fdisk,
  "mmc list or create ms-dos partition tables (MAX TABLE 7)",
  "<dev no>\n"
  " - list partition table info\n"
  "fdisk <dev no> [part table counts] <start:length> <start:length> ...\n"
  " - Note. each arguments seperated with space"
  " - Create partition table info\n"
  " - All numeric parameters are assumed to be hex.\n"
  " - start and length is offset.\n"
  " - If the length is zero, uses the remaining.\n"
);
```

###4.3  decompress\_ext4.c
```
/* copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <common.h>
#include <asm/io.h>
#include <asm/types.h>
#include <environment.h>
#include <command.h>
#include <fastboot.h>

#include "decompress_ext4.h"
#include <mmc.h>

#define SECTOR_BITS   9 /* 512B */

//#define ext4_printf(args, ...) printf(args, ##__VA_ARGS__)
#define ext4_printf(args, ...) 

static int write_raw_chunk(char* data, unsigned int sector, unsigned int sector_size);
static WRITE_RAW_CHUNK_CB write_raw_chunk_cb = write_raw_chunk;

int set_write_raw_chunk_cb(WRITE_RAW_CHUNK_CB cb) {
  write_raw_chunk_cb = cb;

  return 0;
}

int check_compress_ext4(char *img_base, unsigned long long parti_size) {
  ext4_file_header *file_header;

  file_header = (ext4_file_header*)img_base;

  if (file_header->magic != EXT4_FILE_HEADER_MAGIC) {
    return -1;
  }

  if (file_header->major != EXT4_FILE_HEADER_MAJOR) {
    printf("Invalid Version Info! 0x%2x\n", file_header->major);
    return -1;
  }

  if (file_header->file_header_size != EXT4_FILE_HEADER_SIZE) {
    printf("Invalid File Header Size! 0x%8x\n",
                file_header->file_header_size);
    return -1;
  }

  if (file_header->chunk_header_size != EXT4_CHUNK_HEADER_SIZE) {
    printf("Invalid Chunk Header Size! 0x%8x\n",
                file_header->chunk_header_size);
    return -1;
  }

  if (file_header->block_size != EXT4_FILE_BLOCK_SIZE) {
    printf("Invalid Block Size! 0x%8x\n", file_header->block_size);
    return -1;
  }

  if ((parti_size/file_header->block_size)  < file_header->total_blocks) {
    printf("Invalid Volume Size! Image is bigger than partition size!\n");
    printf("partion size %lld , image size %lld \n",
      parti_size, (uint64_t)file_header->total_blocks * file_header->block_size);
    printf("Hang...\n");
    while(1);
  }

  /* image is compressed ext4 */
  return 0;
}

int write_raw_chunk(char* data, unsigned int sector, unsigned int sector_size) {
  char run_cmd[64];

  ext4_printf("write raw data in %d size %d \n", sector, sector_size);
  sprintf(run_cmd,"mmc write 0x%x 0x%x 0x%x", (int)((ulong)data), sector, sector_size);
  run_command(run_cmd, 0);

  return 0;
}

int write_compressed_ext4(char* img_base, unsigned int sector_base) {
  unsigned int sector_size;
  int total_chunks;
  ext4_chunk_header *chunk_header;
  ext4_file_header *file_header;

#if 1
  unsigned char num = 0;
#endif

  file_header = (ext4_file_header*)img_base;
  total_chunks = file_header->total_chunks;

  ext4_printf("total chunk = %d \n", total_chunks);

  img_base += EXT4_FILE_HEADER_SIZE;

  while(total_chunks) {
    chunk_header = (ext4_chunk_header*)img_base;
    sector_size = (chunk_header->chunk_size * file_header->block_size) >> SECTOR_BITS;

    switch(chunk_header->type)
    {
    case EXT4_CHUNK_TYPE_RAW:
      ext4_printf("raw_chunk \n");
      write_raw_chunk_cb(img_base + EXT4_CHUNK_HEADER_SIZE,
              sector_base, sector_size);
      sector_base += sector_size;
      break;

    case EXT4_CHUNK_TYPE_FILL:
      printf("*** fill_chunk ***\n");
      sector_base += sector_size;
      break;

    case EXT4_CHUNK_TYPE_NONE:
      ext4_printf("none chunk \n");
      sector_base += sector_size;
      break;

    default:
      printf("*** unknown chunk type ***\n");
      sector_base += sector_size;
      break;
    }
    total_chunks--;
    ext4_printf("remain chunks = %d \n", total_chunks);
#if 1
    if(70 == num)
    {
      num = 0;
      printf("\n");
    }
    printf(".");

    num++;
#endif

    img_base += chunk_header->total_size;
  };

#if 1
  printf("\n");
#endif

  ext4_printf("write done \n");
  return 0;
}
```

###4.4  decompress\_ext4.h
```
/* copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

typedef struct _ext4_file_header {
  unsigned int magic;
  unsigned short major;
  unsigned short minor;
  unsigned short file_header_size;
  unsigned short chunk_header_size;
  unsigned int block_size;
  unsigned int total_blocks;
  unsigned int total_chunks;
  unsigned int crc32;
}ext4_file_header;


typedef struct _ext4_chunk_header {
  unsigned short type;
  unsigned short reserved;
  unsigned int chunk_size;
  unsigned int total_size;
}ext4_chunk_header;

#define EXT4_FILE_HEADER_MAGIC  0xED26FF3A
#define EXT4_FILE_HEADER_MAJOR  0x0001
#define EXT4_FILE_HEADER_MINOR  0x0000
#define EXT4_FILE_BLOCK_SIZE  0x1000

#define EXT4_FILE_HEADER_SIZE (sizeof(struct _ext4_file_header))
#define EXT4_CHUNK_HEADER_SIZE  (sizeof(struct _ext4_chunk_header))


#define EXT4_CHUNK_TYPE_RAW     0xCAC1
#define EXT4_CHUNK_TYPE_FILL    0xCAC2
#define EXT4_CHUNK_TYPE_NONE    0xCAC3

int write_compressed_ext4(char* img_base, unsigned int sector_base);
int check_compress_ext4(char *img_base, unsigned long long parti_size);

typedef int (*WRITE_RAW_CHUNK_CB)(char* data, unsigned int sector, unsigned int sector_size);
int set_write_raw_chunk_cb(WRITE_RAW_CHUNK_CB cb);
```

##5.修改cmd/Makefile
在cmd/Makefile内添加

```
obj-$(CONFIG_CMD_MMC) += cmd_mmcparts.o
obj-$(CONFIG_CMD_MMC) += cmd_mmc_update.o
obj-y += decompress_ext4.o

```

##6.修改cmd/fastboot.c文件

在#include <usb.h>下新增以下内容

```
#include <blk.h>

#include "decompress_ext4.h"

#define FASTBOOT_PARTS_DEFAULT      \
    "flash=mmc,0:miniloader:2nd:0,0x400000;" \
    "flash=mmc,0:parmter:2nd:0x400000,0x400000;" \
    "flash=mmc,0:uboot:2nd:0x800000,0x800000;" \
    "flash=mmc,0:boot:ext4:0x1000000,0x4000000;" \
    "flash=mmc,0:system:ext4:0x5000000,0x4000000;"

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

```

```
在函数static int do_fastboot(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
内添加内容
  int controller_index;
  char *usb_controller;
  int ret;

//添加内容开始
    static int init_parts = 0;
    int err;
    const char *p;
    int i;
    struct fastboot_device *fd = f_devices;
    struct fastboot_part *fp;
    struct list_head *entry, *n;
    //char resp[RESP_SIZE] = "OKAY";
    const char *cmd = "boot";
//添加内容结束


  if (!g_dnl_board_usb_cable_connected()) {
    puts("\rUSB cable not detected.\n" \
         "Command exit.\n");
    ret = CMD_RET_FAILURE;
    goto exit;
  }
//添加内容开始
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
    }
done_flash:

    printf("goto while\r\n");
//添加内容结束
  while (1) {

```

##7  编译uboot并打包成uboot.img
```
使用make 进行编译
使用m3提供的安卓工程中u-boot/tools/loaderimage工具进行打包
loaderimage --pack u-boot.bin uboot.img
```

##8  uboot烧写
```
uboot烧写方式分为两种
第一种是将uboot生成的uboot.img复制到m3工程中，使用m3工程生成整体的大包然后通过androidtool进行烧写。
第二种实现方式需要首先修改uboot的参数bootdelay为大于0，然后在uboot下输入mmc erase 0x2000 0x4000,然后再输入reset。这是进入到虚拟机，连接usb到虚拟机，这是在androidtool上能看到"发现一个LOADER设备",点击下载镜像，选中第2项“Parameter”和第三项"Uboot"，点击"Uboot"栏右侧方块，重新选择uboot.img，点击执行，如果成功烧写会显示“下载完成”

```

##9  修改uboot使其自动启动内核
```
若想要uboot能够自动启动内核，只需要设置bootcmd命令即可。
在2018-05版本的uboot中通过menuconfig对bootcmd进行设置。
在menuconfig的主页面能够看到
[]Enable a default value for bootcmd 
选中后出现
() bootcmd value 
只需要将参数写入到括号内就能够自动执行。

修改cmd/ext4.c内代码
在文件末尾添加
U_BOOT_CMD(autoload, 1, 1, do_autoload,
        "for m3 boot.img load",
        "autoload");
        
实现do_autoload函数
int do_autoload(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    char *cmd_uImage = "ext4load mmc 0:1 0x60800800 uImage";
    char *cmd_dtb = "ext4load mmc 0:1 0x80800000 m3.dtb";
    char *cmd_bootm = "bootm 0x60800800 - 0x80800000";
    run_command(cmd_uImage, 0); 
    run_command(cmd_dtb, 0); 
    run_command(cmd_bootm, 0); 
}

这样只需要在uboot下输入autoload就可以执行加载内核加载dtb执行内核的一系列操作了。

在看上面设置bootcmd处，在括号内只需要填入autoload，就能够在uboot启动后自动启动内核了。

```
