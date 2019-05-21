#/bin/bash

TMP=/tmp/pingtmp

ip addr show wlan0 | grep inet
if [ $? -ne 0 ]
then
    echo "network disconnect"
else
    gateway=`route | grep 'default' | awk '{print $2}'`
    echo $gateway
    if [ -z "$gateway" ]; then
        echo "no default gateway"
        service network-manager restart
        sleep 3
        exit 0
    fi
    > ${TMP}
    ping -c 3 -i 0.1 $gateway >> ${TMP}
    LOST=`grep loss ${TMP} |awk -F "%" '{print$1"%"}'|awk '{print $NF}' `
    echo $LOST
    if [ "$LOST"x = "0%"x ]
    then
        echo "no packet lost"
    else
        echo "packets lost"
        service network-manager restart
        sleep 3
        exit 0
    fi
fi
