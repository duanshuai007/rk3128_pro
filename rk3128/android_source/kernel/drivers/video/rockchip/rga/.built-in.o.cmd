cmd_drivers/video/rockchip/rga/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/video/rockchip/rga/built-in.o drivers/video/rockchip/rga/rga.o ; scripts/mod/modpost drivers/video/rockchip/rga/built-in.o