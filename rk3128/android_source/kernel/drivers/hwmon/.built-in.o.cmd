cmd_drivers/hwmon/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/hwmon/built-in.o drivers/hwmon/hwmon.o drivers/hwmon/rockchip-hwmon.o drivers/hwmon/rockchip_tsadc.o ; scripts/mod/modpost drivers/hwmon/built-in.o