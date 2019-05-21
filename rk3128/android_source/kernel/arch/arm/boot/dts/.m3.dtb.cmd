cmd_arch/arm/boot/dts/m3.dtb := arm-eabi-gcc -E -Wp,-MD,arch/arm/boot/dts/.m3.dtb.d.pre.tmp -nostdinc -I/home/duanshuai/android_source/kernel/arch/arm/boot/dts -I/home/duanshuai/android_source/kernel/arch/arm/boot/dts/include -I/home/duanshuai/android_source/kernel/include -undef -D__DTS__ -x assembler-with-cpp -o arch/arm/boot/dts/.m3.dtb.dts arch/arm/boot/dts/m3.dts ; /home/duanshuai/android_source/kernel/scripts/dtc/dtc -O dtb -o arch/arm/boot/dts/m3.dtb -b 0 -i arch/arm/boot/dts/  -d arch/arm/boot/dts/.m3.dtb.d.dtc.tmp arch/arm/boot/dts/.m3.dtb.dts ; cat arch/arm/boot/dts/.m3.dtb.d.pre.tmp arch/arm/boot/dts/.m3.dtb.d.dtc.tmp > arch/arm/boot/dts/.m3.dtb.d

source_arch/arm/boot/dts/m3.dtb := arch/arm/boot/dts/m3.dts

deps_arch/arm/boot/dts/m3.dtb := \
  arch/arm/boot/dts/rk3128.dtsi \
  arch/arm/boot/dts/rk312x.dtsi \
  /home/duanshuai/android_source/kernel/arch/arm/boot/dts/include/dt-bindings/interrupt-controller/arm-gic.h \
  /home/duanshuai/android_source/kernel/arch/arm/boot/dts/include/dt-bindings/interrupt-controller/irq.h \
  /home/duanshuai/android_source/kernel/arch/arm/boot/dts/include/dt-bindings/suspend/rockchip-pm.h \
  /home/duanshuai/android_source/kernel/arch/arm/boot/dts/include/dt-bindings/sensor-dev.h \
  /home/duanshuai/android_source/kernel/arch/arm/boot/dts/include/dt-bindings/clock/rk_system_status.h \
  /home/duanshuai/android_source/kernel/arch/arm/boot/dts/include/dt-bindings/rkfb/rk_fb.h \
  arch/arm/boot/dts/skeleton.dtsi \
  arch/arm/boot/dts/rk312x-clocks.dtsi \
  /home/duanshuai/android_source/kernel/arch/arm/boot/dts/include/dt-bindings/clock/rockchip,rk312x.h \
  /home/duanshuai/android_source/kernel/arch/arm/boot/dts/include/dt-bindings/clock/rockchip.h \
  arch/arm/boot/dts/rk312x-pinctrl.dtsi \
  /home/duanshuai/android_source/kernel/arch/arm/boot/dts/include/dt-bindings/gpio/gpio.h \
  /home/duanshuai/android_source/kernel/arch/arm/boot/dts/include/dt-bindings/pinctrl/rockchip.h \
    $(wildcard include/config/to/pull.h) \
    $(wildcard include/config/to/vol.h) \
    $(wildcard include/config/to/drv.h) \
    $(wildcard include/config/to/tri.h) \
  /home/duanshuai/android_source/kernel/arch/arm/boot/dts/include/dt-bindings/pinctrl/rockchip-rk312x.h \
  arch/arm/boot/dts/rk3128-cif-sensor-m3.dtsi \
    $(wildcard include/config/sensor/power/ioctl/usr.h) \
    $(wildcard include/config/sensor/reset/ioctl/usr.h) \
    $(wildcard include/config/sensor/powerdown/ioctl/usr.h) \
    $(wildcard include/config/sensor/flash/ioctl/usr.h) \
    $(wildcard include/config/sensor/af/ioctl/usr.h) \
  arch/arm/boot/dts/../../mach-rockchip/rk_camera_sensor_info.h \
    $(wildcard include/config/soc/camera/ov5642/interpolation/8m.h) \
    $(wildcard include/config/soc/camera/gc0308/interpolation/5m.h) \
    $(wildcard include/config/soc/camera/gc0308/interpolation/3m.h) \
    $(wildcard include/config/soc/camera/gc0308/interpolation/2m.h) \
    $(wildcard include/config/soc/camera/hi253/interpolation/5m.h) \
    $(wildcard include/config/soc/camera/hi253/interpolation/3m.h) \
  arch/arm/boot/dts/rk312x-sdk-m3.dtsi \
  arch/arm/boot/dts/lcd-m3.dtsi \
  arch/arm/boot/dts/rk818.dtsi \

arch/arm/boot/dts/m3.dtb: $(deps_arch/arm/boot/dts/m3.dtb)

$(deps_arch/arm/boot/dts/m3.dtb):
