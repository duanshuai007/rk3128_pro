cmd_drivers/pps/pps_core.o := arm-eabi-ld -EL    -r -o drivers/pps/pps_core.o drivers/pps/pps.o drivers/pps/kapi.o drivers/pps/sysfs.o ; scripts/mod/modpost drivers/pps/pps_core.o
