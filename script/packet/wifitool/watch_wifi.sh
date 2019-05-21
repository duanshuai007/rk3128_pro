#!/bin/sh

WIFIMEM=/dev/shm/wifi
UPGRADEMEM=/dev/shm/upgrade
HTMLWIFIMEM=/usr/share/nginx/html/tmp/wifimanager
HTMLUPGRADEMEM=/usr/share/nginx/html/tmp/upgradetool

mkdir -p ${WIFIMEM} 
mkdir -p ${UPGRADEMEM}
mkdir -p ${HTMLWIFIMEM}
mkdir -p ${HTMLUPGRADEMEM}

chmod 777 ${WIFIMEM}
chmod 777 ${UPGRADEMEM}
chmod 777 ${HTMLWIFIMEM}
chmod 777 ${HTMLUPGRADEMEM}

mount --bind ${WIFIMEM} ${HTMLWIFIMEM}
mount --bind ${UPGRADEMEM} ${HTMLUPGRADEMEM}

while true
do
    ps -ef | grep -w "wifi.sh" | grep -v grep > /dev/null

    if [ $? -ne 0 ]
    then
        sh /root/wifitool/wifi.sh &
    fi

    sleep 1

done
