##蓝牙驱动

###刷固件

首先执行`rfkill unblock bluetooth`，修改`bluez`代码
`hciattach.c`中在`set_speed`函数的开始处增加`usleep(200000);`。

创建目录`mkdir -p /etc/firmware`

将固件文件放到目录下，使用的固件为`4343A0.hcd`。固件大小为29773字节。


###刷固件方法一
蓝牙驱动需要刷入固件，使用`brcm_patchram_plus`执行刷固件的操作。通过`tftp`

或其他方式获得该程序，添加可执行属性，移动到`/usr/bin`目录下。
	
在`/usr/bin/`目录下创建文件`btstart`,在其中添加内容：

```
brcm_patchram_plus --tosleep=200000 --no2bytes --enable_hci 
	--patchram=/system/vendor/firmware/bcm43438a0.hcd /dev/ttyS0 &
```
```
可以在brcm_patchram_plus命令之后追加--bd_addr来设置设备地址。
```

为`btstart`添加可执行属性。
执行`btstart`后会输出如下：

```
root@firefly:~# BT chip name is: ap6335
hcd file: /etc/firmware/bcm43438a0.hcd
...
root@firefly:~# 
root@firefly:~# Done setting line discpline
```



###刷固件方法二
`hciattach /dev/ttyS0 bcm43xx 115200 flow -t 10`

执行完成后可以通过`hciconfig`工具来查看

```
root@firefly:~# hciconfig -a
hci0:   Type: BR/EDR  Bus: UART
        BD Address: B5:46:C8:B4:D0:F2  ACL MTU: 1021:8  SCO MTU: 64:1
        UP RUNNING PSCAN 
        RX bytes:661 acl:0 sco:0 events:38 errors:0
        TX bytes:2193 acl:0 sco:0 commands:38 errors:0
        Features: 0xbf 0xfe 0xcf 0xfe 0xdb 0xff 0x7b 0x87
        Packet type: DM1 DM3 DM5 DH1 DH3 DH5 HV1 HV2 HV3 
        Link policy: RSWITCH SNIFF 
        Link mode: SLAVE ACCEPT 
        Name: 'firefly'
        Class: 0x000000
        Service Classes: Unspecified
        Device Class: Miscellaneous, 
        HCI Version: 4.1 (0x7)  Revision: 0x3b
        LMP Version: 4.1 (0x7)  Subversion: 0x2122
        Manufacturer: Broadcom Corporation (15)
```

`hci`工具和`rfcomm`工具是旧版本的蓝牙工具，现在已不建议使用，推荐使用`bluetoothctl`来使用蓝牙。


`hci`工具的一些命令使用示例。

```
root@firefly:~# hciconfig hci0 up
root@firefly:~# hciconfig hci0 iscan   //使能位于hci0接口的蓝牙芯片的inquery scan模式（使得设备能被其它蓝牙设备发现）
root@firefly:~# hcitool scan
Scanning ...
        74:AC:5F:E3:45:F3       360 N5S
```

###安装bluealsa工具
`bluetoothctl`连接设备还需要`bluealsa`的支持,`bluealsa` 依赖 `libsbc1,libbluetooth3, libasound2`

从树莓派开发板中获取了`bluealsa`的deb包`bluealsa_0.7_armhf.deb`

在树莓派`/var/cache/apt/archives/`目录下。如果没有，则用`apt-get install`命令重新安装`bluez`工具包即可。

复制到开发板中，
执行`dpkg -i bluealsa_0.7_armhf.deb`进行安装，根据提示安装依赖的文件包。

安装完成后需要启动蓝牙服务。

###启动蓝牙服务
`systemctl start bluetooth`

启动蓝牙服务后可以通过`ps -ef | grep blue`来查看是否启动了相应的进程。

```
root@firefly:~# ps -ef | grep blue
root       434     1  0 15:27 ?        00:00:00 /usr/lib/bluetooth/bluetoothd
root      1616     1  0 15:30 ?        00:00:00 /usr/bin/bluealsa
root      1639  1412  0 15:31 ttyFIQ0  00:00:00 grep --color=auto blue
```
* 需要蓝牙模块已经完成了加载固件的操作才能够看到`bluealsa`的进程


在`bluetooth`固件成功加载后，执行`bluetoothctl`会进入`bluetooth`命令行。
分别执行

```
[bluetooth]# power on
Changing power on succeeded
```

```
[bluetooth]# agent on
Agent registered
```

```
[bluetooth]# default-agent
Default agent request successful
```

```
[bluetooth]# pairable on
Changing pairable on succeeded
```

```
[bluetooth]# discoverable on
Changing discoverable on succeeded
[CHG] Controller C2:BA:2F:04:02:84 Discoverable: yes
```

```
scan on				//开启扫描
scan off			//当扫描到想要的设备后可关闭扫描
```
执行`scan on`扫描设备后返回信息如下：

```
[bluetooth]# scan on
Discovery started
[CHG] Controller AC:F1:B6:8B:08:5D Discovering: yes
[NEW] Device 30:AE:A4:12:0F:BA 30-AE-A4-12-0F-BA
[NEW] Device 41:6C:D8:91:76:3C 41-6C-D8-91-76-3C
[NEW] Device C8:93:BB:9C:9F:56 MI Band 2
[CHG] Device 41:6C:D8:91:76:3C RSSI: -60
[NEW] Device 18:65:90:DC:BA:38 duanshuai?的MacBook Pro
[NEW] Device 78:45:61:E9:5A:D4 LENOVO-PC
[NEW] Device 74:AC:5F:E3:45:F3 360N5S
```
记录下`360N5S的设备地址`，该地址在配对，连接阶段都需要使用。

```
trust [dev]			//信任设备
pair [dev]			//与设备配对
connect [dev]			//连接设备	
```

```
[bluetooth]# trust 74:AC:5F:E3:45:F3
[CHG] Device 74:AC:5F:E3:45:F3 Trusted: yes
Changing 74:AC:5F:E3:45:F3 trust succeeded
```


```
[bluetooth]# pair 74:AC:5F:E3:45:F3
Attempting to pair with 74:AC:5F:E3:45:F3
[CHG] Device 74:AC:5F:E3:45:F3 Connected: yes
[CHG] Device 74:AC:5F:E3:45:F3 Modalias: bluetooth:v000Fp1200d1436
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 0000110b-0000-1000-8000-00805f9b34fb
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 0000110e-0000-1000-8000-00805f9b34fb
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 00001200-0000-1000-8000-00805f9b34fb
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 00001800-0000-1000-8000-00805f9b34fb
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 00001801-0000-1000-8000-00805f9b34fb
[CHG] Device 74:AC:5F:E3:45:F3 ServicesResolved: yes
[CHG] Device 74:AC:5F:E3:45:F3 Paired: yes
Pairing successful
[CHG] Device 74:AC:5F:E3:45:F3 ServicesResolved: no
[CHG] Device 74:AC:5F:E3:45:F3 Connected: no
```
当与设备成功连接后，命令行提示符会发生改变，我所连接的设备名是`360N5S`,连接后显示如下。

```
[bluetooth]# connect 74:AC:5F:E3:45:F3
Attempting to connect to 74:AC:5F:E3:45:F3
[CHG] Device 74:AC:5F:E3:45:F3 Connected: yes
[CHG] Device 74:AC:5F:E3:45:F3 Modalias: bluetooth:v000Fp1200d1436
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 00001105-0000-1000-8000-00805f9b34fb
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 00001106-0000-1000-8000-00805f9b34fb
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 0000110a-0000-1000-8000-00805f9b34fb
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 0000110c-0000-1000-8000-00805f9b34fb
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 0000110e-0000-1000-8000-00805f9b34fb
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 00001112-0000-1000-8000-00805f9b34fb
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 0000111f-0000-1000-8000-00805f9b34fb
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 0000112f-0000-1000-8000-00805f9b34fb
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 00001132-0000-1000-8000-00805f9b34fb
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 00001200-0000-1000-8000-00805f9b34fb
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 00001800-0000-1000-8000-00805f9b34fb
[CHG] Device 74:AC:5F:E3:45:F3 UUIDs: 00001801-0000-1000-8000-00805f9b34fb
Request confirmation
[agent] Confirm passkey 775820 (yes/no): yes
[CHG] Device 74:AC:5F:E3:45:F3 Paired: yes
Connection successful
[360N5S]# 

```
连接成功后可以使用`exit`命令退出命令行。


###通过蓝牙设备发送文件
发送文件使用`obexftp`工具，如果系统内没有该工具，执行`apt-get install obexftp`来安装。

在发送之前还需要通过`sdptool`工具来寻找设备的`OBEX Object Push`。

首先需要记下在`bluetoothctl`命令行下扫描到的设备地址，或者可以使用`hcitool scan`重新扫描设备地址。

执行`sdptool browse [dev addr]`会返回很多信息，寻找其中的`OBEX Object Push`项内容。

```
...
Browsing 74:AC:5F:E3:45:F3 ...
Service Search failed: Invalid argument
Service Name: OBEX Object Push
Service RecHandle: 0x1000a
Service Class ID List:
  "OBEX Object Push" (0x1105)
Protocol Descriptor List:
  "L2CAP" (0x0100)
  "RFCOMM" (0x0003)
    Channel: 6
  "OBEX" (0x0008)
Profile Descriptor List:
  "OBEX Object Push" (0x1105)
    Version: 0x0102
...
```
可以发现`channel=6`。

执行
`obexftp --nopath --noconn --uuid none --bluetooth [dev addr] --channel [channel num] -p [file]`

如果蓝牙运行正常，就会在手机上收到蓝牙接收文件的提示，点击接收就能收到开发板发送过来的文件。


###通过蓝牙接收文件

将obexpush在后台运行，按照以下步骤配置。

修改`/etc/systemd/system/dbus-org.bluez.service`
在这句最后加上`“-C” `
`ExecStart=/usr/lib/bluetooth/bluetoothd -C`

新建文件：`/etc/systemd/system/obexpush.service`
文件内容为：

```
[Unit]
Description=OBEX Push service
After=bluetooth.service
Requires=bluetooth.service
[Service]
ExecStart=/usr/bin/obexpushd -B -o /ble_receive -n
[Install]
WantedBy=multi-user.target
```
执行命令：
`systemctl enable obexpush`

* 重启系统

###通过蓝牙连接蓝牙音箱播放音乐

想要让蓝牙通过蓝牙音箱播放音乐，，需要安装`pulseaudio-module-bluetooth`工具，安装完成后重新启动系统。

还需要安装`alsa`工具，`apt-get install alsa`。

在蓝牙已经与蓝牙音箱成功连接之后，执行以下操作：

* 1.执行`pacmd help`，能够打印出帮助信息。

* 2.执行`pactl list cards`，在打印出的设备中应该有蓝牙bluez设备信息。



```
Card #1
	Name: bluez_card.E8_BB_3D_56_8F_DD
	Driver: module-bluez5-device.c
	Owner Module: 24
	Properties:
		device.description = "X1(08:9D)"
		device.string = "E8:BB:3D:56:8F:DD"
		device.api = "bluez"
		device.class = "sound"
		device.bus = "bluetooth"
		device.form_factor = "speaker"
		bluez.path = "/org/bluez/hci0/dev_E8_BB_3D_56_8F_DD"
		bluez.class = "0x2c0414"
		bluez.alias = "X1(08:9D)"
		device.icon_name = "audio-speakers-bluetooth"
	Profiles:
		a2dp_sink: High Fidelity Playback (A2DP Sink) (sinks: 1, sources: 0, priority: 10, available: yes)
		off: Off (sinks: 0, sources: 0, priority: 0, available: yes)
	Active Profile: a2dp_sink
	Ports:
		speaker-output: Speaker (priority: 0, latency offset: 0 usec)
			Part of profile(s): a2dp_sink
		speaker-input: Bluetooth Input (priority: 0, latency offset: 0 usec, not available)
		
```

* 3.执行`pactl list sinks`和`pactl list sources`命令也能够获取`bluez`相关的配置项。记住这些配置项的index。
通过以下命令来进行配置。

	* `pacmd set-card-profile [index] [prifiles]`
		######pacmd set-card-profile 1 a2dp_sink
		
	* `pacmd set-default-sink [index]`
		######pacmd set_default-sink 1
		
	* `pacmd set-default-source [index]`
		######pacmd set-default-source 1

配置完成之后可以通过`pactl info`来查看配置信息。

通过apply或者play命令来播放音乐文件，此时音乐就会通过蓝牙音箱进行播放。

* 当蓝牙模块串口速率较低时播放音乐会卡顿，目前除了提升波特率还没有找到其它解决办法。

#####使音乐通过本机声卡播放

```
amixer controls
amixer cset numid=18,iface=MIXER,name='Preset' 100
```

###相关进程
`bluetoothd`：该进程是蓝牙工具运行的基础，在执行`bluetoothctl`命令后会调用该进程，对蓝牙设备发出控制指令。

`bluealsa`：该进程是蓝牙音频的进程，缺少该进程会导致蓝牙设备在连接的时候出现错误。


`dbus-daemon --fork --session --address=unix:abstract=/tmp/dbus-BBAZLZqIec`

该进程是`pactl list cards`不出现选项的关键。
安装`lubuntu-desktop`桌面系统后会自动创建该进程，由`lightdm->upstart`创建，地址的值在创建时会发给应用程序，相同地址值的应用之间会通过dbus进行通信。但是当`kill`掉该进程后，`pactl list cards`就会什么都不返回。

该进程起到连接音频驱动的功能，使用`pactl list cards` 能够正确看到返回的声卡信息。如果该进程不运行，则`pactl list cards`会不返回信息。


##蓝牙代码分析记录

###蓝牙调试的方法

```
dmesg -n 8 
./bluez-5.37/src/bluetoothd &
./bluez/bin/hciattach /dev/ttyS0 bcm43xx 1500000 noflow -
./bluez/bin/bluetoothctl
./bluez/bin/hciconfig -a

./configure --prefix=/home/firefly/bluez

**** 要将linux内核的带级别控制的printk内容打印出来，在命令行 输入 dmesg -n 8 就将所有级别的信息都打印出来
```

###蓝牙socket

```
在net/bluetooth/hci_sock.c文件内看到hci_sock_init函数。
hci_sock_init中调用了proto_register(&hci_sk_proto, 0)，
bt_sock_register(BTPROTO_HCI, &hci_sock_family_ops)，
其中bt_sock_register实现非常简单，就是把参数中的ops赋值给bt_proto[proto]，proto这里等于BTPROTO_HCI。
bt_procfs_init(&init_net, "hci", &hci_sk_list, NULL)对蓝牙socket进行了初始化。

static struct proto hci_sk_proto = {
    .name       = "HCI",
    .owner      = THIS_MODULE,
    .obj_size   = sizeof(struct hci_pinfo)
};
```
```
net/bluetooth/af_bluetooth.c 中 bt_init 是整个蓝牙驱动的初始化部分，该函数被subsys_initcall(bt_init);引用。
函数内部先后执行了
1.bt_sysfs_init
net/bluetooth/hci_sysfs.c 内的函数 bt_sysfs_init中调用了debugfs_create_dir("bluetooth", NULL)，和class_create(THIS_MODULE, "bluetooth")，分别创建了debugfs文件系统的bluetooth调试信息和在/sys/class/目录下注册添加bluetooth，自动创建设备节点。
```

```
2.sock_register(&bt_sock_family_ops);
static struct net_proto_family bt_sock_family_ops = {
    .owner  = THIS_MODULE,
    .family = PF_BLUETOOTH,
    .create = bt_sock_create,
};
将PF_BLUETOOTH类型的socket接口注册rcu_assign_pointer(net_families[ops->family], ops);.
```

```
3.hci_sock_init
该函数定义在net/bluetooth/hci_sock.c内，先后调用了
	(1)proto_register(&hci_sk_proto, 0)；
	(2)bt_sock_register(BTPROTO_HCI, &hci_sock_family_ops);
	(3)bt_procfs_init(&init_net, "hci", &hci_sk_list, NULL);
```

```
4.l2cap_init
该函数内调用了l2cap_init_sockets
	(1)proto_register(&l2cap_proto, 0);
	(2)bt_sock_register(BTPROTO_L2CAP, &l2cap_sock_family_ops);
	(3)bt_procfs_init(&init_net, "l2cap", &l2cap_sk_list, NULL);
```

```
5.sco_init
该函数内调用了
	(1)proto_register(&sco_proto, 0);
	(2)bt_sock_register(BTPROTO_SCO, &sco_sock_family_ops);
	(3)bt_procfs_init(&init_net, "sco", &sco_sk_list, NULL);

bt_procfs_init函数功能是在/proc/net目录下创建"hci","l2cap","sco"文件。

static const struct net_proto_family hci_sock_family_ops = {
    .family = PF_BLUETOOTH,
    .owner  = THIS_MODULE,
    .create = hci_sock_create,
};

在hci_sock_create中sock->ops = &hci_sock_ops;对设备的操作都定义在static const struct proto_ops hci_sock_ops 中。

其中发送调用hci_sock_sendmsg。

__sys_sendmsg->___sys_sendmsg->sock_sendmsg->__sock_sendmsg[->security_socket_sendmsg]->__sock_sendmsg_nosec->hci_sock_sendmsg

SYSCALL_DEFINE3(sendmsg, int, fd, struct msghdr __user *, msg, unsigned int, flags)
{
    if (flags & MSG_CMSG_COMPAT)
        return -EINVAL;
    return __sys_sendmsg(fd, msg, flags);
}

net/bluetooth/hci_sock.c 为上层提供一个socket接口，应用程序可以通过socket的方式反问HCI
hci_sock_init中注册了BTPROTO_HCI类型的family。
hci_socket_create创建sock的函数，它的sock的ops指向hci_sock_ops。
hci_sock_setsockopt/hci_sock_getsockopt设置获取sock的一些选项。
hci_sock_sendmsg发送消息，根据消息类型把消息放到适当的队列中。
hci_sock_recvmsg接收消息，从接受队列中取出消息。

L2CAP是HCI之上的协议，提供Qos，分组，多路复用，分段和组装之类的功能。
SCO也是运行在HCI之上的协议，它是面向连接的可靠的传输方式，主要用于声音数据传输。

hci_sock_sendmsg会根据消息类型来分别执行，
HCI_CHANNEL_CONTROL直接转交给mgmt_control进行处理。
HCI_CHANNEL_MONITOR不做任何处理。
HCI_CHANNEL_RAW根据数据包类型不同选择放到tx_work/cmd_work中，然后调度工作队列调用hci_tx_work/hci+cmd_work进行处理‘。

hci_uart_register_dev->hci_alloc_dev->
[    
	INIT_WORK(&hdev->rx_work, hci_rx_work);
    INIT_WORK(&hdev->cmd_work, hci_cmd_work);
    INIT_WORK(&hdev->tx_work, hci_tx_work);
]
在hci_tx_work中对不同类型的数据包进行发送，发送部分函数都是调用hci_send_frame。
在hci_send_frame中调用
2600     /* Send copy to monitor */
2601     hci_send_to_monitor(hdev, skb);
2602 
2603     if (atomic_read(&hdev->promisc)) {
2604         /* Send copy to the sockets */
2605         hci_send_to_sock(hdev, skb);
2606     }
2607 
2608     /* Get rid of skb owner, prior to sending to the driver. */
2609     skb_orphan(skb);
2610 
2611     return hdev->send(skb);

先调用hci_send_to_monitor和hci_send_to_sock组成数据包，最终调用hdev->send进行发送。
hdev->send最终指向函数hci_uart_send_frame，这个函数也是将代发送消息放入队列中，然后唤醒队列发送。
hci_uart_send_frame->hci_uart_tx_wakeup(hu);->schedule_work(&hu->write_work);

在hci_uart_tty_open中有对write_work的设置，
INIT_WORK(&hu->init_ready, hci_uart_init_work);
INIT_WORK(&hu->write_work, hci_uart_write_work);

hci_uart_write_work函数内调用len = tty->ops->write(tty, skb->data, skb->len);来实际执行底层通信。
这里的write函数最终会调用drivers/tty/serial/serial_core.c中uart_write
```

###hci工具的一些分析
```
ldisc N_HCI

hciattach.c中uart_init初始化完成后调用ioctl(fd, TIOCSETD, &i)该语句将调用内核tty_ioctl执行其中的tiocsetd,设置ldisc为N_HCI.
在内核代码中drivers/bluetooth/hci_ldisc.c中hci_uart_init在注册时就使用了N_HCI进行注册。
tty_register_ldisc(N_HCI, &hci_uart_ldisc)，就是将当前打开的文件操作方式设置为这个注册的ldisc。
接下来的open，close,ioctl等操作都是作用在这个ldisk的ops上。
然后ioctl(fd, HCIUARTSETFLAGS, flags) 中设置了一些标志位
在ioctl(fd, HCIUARTSETPROTO, u->proto)中通过查看代码可以看到drivers/bluetooth/hci_ldisc.c中hci_uart_tty_ioctl的swictch中的第一个case就是HCIUARTSETPROTO，执行了hci_uart_set_proto的操作。
在hci_uart_set_proto->hci_uart_register_dev->hci_register_dev->hci_add_sysfs->device_add
hci_uart_set_proto中p->open执行的是[drivers/bluetooth/hci_h4.c][h4_open]
hci_register_dev定义在net/bluetooth/hci_core.c中，执行了hci0设备的注册操作。此时就可以通过hciconfig -a看到注册好的hci0设备了。

在kill掉hciattach进程时，会调用hci_unregister_dev对hci设备进行反注册。
```

###蓝牙工具服务进程的分析
```
bluetoothd属于后台服务进程，一直运行。
bluetoothd在代码目录src文件夹内生成。
在main.c中通过执行g_main_loop_run来进入主循环，它会一直阻塞在这，直到让它退出为止。

bluetoothctl在代码根目录的client文件夹内生成，
在启动之后设置相关的属性，创建dbus的client，设置dbus client的回调函数，然后执行bt_shell_run();
bt_shell_run函数在 src/shared/shell.c中定义
在bt_shell_run中首先调用了setup_signalfd，对信号进行屏蔽设置，然后设置了读取的回调函数
然后进入了mainloop_run函数。在该函数内会一直循环判断执行。通过回调函数执行操作。
bluetoothctl中的命令“pair”,"connect","trust"等都定义在client/main.c中。
自己开一个进程，与bluetoothd之间通过dbus进行通信。

bluez通过hci驱动与底层硬件进行通信。

bluetoothd中通过adapter对hci进行操作。
bluetoothd和bluetoothctl之间通过dbus进行通信。
g_dbus_proxy_method_call

dbus_conn
mgmt_master

在lib/hci.c中通过socket对hci借口进行操作，ioctl(sk, HCIGETDEVLIST, (void *) dl) 可以在内核代码net/bluetooth/hci_sock.c中找到对应的宏
case HCIGETDEVLIST：
    return hci_get_dev_list(argp);
通过命令的方式来操作hci接口。

bluez hci工具与内核中间通过ioctl方式来进行命令交互。 HCISETSCAN 

在src/adapter.c 的 函数adapter_bonding_attempt函数内调用了
6618     id = mgmt_send(adapter->mgmt, MGMT_OP_PAIR_DEVICE,
6619                 adapter->dev_id, sizeof(cp), &cp,
6620                 pair_device_complete, data,
6621                 free_pair_device_data);

在mgmt_send函数在src/shared/mgmt.c中定义
584     request = create_request(opcode, index, length, param,
585                     callback, user_data, destroy);

......

594     if (!queue_push_tail(mgmt->request_queue, request)) 

......

600     wakeup_writer(mgmt);


create_request函数内对结构体进行赋值操作
wakeup_writer函数内调用了io_set_write_handler函数。
242     io_set_write_handler(mgmt->io, can_write_data, mgmt,
243                         write_watch_destroy);
在函数can_write_data内调用了send_request，在send_request内调用src/shared/io-glib.c中的
io_send，该函数内调用writev执行了底层操作的功能吗？

io_get_fd内调用g_io_channel_unix_get_fd(io->channel)，返回一个文件描述符7，

hciattach守护进程的ppoll里面的fd=3，而在使用bluetoothctl时在io_send内使用的fd=7.

在__btd_log_init函数内调用logging_open，执行fd = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
获得的fd=5,该值被赋给logging_fd。

fd=3时open(/dev/ttyS0, xxx)后的返回值。
通过打印信息，发现fd=7出现在mgmt_new_default函数内，由语句fd = socket(PF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK, BTPROTO_HCI);执行后返回值。

net/bluetooth/hci_core.c 内定义了 hci_register_dev
```









































