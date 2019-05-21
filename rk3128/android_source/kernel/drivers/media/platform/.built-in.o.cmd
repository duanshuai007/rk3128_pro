cmd_drivers/media/platform/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/media/platform/built-in.o drivers/media/platform/davinci/built-in.o ; scripts/mod/modpost drivers/media/platform/built-in.o
