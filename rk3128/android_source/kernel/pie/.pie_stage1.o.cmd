cmd_pie/pie_stage1.o := arm-eabi-objcopy  --weaken-symbols=pie/pie_weaken.syms --redefine-syms=pie/pie_rename.syms pie/../vmlinux.o pie/pie_stage1.o
