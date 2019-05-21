cmd_firmware/audience/cvqmodels/adnc_cvq_detection_bkg_w_hdrs.bin.gen.o := arm-eabi-gcc -Wp,-MD,firmware/audience/cvqmodels/.adnc_cvq_detection_bkg_w_hdrs.bin.gen.o.d  -nostdinc -isystem /home/duanshuai/m3/rom/m3/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/../lib/gcc/arm-eabi/4.6.x-google/include -I/home/duanshuai/android_source/kernel/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/home/duanshuai/android_source/kernel/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/home/duanshuai/android_source/kernel/include/uapi -Iinclude/generated/uapi -include /home/duanshuai/android_source/kernel/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian  -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a  -include asm/unified.h -msoft-float         -c -o firmware/audience/cvqmodels/adnc_cvq_detection_bkg_w_hdrs.bin.gen.o firmware/audience/cvqmodels/adnc_cvq_detection_bkg_w_hdrs.bin.gen.S

source_firmware/audience/cvqmodels/adnc_cvq_detection_bkg_w_hdrs.bin.gen.o := firmware/audience/cvqmodels/adnc_cvq_detection_bkg_w_hdrs.bin.gen.S

deps_firmware/audience/cvqmodels/adnc_cvq_detection_bkg_w_hdrs.bin.gen.o := \
  /home/duanshuai/android_source/kernel/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \

firmware/audience/cvqmodels/adnc_cvq_detection_bkg_w_hdrs.bin.gen.o: $(deps_firmware/audience/cvqmodels/adnc_cvq_detection_bkg_w_hdrs.bin.gen.o)

$(deps_firmware/audience/cvqmodels/adnc_cvq_detection_bkg_w_hdrs.bin.gen.o):
