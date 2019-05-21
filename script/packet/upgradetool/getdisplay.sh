#!/bin/sh

#最终目标文件
Action=$1
Display=$2

#目录
#DIR=/home/frog/Linux_Upgrade_Tool_v1.21/status
DIR=$3
#最终目标保存文件
LFile=${DIR}/last
#临时显示信息文件
SFile=${DIR}/disstatus

curline=1
tolline=1
last_tolline=0

SaveLastFile() {
    size=`cat ${LFile} | wc -c`
    time=`date +"%Y-%m-%d %H:%M:%S"`
    timename=`date +"%Y%m%d%H%M%S"`
    if [ ${size} -gt 100000 ]
    then
        mv ${LFile} ${DIR}/"last-"${timename}".log"
        touch ${LFile}
    fi

    echo "------------ [${time}] ------------" >> ${LFile}
    cat ${Display} | sed "s/^M//" >> ${LFile}
}

#清空文件
cat /dev/null > ${Display}

sname=`echo $0 | awk -F/ '{print $NF}'`
echo "--------- Script[ ${sname} ] Start ---------"
#修改文件权限，防止写入
chmod 444 ${Action}
chattr +i ${Action}

count=0

while [ 1 ];
do
    sleep 0.02
    #获取临时文件内信息行数，以此来判断文件内容是否有更新
    last_tolline=${tolline}
    tolline=`cat ${SFile} | wc -l`

    if [ ${last_tolline} -eq ${tolline} ]
    then
        count=`expr ${count} + 1`
        if [ ${count} -gt 100 ]
        then
            SaveLastFile
            #使能文件的写权限
            chattr -i ${Action}
            chmod 666 ${Action}
            echo "getdisplay timeout exit"
            exit
        fi
    else
        count=0
    fi

    tmp=`expr ${tolline} + 1`
    if [ ${tmp} -lt ${curline} ]
    then
        curline=1
    fi

    while [ ${curline} -le ${tolline} ]
    do
        line=`sed -n "${curline}"p ${SFile}`
        line=`echo ${line} | sed "s/.*$(printf '\033\[1A\033\[2K')//"`
        line=`echo ${line} | sed "s///"`  #去掉每行末尾的^M字符
        curline=`expr ${curline} + 1`
        #判断该行内容是否包含Success类信息
        echo ${line} | grep Success > /dev/null
        #如果当前条信息是包含Success
        if [ $? -eq 0 ] #等于0则包含字符串
        then
            #取出文件末尾一行的内容
            d_lastline=`sed -n "$"p ${Display}`
            echo ${d_lastline} | grep Start > /dev/null
            #如果上一条信息是包含Start
            if [ $? -eq 0 ]
            then
                #匹配了，用当前条信息替换上一条
                #sed -i "$"d ${Display}
                lineno=`cat ${Display} | wc -l`
                sed -i "${lineno}s/Start/Success/" ${Display}
                continue
            fi
        else
            #判断改行内容是否包含进度信息
            echo ${line} | grep Total | grep Current > /dev/null
            if [ $? -eq 0 ]
            then    #如果是下载进度信息
                d_lastline=`sed -n "$"p ${Display}`
                echo ${d_lastline} | grep Total | grep Current > /dev/null
                if [ $? -eq 0 ]
                then
                    q1=`echo ${d_lastline} | grep Download`
                    q2=`echo ${line} | grep Check`
                    if [ -z "${q1}" ] || [ -z "${q2}" ];
                    then
                        lineno=`cat ${Display} | wc -l`
                        shenme=`echo ${line} | awk -F"Current" '{print $2}'`
                        sed -i "${lineno}s/Current(.*)/Current${shenme}/" ${Display}
                        continue
                    fi
                fi
            else
                res1=`echo ${line} | grep ok`
                res2=`echo ${line} | grep Fail`
                res3=`echo ${line} | grep OK`
                if [ -n "${res1}" ] || [ -n "${res2}" ] || [ -n "${res3}" ]
                then
                    unbuffer echo ${line} >> ${Display}
                    SaveLastFile
                    #使能文件的写权限
                    chattr -i ${Action}
                    chmod 666 ${Action}
                    echo "getdisplay resp exit"
                    exit
                fi
            fi
        fi
        unbuffer echo ${line} >> ${Display}
        sed -i 's///g' ${Display}
    done

    #刷机过程中出现了【没发生错误但是刷机进程不更新了】则会开始
   # ps -ef | grep upgrade_tool | grep -v grep > /dev/null
   # if [ $? -ne 0 ]
   # then
   #     count=`expr ${count} + 1`
   #     if [ ${count} -gt 30 ]
   #     then
   #         SaveLastFile
   #         #使能文件的写权限
   #         chattr -i ${Action}
   #         echo "getdisplay timenou exit"
   #         exit
   #     fi
   #     sleep 0.1
   # else
   #     count=0
   # fi
done
