cmd_crypto/crypto.o := arm-eabi-ld -EL    -r -o crypto/crypto.o crypto/api.o crypto/cipher.o crypto/compress.o ; scripts/mod/modpost crypto/crypto.o
