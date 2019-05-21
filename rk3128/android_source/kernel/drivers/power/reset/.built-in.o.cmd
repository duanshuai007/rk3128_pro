cmd_drivers/power/reset/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/power/reset/built-in.o drivers/power/reset/gpio-poweroff.o ; scripts/mod/modpost drivers/power/reset/built-in.o
