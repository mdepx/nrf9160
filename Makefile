APP = nrf9160

OSDIR = mdepx

CMD = python3 -B ${OSDIR}/tools/emitter.py

all:
	@${CMD} -j mdepx.conf
	@${CROSS_COMPILE}objcopy -O ihex obj/${APP}.elf obj/${APP}.hex

debug:
	@${CMD} -d mdepx.conf
	@${CROSS_COMPILE}objcopy -O ihex obj/${APP}.elf obj/${APP}.hex

flash:
	nrfjprog -f NRF91 --erasepage 0x40000-0x60000
	nrfjprog -f NRF91 --program obj/nrf9160.hex -r

reset:
	nrfjprog -f NRF91 -r

clean:
	@rm -rf obj/*

include ${OSDIR}/mk/user.mk
