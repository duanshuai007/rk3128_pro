#!/bin/bash

_INSTALL_DIR=/root
USBRULE=/etc/udev/rules.d/86-my_usb_device_rule.rules
USER=`who | sed -n 1p | cut -d " " -f 1`
DownLoadDir=/tmp/download
RootWifiDir=/root/wifitool
RootUpgradeDir=/root/upgradetool
Rules="ACTION=\"add\", SUBSYSTEM==\"usb\", ATTRS{idVendor}=\"2207\", RUN+=\"/root/upgradetool/shuaji.sh\""

mkdir -p ${DownLoadDir}
mkdir -p ${RootWifiDir}
mkdir -p ${RootUpgradeDir}

#net_iface=`ifconfig -a | grep Ethernet | grep -v ^wl* | cut -d " " -f 1`
net_iface=`ifconfig -a | sed -n '/^[a-zA-Z0-9]/p' | cut -d : -f 1` | grep -v lo | grep -v wl*
echo "************* Start Config Netwoek[  "${net_iface}"  ]  *************"
dhclient ${net_iface}

#获取安装包
echo "************* Get Packet *******************"
curdir=`pwd`
cd ${DownLoadDir}
wget http://192.168.200.202/packet.tar.jz
#解压
tar xvf packet.tar.jz 
cd ${curdir}

echo "************* Start Looking for Update && Upgrade *************"
version=`lsb_release -a 2>/dev/null | grep Description | cut -d " " -f 2 | cut -d . -f 1`
if [ ${version} -eq 16 ]
then
    cp ${DownLoadDir}/packet/sources.list /etc/apt/
    sleep 1
    sudo apt-get update > ${DownLoadDir}/result_update.log
    sleep 1
    apt-get upgrade 
fi

#设置udev规则
echo "************* Setting udev rules *************"
if [ ! -e ${USBRULE} ] 
then
    touch ${USBRULE}
fi
cat ${USBRULE} | grep "upgradetool" > /dev/null
if [ $? -ne 0 ]
then
    echo ${Rules} > ${USBRULE}
fi

/etc/init.d/udev restart

Install() {
    name=$1
    echo "************* Start Install ${name} *************"
    touch ${DownLoadDir}/result_${name}.log
    dpkg --get-selections | grep "${name}" > /dev/null
    if [ $? -eq 0 ]
    then
        dpkg --get-selections | grep "${name}" | grep deinstall > /dev/null
        if [ $? -eq 0 ]
        then
            echo y | apt-get install ${name} > ${DownLoadDir}/result_${name}.log 2>&1
        fi
    else
        echo y | apt-get install ${name} > ${DownLoadDir}/result_${name}.log 2>&1
    fi

    cat ${DownLoadDir}/result_${name}.log | grep "dpkg --configure -a" > /dev/null
    if [ $? -eq 0 ]
    then
        dpkg --configure -a
        echo y | apt-get install ${name} > ${DownLoadDir}/result_${name}.log 2>&1
    fi
}
#安装ubuffer工具包
Install expect-dev
#Install libX11-dev
#Install libXext-dev 
#Install libXtst-dev
#Install libXrender-dev
#Install libxmu-dev
#Install libxmuu-dev
#Install xserver-xorg
Install xorg
Install xorg
Install network-manager
Install chromium-browser
Install nginx
Install php
Install php-fpm

echo "************* Start Remove apache2  *************"
while true
do
    num=`dpkg --get-selections | grep apache | grep -w install | wc -l`
    while [ ${num} -ne 0 ]
    do
        name=`dpkg --get-selections | grep apache | grep -w install | sed -n 1p | awk -F" " '{print $1}'`
        echo y | apt-get remove ${name} > /dev/null
        sleep 1
        num=`expr ${num} - 1`
    done
    dpkg --get-selections | grep apache | grep -w install
    if [ $? -ne 0 ]
    then
        break
    fi
    sleep 1
done

echo "************* Start autoremove  *************"
echo y | apt-get autoremove > /dev/null
sleep 1

echo "************* update && upgrade ***************"
sudo apt-get update > /dev/null
echo y | apt-get upgrade > /dev/null

/etc/init.d/network-manager start 
/etc/init.d/nginx start
#设置启动脚本
echo "************* Setting user profile ***************"
cat /home/${USER}/.profile | grep "chromium-browser" > /dev/null
if [ $? -ne 0 ]
then
    unbuffer echo "[[ ! \$DISPLAY && \$XDG_VTNR -eq 1 && \$(id --group) -ne 0 ]] && exec startx /usr/bin/chromium-browser --noerrdialogs --kiosk localhost" >> /home/${USER}/.profile
fi
sleep 1

#设置/etc/nginx/sites-enabled/default文件
echo "************* Install wifitool && upgradetool ******************"
TAR=/etc/nginx/sites-enabled/default
if [ ! -e "${TAR}" ]
then
    echo "/etc/nginx/sites-enabled/default not exist!"
    exit
fi

cp ${DownLoadDir}/packet/nginx_se_default ${TAR}

cp -r ${DownLoadDir}/packet/html /usr/share/nginx/
#把功能脚本移动到root目录下
cp ${DownLoadDir}/packet/wifitool/* ${RootWifiDir}
cp -r ${DownLoadDir}/packet/upgradetool/* ${RootUpgradeDir}
#修改文件权限和归属
chown -R root:root ${RootWifiDir}
chown -R root:root ${RootUpgradeDir}

#设置开机启动脚本
echo "************* Setting autorun Script ******************"
SCRIPT_NAME=myscript
SCRIPT=/etc/init.d/${SCRIPT_NAME}
touch ${SCRIPT} 
chmod +x ${SCRIPT}
echo "#!/bin/sh
### BEGIN INIT INFO
# Provides:          bbzhh.com
# Required-Start:    $local_fs $network
# Required-Stop:     $local_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: tomcat service
# Description:       tomcat service daemon
### END INIT INFO
" > ${SCRIPT}
#在这里添加具体的想要执行的程序
echo "
sh ${RootWifiDir}/watch_wifi.sh &
sh ${RootUpgradeDir}/watch_upgrade.sh &
exit 0
" >> ${SCRIPT}

update-rc.d ${SCRIPT_NAME} defaults 90

mkdir -p /usr/share/nginx/html/tmp
echo "************* Install Finish ******************"
echo "***********************************************"
