cmd_net/rfkill/rfkill.o := arm-eabi-ld -EL    -r -o net/rfkill/rfkill.o net/rfkill/core.o ; scripts/mod/modpost net/rfkill/rfkill.o
