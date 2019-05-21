cmd_drivers/power/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/power/built-in.o drivers/power/power_supply.o drivers/power/reset/built-in.o ; scripts/mod/modpost drivers/power/built-in.o
