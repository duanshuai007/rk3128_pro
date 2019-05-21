cmd_drivers/irqchip/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/irqchip/built-in.o drivers/irqchip/irqchip.o drivers/irqchip/irq-gic.o ; scripts/mod/modpost drivers/irqchip/built-in.o
