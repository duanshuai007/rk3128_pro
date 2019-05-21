##编译busybox镜像时使用命令：

```
make_ext4fs -s -l 50331648 -L linux ./system_m3.img ./rootfs
                  (内存大小) 			 (生成的镜像)  (目标文件夹)

50331648对应0x3000000，因为system分区在uboot被通过宏设置为0x4000000
所以必须要小于0x4000000。    
```
	
##解决can't open /dev/tty4: No such file or directory问题

出现这个的根本原因是busybox的源码，在init/init.c中

```
static void parse_inittab(void)
{
	...
	new_init_action(ASKFIRST, bb_default_login_shell, VC_2);
	new_init_action(ASKFIRST, bb_default_login_shell, VC_3);
	new_init_action(ASKFIRST, bb_default_login_shell, VC_4);
	...
}
```
在include/libbb.h中能够看到`# define VC_2 "/dev/tty2"`,tty1,tty2,tty3...ttyn都属于linux虚拟出来的设备，需要在内核中开启对应的支持才能过自动生成，

```
打开内核menuconfig->device drivers->Character devices->
  │ │        [*]   Virtual terminal                                                        │ │  
  │ │        [*]     Enable character translations in console                              │ │  
  │ │        [*]     Support for console on virtual terminal                               │ │  
  │ │        [ ]     Support for binding and unbinding console drivers  
```
重新编译内核，烧写，就能在/dev/目录下发现一连串的tty设备了。

####或者如果不想更改内核配置，也可以按照以下方式排除问题

在根文件系统etc目录下添加文件inittab
在inittab中添加以下内容：

```
::sysinit:/etc/init.d/rcS     
console::askfirst:-/bin/sh
::ctrlaltdel:-/sbin/reboot
::shutdown:/bin/umount -a -r
::restart:/sbin/init
```						

##busybox配置选项

```
Shells  ---> [*]   Expand prompt string 

```
选中该选项使命令id -un 能够返回root，使shell界面输出[root@RK3128],不选中则会显示[#USER@#HOSTNAME]
