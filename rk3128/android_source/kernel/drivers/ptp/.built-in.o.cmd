cmd_drivers/ptp/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/ptp/built-in.o drivers/ptp/ptp.o ; scripts/mod/modpost drivers/ptp/built-in.o
