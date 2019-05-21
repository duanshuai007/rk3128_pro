cmd_drivers/pwm/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/pwm/built-in.o drivers/pwm/core.o drivers/pwm/pwm-rockchip.o ; scripts/mod/modpost drivers/pwm/built-in.o
