#M3上运行Ubuntu方法
1、使用Android开发工具烧写FirePrime_Ubuntu15.04(Server)_201706271721.img到M3

2、解压FirePrime_Ubuntu15.04(Server)_201706271721.img得到parameter

3、M3启动后在uboot下执行rockusb，M3进入loader状态

4、修改M3的uboot代码中的rk30plat.h，设置CONFIG_BOOTDELAY=3

5、修改M3的kernel代码中的m3.dts，注释掉zigbee_h

6、修改M3的kernel代码中的rk312x-sdk-m3.dtsi，设置fiq-debugger的status=okay，设置uart2的status=disabled

7、修改M3的kernel代码中的m3_defconfig，设置CONFIG_FHANDLE=y

8、正常编译uboot && kernel

9、在kernel目录下，执行git clone -b fireprime https://github.com/TeeFirefly/initrd.git && make -C initrd && truncate -s "%4" initrd.img && mkbootimg --kernel arch/arm/boot/zImage --ramdisk initrd.img -o linux-boot.img

10、取uboot目录下RK3128MiniLoaderAll_V2.25.bin、uboot.img；kernel目录下linux-boot.img、resource.img；2步骤中的parameter烧写到M3上，地址参照parameter
