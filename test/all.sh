#!/bin/bash

#如果没输入参数，则用文件名中的mac地址设备作为主root节点解析
#如果有输入参数
#参数1必须是子设备mac地址
#参数2必须是root设备mac地址

rootaddr="WM_"$1
macaddr[1]=$2
macaddr[2]=$3
macaddr[3]=$4
macaddr[4]=$5

base[1]=base1.js
base[2]=base2.js
base[3]=base3.js
base[4]=base4.js

NUM=1
while [ ${NUM} -lt 5 ]
do
    echo "find base file:${base[${NUM}]}"
    line=`cat ${base[${NUM}]} | grep -n mac | cut -d ":" -f 1`
    sed -i "${line}s/'.*'/'${macaddr[${NUM}]}'/" ${base[${NUM}]}
    line=`cat ${base[${NUM}]} | grep -wn publish | cut -d ":" -f 1`
    sed -i "${line}s/'.*'/'${rootaddr}'/" ${base[${NUM}]}
    NUM=`expr ${NUM} + 1`
done

#
#   par1: base[x].js
#   par2: action
#   par3: time
#

sleep 1

FUNC(){
    echo "IN FUNC"
    NUM=1
    while [ ${NUM} -lt 5 ]
    do
        line=`cat ${base[${NUM}]} | grep -n action | cut -d ":" -f 1`
        echo "FUNC:$1, Time:$2"
        sed -i "${line}s/\(action:\).*/\1 $1,/" ${base[${NUM}]}
        node ${base[${NUM}]}
        NUM=`expr ${NUM} + 1`
    done

    sleep $2
}

while true
do
    FUNC 1 6
    FUNC 2 6
    FUNC 3 3 
    FUNC 4 3
    FUNC 5 3
    FUNC 6 3
    FUNC 7 3
done
