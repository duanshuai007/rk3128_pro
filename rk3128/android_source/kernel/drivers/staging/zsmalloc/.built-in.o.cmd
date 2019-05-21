cmd_drivers/staging/zsmalloc/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/staging/zsmalloc/built-in.o drivers/staging/zsmalloc/zsmalloc.o ; scripts/mod/modpost drivers/staging/zsmalloc/built-in.o
