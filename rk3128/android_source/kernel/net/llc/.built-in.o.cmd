cmd_net/llc/built-in.o :=  arm-eabi-ld -EL    -r -o net/llc/built-in.o net/llc/llc.o ; scripts/mod/modpost net/llc/built-in.o
