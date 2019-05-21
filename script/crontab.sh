#!/bin/bash
step=1 #间隔的秒数，不能大于60
 
for (( i = 0; i < 60; i=(i+step) )); do
    ps -ef | grep -w "netmonitor.sh" | grep -v grep > /dev/null
    if [ $? -ne 0 ]
    then
        sh /root/netmonitor.sh 
    fi
    sleep $step
done
         
exit 0
