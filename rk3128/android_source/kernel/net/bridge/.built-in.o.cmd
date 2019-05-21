cmd_net/bridge/built-in.o :=  arm-eabi-ld -EL    -r -o net/bridge/built-in.o net/bridge/bridge.o ; scripts/mod/modpost net/bridge/built-in.o
