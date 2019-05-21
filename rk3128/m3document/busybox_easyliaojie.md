#根文件系统简单了解

##ramfs
	
ramfs(内存文件系统)是Linux下一种基于RAM做存储的文件系统,但是ramfs有一个很大的缺陷就是它会吃光系统所有的内存，即使你mount的时候指定了大小，同时它也只能被root用户访问。umount后数据会丢失。

* df无法显示ramfs信息的原因（无-a选项）

根据superuser.com上的问答《Have I successfully created an ramfs drive?》，Sachin Divekar给出了一段资料引用：
For a ramfs filesystem, the newer kernels report nothing back using "df". There is meant to be a patch for this (to allow for accounting in a ramfs). Philosophically, ramfs is mean to be as simple as possible, apparently, hence the lack of accounting. So data can be stored and used on the ramfs disk, but no accounting of it is possible, other than a loss of memory shown with "free". For this reason the tmpfs is better, since it does keep accounting and "df" shows what's going on.
	
即，tmpfs会对内存进行accounting（统计内存的使用情况），而ramfs被设计为尽可能的简单，所以不会进行accounting。因此，针对ramfs，在较新的内核中，使用df不会返回ramfs的信息。
	
######mount -t ramfs -o size=1024m ramfs /tmp
#####共同点tmpfs或ramfs可以通过挂载实现对内存的访问使用，提高效率。
	
| 特性 | tmpfs | ramfs |
|:---- |:---------:|:-------:|
| 达到空间上限时继续写入 | 提示错误信息并终止 | 可以继续写尚未分配的空间 |
| 是否固定大小 | 是 | 否 |
| 是否使用swap | 是 | 否 |
| 具有易失性 | 是 |   是 |
| 性能 | 更好 | 稍逊 |
	
测试方法：
	
```
mount -t ramfs -o size=10M ramfs ./ramfs/
dd if=/dev/zero of=./ramfs/test.file bs=1M count=20
```

##tmpfs
	
tmpfs 是 Linux/Unix 系统上的一种基于内存的文件系统，即 tmpfs 使用内存或 swap 分区来存储文件。

tmpfs(虚拟内存文件系统)，不同于传统的用块设备形式来实现的ramdisk，也不同于针对物理内存的ramfs。tmpfs可以使用物理内存也可以使用交换分区。也是Linux下的一个文件系统，它将所有的文件都保存在虚拟内存中，umount tmpfs后所有的数据也会丢失，tmpfs就是ramfs的衍生品。tmpfs使用了虚拟内存的机制，它会进行swap，但是它有一个相比ramfs的好处：**mount时指定的size参数是起作用的，这样就能保证系统的安全，而不是像ramfs那样，一不留心因为写入数据太大吃光系统所有内存导致系统被hang住**。
	
**[*] glibc 2.2以上的版本，必须有一个tmpfs被mount在/dev/shm用做POSIX shared memory**

tmpfs主要存储暂存的文件。它有如下2个优势 : 

1. 动态文件系统的大小。
2. tmpfs 使用VM建的文件系统，速度当然快。
3. 重启后数据丢失。
	
当删除tmpfs中的文件时,tmpfs会动态减少文件系统并释放VM资源,LINUX中可以把一些程序的临时文件放置在tmpfs中，利用tmpfs比硬盘速度快的特点提升系统性能。实际应用中，为应用的特定需求设定此文件系统，可以提升应用读写性能，如将squid 缓存目录放在/tmp, php session 文件放在/tmp, socket文件放在/tmp, 或者使用/tmp作为其它应用的缓存设备.

Linux 内核中的 VM 子系统负责在后台管理虚拟内存资源 Virtual Memory，即 RAM 和 swap 资源，透明地将 RAM 页移动到交换分区或从交换分区到 RAM 页，tmpfs 文件系统需要 VM 子系统的页面来存储文件。tmpfs 自己并不知道这些页面是在交换分区还是在 RAM 中；做这种决定是 VM 子系统的工作。tmpfs 文件系统所知道的就是它正在使用某种形式的虚拟内存。

由于 tmpfs 是基于内存的，因此速度是相当快的。另外 tmpfs 使用的 VM 资源是动态的，当删除 tmpfs 中文件，tmpfs 文件系统驱动程序会动态地减小文件系统并释放 VM 资源，当然在其中创建文件时也会动态的分配VM资源。另外，tmpfs 不具备持久性，重启后数据不保留。
/dev/shm 就是一个基于 tmpfs 的设备，在有些 Linux 发行版中 /dev/shm 是 /run/shm/ 目录的一个软链接。实际上在很多系统上的 /run 是被挂载为 tmpsf 的。用 df -T 可以查看系统中的磁盘挂载情况：

```
文件系统  1K-块 已用 可用 已用% 挂载点
udev  1859684 4 1859680 1% /dev
tmpfs  374096 1524 372572 1% /run
/dev/sda8 76561456 36029540 36619724 50% /
none   4 0 4 0% /sys/fs/cgroup
none  5120 0 5120 0% /run/lock
none  1870460 27688 1842772 2% /run/shm
none  102400 56 102344 1% /run/user
```

那么，我们就先来说说 /run 目录。现在我们知道，该目录是基于内存的，实际上它的前身是 /var/run 目录，后来被 /run 替换。这是因为 /var/run 文件系统并不是在系统一启动就是就绪的，而在此之前已经启动的进程就先将自己的运行信息存放在 /dev 中，/dev 同样是一种 tmpfs，而且是在系统一启动就可用的。但是 /dev 设计的本意是为了存放设备文件的，而不是为了保存进程运行时信息的，所以为了不引起混淆，/dev 中存放进程信息的文件都以 "." 开始命名，也就是都是隐藏文件夹。但是即便是这样，随着文件夹的数量越来越多，/dev 里面也就越来越混乱，于是就引入了替代方案，也就是 /run。实际上在很多系统上 /var/run 目录仍然存在，但其是 /run 目录的一个软链接。

/var/run 目录中主要存放的是自系统启动以来描述系统信息的文件。比较常见的用途是 daemon 进程将自己的 pid 保存到这个目录。

/dev/shm/ 是 Linux 下一个非常有用的目录，它的意思是 Shared memory，也就是共享内存。由于它在内存上，所以所有系统进程都能共享该目录。默认情况下它的大小是内存的一半。如果希望改变它的大小，可以用 mount 来管理：
mount -o size=4000M -o nr_inodes=1000000 -o noatime,nodiratime -o remount /dev/shm

如果希望永久生效，可以修改 /etc/fstab 文件：
tmpfs /dev/shm tmpfs defaults,size=4G 0 0
利用 /dev/shm 可以做很多事情，这里说一个 Python 的应用。用 Python 做数据处理时，可能会用到 numpy，通常做数据处理时的数据量都是很大的，如果有多个进程都需要用到同样的数据，那么 /dev/shm 就派上了用场，也就是用共享内存技术。Python 有一个第三方库可以用来在多个进程间共享 numpy 数组，即 SharedArray。SharedArray 便是基于 /dev/shm 的，并且采用 POSIX 标准，能够兼容多个平台。


######mount -n -t tmpfs tmpfs /dev/shm


##dev/pts  

 是远程登录终端,比如ssh telet

 devpts是一种文件系统

 /bin/mount -n -t devpts none /dev/pts -o mode=0622
 
 pts是远程虚拟终端。devpts即远程虚拟终端文件设备。通过/dev/pts可以了解目前远程虚拟终端的基本情况。



##/etc/fstab
是用来存放文件系统的静态信息的文件

mount -a命令会读取fstab文件的内容，根据内容进行挂载

当系统启动的时候，系统会自动地从这个文件读取信息，并且会自动将此文件中指定的文件系统挂载到指定的目录。下面我来介绍如何在此文件下填写信息。

/etc/fstab 文件包含了如下字段，通过空格或 Tab 分隔：


`<file system>	 <dir> <type> <options> <dump> <pass>`

`<file systems>` - 要挂载的分区或存储设备.

`<dir> - <file systems>` - 的挂载位置。

`<type>` - 要挂载设备或是分区的文件系统类型，支持许多种不同的文件系统：ext2, ext3, ext4, reiserfs, xfs, jfs, smbfs, iso9660, vfat, ntfs, swap 及 auto。设置成auto类型，mount 命令会猜测使用的文件系统类型，对 CDROM 和 DVD 等移动设备是非常有用的。

`<options>` - 挂载时使用的参数，注意有些mount参数是特定文件系统才有的。

一些比较常用的参数有：

```
auto - 在启动时或键入了 mount -a 命令时自动挂载。
noauto - 只在你的命令下被挂载。	
exec - 允许执行此分区的二进制文件。
noexec - 不允许执行此文件系统上的二进制文件。
ro - 以只读模式挂载文件系统。
rw - 以读写模式挂载文件系统。
user - 允许任意用户挂载此文件系统，若无显示定义，隐含启用 noexec, nosuid, nodev 参数。
users - 允许所有 users 组中的用户挂载文件系统.
nouser - 只能被 root 挂载。
owner - 允许设备所有者挂载.
sync - I/O 同步进行。
async - I/O 异步进行。
dev - 解析文件系统上的块特殊设备。
nodev - 不解析文件系统上的块特殊设备。
suid - 允许 suid 操作和设定 sgid 位。这一参数通常用于一些特殊任务，使一般用户运行程序时临时提升权限。
nosuid - 禁止 suid 操作和设定 sgid 位。
noatime - 不更新文件系统上 inode 访问记录，可以提升性能(参见 atime 参数)。
nodiratime - 不更新文件系统上的目录 inode 访问记录，可以提升性能(参见 atime 参数)。
relatime - 实时更新 inode access 记录。只有在记录中的访问时间早于当前访问才会被更新。（与 noatime 相似，但不会打断如 mutt 或其它程序探测文件在上次访问后是否被修改的进程。），可以提升性能(参见 atime 参数)。
flush - vfat 的选项，更频繁的刷新数据，复制对话框或进度条在全部数据都写入后才消失。
defaults - 使用文件系统的默认挂载参数，例如 ext4 的默认参数为:rw, suid, dev, exec, auto, nouser, async.
<dump> dump 工具通过它决定何时作备份. dump 会检查其内容，并用数字来决定是否对这个文件系统进行备份。 允许的数字是 0 和 1 。0 表示忽略， 1 则进行备份。大部分的用户是没有安装 dump 的 ，对他们而言 <dump> 应设为 0。
<pass> fsck 读取 <pass> 的数值来决定需要检查的文件系统的检查顺序。允许的数字是0, 1, 和2。 根目录应当获得最高的优先权 1, 其它所有需要被检查的设备设置为 2. 0 表示设备不会被 fsck 所检查。
```

##devtmpfs
devtmpfs的功用是在Linux核心启动早期建立一个初步的/dev，令一般启动程序不用等待udev，缩短GNU/Linux的开机时间。

/dev/shm这个目录是linux下一个利用内存虚拟出来的一个目录,这个目录中的文件都是保存在内存中，而不是磁盘上。速度很快。其大小是非固定的，即不是预先分配好的内存来存储的。(shm == shared memory)

######sudo mount -o size=5128M  -o remount /dev/shm

修改/dev/shm的大小

######mount -o size=1500M -o nr_inodes=1000000 -o noatime,nodiratime -o remount /dev/shm


##/etc/inittab
inittab 该文件并不是一定要用，在busybox的源代码中init/init.c中的static void parse_inittab(void)函数内可以看到，如果inittab文件不存在，则会按照默认的配置执行初始化操作。否则按照inittab中设定的值进行初始化。

```
#define INIT_SCRIPT  "/etc/init.d/rcS"
const char bb_default_login_shell[] ALIGN1 = LIBBB_DEFAULT_LOGIN_SHELL;
#define LIBBB_DEFAULT_LOGIN_SHELL  "-/bin/sh"
 
 busybox/init/init.c
 ...
 673 #if ENABLE_FEATURE_USE_INITTAB
 674     char *token[4];
 675     parser_t *parser = config_open2("/etc/inittab", fopen_for_read);
 676 
 677     if (parser == NULL)
 678 #endif
 ...
```

默认的inittab

```
::sysinit:/etc/init.d/rcS
::askfirst:/bin/sh
::ctrlaltdel:/sbin/reboot
::shutdown:/sbin/swapoff -a
::shutdown:/bin/umount -a -r
::restart:/sbin/init
tty2::askfirst:/bin/sh
tty3::askfirst:/bin/sh
tty4::askfirst:/bin/sh

```

##/etc/profile
/etc/profile 这个文件是每个用户登陆时都会运行的环境变量设置，用户可以在在该文件内增加自己想要设置的环境变量。与.bashrc类似，只是.bashrc时针对当前用户适用，
/etc/profile 内新增的环境变量需要重启之后才能生效

##/etc/passwd
该文件用来记录每个用户的基本属性。这个文件对所有用户都是可读的。

```
-[用户名]:[密码]:[UID]:[GID]:[用户全名或本地账号]:[用户主目录]:[登陆使用的shell]
root::0:0:root:/:/bin/sh
bin:*:1:1:bin:/bin:
daemon:*:2:2:daemon:/sbin:
nobody:*:99:99:Nobody:/:
```

```
steve:xyDfccTrt180x,M.y8:0:0:admin:/:/bin/sh   
restrict:pomJk109Jky41,.1:0:0:admin:/:/bin/sh   
pat:xmotTVoyumjls:0:0:admin:/:/bin/sh   
```
可以看到,steve的口令逗号后有4个字符,restrict有2个,pat没有逗号. 

逗号后第一个字符是口令有效期的最大周数,第二个字符决定了用户再次 修改口信之前,原口令应使用的最小周数(这就防止了用户改了新口令后立刻 又改回成老口令).其余字符表明口令最新修改时间. 

要能读懂口令中逗号后的信息,必须首先知道如何用passwd_esc计数,计数的方法是:
 
`.=0 /=1 0-9=2-11 A-Z=12-37 a-z=38-63`

系统管理员必须将前两个字符放进/etc/passwd文件,以要求用户定期的 修改口令,另外两个字符当用户修改口令时,由passwd命令填入. 

注意:若想让用户修改口令,可在最后一次口令被修改时,放两个".",则下一次用户登录时将被要求修改自己的口令. 


##/etc/fstab

该文件的作用：记录了计算机上硬盘分区的相关信息，启动 Linux 的时候，检查分区的fsck命令，和挂载分区的mount命令，都需要fstab中的信息，来正确的检查和挂载硬盘。 

##/etc/mtab

先看它的英文是: 

```
     This changes continuously as the file /proc/mount changes. In  other words, when filesystems are 
     mounted and unmounted, the change is   immediately reflected in this file. 
```

记载的是现在系统已经装载的文件系统，包括操作系统建立的虚拟文件等；而/etc/fstab是系统准备装载的。 

每当 mount 挂载分区、umount 卸载分区，都会动态更新 mtab，mtab  总是保持着当前系统中已挂载的分区信息，fdisk、df 这类程序，必须要读取 mtab   文件，才能获得当前系统中的分区挂载情况。当然我们自己还可以通过读取/proc/mount也可以来获取当前挂载信息 

在m3 busybox中默认没有配置CONFIG_FEATURE_MTAB_SUPPORT

在新版本的busybox中使用/proc/mounts代替/etc/mtab文件，现在/etc/mtab文件应该是/proc/mounts的一个符号链接。只有当你没有/proc目录的时候才应该启用/etc/mtab功能。

##/proc

/proc目录是一种文件系统，即proc文件系统。与其它常见的文件系统不同的是，/proc是一种伪文件系统（也即虚拟文件系统），存储的是当前内核运行状态的一系列特殊文件，用户可以通过这些文件查看有关系统硬件及当前正在运行进程的信息，甚至可以通过更改其中某些文件来改变内核的运行状态。

基于/proc文件系统如上所述的特殊性，其内的文件也常被称作虚拟文件，并具有一些独特的特点。例如，其中有些文件虽然使用查看命令查看时会返回大量信息，但文件本身的大小却会显示为0字节。此外，这些特殊文件中大多数文件的时间及日期属性通常为当前系统时间和日期，这跟它们随时会被刷新（存储于RAM中）有关。

为了查看及使用上的方便，这些文件通常会按照相关性进行分类存储于不同的目录甚至子目录中，如/proc/scsi目录中存储的就是当前系统上所有SCSI设备的相关信息，/proc/N中存储的则是系统当前正在运行的进程的相关信息，其中N为正在运行的进程（可以想象得到，在某进程结束后其相关目录则会消失）。

大多数虚拟文件可以使用文件查看命令如cat、more或者less进行查看，有些文件信息表述的内容可以一目了然，但也有文件的信息却不怎么具有可读性。不过，这些可读性较差的文件在使用一些命令如apm、free、lspci或top查看时却可以有着不错的表现。
可以对proc下的文件进行读写来获取或设置内核的一些状态

清单 2 展示了对 /proc 中的一个虚拟文件进行读写的过程。这个例子首先检查内核的 TCP/IP 栈中的 IP 转发的目前设置，然后再启用这种功能。
清单 2. 对 /proc 进行读写（配置内核）

```
[root@plato]# cat /proc/sys/net/ipv4/ip_forward
0
[root@plato]# echo "1" > /proc/sys/net/ipv4/ip_forward
[root@plato]# cat /proc/sys/net/ipv4/ip_forward
1
[root@plato]#
```

##sysfs

sysfs是一个与 /proc 类似的文件系统，但是它的组织更好（从 /proc 中学习了很多教训）。不过 /proc 已经确立了自己的地位，因此即使 sysfs 与 /proc 相比有一些优点，/proc 也依然会存在。sysfs文件系统总是被挂载在 /sys 挂载点上。虽然在较早期的2.6内核系统上并没有规定 sysfs 的标准挂载位置，可以把 sysfs挂载在任何位置，但较近的2.6内核修正了这一规则，要求 sysfs 总是挂载在 /sys 目录上.

sysfs 与 proc 相比有很多优点，最重要的莫过于设计上的清晰。一个 proc 虚拟文件可能有内部格式，如 /proc/scsi/scsi,它是可读可写的，(其文件权限被错误地标记为了 0444，这是内核的一个BUG)，并且读写格式不一样，代表不同的操作，应用程序中读到了这个文件的内容一般还需要进行字符串解析，而在写入时需要先用字符串格式化按指定的格式写入字符串进行操作；相比而言， sysfs 的设计原则是一个属性文件只做一件事情， sysfs属性文件一般只有一个值，直接读取或写入。整个 /proc/scsi 目录在2.6内核中已被标记为过时(LEGACY)，它的功能已经被相应的 /sys 属性文件所完全取代。新设计的内核机制应该尽量使用 sysfs 机制，而将 proc 保留给纯净的“进程文件系统”。

###/sys/devices
下是所有设备的真实对象，包括如视频卡和以太网卡等真实的设备，也包括 ACPI 等不那么显而易见的真实设备、还有 tty, bonding
等纯粹虚拟的设备；在其它目录如 class, bus 等中则在分类的目录中含有大量对 devices 中真实对象引用的符号链接文件

###/sys
下的子目录所包含的内容/sys/devices这是内核对系统中所有设备的分层次表达模型，也是 /sys 文件系统管理设备的最重要的目录结构，下文会对它的内部结构作进一步分析；
###/sys/dev 
这个目录下维护一个按字符设备和块设备的主次号码(major:minor)链接到真实的设备(/sys/devices下)的符号链接文件，它是在内核 2.6.26 首次引入；
###/sys/bus
这是内核设备按总线类型分层放置的目录结构， devices中的所有设备都是连接于某种总线之下，在这里的每一种具体总线之下可以找到每一个具体设备的符号链接，它也是构成 Linux 统一设备模型的一部分；
###/sys/class
这是按照设备功能分类的设备模型，如系统所有输入设备都会出现在/sys/class/input之下，而不论它们是以何种总线连接到系统。它也是构成 Linux 统一设备模型的一部分；
###/sys/block
这里是系统中当前所有的块设备所在，按照功能来说放置在 /sys/class 之下会更合适，但只是由于历史遗留因素而一直存在于/sys/block, 但从 2.6.22 开始就已标记为过时，只有在打开了 CONFIG_SYSFS_DEPRECATED配置下编译才会有这个目录的存在，并且在 2.6.26 内核中已正式移到 /sys/class/block, 旧的接口 /sys/block为了向后兼容保留存在，但其中的内容已经变为指向它们在 /sys/devices/ 中真实设备的符号链接文件；
###/sys/firmware
这里是系统加载固件机制的对用户空间的接口，关于固件有专用于固件加载的一套API，在附录 LDD3 一书中有关于内核支持固件加载机制的更详细的介绍；
###/sys/fs
这里按照设计是用于描述系统中所有文件系统，包括文件系统本身和按文件系统分类存放的已挂载点，但目前只有 fuse,gfs2 等少数文件系统支持sysfs 接口，一些传统的虚拟文件系统(VFS)层次控制参数仍然在 sysctl (/proc/sys/fs) 接口中中；
###/sys/kernel
这里是内核所有可调整参数的位置，目前只有 uevent_helper, kexec_loaded, mm, 和新式的 slab 分配器等几项较新的设计在使用它，其它内核可调整参数仍然位于 sysctl (/proc/sys/kernel) 接口中.
###/sys/module
这里有系统中所有模块的信息，不论这些模块是以内联(inlined)方式编译到内核映像文件(vmlinuz)中还是编译为外部模块(ko文件)，都可能会出现在 /sys/module中,编译为外部模块(ko文件)在加载后会出现对应的 /sys/module//, 并且在这个目录下会出现一些属性文件和属性目录来表示此外部模块的一些信息，如版本号、加载状态、所提供的驱动程序等；编译为内联方式的模块则只在当它有非0属性的模块参数时会出现对应的 /sys/module/, 这些模块的可用参数会出现在 /sys/modules//parameters/ 中，如 /sys/module/printk/parameters/time 这个可读写参数控制着内联模块 printk 在打印内核消息时是否加上时间前缀；所有内联模块的参数也可以由".="的形式写在内核启动参数上，如启动内核时加上参数 "printk.time=1" 与 向"/sys/module/printk/parameters/time" 写入1的效果相同；没有非0属性参数的内联模块不会出现于此。
###/sys/power
这里是系统中电源选项，这个目录下有几个属性文件可以用于控制整个机器的电源状态，如可以向其中写入控制命令让机器关机、重启等。
###/sys/slab 
(对应 2.6.23 内核，在 2.6.24 以后移至 /sys/kernel/slab)从2.6.23开始可以选择 SLAB 内存分配器的实现，并且新的 SLUB（Unqueued Slab Allocator）被设置为缺省值；如果编译了此选项，在 /sys 下就会出现 /sys/slab ，里面有每一个 kmem_cache
结构体的可调整参数。对应于旧的 SLAB 内存分配器下的 /proc/slabinfo 动态调整接口，新式的
/sys/kernel/slab/ 接口中的各项信息和可调整项显得更为清晰。















