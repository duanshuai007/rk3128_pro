#!/bin/sh

ToolsDir=/root/upgradetool
ToolsName=${ToolsDir}/upgrade_tool
ToolsLoader=${ToolsDir}/m3/RK3128MiniLoaderAll_V2.25.bin

Config=${ToolsDir}/config

CheckENV() {
    #判断配置文件是否存在
    if [ ! -e ${Config} ]
    then
        echo "Config is not exist"
        exit
    else
        #判断所需要的action和display文件是否存在
        action=`sed -n 1p ${Config} | awk -F= '{print $2}'`
        if [ ! -e ${action} ]
        then
            echo "action file is not exist"
            exit
        fi

        display=`sed -n 2p ${Config} | awk -F= '{print $2}'`
        if [ ! -e ${display} ]
        then
            echo "display file is not exist"
            touch ${display}
        fi

        device=`sed -n 3p ${Config} | awk -F= '{print $2}'`
        if [ ! -e ${device} ]
        then
            echo "device file is not exist"
            touch ${device}
        fi
    fi
    #判断所需要的工具是否存在
    if [ ! -d ${ToolsDir} ]
    then
        echo "${ToolsDir} is not eixst"
        exit 0
    fi

    if [ ! -e ${ToolsName} ]
    then
        echo "tools is not exist"
        exit 0
    fi  

    if [ ! -e ${ToolsLoader} ]
    then
        echo "ToolsLoader is not exist"
        exit 0
    fi

    #判断脚本是否已经开始运行
    #ps -ef | grep getstart | grep -v grep > /dev/null
    #if [ $? -eq 0 ] #$?等于0说明进程正在运行
    #then
    #    echo "Script[$0] Alread Runing, Don't Continue."
    #    exit 0
    #else
    #    echo "Script[$0] Start Runing ..."
    #fi 

    sname=`echo $0 | awk -F/ '{print $NF}'`
    no=`ps -ef | grep ${sname} | grep -v grep | wc -l`
    if [ ${no} -ge 3 ]
    then
        echo "more than 1 process running"
        exit
    fi
}

echo "------------ Script[watchme.sh] Start ------------"
CheckENV

while true
do
    PRun=`ps -ef | grep getstart | grep -v grep`
    lsusb | grep 2207 > /dev/null
    if [ $? -eq 0 ]
    then
    #如果发现2207 USB 描述符
        if [ -z "${PRun}" ]
        then
            sh ${ToolsDir}/getstart.sh &
        fi
    fi

    sleep 1 
done
