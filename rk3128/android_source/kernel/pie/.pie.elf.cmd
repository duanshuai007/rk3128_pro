cmd_pie/pie.elf := arm-eabi-ld -EL   --no-undefined -X  -T arch/arm/kernel/pie.lds --pie --gc-sections pie/pie_stage3.o arch/arm/kernel/pie.lds -o pie/pie.elf 
