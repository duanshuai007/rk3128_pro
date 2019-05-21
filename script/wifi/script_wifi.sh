#!/bin/sh

#没有相同ssid不同bssid的过滤功能。

#Action=/home/frog/wifi/action
#Result=/home/frog/wifi/result
Action=/usr/share/nginx/html/wifi_action
Result=/usr/share/nginx/html/wifi_result
TMP=/home/frog/wifi/tmp

#当前命令内容
curcmd=0

curcmdtime=0
lastcmdtime=0

cmd_list="list"
cmd_connect="connect"
cmd_disconnect="disconnect"
cmd_check="check"
cmd_reset="reset"
cmd_forget="forget"

GetFileModifyTime() {
    lastcmdtime=${curcmdtime}
    curcmdtime=`stat ${Action} | grep Modify | awk -F: '{print $2$3$4}' | awk -F' ' '{print $1$2}' | awk -F- '{print $1$2$3}' | awk -F. '{print $1$2}' | cut -c 5-16`
}

#从action中获取命令
GetCMDFromAction() {
    curcmd=""
    #获取文件修改时间戳
    GetFileModifyTime

    if [ ${curcmdtime} -ne ${lastcmdtime} ]
    then
        curcmd=`sed -n 1p ${Action}`
    fi  
}

GetWifiList() {
    #nmcli dev wifi | grep -v yes | awk -F" " '{print $1}' > ${Result}
    #echo "GetWifiList"
    unbuffer nmcli dev wifi list | grep -v yes | grep -o "'.*'" | sed "s/'$//" | sed "s/'//" > ${TMP}
    
    #获取当前连接的ssid
    connectname=`nmcli con status | sed -n 2p`
    
    i=6
    while [ ${i} -ne 0 ]
    do
        connectname=`echo ${connectname% *}`
        i=`expr ${i} - 1`
    done
    #echo "connectname=${connectname}"
    
    #判断曾经连接过的ssid和已经连接的ssid
    num=`cat ${TMP} | wc -l`
    while [ ${num} -ne 0 ]
    do
        name=`sed -n "${num}"p ${TMP}`
        
        #echo "name=${name}"
        #如果是当前连接的ssid
        if [ -n "${connectname}" ]
        then
            echo "${name} " | grep "${connectname} " > /dev/null     #grep后的字符串末尾加一个空格用于完全匹配
            if [ $? -eq 0 ]
            then
                #echo "this is cur wifi"
                sed -i "${num}s/${name}/${name} connected/" ${TMP}
                num=`expr ${num} - 1`
                continue
            fi
        fi
        #如果是曾经连接过的ssid
        nmcli con list | grep "${name} " > /dev/null
        if [ $? -eq 0 ]
        then
            #echo "in con list"
            sed -i "${num}s/${name}/${name} remember/" ${TMP}
        fi
        num=`expr ${num} - 1`
    done

    cat ${TMP} > ${Result}
    cat /dev/null > ${TMP}
}

#从连接列表中获取wifi名
#针对wifi名字含有特殊字符的情况需要特殊处理才能正确的找到名字
#为了防止wifi名字内的特殊字符的影响
#GetConListAllName() {
#    #nmcli con list | sed "s/802-11-wireless.*//g" > ${TMP}
#    nmcli con list > ${TMP}
#    #| tr -cd " " | wc -c 
#    num=`cat ${TMP} | wc -l`
#    i=2
#    while [ ${i} -le ${num} ]
#    do
#        line=`sed -n "${i}"p ${TMP}`
#        echo ${line} | grep never > /dev/null
#        if [ $? -eq 0 ]
#        then
#            loop=3
#        else
#            loop=9
#        fi
#
#        #该段代码用于从字符串右侧开始删除以空格作为分隔符的内容
#        j=0
#        while [ ${j} -le ${loop} ]
#        do
#            line=`echo ${line% *}`  #从字符串中截取，删除右边第一个空格开始的右边字符串，保留左边字符串
#            #echo "j=${j},line=${line}"
#            j=`expr ${j} + 1`
#        done
#
#        echo ${line}
#        echo "${line} " | grep "$1 " > /dev/null
#        if [ $? -eq 0 ]
#        then
#            return 0
#        fi
#        i=`expr ${i} + 1`
#    done
#
#    return 1
#}

ConnectWifi() {
    SSID=`cat ${Action} | cut -d = -f 2 | cut -d , -f 1`
    PASSWD=`cat ${Action} | cut -d = -f 3` 
    echo "ConnectWifi, ssid=${SSID},passwd=${PASSWD}"
    #先查找看想要连接的wifi是否在网络列表内，如果在直接启用就可以，如果不在才执行添加操作
    line=`cat ${Result} | grep "${SSID} "`
    line=`echo ${line#* }`  #截取最后一个空格右边的内容
    echo ${line} | grep "connected" > /dev/null #执行这条指令需要在执行ConnectWifi之前先执行一次GetWifiList才能起作用
    if [ $? -eq 0 ]
    then    
        #如果当前选择的ssid是已经连接的，则不做任何操作，退出。
        return 0
    fi

    #需要一个超时判断，经过N秒后仍未连接上则判定连接失败
    echo ${line} | grep "remember" > /dev/null
    if [ $? -ne 0 ]
    then    #新wifi不在列表内
        echo "ssid=${SSID}, Not in list"
        if [ -n "${PASSWD}" ]
        then
            #对应有密码的普通wifi
            sudo nmcli dev wifi connect "${SSID}" password "${PASSWD}" iface wlan0 > ${TMP} 2>&1 &
        else
            #对应无密码的wifi
            sudo nmcli dev wifi connect "${SSID}" iface wlan0 > ${TMP} 2>&1 &
        fi
    else    #新wifi在列表内
        echo "ssid=${SSID}, in list"
        if [ -n "${PASSWD}" ]
        then
            #特殊情况，对应在列表内但需要重新输入密码连接的情况，不会遇到这种情况
            sudo nmcli con delete id "${SSID}"
            sleep 1
            sudo nmcli dev wifi connect "${SSID}" password "${PASSWD}" iface wlan0 > ${TMP} 2>&1 &
        else
            #普通情况，对于曾经成功连接的wifi只需要up一下就可以了
            sudo nmcli con up id "${SSID}" > ${TMP} &
        fi
    fi 

    timeout=15
    while [ ${timeout} -ne 0 ]
    do
        sleep 1
        status=`nmcli dev status | sed -n 2p | awk -F" " '{print $3}'`
        case ${status} in
            "connected")
                echo "connected"
                break
                ;;
            "connecting")
                echo "connecting"
                ;;
            "disconnected")
                echo "disconnected"
                break
                ;;
            *)
                ;;
        esac

        timeout=`expr ${timeout} - 1`
    done

    #判断当前的连接是不是指定的连接，不是则返回错误。
    nmcli con status | sed -n 2p | grep "${SSID} " > /dev/null
    if [ $? -ne 0 ]
    then
        pid=`ps -ef | grep nmcli | grep -v grep | awk -F" " '{print $2}'`
        if [ -n "${pid}" ]
        then
            echo "pid=${pid}"
            sudo kill ${pid}
        fi
        sudo nmcli con delete id ${SSID}
        echo "connect to ${SSID} failed"
        return 1
    fi

    echo "connect to ${SSID} success"
    return 0
}

DisconnectWifi() {
    echo "DisconnectWifi"
    sudo nmcli dev disconnect iface wlan0
}

EnableWifi() {
    echo "EnableWifi"
    ifconfig wlan0 up
    sleep 1
    nmcli nm wifi on
}

DisableWifi() {
    echo "DisableWifi"
    sudo nmcli dev disconnect iface wlan0
    nmcli nm wifi off
    ifconfig wlan0 down
}

#忘记密码时调用此函数
ForgetPassWord() {
    echo "Forget $1"
    sudo nmcli con delete id "$1"
}

#查看当前所连接的wifi
#CheckWifi() { 
#    #line=`nmcli con status | sed -n 2p`
#    #echo ${line}
#    ssid=`cat ${Result} | grep connected`
#    ssid=`echo ${ssid% *}`  #截取右数第一个空格左边的内容
#}

ResetWifi() {
    echo "ResetWifi"
    nmcli nm wifi off
    sleep 3
    nmcli nm wifi on
}

touch ${Result}
touch ${Action}

cat /dev/null > ${Action}
cat /dev/null > ${Result}

list_switch=0
while true
do
    GetCMDFromAction
    if [ -n "${curcmd}" ]
    then
        #夺取文件写权限
        chmod 444 ${Action}
        chattr +i ${Action}
    
        echo "cmd=${curcmd}"
        case ${curcmd} in
            "${cmd_list} "*)
                #GetWifiList
                list_switch=`echo ${curcmd#* }`
                ;;
            "${cmd_connect} "*)
                GetWifiList
                ConnectWifi
                ;;
            "${cmd_disconnect}")
                DisconnectWifi
                ;;
#            "${cmd_check}")
#                CheckWifi
#                ;;
            "${cmd_forget} "*)
                ssid=`echo ${curcmd#*=}`
                ForgetPassWord ${ssid}
                ;;
            "${cmd_reset}")
                ResetWifi
                ;;
            *)
                ;;
        esac
        #归还文件写权限
        chattr -i ${Action}
        chmod 666 ${Action}
    fi
    
    case ${list_switch} in
        "start")
            GetWifiList
            ;;
        "stop")
            ;;
    esac

    sleep 1

done
