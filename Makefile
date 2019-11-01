APP = nrf9160

OSDIR = mdepx

CMD = python3 -B ${OSDIR}/tools/emitter.py

all:
	@${CMD} mdepx.conf

clean:
	@rm -rf obj/*

include ${OSDIR}/mk/user.mk
