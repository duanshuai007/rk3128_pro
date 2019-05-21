cmd_drivers/video/omap2/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/video/omap2/built-in.o drivers/video/omap2/displays/built-in.o ; scripts/mod/modpost drivers/video/omap2/built-in.o
