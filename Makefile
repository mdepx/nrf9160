APP = nrf9160

OSDIR = mdepx

CMD = python3 -B ${OSDIR}/tools/emitter.py

# Set your jlink id
ADAPTOR_ID ?= 1050964301

all:
	@${CMD} -j mdepx.conf
	@${CROSS_COMPILE}objcopy -O ihex obj/${APP}.elf obj/${APP}.hex
	@${CROSS_COMPILE}size obj/${APP}.elf

debug:
	@${CMD} -d mdepx.conf
	@${CROSS_COMPILE}objcopy -O ihex obj/${APP}.elf obj/${APP}.hex
	@${CROSS_COMPILE}size obj/${APP}.elf

dtb:
	cpp -nostdinc -Imdepx/dts -Imdepx/dts/arm -Imdepx/dts/common	\
	    -Imdepx/dts/include -undef, -x assembler-with-cpp		\
	    nrf9160dk.dts -O obj/nrf9160dk.dts
	dtc -I dts -O dtb obj/nrf9160dk.dts -o obj/nrf9160dk.dtb
	bin2hex.py --offset=1015808 obj/nrf9160dk.dtb obj/nrf9160dk.dtb.hex
	nrfjprog -f NRF91 --erasepage 0xf8000-0xfc000
	nrfjprog -f NRF91 --program obj/nrf9160dk.dtb.hex -r

flash:
	nrfjprog -s ${ADAPTOR_ID} -f NRF91 --erasepage 0x40000-0xb2000
	nrfjprog -s ${ADAPTOR_ID} -f NRF91 --program obj/nrf9160.hex -r

reset:
	nrfjprog -s ${ADAPTOR_ID} -f NRF91 --reset

objdump:
	@${CROSS_COMPILE}objdump -d obj/${APP}.elf | less

readelf:
	@${CROSS_COMPILE}readelf -a obj/${APP}.elf | less

clean:
	@rm -rf obj/*

include ${OSDIR}/mk/user.mk
