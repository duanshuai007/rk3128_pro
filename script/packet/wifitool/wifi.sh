#!/bin/bash

Action=/usr/share/nginx/html/tmp/wifimanager/action
Result=/usr/share/nginx/html/tmp/wifimanager/result
ConResult=/usr/share/nginx/html/tmp/wifimanager/conresult

#Action=/home/frog/wifi/wifi_action
#Result=/home/frog/wifi/wifi_result
#ConResult=/home/frog/wifi/wifi_conresult

#当前命令内容
curcmd=0

curcmdtime=0
lastcmdtime=0

cmd_list="list"
cmd_connect="connect"
cmd_disconnect="disconnect"
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

#对具有相同ssid的wifi追加bssid进行区分。
#目前还不确定电脑是怎么区分同名wifi的。
#暂时设定为ssid+bssid的前四位来组成wifi名。

GetWifiList() {
    tmp=`nmcli -f ssid,bssid,signal,security,active dev wifi list | cat`
    #获取行数
    remember=`nmcli connection show`

    #echo "------------in script------------"
    #echo "${tmp}"
    #echo "------------in script------------"
    #echo "${remember}"
    #echo "=================================="

    /root/wifitool/gg "${tmp}" "${remember}"
}

#连接wifi
#命令格式: connect bssid=xx:xx:xx:xx:xx:xx,password=1234567890
ConnectWifi() {
    #从action文件获取内容
    line=`cat ${Action}`
    echo "${line}" | grep -w "password" > /dev/null
    if [ $? -eq 0 ]
    then
        BSSID=`echo ${line} | cut -d = -f 2 | cut -d , -f 1`
        PASSWD=`echo ${line} | cut -d = -f 3` 
    else
        BSSID=`echo ${line} | awk -F= '{print $2}'`
        PASSWD=""
    fi

    #把当前连接的wifi设置为remembered状态
    line=`awk '/connected/{print}' ${Result}`
    line_no=`awk '/connected/{print NR}' ${Result}`
    sed -i "${line_no}s/connected/remembered/" ${Result}

    #获取对应的行内容
    line=`cat ${Result} | grep "${BSSID}"`

    echo "${line}" | grep "connected" > /dev/null
    if [ $? -eq 0 ]
    then    
        #如果当前选择的ssid是已经连接的，则不做任何操作，退出。
        unbuffer echo "*** Cur BSSID is Connected ***" > ${ConResult}
        return 0
    fi

    #以连续的三个+号作为分隔符
    SSID=`echo "${line}" | awk -F "[+][+][+]" '{print $1}'`

    if [ -z "${SSID}" ] || [ -z "${BSSID}" ]
    then
        unbuffer echo "Get SSID/BSSID Failed" > ${ConResult}
        return
    fi

    #把即将要连接的设置为connecting
    line_no=`awk "/${BSSID}/{print NR}" ${Result}`
    #echo "LINE_NO:${line_no}"
    echo "${line}" | grep -w "remembered" > /dev/null
    if [ $? -ne 0 ]
    then
        sed -i "${line_no}s/\(.*\)no\(.*\)/\1connecting\2/" ${Result}
    else
        sed -i "${line_no}s/\(.*\)remembered\(.*\)/\1connecting\2/" ${Result}
    fi

    bssid4=`echo ${BSSID%:*}`
    bssid4=`echo ${bssid4%:*}`
    REALNAME="${SSID}+${bssid4}"
    cat /dev/null > ${ConResult}
    echo "ConnectWifi, ssid=${SSID}, bssid=${BSSID}, REALNAME=${REALNAME}, password=${PASSWD}"
    #先查找看想要连接的wifi是否在网络列表内，如果在直接启用就可以，如果不在才执行添加操作
    status=`echo ${line} | awk -F "[+][+][+]" '{print $5}'`
    if [ -z "${status}" ]
    then
        unbuffer echo "Get SSID[${SSID}] Link Status Failed" > ${ConResult}
        return
    fi

    #需要一个超时判断，经过N秒后仍未连接上则判定连接失败
    echo ${status} | grep "remembered" > /dev/null
    if [ $? -ne 0 ]
    then    #新wifi不在列表内
        echo "ssid=${SSID}, Not in list"
        inList=0
        if [ -n "${PASSWD}" ]
        then
            #对应有密码的普通wifi
            unbuffer nmcli device wifi connect "${BSSID}" password "${PASSWD}" name "${REALNAME}" > ${ConResult} 2>&1 &
        else
            #对应无密码的wifi
            unbuffer nmcli device wifi connect "${BSSID}" name "${REALNAME}" > ${ConResult} 2>&1 &
        fi
    else    #新wifi在列表内
        echo "ssid=${SSID}, in list"
        inList=1
        if [ -n "${PASSWD}" ]
        then
            #特殊情况，对应在列表内但需要重新输入密码连接的情况，不会遇到这种情况
            unbuffer nmcli connection delete id "${REALNAME}" > ${ConResult} 2>&1 &
            sleep 1
            unbuffer nmcli device wifi connect "${BSSID}" password "${PASSWD}" name "${REALNAME}" > ${ConResult} 2>&1 &
        else
            #普通情况，对于曾经成功连接的wifi只需要up一下就可以了
            unbuffer nmcli connection up "${REALNAME}" > ${ConResult} 2>&1 &
        fi
    fi 

    timeout=15
    while [ ${timeout} -ne 0 ]
    do
        sleep 1
        status=`nmcli dev status | sed -n 2p | awk -F" " '{print $3}'`
        if [ -n "${status}" ]
        then
            case ${status} in
                "connected")
                    unbuffer echo "connected" >> ${ConResult}
                    break
                    ;;
                "connecting")
                    unbuffer echo "connecting" >> ${ConResult}
                    ;;
                "disconnected")
                    unbuffer echo "disconnected" >> ${ConResult}
                    break
                    ;;
                *)
                    ;;
            esac
        else
            unbuffer echo "no dev status" >> ${ConResult}
        fi
        unbuffer echo "time=${timeout}" >> ${ConResult}
        timeout=`expr ${timeout} - 1`
    done

    reason=`cat ${ConResult} | grep -w "Error:"`
    
    error=0
    echo "${reason}" | grep "802.1X supplicant failed" > /dev/null
    if [ $? -eq 0 ]
    then
        error=10 
    else
        echo "${reason}" | grep "Secrets were required, but not provided" > /dev/null
        if [ $? -eq 0 ]
        then
            error=7
        fi
    fi

    unbuffer echo "error=${error}" >> ${ConResult}
    #判断当前的连接是不是指定的连接，不是则返回错误。
    nmcli -f NAME,UUID,STATE connection show | grep -w "activated" | grep -w "${REALNAME} " > /dev/null
    if [ $? -ne 0 ] || [ ${error} -ne 0 ]
    then
        pid=`ps -ef | grep nmcli | grep -v grep | awk -F" " '{print $2}'`
        if [ -n "${pid}" ]
        then
            echo "pid=${pid}"
            kill ${pid}
        fi
        if [ ${error} -ne 10 ]
        then
            unbuffer nmcli connection delete id "${REALNAME}" >> ${ConResult} 2>&1
        else
            #出现10错误，重新连接一下试试
            if [ ${inList} -eq 1 ]
            then
                unbuffer nmcli connection up "${REALNAME}" >> ${ConResult} 2>&1
                sleep 2
                cat ${ConResult} | grep "Connection successfully activated" > /dev/null
                if [ $? -eq 0 ]
                then
                    echo "connect to ${SSID} success" >> ${ConResult}
                    return 0
                else
                    #如果重试连接依然失败，必须删除掉没能成功连接的热点，否则会显示错误。
                    unbuffer nmcli connection delete id "${REALNAME}" >> ${ConResult} 2>&1
                fi
            else
                unbuffer nmcli connection delete id "${REALNAME}" >> ${ConResult} 2>&1
            fi
        fi
        sed -i "${line_no}s/\(.*\)connecting\(.*\)/\1connect failed\2/" ${Result}
        sleep 1
        echo "connect to ${SSID} failed" >> ${ConResult}
        return 1
    fi

    echo "connect to ${SSID} success" >> ${ConResult}
    return 0
}

EnConnectWifi() {
    nmcli device connect wlp1s0
}

DisConnectWifi() {
    echo "DisconnectWifi"
    nmcli device disconnect wlp1s0 
}

EnableWifi() {
    echo "EnableWifi"
    ifconfig wlan0 up
    sleep 1
    nmcli device connect wlp1s0
}

DisableWifi() {
    echo "DisableWifi"
    nmcli device disconnect wlp1s0
    sleep 1
    ifconfig wlan0 down
}

#忘记密码时调用此函数
ForgetPassWord() {
    BSSID=`cat ${Action} | awk -F= '{print $2}'`
    bssid4=`echo ${BSSID%:*}`
    bssid4=`echo ${bssid4%:*}`
    SSID=`cat ${Result} | grep "${BSSID}" | awk -F"+++" '{print $1}'`
    REALNAME="${SSID}+${bssid4}"
    nmcli con delete id "${REALNAME}"
}

ResetWifi() {
    echo "ResetWifi"
    nmcli nm wifi off
    sleep 3
    nmcli nm wifi on
}

touch ${Action}
touch ${Result}
touch ${ConResult}

chmod 666 ${Action}
chmod 666 ${Result}
chmod 666 ${ConResult}

cat /dev/null > ${Action}
cat /dev/null > ${Result}
cat /dev/null > ${ConResult}

list_switch=0
#stoptime=0
rescantime=0
while true
do
    GetCMDFromAction
    if [ -n "${curcmd}" ]
    then
        #夺取文件写权限
        chmod 444 ${Action}
        #chattr +i ${Action}

        echo "cmd=${curcmd}"
        case ${curcmd} in
            "${cmd_list} "*)
                #GetWifiList
                list_switch=`echo ${curcmd#* }`
                ;;
            "${cmd_connect} "*)
                ConnectWifi
                ;;
            "${cmd_disconnect}")
                DisconnectWifi
                ;;
            "${cmd_forget} "*)
                ForgetPassWord
                ;;
            "${cmd_reset}")
                ResetWifi
                ;;
            *)
                ;;
        esac
        #归还文件写权限
        #chattr -i ${Action}
        chmod 666 ${Action}
    fi

    case ${list_switch} in
        "start")
            GetWifiList
            if [ ${rescantime} -eq 0 ]
            then
                nmcli device wifi rescan > /dev/null 2>&1
            fi
            rescantime=`expr ${rescantime} + 1`
            if [ ${rescantime} -ge 10 ]
            then
                rescantime=0
            fi
            ;;
        "stop")
            cat /dev/null > ${Result}
            #stoptime=`expr ${stoptime} + 1`
            #if [ ${stoptime} -gt 10 ]
            #then
            #    echo "stop 10 sec, wifi.sh exit!"
            #    exit 
            #fi
            ;;
    esac

    sleep 1

done
