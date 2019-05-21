cmd_drivers/char/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/char/built-in.o drivers/char/mem.o drivers/char/random.o drivers/char/misc.o ; scripts/mod/modpost drivers/char/built-in.o
