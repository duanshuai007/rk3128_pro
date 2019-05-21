cmd_pie/libpie_stage2.o := arm-eabi-objcopy  --redefine-syms=pie/pie_rename.syms --rename-section .text=.pie.text pie/libpie_stage1.o pie/libpie_stage2.o
