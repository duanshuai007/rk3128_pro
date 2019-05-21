cmd_drivers/mmc/card/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/mmc/card/built-in.o drivers/mmc/card/mmc_block.o ; scripts/mod/modpost drivers/mmc/card/built-in.o
