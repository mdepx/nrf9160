APP =		nrf9160
MACHINE =	arm

LDSCRIPT =	${CURDIR}/ldscript

OBJDIR =	obj
OSDIR =		mdepx

OBJECTS =								\
		bsd_os.o						\
		main.o							\
		${OSDIR}/kernel/arm/nordicsemi/nrf_uarte.o		\
		${OSDIR}/kernel/arm/nordicsemi/nrf9160_power.o		\
		${OSDIR}/kernel/arm/nordicsemi/nrf9160_spu.o		\
		${OSDIR}/kernel/arm/nordicsemi/nrf9160_timer.o		\
		${OSDIR}/kernel/arm/nordicsemi/nrf9160_uicr.o		\
		start.o							\

NRFXLIB ?=	${CURDIR}/nrfxlib/
BSDLIB =	${NRFXLIB}/bsdlib/
OBERON =	${NRFXLIB}/crypto/nrf_oberon/

LDADD =		\
	${BSDLIB}/lib/cortex-m33/soft-float/libbsd_nrf9160_xxaa.a	\
	${OBERON}/lib/cortex-m33/soft-float/liboberon_3.0.0.a

define MDX_CONFIG
kernel {
	module arm;
	module callout;
	module malloc;
	module systm;

	callout {
		include tsleep;
	};

	malloc {
		include fl fl_wrapper;
	};

	systm {
		include console;
	};
};
endef

LIBRARIES = libc libaeabi mbedtls_mdsha

CFLAGS =-mthumb -mcpu=cortex-m4 -g -nostdlib -nostdinc			\
	-fshort-enums -fno-builtin-printf -ffreestanding		\
	-Wredundant-decls -Wnested-externs -Wstrict-prototypes		\
	-Wmissing-prototypes -Wpointer-arith -Winline			\
	-Wcast-qual -Wundef -Wmissing-include-dirs -Wall -Werror

all:	${OBJDIR}/${APP}.elf

clean:
	@rm -f ${OBJECTS} ${OBJDIR}/${APP}.*

include ${OSDIR}/lib/libaeabi/Makefile.inc
include ${OSDIR}/lib/libc/Makefile.inc
include ${OSDIR}/lib/mbedtls/Makefile.inc
include ${OSDIR}/mk/default.mk
