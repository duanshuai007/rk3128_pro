cmd_drivers/staging/zram/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/staging/zram/built-in.o drivers/staging/zram/zram.o ; scripts/mod/modpost drivers/staging/zram/built-in.o
