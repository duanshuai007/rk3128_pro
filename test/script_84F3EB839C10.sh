#!/bin/sh

#如果没输入参数，则用文件名中的mac地址设备作为主root节点解析
#如果有输入参数
#参数1必须是子设备mac地址
#参数2必须是root设备mac地址

if [ -z $1 ]
then 
    echo "no paramters"
    echo $0
    macaddr=`echo $0 | cut -d "_" -f 2 | cut -d "." -f 1`
    rootaddr="WM_"${macaddr}
    echo ${macaddr}
else
    macaddr=$1
    rootaddr="WM_"$2
fi

BASE=$3

line=`cat adc.js | grep -n mac | cut -d ":" -f 1`

sed -i "${line}s/'.*'/'${macaddr}'/" ${BASE}

line=`cat ${BASE} | grep -wn publish | cut -d ":" -f 1`

sed -i "${line}s/'.*'/'${rootaddr}'/" ${BASE}

FUNC(){
    line=`cat ${BASE} | grep -n action | cut -d ":" -f 1`
    echo "FUNC:$1, Time:$2"
    sed -i "${line}s/\(action:\).*/\1 $1,/" ${BASE}
    node ${BASE}
    sleep $2
}

while true
do
    FUNC 1 5
    FUNC 2 5
    FUNC 3 1
    FUNC 4 1
    FUNC 5 1
    FUNC 6 1
    FUNC 7 1
done
