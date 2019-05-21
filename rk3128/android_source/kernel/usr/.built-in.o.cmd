cmd_usr/built-in.o :=  arm-eabi-ld -EL    -r -o usr/built-in.o usr/initramfs_data.o ; scripts/mod/modpost usr/built-in.o
