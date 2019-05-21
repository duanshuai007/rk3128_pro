cmd_fs/configfs/built-in.o :=  arm-eabi-ld -EL    -r -o fs/configfs/built-in.o fs/configfs/configfs.o ; scripts/mod/modpost fs/configfs/built-in.o
