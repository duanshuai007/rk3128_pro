cmd_drivers/gpio/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/gpio/built-in.o drivers/gpio/devres.o drivers/gpio/gpiolib.o drivers/gpio/gpiolib-of.o ; scripts/mod/modpost drivers/gpio/built-in.o
