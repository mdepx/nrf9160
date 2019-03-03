/*-
 * Copyright (c) 2018 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/console.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <nrfxlib/bsdlib/include/nrf_socket.h>
#include <nrfxlib/bsdlib/include/bsd.h>
#include <nrfxlib/bsdlib/include/bsd_os.h>

#include <arm/arm/nvic.h>
#include <arm/nordicsemi/nrf9160.h>

struct uarte_softc uarte_sc;
struct arm_nvic_softc nvic_sc;
struct spu_softc spu_sc;

#define	UART_PIN_TX	22
#define	UART_PIN_RX	21
#define	UART_BAUDRATE	115200
#define	NVIC_NINTRS	128

void rpc_proxy_intr(void *arg, struct trapframe *tf, int irq);
void trace_proxy_intr(void *arg, struct trapframe *tf, int irq);
void ipc_proxy_intr(void *arg, struct trapframe *tf, int irq);
void IPC_IRQHandler(void);
void app_main(void);

extern uint32_t _smem;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

static const struct nvic_intr_entry intr_map[NVIC_NINTRS] = {
	[ID_EGU1] = { rpc_proxy_intr, NULL },
	[ID_EGU2] = { trace_proxy_intr, NULL },
	[ID_IPC] = { ipc_proxy_intr, NULL },
};

void
rpc_proxy_intr(void *arg, struct trapframe *tf, int irq)
{

	bsd_os_application_irq_handler();
}

void
trace_proxy_intr(void *arg, struct trapframe *tf, int irq)
{

	bsd_os_trace_irq_handler();
}

void
ipc_proxy_intr(void *arg, struct trapframe *tf, int irq)
{

	IPC_IRQHandler();
}

static void
uart_putchar(int c, void *arg)
{
	struct uarte_softc *sc;
 
	sc = arg;
 
	if (c == '\n')
		uarte_putc(sc, '\r');

	uarte_putc(sc, c);
}

static void
clear_bss(void)
{
	uint8_t *sbss;
	uint8_t *ebss;

	sbss = (uint8_t *)&_sbss;
	ebss = (uint8_t *)&_ebss;

	while (sbss < ebss)
		*sbss++ = 0;
}

static void
copy_sdata(void)
{
	uint8_t *dst;
	uint8_t *src;

	/* Copy sdata to RAM if required */
	for (src = (uint8_t *)&_smem, dst = (uint8_t *)&_sdata;
	    dst < (uint8_t *)&_edata; )
		*dst++ = *src++;
}

void
bsd_recoverable_error_handler(uint32_t error)
{

	printf("%s: error %d\n", __func__, error);
}

void
bsd_irrecoverable_error_handler(uint32_t error)
{

	printf("%s: error %d\n", __func__, error);
	while (1);
}

void
app_main(void)
{

	clear_bss();

	uarte_init(&uarte_sc, BASE_UARTE0 | PERIPH_SECURE_ACCESS,
	    UART_PIN_TX, UART_PIN_RX, UART_BAUDRATE);
	console_register(uart_putchar, (void *)&uarte_sc);

	copy_sdata();

	fl_init();
	fl_add_region(0x20030000, 0x10000);

	spu_init(&spu_sc, BASE_SPU);
	spu_periph_set_attr(&spu_sc, ID_CLOCK, 0, 0);
	spu_periph_set_attr(&spu_sc, ID_RTC1, 0, 0);
	spu_periph_set_attr(&spu_sc, ID_IPC, 0, 0);
	spu_periph_set_attr(&spu_sc, ID_NVMC, 0, 0);
	spu_periph_set_attr(&spu_sc, ID_VMC, 0, 0);
	spu_periph_set_attr(&spu_sc, ID_GPIO, 0, 0);
	spu_periph_set_attr(&spu_sc, ID_GPIOTE1, 0, 0);
	spu_periph_set_attr(&spu_sc, ID_UARTE1, 0, 0);
	spu_periph_set_attr(&spu_sc, ID_UARTE2, 0, 0);
	spu_periph_set_attr(&spu_sc, ID_EGU1, 0, 0);
	spu_periph_set_attr(&spu_sc, ID_EGU2, 0, 0);
	spu_periph_set_attr(&spu_sc, ID_FPU, 0, 0);
	spu_periph_set_attr(&spu_sc, ID_TWIM2, 0, 0);
	spu_periph_set_attr(&spu_sc, ID_SPIM3, 0, 0);
	spu_periph_set_attr(&spu_sc, ID_TIMER0, 0, 0);
	spu_periph_set_attr(&spu_sc, ID_TIMER1, 0, 0);
	spu_periph_set_attr(&spu_sc, ID_TIMER2, 0, 0);

	arm_nvic_init(&nvic_sc, BASE_NVIC);
	arm_nvic_install_intr_map(&nvic_sc, intr_map);

	printf("Hello world!\n");

	bsd_init();

	printf("bsd library initialized\n");

	while (1);
}
