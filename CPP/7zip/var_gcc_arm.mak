PLATFORM=arm
O=b/g_$(PLATFORM)
IS_X64=
IS_X86=
IS_ARM64=

CROSS_COMPILE_ABI=
CROSS_COMPILE_ABI=arm-linux-musleabi
CROSS_COMPILE_ABI=arm-linux-musleabihf
CROSS_COMPILE_ABI=aarch64-linux-musl
CROSS_COMPILE_ABI=arm-linux-gnueabi
CROSS_COMPILE_ABI=arm-linux-gnueabihf

COMPILER_VER_POSTFIX=-12
COMPILER_VER_POSTFIX=

CROSS_COMPILE_PREFIX=

CROSS_COMPILE=$(CROSS_COMPILE_PREFIX)$(CROSS_COMPILE_ABI)-
CROSS_COMPILE=

MY_ARCH=
MY_ARCH=-mtune=cortex-a53 -march=armv7-a
MY_ARCH=-mtune=cortex-a53 -march=armv4
MY_ARCH=-mtune=cortex-a53

USE_ASM=

LDFLAGS_STATIC_3=-static
LDFLAGS_STATIC_3=

CC=$(CROSS_COMPILE)gcc$(COMPILER_VER_POSTFIX)
CXX=$(CROSS_COMPILE)g++$(COMPILER_VER_POSTFIX)
# -marm -march=armv5t -march=armv6 -march=armv7-a -march=armv8-a -march=armv8-a+crc+crypto
