cmd_net/l2tp/built-in.o :=  arm-eabi-ld -EL    -r -o net/l2tp/built-in.o net/l2tp/l2tp_core.o net/l2tp/l2tp_ppp.o ; scripts/mod/modpost net/l2tp/built-in.o
