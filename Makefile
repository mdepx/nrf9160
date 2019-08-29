APP =		nrf9160
MACHINE =	arm

LDSCRIPT =	${CURDIR}/ldscript

OBJDIR =	obj
OSDIR =		mdepx

OBJECTS =								\
		bsd_os.o						\
		main.o							\
		start.o							\

NRFXLIB ?=	${CURDIR}/nrfxlib/
BSDLIB =	${NRFXLIB}/bsdlib/
OBERON =	${NRFXLIB}/crypto/nrf_oberon/

LDADD =		\
	${BSDLIB}/lib/cortex-m33/soft-float/libbsd_nrf9160_xxaa.a	\
	${OBERON}/lib/cortex-m33/soft-float/liboberon_3.0.0.a

CFLAGS =-mthumb -mcpu=cortex-m4 -g -nostdlib -nostdinc			\
	-fshort-enums -fno-builtin-printf -ffreestanding		\
	-Wredundant-decls -Wnested-externs -Wstrict-prototypes		\
	-Wmissing-prototypes -Wpointer-arith -Winline			\
	-Wcast-qual -Wundef -Wmissing-include-dirs -Wall -Werror

all:	${OBJDIR}/${APP}.elf

clean:
	@rm -f ${OBJECTS} ${OBJDIR}/${APP}.*

include ${OSDIR}/mk/default.mk
