#!/bin/sh

#新增了同名Wi-Fi过滤功能。
#在连接wifi时点击列表时会向action文件写入input。为了防止getwifilist再点击列表时执行
#但是在input和connect相差时间太短的话就会导致input可能会被忽略而直接执行connect，
#在这过程中getwifilist就不会被停止，list可能发生改变导致连接的wifi与点击的wifi不一致

#Action=/home/frog/wifi/action
#Result=/home/frog/wifi/result
TMP=/home/frog/wifi/new_tmp
DTMP=/home/frog/wifi/.new_tmp
#ConResult=/home/frog/wifi/con_result

Action=/usr/share/nginx/html/wifi_action
Result=/usr/share/nginx/html/wifi_result
ConResult=/usr/share/nginx/html/wifi_conresult

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

#对具有相同ssid的wifi追加bssid进行区分。
#目前还不确定电脑是怎么区分同名wifi的。
#暂时设定为ssid+bssid的前四位来组成wifi名。
#上一次action内容更新的时间
lastupdatetime=0

GetWifiList() {
    cat ${Action} | grep "input" > /dev/null
    if [ $? -eq 0 ]
    then
        return
    fi
    #保存当前tmp文件的内容
    cp ${TMP} ${DTMP} 
    
    cat /dev/null > ${TMP}
    nmcli dev wifi list > ${TMP}
    #删掉第一行无用信息
    sed -i 1d ${TMP}
    #删除行尾空格
    sed -i 's/[ \t]*$//g' ${TMP}

    #获取当前连接的ssid
    connectname=`nmcli con status | sed -n 2p`
    if [ -n "${connectname}" ]
    then
        i=6
        while [ ${i} -ne 0 ]
        do
            connectname=`echo ${connectname% *}`
            i=`expr ${i} - 1`
        done
    fi
    echo "connectname=${connectname}"
    #获取当前列表内容
    curlist=`nmcli con list`

    #准备分析内容
    num=`cat ${TMP} | wc -l`
    i=1
    j=2

    while [ ${i} -le ${num} ]
    do
        line=`sed -n "${i}"p ${TMP}`
        ssid=`echo ${line} | grep -o "'.*'"`
        line=`echo ${line} | sed "s/'.*'//"`
        bssid=`echo ${line} | awk -F" " '{print $1}'`
        signal=`echo ${line} | awk -F" " '{print $7}'`
        count=8
        while [ ${count} -ne 0 ]
        do
            line=`echo ${line#* }`
            count=`expr ${count} - 1`
        done
        #echo "line=${line}"
        #active=`echo ${line##* }`
        security=`echo ${line% *}` 
        #echo "line=${line},ssid=${ssid},bssid=${bssid}"
        #echo "active=${active}, security=${security}"

        #获取bssid前四位
        bssid4=`echo ${bssid%:*}`
        bssid4=`echo ${bssid4%:*}`

        if [ -z "${ssid}" ] || [ -z "${bssid4}" ]
        then
            i=`expr ${i} + 1`
            continue
        fi
        #判断当前的ssid的状态
        active="no"
        if [ -n "${curlist}" ]
        then
            echo ${curlist} | grep "${ssid}+${bssid4}" > /dev/null
            if [ $? -eq 0 ]
            then
                active="remembered"
            fi
        fi

        if [ -n "${connectname}" ]
        then
            echo ${connectname} | grep "${ssid}+${bssid4}" > /dev/null
            if [ $? -eq 0 ]
            then
                active="connected"
            fi
        fi
        line="${ssid}+++${bssid}+++${signal}+++${security}+++${active}"
        #echo "line=${line}"
        #修改当前行的内容
        sed -i "${i}s/.*/${line}/" ${TMP}
        
        #加入过滤重名wifi的代码
        if [ -z "${ssid}" ] || [ -z "${bssid4}" ]
        then
            i=`expr ${i} + 1`
            continue
        fi
        while [ ${j} -le ${num} ]
        do
            line=`sed -n "${j}"p ${TMP}`
            src1=`echo ${line} | grep "${ssid}"`
            src2=`echo ${line} | grep "${bssid4}"`
            #echo "src1=${src1}, src2=${src2}"
            if [ -n "${src1}" ] && [ -n "${src2}" ]
            then
                #bssid相同,删除这一行
                #echo "find same bssid:${bssid}, j=$j"
                #判断信号强弱，保留信号强的ssid
                #echo "reade delete:${line}"
                t_signal=`echo ${line} | sed "s/'.*'//" | awk -F" " '{print $7}'`
                #echo "find signal:${t_signal}"
                if [ ${signal} -lt ${t_signal} ]
                then
                    #需要用这行内容替换掉上面的那行内容，然后再删除掉这行的内容
                    bssid=`echo ${line} | sed "s/'.*'//" | awk -F" " '{print $1}'`
                    line="${ssid}+++${bssid}+++${t_signal}+++${security}+++${active}"
                    sed -i "${i}s/.*/${line}/" ${TMP}
                fi
                sed -i "${j}"d ${TMP}
                #sleep 0.01
                #因为删除了一行，需要j当前的值自动跳到了下一行内容上
                #所以不需要再执行j++的操作，为了补偿，这里执行j--
                j=`expr ${j} - 1`
                #删掉一行之后总行数有改变
                num=`expr ${num} - 1`
            fi

            j=`expr ${j} + 1`
        done

        i=`expr ${i} + 1`
        j=`expr ${i} + 1`
    done

    echo "*****************************"
    cat ${TMP}
    echo "******************************"
              
    cat ${Action} | grep "input" > /dev/null
    if [ $? -eq 0 ]
    then
        #如果在getwifilist的过程中发现了input的命令，
        #则不会更新result文件，并且将tmp文件还原
        cp ${DTMP} ${TMP}
    else
        cat ${TMP} | awk -F"+++" '{print $1"+++"$3"+++"$4"+++"$5}' > ${Result}
    fi
}

ConnectWifi() {
    NO=`cat ${Action} | cut -d = -f 2 | cut -d , -f 1`
    PASSWD=`cat ${Action} | cut -d = -f 3` 

    line=`sed -n "${NO}"p ${TMP}`
    BSSID=`echo ${line} | awk -F"+++" '{print $2}'`
    SSID=`echo ${line} | awk -F"+++" '{print $1}'`
    if [ -z "${SSID}" ] || [ -z "${BSSID}" ]
    then
        echo "Get SSID/BSSID Failed" > ${ConResult}
        return
    fi
    bssid4=`echo ${BSSID%:*}`
    bssid4=`echo ${bssid4%:*}`
    REALNAME="${SSID}+${bssid4}"
    cat /dev/null > ${ConResult}
    echo "ConnectWifi, ssid=${SSID}, bssid=${BSSID}, REALNAME=${REALNAME}, password=${PASSWD}"
    #先查找看想要连接的wifi是否在网络列表内，如果在直接启用就可以，如果不在才执行添加操作
    status=`echo ${line} | awk -F"+++" '{print $5}'`
    if [ -z "${status}" ]
    then
        echo "Get SSID[${SSID}] Link Status Failed" > ${ConResult}
        return
    fi
    echo ${status} | grep "connected" > /dev/null #执行这条指令需要在执行ConnectWifi之前先执行一次GetWifiList才能起作用
    if [ $? -eq 0 ]
    then    
        #如果当前选择的ssid是已经连接的，则不做任何操作，退出。
        return 0
    fi

    #需要一个超时判断，经过N秒后仍未连接上则判定连接失败
    echo ${status} | grep "remembered" > /dev/null
    if [ $? -ne 0 ]
    then    #新wifi不在列表内
        echo "ssid=${SSID}, Not in list"
        if [ -n "${PASSWD}" ]
        then
            #对应有密码的普通wifi
            sudo nmcli dev wifi connect "${BSSID}" password "${PASSWD}" iface wlan0 name "${REALNAME}" > ${ConResult} 2>&1 &
        else
            #对应无密码的wifi
            sudo nmcli dev wifi connect "${BSSID}" iface wlan0 name "${REALNAME}" > ${ConResult} 2>&1 &
        fi
    else    #新wifi在列表内
        echo "ssid=${SSID}, in list"
        if [ -n "${PASSWD}" ]
        then
            #特殊情况，对应在列表内但需要重新输入密码连接的情况，不会遇到这种情况
            sudo nmcli con delete id "${REALNAME}"
            sleep 1
            sudo nmcli dev wifi connect "${BSSID}" password "${PASSWD}" iface wlan0 name "${REALNAME}" > ${ConResult} 2>&1 &
        else
            #普通情况，对于曾经成功连接的wifi只需要up一下就可以了
            sudo nmcli con up id "${REALNAME}" > ${ConResult} &
        fi
    fi 

    timeout=15
    while [ ${timeout} -ne 0 ]
    do
        sleep 1
        status=`nmcli dev status | sed -n 2p | awk -F" " '{print $3}'`
        case ${status} in
            "connected")
                echo "connected" >> ${ConResult}
                break
                ;;
            "connecting")
                echo "connecting" >> ${ConResult}
                ;;
            "disconnected")
                echo "disconnected" >> ${ConResult}
                break
                ;;
            *)
                ;;
        esac

        timeout=`expr ${timeout} - 1`
    done

    #判断当前的连接是不是指定的连接，不是则返回错误。
    nmcli con status | sed -n 2p | grep "${REALNAME}" > /dev/null
    if [ $? -ne 0 ]
    then
        pid=`ps -ef | grep nmcli | grep -v grep | awk -F" " '{print $2}'`
        if [ -n "${pid}" ]
        then
            echo "pid=${pid}"
            sudo kill ${pid}
        fi
        sudo nmcli con delete id "${REALNAME}"
        echo "connect to ${SSID} failed" >> ${ConResult}
        return 1
    fi

    echo "connect to ${SSID} success" >> ${ConResult}
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
    no=`cat ${Action} | awk -F= '{print $2}'`
    SSID=`sed -n "${no}"p ${TMP} | awk -F"+++" '{print $1}'`
    BSSID=`sed -n "${no}"p ${TMP} | awk -F"+++" '{print $2}'`
    bssid4=`echo ${BSSID%:*}`
    bssid4=`echo ${bssid4%:*}`
    REALNAME=`${SSID}+${bssid4}`
    sudo nmcli con delete id "${REALNAME}"
}

ResetWifi() {
    echo "ResetWifi"
    nmcli nm wifi off
    sleep 3
    nmcli nm wifi on
}

touch ${TMP}
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
while true
do
    GetCMDFromAction
    if [ -n "${curcmd}" ]
    then
        #夺取文件写权限
        chmod 444 ${Action}
        sudo chattr +i ${Action}
    
        echo "cmd=${curcmd}"
        case ${curcmd} in
            "${cmd_list} "*)
                #GetWifiList
                list_switch=`echo ${curcmd#* }`
                ;;
            "${cmd_connect} "*)
                #GetWifiList
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
        sudo chattr -i ${Action}
        chmod 666 ${Action}
    fi
    
    #如果action文件内容是input则不刷新列表
    cat ${Action} | grep "input" > /dev/null
    if [ $? -ne 0 ]
    then
        case ${list_switch} in
            "start")
                GetWifiList
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
    fi

    sleep 1

done
