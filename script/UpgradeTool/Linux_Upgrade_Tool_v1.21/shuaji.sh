#!/bin/sh

ps -ef | grep watchme | grep -v grep
if [ $? -ne 0 ]
then
    sh /root/Linux_Upgrade_Tool_v1.21/watchme.sh &
fi
