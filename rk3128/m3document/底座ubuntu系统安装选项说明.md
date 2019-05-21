#底座系统安装指南（通过U盘给底座安装Ubuntu16.04系统）

##第一步、设置通过U盘启动系统

1.在设备上插入安装U盘，持续点击del按钮，直到屏幕出现如下界面
![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG25.jpeg)

按方向右键选中boot菜单，

![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG27.jpeg)

按方向下选中Boot Option #1 点击enter按钮，选择`UEFI：USB2.0,Partition 1`

![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG26.jpeg)

然后按方向右键，选择Save Changes and Exit

![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG24.jpeg)
设备会重新启动。

##第二步、安装ubuntu16.04系统

* 操作按顺序执行，没有出现在说明中的选项用默认配置即可，直接enter跳过。

在设备重启后会弹出下面的界面
![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG23.jpeg)

选中Install Ubuntu Server
等待片刻，出现如下图
![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG22.jpeg)
默认即可，点击enter按钮

![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG21.jpeg)
默认跳过。

接下来出现的界面选择<NO>
然后接下啦的两个界面均用默认选项跳过。

在配置网络是出现找不到网卡的提示，不用理会，选择continue
![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG17.jpeg)

在set up users and passwords界面需要用户输入账户名frog
输入后会提醒用户确认，直接按enter继续。
![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG18.jpeg)

然后出现set up users and passwords选项，用来设置密码。
输入密码frogshealth后确认，需要用户再次输入密码确认。

再次输入完成后需要选择yes继续。

![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG19.jpeg)

然后默认选择no继续安装。

接下来默认选择时钟源使用默认选项跳过。

然后弹出如下界面，选择Guided - use enture disk
![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG20.jpeg)

默认选择MMC/SD card #1 (mmcblk0) ...

![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG31.jpeg)

然后在弹出页面选择yes继续安装
![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG16.jpeg)

在Configure the package manager 
![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG15.jpeg)
直接enter跳过

选择No automatic updates
![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG14.jpeg)

选中如图中的选项，点击enter
![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG13.jpeg)
然后就等待安装完成后，点击enter完成安装了。

##最后一步
开机登陆,账号frog，密码frogshealth

在安装过程中需要设备能够接入互联网才能够正常获得安装包。

执行sudo ./autoinstall.sh

输入密码frogshealth

就会开始自动安装了，安装过程中有一处需要用户输入y，然后按enter按钮。
![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG29.jpeg)

在安装过程中会在显示器上显示一个界面，需要选择第一排的选项执行安装。此后就没有需要再用户干预的地方了。
![MacDown Screenshot](/Users/duanshuai/Downloads/WPA_TOOLS/WechatIMG30.jpeg)

在安装完后执行sudu reboot，输入密码。设备会重新启动。

启动后如果安装正确，再用户输入账号密码后会自动启动页面程序。用户可以用鼠标点击按钮开启wifi或刷机了。