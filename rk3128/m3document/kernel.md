#内核修改
******
##内核版本
* 使用了m3的内核，对应版本为3.10.49

*****
##内核控制台串口配置

修改arch/arm/boot/dts/rk312x-sdk-m3.dtsi

```
chosen {
	bootargs = "noitinrd console=ttyS2,115200 root=/dev/mmcblk0p2 rw
	rootfs=ext4fs init=/linuxrc";
    };
```
******

###make menuconfig

```
 Device Drivers->Character devices->Serial drivers->[*]   Serial console support

```
******
##uboot启动kernel

```
带有设备书的内核启动需要实现加载内核uImage和*.dtb到内存中

ext4load mmc 0:1 0x60800800 uImage
ext4load mmc 0:1 0x80800000 m3.dtb
bootm 0x60800800 - 0x80800000


bootm [kernel addr] [ramdisk addr] [dtb addr]
我们不适用ramdisk所以第二项用"-"表示没有ramdisk

```

******
