cmd_drivers/iio/imu/built-in.o :=  arm-eabi-ld -EL    -r -o drivers/iio/imu/built-in.o drivers/iio/imu/inv_mpu6050/built-in.o ; scripts/mod/modpost drivers/iio/imu/built-in.o
