#!/bin/sh

case $1 in
    *"list"*)
        echo "list" > wifi/action
        ;;
    *"connect"*)
        echo "connect ssid=$2,password=$3" > wifi/action
        ;;
    *)
        ;;
esac
