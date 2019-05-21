cmd_crypto/crypto_blkcipher.o := arm-eabi-ld -EL    -r -o crypto/crypto_blkcipher.o crypto/ablkcipher.o crypto/blkcipher.o ; scripts/mod/modpost crypto/crypto_blkcipher.o
