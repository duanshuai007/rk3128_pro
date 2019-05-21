cmd_drivers/net/ethernet/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/net/ethernet/built-in.o drivers/net/ethernet/rockchip/built-in.o ; scripts/mod/modpost drivers/net/ethernet/built-in.o
