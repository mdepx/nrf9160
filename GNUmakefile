APP =		nrf9160
ARCH =		arm

CC =		${CROSS_COMPILE}gcc
LD =		${CROSS_COMPILE}ld
OBJCOPY =	${CROSS_COMPILE}objcopy

OBJDIR =	obj

LDSCRIPT =	${CURDIR}/ldscript

OBJECTS =	main.o						\
		osfive/sys/arm/nordicsemi/nrf_uarte.o		\
		osfive/sys/arm/arm/nvic.o			\
		osfive/sys/arm/arm/trap.o			\
		osfive/sys/arm/arm/exception.o			\
		osfive/sys/kern/subr_prf.o			\
		osfive/sys/kern/subr_console.o			\
		osfive/sys/kern/kern_panic.o			\
		start.o

LIBRARIES = LIBC

CFLAGS =	-mthumb -mcpu=cortex-m4		\
		-nostdlib -fno-builtin-printf	\
		-g -Wall -Werror

all:	compile link

clean:
	rm -f ${OBJECTS} ${APP}.elf

include osfive/lib/libc/Makefile.inc
include osfive/mk/gnu.user.mk
include osfive/mk/gnu.compile.mk
include osfive/mk/gnu.link.mk
include osfive/mk/gnu.binutils.mk
