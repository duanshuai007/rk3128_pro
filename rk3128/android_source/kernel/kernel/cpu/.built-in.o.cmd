cmd_kernel/cpu/built-in.o :=  arm-eabi-ld -EL    -r -o kernel/cpu/built-in.o kernel/cpu/idle.o ; scripts/mod/modpost kernel/cpu/built-in.o
