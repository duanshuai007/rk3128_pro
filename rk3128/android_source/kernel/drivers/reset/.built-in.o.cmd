cmd_drivers/reset/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/reset/built-in.o drivers/reset/core.o drivers/reset/reset-rockchip.o ; scripts/mod/modpost drivers/reset/built-in.o
