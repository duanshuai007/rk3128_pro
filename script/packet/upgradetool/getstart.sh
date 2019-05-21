#!/bin/sh

ToolsDir=/root/upgradetool
#配置文件
Config=${ToolsDir}/config
#工具主目录
StatusDir=${ToolsDir}/status
#工具
ToolsName=${ToolsDir}/upgrade_tool
ToolsLoader=${ToolsDir}/m3/RK3128MiniLoaderAll_V2.25.bin
DisplayTool=${ToolsDir}/getdisplay.sh
#状态文件,用来对指令返回进行显示
DStatus=${ToolsDir}/status/disstatus
#记录文档
LogFile=${ToolsDir}/operation_log.txt

#控制软件提供的三个文件，只读。
Action=`sed -n 1p ${Config} | awk -F= '{print $2}'`
Display=`sed -n 2p ${Config} | awk -F= '{print $2}'`
CDevice=`sed -n 3p ${Config} | awk -F= '{print $2}'`
FirmWare=`sed -n 4p ${Config} | awk -F= '{print $2}'`

#LogFile最大尺寸
fsize=20000

MSG_NO_USB="Not find USB Devices"
MSG_SWITCH_FAILED="Device Not work in Msc Mode, Switch Fail"

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
#当前命令内容
curcmd=0

curcmdtime=0
lastcmdtime=0

InitCMDTime()
{
    curcmdtime=`stat ${Action} | grep Modify | awk -F: '{print $2$3$4}' | awk -F' ' '{print $1$2}' | awk -F- '{print $1$2$3}' | awk -F. '{print $1$2}' | cut -c 5-16`
    lastcmdtime=${curcmdtime}
}

GetFileModifyTime() {
    lastcmdtime=${curcmdtime}
    curcmdtime=`stat ${Action} | grep Modify | awk -F: '{print $2$3$4}' | awk -F' ' '{print $1$2}' | awk -F- '{print $1$2$3}' | awk -F. '{print $1$2}' | cut -c 5-16`
}

#从action中获取命令
GetCMDFromAction() {
    curcmd=""
    #获取文件修改时间戳
    GetFileModifyTime
    
    #此处时间取值需要根据主循环中的时间来去定，
    #务必要大于单次循环时间值   
    #if[ ${subv} -lt 100]
    #then
    #    curcmd=`sed -n 1p ${Action}`
    #fi

    if [ ${curcmdtime} -ne ${lastcmdtime} ]
    then
        curcmd=`sed -n 1p ${Action}`
    fi
}

GetUSBDevice() {
    #获取连接的rockusb设备信息输出到指定文件中
    lsusb | grep 2207 > /dev/null
    if [ $? -ne 0 ]
    then
        mode=0
        #echo "can't found usb device" > ${CDevice}
        echo ${MSG_NO_USB} > ${CDevice}
        return
    fi

    src=`echo "Q" |  ${ToolsName} | grep Vid`
    #如果字符串是空的，则返回
    if [ -z "${src}" ]
    then
        mode=0  #未发现设备
        #echo "can't found rockusb device." > ${CDevice}
        echo ${MSG_NO_USB} > ${CDevice}
        return
    fi

    #获取usb设备描述
    VID=`echo ${src} | cut -d '=' -f 3 | cut -d ',' -f 1`
    PID=`echo ${src} | cut -d '=' -f 4 | cut -d ',' -f 1`
    LID=`echo ${src} | cut -d '=' -f 5 | cut -d ' ' -f 1`
    MOD=`echo ${src} | cut -d '=' -f 5 | cut -d ' ' -f 2`

    case "${src}" in
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
        #echo "mode=${mode},vid=${VID},pid=${PID},LocationID=${LID}" > ${CDevice}
        echo "${MOD}" > ${CDevice}
    else
        #echo "can't found rockusb device." > ${CDevice}
        echo ${MSG_NO_USB} > ${CDevice}
    fi
}

ResetDevice() {
     ${ToolsName} rd > ${DStatus}
}

#参数为固件的全路径名
UpdateFirmware() {
    echo "UpdateFirmware ${FirmWare}"
    
 #   if [ ${mode} -eq ${MscMode} ]
 #   then
 #       SwitchMode
 #   fi

    if [ ! -e ${ToolsDir}/Firmware/${FirmWare} ]
    then
        echo "Firmware[${FirmWare}] is not exist, Update Fail" > ${DStatus}
        return
    fi

    unbuffer  ${ToolsName} uf ${ToolsDir}/Firmware/${FirmWare} > ${DStatus}
}

#只有在msc模式下能够进行模式切换,Android启动后就是Msc模式
SwitchMode() {
    echo "SwitchMode"
    if [ ${mode} -ne ${MscMode} ]
    then
        echo ${MSG_SWITCH_FAILED} > ${DStatus}
        return
    fi

    unbuffer  ${ToolsName} sd > ${DStatus}

    sleep 0.3
    
    GetUSBDevice
    while [ ${mode} -ne ${LoaderMode} ]
    do
        GetUSBDevice
        sleep 0.1
    done
}

EraseAllChip() {
    echo "EraseAllChip"
    #如果当前实在Msc模式下，需要先切换
    #if [ ${mode} -eq ${MscMode} ]
    #then
    #    SwitchMode
    #fi

    unbuffer  ${ToolsName} ef ${ToolsLoader} > ${DStatus}

    sleep 0.3

    GetUSBDevice
    while [ ${mode} -eq 0 ]
    do
        GetUSBDevice
        sleep 0.1
    done
}

RunGetDisplay() {
    sname=`echo ${DisplayTool} | awk -F/ '{print $NF}'`
    ps -ef | grep ${sname} | grep -v grep > /dev/null
    if [ $? -ne 0 ] #不等于0，说明进程未运行
    then
        sh ${DisplayTool} ${Action} ${Display} ${StatusDir} &
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

#检查自身是否是唯一的进程
#当脚本检测自身是否在进程中使，返回的值会比实际大1.
CheckMySelf() {
    sname=`echo $0 | awk -F/ '{print $NF}'`
    num=`ps -ef | grep "${sname}" | grep -v grep | wc -l`
    echo "total [${num}][${sname}] process running..."
    if [ ${num} -gt 2 ]
    then
        exit
    fi
    echo "--------- Script[${sname}] Start ---------"
}

#主程序开始
CheckMySelf

InitCMDTime

if [ ! -e ${Action} ]
then
    touch ${Action}
fi

if [ ! -e ${Display} ]
then
    touch ${Display}
fi

if [ ! -e ${CDevice} ]
then
    touch ${CDevice}
fi

cat /dev/null > ${Display}

#开始循环监视action文件
while [ 1 ];
do
    GetUSBDevice

    #如果在空闲状态下持续[x]s没有发现USB设备，则会退出脚本。
    if [ ${mode} -eq 0 ]
    then
        chattr +i ${Action}
        chmod 444 ${Action}
        count=`expr ${count} + 1`
        if [ ${count} -gt 15 ]
        then
            count=0
            echo "USB Device Lost"
            cat /dev/null > ${Display}
            exit
        fi
    else
        count=0
        chattr -i ${Action}
        chmod 666 ${Action}
        GetCMDFromAction

        if [ -n "${curcmd}" ]
        then
            #echo "curcmd=${curcmd}"
            case ${curcmd} in
                *"${cmd_update}"*)
                    #name=`echo ${curcmd} | cut -d ' ' -f 2`
                    RunGetDisplay
                    SwitchMode
                    RunGetDisplay
                    UpdateFirmware 
                    ;;
                *"${cmd_erase}"*)
                    #
                    RunGetDisplay
                    SwitchMode
                    #
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

            MyLog "${curcmd}"

        fi
    fi

    sleep 0.2

done
