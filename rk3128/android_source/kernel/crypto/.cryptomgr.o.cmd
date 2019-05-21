cmd_crypto/cryptomgr.o := arm-eabi-ld -EL    -r -o crypto/cryptomgr.o crypto/algboss.o crypto/testmgr.o ; scripts/mod/modpost crypto/cryptomgr.o
