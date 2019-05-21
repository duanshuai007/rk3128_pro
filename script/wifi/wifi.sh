#!/bin/sh

#基于nmcli工具0.9版本的程序

TMP=/root/wifi/tmp

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
    cat /dev/null > ${TMP}
    nmcli dev wifi list > ${TMP}
    #删掉第一行无用信息
    sed -i 1d ${TMP}
    #删除行尾空格
    sed -i 's/[ \t]*$//g' ${TMP}

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
        echo ${line} | grep -w "yes" > /dev/null
        if [ $? -eq 0 ]
        then
            active="connected"
        else
            if [ -n "${curlist}" ]
            then
                echo ${curlist} | grep "${ssid}+${bssid4}" > /dev/null
                if [ $? -eq 0 ]
                then
                    active="remembered"
                fi
            fi
        fi

        line="${ssid}+++${bssid}+++${signal}+++${security}+++${active}"
        #echo "line=${line}"
        #修改当前行的内容
        sed -i "${i}s/.*/${line}/" ${TMP}

        #加入过滤重名wifi的代码
        while [ ${j} -le ${num} ]
        do
            line=`sed -n "${j}"p ${TMP}`
            src1=`echo ${line} | grep "${ssid}"`
            src2=`echo ${line} | grep "${bssid4}"`
            #echo "src1=${src1}, src2=${src2}"
            if [ -n "${src1}" ] && [ -n "${src2}" ]
            then
                #如果ssid和bssid前四位都相同,删除这一行
                #同名的wifi有可能是连接的未连接的连一个在列表上方，另一个连接的列表下方。
                echo ${line} | grep -w "yes" > /dev/null
                if [ $? -eq 0 ]
                then
                    active="connected"
                fi

                #判断信号强弱，保留信号强的ssid
                t_signal=`echo ${line} | sed "s/'.*'//" | awk -F" " '{print $7}'`
                if [ ${signal} -lt ${t_signal} ]
                then
                    #获取原来那一行内容的bssid
                    bssid=`echo ${line} | sed "s/'.*'//" | awk -F" " '{print $1}'`
                fi

                #需要用这行内容替换掉上面的那行内容，然后再删除掉这行的内容
                line="${ssid}+++${bssid}+++${t_signal}+++${security}+++${active}"
                sed -i "${i}s/.*/${line}/" ${TMP}
                #删除掉重复的这行内容
                sed -i "${j}"d ${TMP}
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

    #echo "*****************************"
    #cat ${TMP}
    #echo "******************************"

    #cat ${TMP} | awk -F"+++" '{print $1"+++"$3"+++"$4"+++"$5}' > ${Result}
    cat ${TMP} > ${Result}
}

#连接wifi
#命令格式: connect bssid=xx:xx:xx:xx:xx:xx,password=1234567890
ConnectWifi() {
    #从action文件获取内容
    BSSID=`cat ${Action} | cut -d = -f 2 | cut -d , -f 1`
    PASSWD=`cat ${Action} | cut -d = -f 3` 
    #获取对应的行内容
    #line=`sed -n "${NO}"p ${TMP}`
    line=`cat ${Result} | grep "${BSSID}"`

    echo ${line} | grep "connected" > /dev/null
    if [ $? -eq 0 ]
    then    
        #如果当前选择的ssid是已经连接的，则不做任何操作，退出。
        return 0
    fi
    
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

    #需要一个超时判断，经过N秒后仍未连接上则判定连接失败
    echo ${status} | grep "remembered" > /dev/null
    if [ $? -ne 0 ]
    then    #新wifi不在列表内
        echo "ssid=${SSID}, Not in list"
        if [ -n "${PASSWD}" ]
        then
            #对应有密码的普通wifi
            nmcli dev wifi connect "${BSSID}" password "${PASSWD}" iface wlan0 name "${REALNAME}" > ${ConResult} 2>&1 &
        else
            #对应无密码的wifi
            nmcli dev wifi connect "${BSSID}" iface wlan0 name "${REALNAME}" > ${ConResult} 2>&1 &
        fi
    else    #新wifi在列表内
        echo "ssid=${SSID}, in list"
        if [ -n "${PASSWD}" ]
        then
            #特殊情况，对应在列表内但需要重新输入密码连接的情况，不会遇到这种情况
            nmcli con delete id "${REALNAME}"
            sleep 1
            nmcli dev wifi connect "${BSSID}" password "${PASSWD}" iface wlan0 name "${REALNAME}" > ${ConResult} 2>&1 &
        else
            #普通情况，对于曾经成功连接的wifi只需要up一下就可以了
            nmcli con up id "${REALNAME}" > ${ConResult} 2>&1 &
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
        else
            echo "no dev status" >> ${ConResult}
        fi
        echo "time=${timeout}" >> ${ConResult}
        timeout=`expr ${timeout} - 1`
    done

    reason=`cat ${ConResult} | grep -w "Error:" | awk -F: '{print $3}'`
    
    case ${reason} in
        "The Wi-Fi network could not be found")
            error=53
            ;;
        "Secrets were required, but not provided")
            error=17
            ;;
        "")
            error=0
            ;;
    esac

    echo "error=${error}" >> ${ConResult}
    #判断当前的连接是不是指定的连接，不是则返回错误。
    nmcli con status | sed -n 2p | grep "${REALNAME}" > /dev/null
    if [ $? -ne 0 ] || [ ${error} -ne 0 ]
    then
        pid=`ps -ef | grep nmcli | grep -v grep | awk -F" " '{print $2}'`
        if [ -n "${pid}" ]
        then
            echo "pid=${pid}"
            kill ${pid}
        fi
        nmcli con delete id "${REALNAME}"
        echo "connect to ${SSID} failed" >> ${ConResult}
        return 1
    fi

    echo "connect to ${SSID} success" >> ${ConResult}
    return 0
}

DisconnectWifi() {
    echo "DisconnectWifi"
    nmcli dev disconnect iface wlan0
}

EnableWifi() {
    echo "EnableWifi"
    ifconfig wlan0 up
    sleep 1
    nmcli nm wifi on
}

DisableWifi() {
    echo "DisableWifi"
    nmcli dev disconnect iface wlan0
    nmcli nm wifi off
    ifconfig wlan0 down
}

#忘记密码时调用此函数
ForgetPassWord() {
    BSSID=`cat ${Action} | awk -F= '{print $2}'`
    #SSID=`sed -n "${no}"p ${TMP} | awk -F"+++" '{print $1}'`
    #BSSID=`sed -n "${no}"p ${TMP} | awk -F"+++" '{print $2}'`
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
        chattr +i ${Action}

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
        chattr -i ${Action}
        chmod 666 ${Action}
    fi

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

    sleep 1

done
