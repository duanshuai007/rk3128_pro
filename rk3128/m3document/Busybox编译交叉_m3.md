#Busybox编译交叉编译文档总结
##一、Busybox源码获取
1、下载busybox-1.28.3源码: [https://busybox.net/downloads/busybox-1.28.3.tar.bz2](https://busybox.net/downloads/busybox-1.28.3.tar.bz2)

2、将busybox-1.28.3.tar.bz2拷贝到我们的服务器Ubuntu系统上

3、解压busybox-1.28.3源码（此说明中将代码解压到个人用户的根目录下）:     
`tar -vxf busybox-1.28.3.tar.bz2 ~/`

##二、交叉编译工具
交叉编译工具使用的是`arm-none-linux-gnueabi`，针对`busybox-1.25.＊`版本之后需要使用最新版本的交叉编译工具，本文中采用的是`arm-2014.05-29-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2`

1、交叉编译工具地址: [https://blog.csdn.net/alan00000/article/details/51724252](https://blog.csdn.net/alan00000/article/details/51724252)   
本文给出的是一个博客地址，可以通过该博客下载最新的交叉编译工具，也可以到官网上下载。

**注，必须使用最新版本的交叉编译工具，否则编译会出现error!!**

2、`解压arm-2014.05-29-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2`
`tar -vxf arm-2014.05-29-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2`

3、添加环境变量   
在.bashrc文件中添加：
`export PATH=$PATH:/arm-2014.05/bin/`

4、执行source .bashrc  
`source .bashrc`

5、检查编译工具  
在命令行中敲入which arm-none-linux-gnueabi-gcc，输出的内容是gcc的位置，对比是否与arm-2014.05/bin/相同.

##三、Busybox配置
Busybox的配置和Linux内核编译配置方法相同，下面我们来开始配置Busybox：
   
1、进入busybox-1.28.3目录，执行make defconfig，使Busybox使用缺省的配置

**busybox提供了几种配置：defconfig (缺省配置)、allyesconfig（最大配置）、 allnoconfig（最小配置），一般选择缺省配置即可。**

**这一步结束后，将生成.config**

2、执行make menuconfig，打开Busybox配置界面
`make menuconfig`

* 修改配置项CONFIG\_CROSS\_COMPILER\_PREFIX（配置使用的交叉编译工具） 
   
   选中Settings－－》Cross compiler prefix，按下回车，进入交叉编译工具输入界面，
然后输入`arm-none-linux-gnueabi-`，改完之后如下：

```
 [ ] Build shared libbusybox                      
 (arm-none-linux-gnueabi-) Cross compiler prefix  
 ()  Path to sysroot                              
```
* 修改menuconfig，Settings－－》[*] Build static binary (no shared libs) 

* 修改CONFIG_PREFIX(make install后文件的存放路径)
	
	选中Settings－－》Destination path for 'make install'，按下回车，进入数据界面，将`./_install`改为`./system`,改完之后显示如下：

```
 --- Installation Options ("make install" behavior)            
     What kind of applet links to install (as soft-links)  --->
 (./system) Destination path for 'make install'                
 --- Debugging Options                                         
```

退出menuconfig,选择“YES”进行保存。

##四、编译
1、进入busybox-1.28.3目录,执行make   

```
编译log：
......
  CC      util-linux/volume_id/udf.o
  CC      util-linux/volume_id/util.o
  CC      util-linux/volume_id/volume_id.o
  CC      util-linux/volume_id/xfs.o
  AR      util-linux/volume_id/lib.a
  LINK    busybox_unstripped
Trying libraries: crypt m
 Library crypt is not needed, excluding it
 Library m is needed, can't exclude it (yet
Final link with: m
  DOC     busybox.pod
  DOC     BusyBox.txt
  DOC     busybox.1
  DOC     BusyBox.html
user@ubuntu:~/busybox-1.28.3$ 

```

3、执行安装   
`user@ubuntu:~/busybox-1.28.3$ make install`
执行该操作会在3-2步骤中指定的./system文件夹能输出bin,sbin,usr,linuxrc等文件。

```
log:
....
  ./system//usr/sbin/ubirmvol -> ../../bin/busybox
  ./system//usr/sbin/ubirsvol -> ../../bin/busybox
  ./system//usr/sbin/ubiupdatevol -> ../../bin/busybox
  ./system//usr/sbin/udhcpd -> ../../bin/busybox


--------------------------------------------------
You will probably need to make your busybox binary
setuid root to ensure all configured applets will
work properly.
--------------------------------------------------

user@ubuntu:~/busybox-1.28.3$ 
```
目前busybox已经完成编译，system下的文件如下：

```
user@ubuntu:~/busybox-1.28.3$ ls system/ -al
total 20
drwxrwxr-x  5 user user 4096 Apr 13 16:07 .
drwxr-xr-x 37 user user 4096 Apr 13 16:07 ..
drwxrwxr-x  2 user user 4096 Apr 13 16:07 bin
lrwxrwxrwx  1 user user   11 Apr 13 16:07 linuxrc -> bin/busybox
drwxrwxr-x  2 user user 4096 Apr 13 16:07 sbin
drwxrwxr-x  4 user user 4096 Apr 13 16:07 usr
user@ubuntu:~/busybox-1.28.3$ 

```
##五、制作文件系统
1、进入system目录,创建文件目录（可以把如下的命令当成脚本运行）

```
mkdir dev etc lib mnt proc sys tmp var
mkdir etc/init.d
mkdir etc/rc.d
mkdir etc/rc.d/init.d
mkdir var/lib
mkdir var/lock
mkdir var/log
mkdir var/run
mkdir var/tmp

将交叉编译工具中的库文件复制到根文件系统根目录的lib目录下
cp ~/arm-2014.05/arm-none-linux-gnueabi/libc/lib/* lib/
```
2、创建根文件系统所需要的文件，当前工作目录为./system

* etc/eth0-setting 
  
使用vi命令创建文件：`vi etc/eth0-setting`
	
添加以下内容：
	
```
IP=192.168.200.158 
Mask=255.255.255.0 
Gateway=192.168.200.1 
DNS=192.168.200.1 
MAC=08:90:90:90:90:90
```

* etc/init.d/ifconfig-eth0

使用vi创建文件：`vi etc/init.d/ifconfig-eth0`
	
添加以下内容：
	
```
#!/bin/sh

echo -n Try to bring eth0 interface up......>/dev/ttyS2
	
if [ -f /etc/eth0-setting ] ; then
    source /etc/eth0-setting
	
    if grep -q "/dev/root / nfs " /etc/mtab ; then
        echo -n NFS root ... > /dev/ttyS2
    else
        ifconfig eth0 down
        ifconfig eth0 hw ether $MAC
        ifconfig eth0 $IP netmask $Mask up
        route add default gw $Gateway
    fi
	
    echo nameserver $DNS > /etc/resolv.conf
else
    if grep -q "^/dev/root / nfs " /etc/mtab ; then
        echo -n NFS root ... > /dev/ttyS2
    else
        /sbin/ifconfig eth0 192.168.253.12 netmask 255.255.255.0 up
    fi
fi
	
    echo Done > /dev/ttyS2
	
```
	
* etc/init.d/rcS

使用vi创建文件：`vi etc/init.d/rcS`
	
添加以下内容：
	
```
#! /bin/sh
PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin:
runlevel=S
prevlevel=N
umask 022
export PATH runlevel prevlevel
	
#
# Trap CTRL-C &c only in this shell so we can interrupt subprocesses.
#
	
trap ":" INT QUIT TSTP
	
/bin/hostname RK3128

mount -a

echo /sbin/mdev > /proc/sys/kernel/hostplug

mdev -s	
#解决read-noly filesystem的问题
mount -o remount rw /

```

* etc/passwd

使用vi创建文件：`vi etc/passwd`
	
添加以下内容：
	
```
root::0:0:root:/:/bin/sh
bin:*:1:1:bin:/bin:
daemon:*:2:2:daemon:/sbin:
nobody:*:99:99:Nobody:/:
```
* etc/profile

使用vi创建文件：`vi etc/profile`
	
添加以下内容：
	
```
# Ash profile
# vim: syntax=sh
	
# No core files by default
ulimit -S -c 0 > /dev/null 2>&1
	
USER="`id -un`"
LOGNAME=$USER
PS1='[$USER@$HOSTNAME]# '
PATH=$PATH
HOSTNAME=`/bin/hostname`
	
export USER LOGNAME PS1 PATH
```

* etc/rc.d/init.d/netd

使用vi创建文件：`vi etc/rc.d/init.d/netd`
	
添加以下内容：
	
```
#!/bin/sh
	
base=inetd

# See how we were called.
case "$1" in
    start)
        /usr/sbin/$base
        ;;
    stop)
        pid=`/bin/pidof $base`
        if [ -n "$pid" ]; then
            kill -9 $pid
        fi
        ;;
esac
exit 0

```

* etc/fstab

使用vi创建文件：`vi etc/fstab`

添加以下内容：

```
proc    /proc   proc    defaults    0   0
tmpfs   /tmp    tmpfs   defaults    0   0
sysfs   /sys    sysfs   defaults    0   0
tmpfs   /dev    tmpfs   defaults    0   0
```

* etc/inittab

使用vi创建文件:`vi etc/inittab`

添加以下内容：

```
::sysinit:/etc/init.d/rcS     
console::askfirst:-/bin/sh
::ctrlaltdel:-/sbin/reboot
::shutdown:/bin/umount -a -r
::restart:/sbin/init
```

* 修改文件权限

```
chmod 755 etc/eth0-setting
chmod 755 etc/init.d/ifconfig-eth0
chmod 755 etc/init.d/rcS
chmod 755 etc/passwd
chmod 755 etc/profile
chmod 755 etc/rc.d/init.d/netd
chmod 755 etc/fstab
chmod 755 etc/inittab
	
```

##六、打包

使用make_ext4fs将busybox打包成system.img:

`make_ext4fs -s -l 50331648 -L linux ./system.img ./system`

```
50331648对应文件系统所使用的空间大小
./system.img是生成的目标文件
./system是根文件系统的根目录
```

其中make_ext4fs为4418 android/qt_system/linux-x86/bin源码，也可以自行下载make_ext4fs。