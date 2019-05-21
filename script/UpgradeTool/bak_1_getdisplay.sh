#!/bin/sh

#目录
DIR=/home/frog/Linux_Upgrade_Tool_v1.21/status
#最终目标保存文件
LFile=${DIR}/last
#临时显示信息文件
SFile=${DIR}/disstatus

#最终目标文件
Action=$1
Display=$2

curline=1
tolline=1

#清空文件
cat /dev/null > ${Display}

echo "--------- Script[  getdisplay.sh  ] Start ---------"
#修改文件权限，防止写入
chmod 444 ${Action}

while [ 1 ];
do
    #获取临时文件内信息行数，以此来判断文件内容是否有更新
    tolline=`cat ${SFile} | wc -l`
    
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
            echo ${line} | grep Total | grep Current | grep Download > /dev/null
            if [ $? -eq 0 ]
            then    #如果是下载进度信息
                d_lastline=`sed -n "$"p ${Display}`
                echo ${d_lastline} | grep Total | grep Current | grep Download > /dev/null
                if [ $? -eq 0 ]
                then
                    #sed -i "$"d ${Display}
                    lineno=`cat ${Display} | wc -l`
                    shenme=`echo ${line} | awk -F"Current" '{print $2}'`
                    sed -i "${lineno}s/Current(.*)/Current${shenme}/" ${Display}
                    continue
                fi
            else
                echo ${line} | grep Total | grep Current | grep Check > /dev/null
                if [ $? -eq 0 ]
                then    #如果是检查进度信息
                    d_lastline=`sed -n "$"p ${Display}`
                    echo ${d_lastline} | grep Total | grep Current | grep Check > /dev/null
                    if [ $? -eq 0 ]
                    then
                        lineno=`cat ${Display} | wc -l`
                        shenme=`echo ${line} | awk -F"Current" '{print $2}'`
                        sed -i "${lineno}s/Current(.*)/Current${shenme}/" ${Display}
                        continue
                    fi
                else
                    echo ${line} | grep Total | grep Current | grep Erase > /dev/null
                    if [ $? -eq 0 ]
                    then    ##如果是擦除进度信息
                        d_lastline=`sed -n "$"p ${Display}`
                        echo ${d_lastline} | grep Total | grep Current | grep Erase > /dev/null
                        if [ $? -eq 0 ]
                        then
                            lineno=`cat ${Display} | wc -l`
                            shenme=`echo ${line} | awk -F"Current" '{print $2}'`
                            sed -i "${lineno}s/Current(.*)/Current${shenme}/" ${Display}
                            continue
                        fi
                    fi
                fi      
            fi
        fi
        unbuffer echo ${line} >> ${Display}
        sed -i 's///g' ${Display}
    done

    ps -ef | grep upgrade_tool | grep -v grep > /dev/null
    if [ $? -ne 0 ]
    then
        count=`expr ${count} + 1`
        if [ ${count} -gt 10 ]
        then
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

            echo "getdisplay exit"
            break
        fi
        sleep 0.1
    else
        count=0
    fi

done

#使能文件的写权限
chmod 664 ${Action}

