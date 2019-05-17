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
#define	UART_PIN_RX	28
#define	UART_BAUDRATE	115200
#define	NVIC_NINTRS	128

#define	LC_MAX_READ_LENGTH	128
#define	AT_CMD_SIZE(x)		(sizeof(x) - 1)

void rpc_proxy_intr(void *arg, struct trapframe *tf, int irq);
void trace_proxy_intr(void *arg, struct trapframe *tf, int irq);
void ipc_proxy_intr(void *arg, struct trapframe *tf, int irq);
void IPC_IRQHandler(void);

static const char cind[] = "AT+CIND?";
static const char subscribe[] = "AT+CEREG=5";
static const char lock_bands[] = "AT%XBANDLOCK=2,\"10000001000000001100\"";
static const char normal[] = "AT+CFUN=1";
static const char edrx_req[] = "AT+CEDRXS=1,4,\"1000\"";
static const char cgdcont[] __unused = "AT+CGDCONT?";
static const char cgdcont_req[] = "AT+CGDCONT=0,\"IPv4v6\",\"internet.apn\"";
static const char cgpaddr[] __unused = "AT+CGPADDR";
static const char cesq[] = "AT+CESQ";

char buffer[LC_MAX_READ_LENGTH];
int buffer_fill;
int ready_to_send;

static const struct nvic_intr_entry intr_map[NVIC_NINTRS] = {
	[ID_UARTE0] = { uarte_intr, &uarte_sc },
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

static void
lte_at_client(void *arg)
{
	int fd;
	int len;

	fd = nrf_socket(NRF_AF_LTE, 0, NRF_PROTO_AT);
	if (fd < 0)
		printf("failed to create socket\n");

	while (1) {
		if (ready_to_send) {
			nrf_send(fd, buffer, buffer_fill, 0);

			ready_to_send = 0;
			buffer_fill = 0;

			len = nrf_recv(fd, buffer, LC_MAX_READ_LENGTH, 0);
			if (len)
				printf("%s\n", buffer);
		}
		raw_sleep(10000);
	}
}

static void __unused
lte_test(void)
{
	int fd;

	fd = nrf_socket(NRF_AF_LTE, 0, NRF_PROTO_AT);
	if (fd < 0)
		printf("failed to create socket\n");

	at_cmd(fd, cind, AT_CMD_SIZE(cind));
	at_cmd(fd, edrx_req, AT_CMD_SIZE(edrx_req));
	at_cmd(fd, subscribe, AT_CMD_SIZE(subscribe));

	/* Lock bands 3,4,13,20. */
	at_cmd(fd, lock_bands, AT_CMD_SIZE(lock_bands));

	/* Define PDP Context */
	at_cmd(fd, cgdcont_req, AT_CMD_SIZE(cgdcont_req));

	/* Normal mode. */
	at_cmd(fd, normal, AT_CMD_SIZE(normal));

	while (1) {
		/* Extended signal quality */
		at_cmd(fd, cesq, AT_CMD_SIZE(cesq));

		raw_sleep(1000000);
	}
}

static void
nrf_input(int c, void *arg)
{

	if (c == 13)
		ready_to_send = 1;
	else if (buffer_fill < LC_MAX_READ_LENGTH)
		buffer[buffer_fill++] = c;
}

int
app_init(void)
{

	uarte_init(&uarte_sc, BASE_UARTE0,
	    UART_PIN_TX, UART_PIN_RX, UART_BAUDRATE);
	console_register(uart_putchar, (void *)&uarte_sc);
	uarte_register_callback(&uarte_sc, nrf_input, NULL);

	printf("osfive initialized\n");

	fl_init();
	fl_add_region(0x20030000, 0x10000);

	power_init(&power_sc, BASE_POWER);

	arm_nvic_init(&nvic_sc, BASE_SCS);
	arm_nvic_install_intr_map(&nvic_sc, intr_map);
	arm_nvic_set_prio(&nvic_sc, ID_IPC, 6);

	timer_init(&timer0_sc, BASE_TIMER0);
	arm_nvic_enable_intr(&nvic_sc, ID_TIMER0);
	arm_nvic_enable_intr(&nvic_sc, ID_UARTE0);

	return (0);
}

int
main(void)
{

	bsd_init();

	printf("bsd library initialized\n");

	buffer_fill = 0;
	ready_to_send = 0;
	lte_at_client(NULL);

	panic("lte_test returned!\n");

	return (0);
}
