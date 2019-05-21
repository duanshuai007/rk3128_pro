cmd_lib/fdt_ro.o := arm-eabi-gcc -Wp,-MD,lib/.fdt_ro.o.d  -nostdinc -isystem /home/duanshuai/m3/rom/m3/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/../lib/gcc/arm-eabi/4.6.x-google/include -I/home/duanshuai/android_source/kernel/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/home/duanshuai/android_source/kernel/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/home/duanshuai/android_source/kernel/include/uapi -Iinclude/generated/uapi -include /home/duanshuai/android_source/kernel/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -Os -Wno-maybe-uninitialized -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a -msoft-float -Uarm -Wframe-larger-than=1024 -fno-stack-protector -Wno-unused-but-set-variable -fomit-frame-pointer -fno-inline-functions-called-once -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -Ilib/../scripts/dtc/libfdt    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(fdt_ro)"  -D"KBUILD_MODNAME=KBUILD_STR(fdt_ro)" -c -o lib/fdt_ro.o lib/fdt_ro.c

source_lib/fdt_ro.o := lib/fdt_ro.c

deps_lib/fdt_ro.o := \
  include/linux/libfdt_env.h \
  include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
    $(wildcard include/config/kprobes.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  include/linux/compiler-gcc4.h \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  include/uapi/linux/types.h \
  arch/arm/include/generated/asm/types.h \
  /home/duanshuai/android_source/kernel/include/uapi/asm-generic/types.h \
  include/asm-generic/int-ll64.h \
  include/uapi/asm-generic/int-ll64.h \
  arch/arm/include/generated/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/uapi/asm-generic/bitsperlong.h \
  /home/duanshuai/android_source/kernel/include/uapi/linux/posix_types.h \
  include/linux/stddef.h \
  include/uapi/linux/stddef.h \
  /home/duanshuai/android_source/kernel/arch/arm/include/uapi/asm/posix_types.h \
  /home/duanshuai/android_source/kernel/include/uapi/asm-generic/posix_types.h \
  /home/duanshuai/m3/rom/m3/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/../lib/gcc/arm-eabi/4.6.x-google/include/stdarg.h \
  include/uapi/linux/string.h \
  /home/duanshuai/android_source/kernel/arch/arm/include/asm/string.h \
  /home/duanshuai/android_source/kernel/arch/arm/include/uapi/asm/byteorder.h \
  include/linux/byteorder/little_endian.h \
  include/uapi/linux/byteorder/little_endian.h \
  include/linux/swab.h \
  include/uapi/linux/swab.h \
  /home/duanshuai/android_source/kernel/arch/arm/include/asm/swab.h \
  /home/duanshuai/android_source/kernel/arch/arm/include/uapi/asm/swab.h \
  include/linux/byteorder/generic.h \
  lib/../scripts/dtc/libfdt/fdt_ro.c \
  lib/../scripts/dtc/libfdt/libfdt_env.h \
  lib/../scripts/dtc/libfdt/fdt.h \
  lib/../scripts/dtc/libfdt/libfdt.h \
  lib/../scripts/dtc/libfdt/libfdt_env.h \
  lib/../scripts/dtc/libfdt/libfdt_internal.h \

lib/fdt_ro.o: $(deps_lib/fdt_ro.o)

$(deps_lib/fdt_ro.o):
