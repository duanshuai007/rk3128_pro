cmd_net/llc/llc.o := arm-eabi-ld -EL    -r -o net/llc/llc.o net/llc/llc_core.o net/llc/llc_input.o net/llc/llc_output.o ; scripts/mod/modpost net/llc/llc.o
