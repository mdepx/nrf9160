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
#include <sys/callout.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/thread.h>

#include <nrfxlib/bsdlib/include/nrf_socket.h>
#include <nrfxlib/bsdlib/include/bsd.h>
#include <nrfxlib/bsdlib/include/bsd_os.h>

#include <arm/arm/nvic.h>
#include <arm/nordicsemi/nrf9160.h>

struct uarte_softc uarte_sc;
struct arm_nvic_softc nvic_sc;
struct spu_softc spu_sc;
struct power_softc power_sc;
struct timer_softc timer0_sc;

#define	UART_PIN_TX	29
#define	UART_PIN_RX	21
#define	UART_BAUDRATE	115200
#define	NVIC_NINTRS	128

#define	LC_MAX_READ_LENGTH	128
#define	AT_CMD_SIZE(x)		(sizeof(x) - 1)

void rpc_proxy_intr(void *arg, struct trapframe *tf, int irq);
void trace_proxy_intr(void *arg, struct trapframe *tf, int irq);
void ipc_proxy_intr(void *arg, struct trapframe *tf, int irq);
void IPC_IRQHandler(void);
void app_main(void);

static const char cind[] = "AT+CIND?";

extern uint32_t _smem;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

static const struct nvic_intr_entry intr_map[NVIC_NINTRS] = {
	[ID_TIMER0] = { timer_intr, &timer0_sc },
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

static int
at_cmd(int fd, const char *cmd, size_t size)
{
	uint8_t buffer[LC_MAX_READ_LENGTH];
	int len;

	printf("send: %s\n", cmd);

	len = nrf_send(fd, cmd, size, 0);
	if (len != size) {
		printf("send failed\n");
		return (-1);
	}

	len = nrf_recv(fd, buffer, LC_MAX_READ_LENGTH, 0);
	printf("recv: %s\n", buffer);

	return (0);
}

void
app_main(void)
{
	int at_socket_fd;

	zero_bss();
	relocate_data();
	md_init();

	uarte_init(&uarte_sc, BASE_UARTE0,
	    UART_PIN_TX, UART_PIN_RX, UART_BAUDRATE);
	console_register(uart_putchar, (void *)&uarte_sc);

	printf("osfive initialized\n");

	fl_init();
	fl_add_region(0x20030000, 0x10000);

	power_init(&power_sc, BASE_POWER);

	arm_nvic_init(&nvic_sc, BASE_SCS);
	arm_nvic_install_intr_map(&nvic_sc, intr_map);
	arm_nvic_set_prio(&nvic_sc, ID_IPC, 6);

	timer_init(&timer0_sc, BASE_TIMER0);
	arm_nvic_enable_intr(&nvic_sc, ID_TIMER0);

	bsd_init();

	printf("bsd library initialized\n");

	at_socket_fd = nrf_socket(NRF_AF_LTE, 0, NRF_PROTO_AT);
	if (at_socket_fd < 0)
		printf("failed to create socket\n");

	at_cmd(at_socket_fd, cind, AT_CMD_SIZE(cind));

	while (1)
		__asm __volatile("wfi");
}
