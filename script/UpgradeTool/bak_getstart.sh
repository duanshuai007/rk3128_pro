#!/bin/sh

#工具主目录
ToolsDir=/home/frog/Linux_Upgrade_Tool_v1.21
StatusDir=${ToolsDir}/status
#工具
ToolsName=${ToolsDir}/upgrade_tool
ToolsLoader=${ToolsDir}/m3/RK3128MiniLoaderAll_V2.25.bin
#状态文件
#用来对指令返回进行显示
DStatus=${StatusDir}/disstatus
#连接的设备信息
CDevice=${StatusDir}/cdevice
#控制指令文件，保存控制软件下发的控制
Action=${StatusDir}/action
#记录action文件修改时间戳和当前指令的所在的行数
ActionFile=${StatusDir}/actionstatus
#记录文档
LogFile=${ToolsDir}/operation_log.txt

fsize=20000

cmd_update="update"
cmd_erase="erase"
cmd_switch="switch"
cmd_reset="reset"
cmd_exit="exit"

Loader="Loader"
Maskrom="Maskrom"
Msc="Msc"

LoaderMode=1
MaskromMode=2
MscMode=3

count=0

#文件的最近修改时间
newtime=0
#文件的上一次修改时间
oldtime=1
#当前命令所在行数
curline=0
#命令一共有多少行
tolline=1
#当前命令内容
curcmd=0
#上一次查询的命令总行数，用于判断文件是否被覆盖写入
lastline=1

GetFileModifyTime() {
    oldtime=${newtime}
    newtime=`stat ${Action} | grep Modify | awk -F: '{print $2$3$4}' | awk -F' ' '{print $1$2}' | awk -F- '{print $1$2$3}' | awk -F. '{print $1$2}' | cut -c 5-16`
}

#从action中获取命令
GetCMDFromAction() {
    #获取文件修改时间戳
    GetFileModifyTime
    #echo "new=${newtime}, old=${oldtime}"
    while [ ${newtime} -ne ${oldtime} ]
    do
        lastline=${tolline}
        tolline=`cat ${Action} | wc -l`
        #如果文件被覆盖写入则会出现总行数小于等于上一次的行数
        #小于等于是仅有一条命令的情况
        if [ ${tolline} -le ${lastline} ]
        then
            curline=0
        fi
        GetFileModifyTime
    done

    curcmd=""
    #获取当前位置的命令
    #echo "cur=${curline},tol=${tolline}"
    if [ ${curline} -lt ${tolline} ]
    then
        line=`expr ${curline} + 1`
        curcmd=`sed -n ${line}p ${Action}`
        curline=`expr ${curline} + 1`
    fi

    echo "time=${newtime},line=${curline}" > ${ActionFile}
}

CheckEnv() {
    #判断脚本是否已经开始运行
    num=`ps -ef | grep $0 | grep -v grep | wc -l`
    ps -ef | grep $0 | grep -v grep | wc -l
    echo "num=${num}"
    if [ ${num} -gt 1 ]
    then
        echo "Script[$0] Alread runing."
        exit 0
    else
        echo "Script[$0] runing..."
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

    touch ${DStatus}
    touch ${CDevice}
    touch ${LogFile}
    touch ${ActionFile}

    #如果action不存在则循环判断，周期1s   
    i=0
    while [ ! -e ${Action} ]
    do
        sleep 1
        if [ ${i} -eq 0 ]
        then
            echo "can' found action file"
        fi
        i=`expr ${i} + 1`
        if [ ${i} -gt 10 ]
        then
            i=0
        fi
    done

    newtime=`cat ${ActionFile} | awk -F, '{print $1}' | awk -F= '{print $2}'`
    oldtime=${newtime}
    curline=`cat ${ActionFile} | awk -F, '{print $2}' | awk -F= '{print $2}'`
    tolline=${curline}
}

GetUSBDevice() {
    #获取连接的rockusb设备信息输出到指定文件中
    echo "Q" | sudo ${ToolsName} | grep Vid > ${CDevice}
    #从第一行往下开始获取设备的信息[Vid][Pid][Status]
    src=`sed -n 1p ${CDevice}`
    #如果字符串是空的，则返回
    if [ -z "${src}" ]
    then
        mode=0
        return
    fi

    #获取usb设备描述
    VID=`echo ${src} | cut -d '=' -f 3 | cut -d ',' -f 1`
    PID=`echo ${src} | cut -d '=' -f 4 | cut -d ',' -f 1`
    LID=`echo ${src} | cut -d '=' -f 5 | cut -d ' ' -f 1`

    case ${src} in
        *"${Loader}"*)
            mode=${LoaderMode}
            ;;
        *"${Maskrom}"*)
            mode=${MaskromMode}
            ;;
        *"${Msc}"*)
            mode=${MscMode}
            ;;
        *)
            mode=0
            ;;
    esac

    if [ ${mode} -ne 0 ]
    then
        echo "mode=${mode},vid=${VID},pid=${PID},LocationID=${LID}" > ${CDevice}
    else
        echo "can't found rockusb device." > ${CDevice}
    fi
}

ResetDevice() {
    sudo ${ToolsName} rd > ${DStatus}
}

#参数为固件的全路径名
UpdateFirmware() {
    echo "UpdateFirmware $1"
    
    if [ ${mode} -eq ${MscMode} ]
    then
        SwitchMode
    fi

    if [ ! -e $1 ]
    then
        echo "firmware[$1] is not exist" > ${DStatus}
        return
    fi

    unbuffer sudo ${ToolsName} uf $1 > ${DStatus}
}

#只有在msc模式下能够进行模式切换,Android启动后就是Msc模式
SwitchMode() {
    echo "SwitchMode"
    if [ ${mode} -ne ${MscMode} ]
    then
        echo "Device not work in Msc,Can't switch mode" > ${DStatus}
        return
    fi

    sudo ${ToolsName} sd > ${DStatus}

    sleep 1
    
    GetUSBDevice
    while [ ${mode} -eq 0 ]
    do
        GetUSBDevice
        sleep 0.2
    done
}

EraseAllChip() {
    echo "EraseAllChip"
    #如果当前实在Msc模式下，需要先切换
    if [ ${mode} -eq ${MscMode} ]
    then
        SwitchMode
    fi

    unbuffer sudo ${ToolsName} ef ${ToolsLoader} > ${DStatus}

    sleep 1

    GetUSBDevice
    while [ ${mode} -eq 0 ]
    do
        GetUSBDevice
        sleep 0.2
    done
}

RunGetDisplay() {
    ps -ef | grep getdisplay | grep -v grep > /dev/null
    if [ $? -ne 0 ] #不等于0，说明进程未运行
    then
        sh ${ToolsDir}/getdisplay.sh &
    fi
}

MyLog() {
    if [ $# -lt 1 ]
    then
        #echo "parameter not right in MyLog function"
        return
    fi

    if [ -e ${LogFile} ]
    then
        touch ${LogFile}
    fi

    local curtime
    curtime=`date +"%Y%m%d%H%M%S"`

    local cursize
    cursize=`cat ${LogFile} | wc -c`

    if [ ${fsize} -lt ${cursize} ]
    then
        mv ${LogFile} ${curtime}".out"
        touch ${LogFile}
    fi

    echo "${curtime} $*" >> ${LogFile}
}

#主程序开始
CheckEnv
#开始循环监视action文件
while [ 1 ];
do
    GetUSBDevice
    #如果在空闲状态下持续20s没有发现USB设备，则会退出脚本。
    if [ ${mode} -eq 0 ]
    then
        count=`expr ${count} + 1`
        if [ ${count} -gt 100 ]
        then
            echo "USB Device Lost"
            exit
        fi
    else
        count=0
    fi

    GetCMDFromAction
    if [ -n "${curcmd}" ]
    then
        echo "curcmd=${curcmd}"
    fi

    case ${curcmd} in
        *"${cmd_update}"*)
            name=`echo ${curcmd} | cut -d ' ' -f 2`
            RunGetDisplay
            UpdateFirmware ${name}
            ;;
        *"${cmd_erase}"*)
            RunGetDisplay
            EraseAllChip
            ;;
        *"${cmd_switch}"*)
            RunGetDisplay
            SwitchMode
            ;;
        *"${cmd_reset}"*)
            RunGetDisplay
            ResetDevice
            ;;
        *"${cmd_exit}"*)
            RunGetDisplay
            echo "Script[getstart.sh] exit" > ${DStatus}
            echo "Script Exit"
            exit 0
            ;;
        *)
            ;;
    esac

    MyLog ${curcmd}

    sleep 0.2
done
