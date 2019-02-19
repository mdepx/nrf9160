APP =		nrf9160
ARCH =		arm

CC =		${CROSS_COMPILE}gcc
LD =		${CROSS_COMPILE}ld
OBJCOPY =	${CROSS_COMPILE}objcopy

LDSCRIPT =	${CURDIR}/ldscript

OBJECTS =	alloc.o						\
		main.o						\
		osfive/sys/arm/nordicsemi/nrf_uarte.o		\
		osfive/sys/arm/nordicsemi/nrf9160_uicr.o	\
		osfive/sys/arm/nordicsemi/nrf9160_power.o	\
		osfive/sys/arm/nordicsemi/nrf9160_timer.o	\
		osfive/sys/arm/arm/machdep.o			\
		osfive/sys/arm/arm/nvic.o			\
		osfive/sys/arm/arm/trap.o			\
		osfive/sys/arm/arm/exception.o			\
		osfive/sys/kern/kern_malloc_fl.o		\
		osfive/sys/kern/subr_prf.o			\
		osfive/sys/kern/subr_console.o			\
		osfive/sys/kern/kern_panic.o			\
		start.o

OBJECTS_LINK =		\
  ${CURDIR}/nrfxlib/bsdlib/lib/cortex-m33/soft-float/libbsd_nrf9160_xxaa.a \
  ${CURDIR}/nrfxlib/bsdlib/lib/cortex-m33/soft-float/liboberon_2.0.5.a

LIBRARIES = LIBC LIBAEABI MBEDTLS_MDSHA

CFLAGS =-mthumb -mcpu=cortex-m4 -g -nostdlib -nostdinc	\
	-fno-builtin-printf -ffreestanding		\
	-Wredundant-decls -Wnested-externs		\
	-Wstrict-prototypes -Wmissing-prototypes	\
	-Wpointer-arith -Winline -Wcast-qual		\
	-Wundef -Wmissing-include-dirs -Wall -Werror

all:	compile link

include osfive/lib/libaeabi/Makefile.inc
include osfive/lib/libc/Makefile.inc
include osfive/lib/mbedtls/Makefile.inc
include osfive/mk/gnu.binutils.mk
include osfive/mk/gnu.clean.mk
include osfive/mk/gnu.compile.mk
include osfive/mk/gnu.link.mk
include osfive/mk/gnu.user.mk
