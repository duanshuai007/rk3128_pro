cmd_drivers/block/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/block/built-in.o drivers/block/loop.o ; scripts/mod/modpost drivers/block/built-in.o
