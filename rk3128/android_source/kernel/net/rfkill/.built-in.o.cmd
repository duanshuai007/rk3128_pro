cmd_net/rfkill/built-in.o :=  arm-eabi-ld -EL    -r -o net/rfkill/built-in.o net/rfkill/rfkill.o net/rfkill/rfkill-wlan.o net/rfkill/rfkill-bt.o ; scripts/mod/modpost net/rfkill/built-in.o
