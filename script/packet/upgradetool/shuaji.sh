#!/bin/sh

ps -ef | grep watch_upgrade | grep -v grep
if [ $? -ne 0 ]
then
    sh /root/upgradetool/watch_upgrade.sh &
fi
