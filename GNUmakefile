APP =		nrf9160
MACHINE =	arm

LDSCRIPT =	${CURDIR}/ldscript

OBJECTS =							\
		bsd_os.o					\
		main.o						\
		osfive/sys/arm/nordicsemi/nrf_uarte.o		\
		osfive/sys/arm/nordicsemi/nrf9160_power.o	\
		osfive/sys/arm/nordicsemi/nrf9160_spu.o		\
		osfive/sys/arm/nordicsemi/nrf9160_timer.o	\
		osfive/sys/arm/nordicsemi/nrf9160_uicr.o	\
		start.o						\

NRFXLIB ?=	${CURDIR}/nrfxlib/
BSDLIB =	${NRFXLIB}/bsdlib/
OBERON =	${NRFXLIB}/crypto/nrf_oberon/

OBJECTS_LINK =		\
	${BSDLIB}/lib/cortex-m33/soft-float/libbsd_nrf9160_xxaa.a \
	${OBERON}/lib/cortex-m33/soft-float/liboberon_3.0.0.a

KERNEL = malloc
LIBRARIES = libc libaeabi mbedtls_mdsha

CFLAGS =-mthumb -mcpu=cortex-m4 -g -nostdlib -nostdinc	\
	-fshort-enums					\
	-fno-builtin-printf -ffreestanding		\
	-Wredundant-decls -Wnested-externs		\
	-Wstrict-prototypes -Wmissing-prototypes	\
	-Wpointer-arith -Winline -Wcast-qual		\
	-Wundef -Wmissing-include-dirs -Wall -Werror

all:	_compile _link _info

clean:	_clean

include osfive/mk/default.mk
include osfive/mk/gnu-toolchain.mk
