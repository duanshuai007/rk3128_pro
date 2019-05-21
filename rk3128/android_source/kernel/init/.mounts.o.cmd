cmd_init/mounts.o := arm-eabi-ld -EL    -r -o init/mounts.o init/do_mounts.o init/do_mounts_initrd.o ; scripts/mod/modpost init/mounts.o
