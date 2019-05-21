cmd_drivers/ptp/ptp.o := arm-eabi-ld -EL    -r -o drivers/ptp/ptp.o drivers/ptp/ptp_clock.o drivers/ptp/ptp_chardev.o drivers/ptp/ptp_sysfs.o ; scripts/mod/modpost drivers/ptp/ptp.o
