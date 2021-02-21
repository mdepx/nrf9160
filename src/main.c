/*-
 * Copyright (c) 2018-2021 Ruslan Bukin <br@bsdpad.com>
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

#include <sys/mbuf.h>
#include <net/if.h>
#include <net/netinet/in.h>
#include <arpa/inet.h>

#include <arm/nordicsemi/nrf9160.h>

#include <nrfxlib/nrf_modem/include/nrf_socket.h>
#include <nrfxlib/nrf_modem/include/nrf_modem.h>
#include <nrfxlib/nrf_modem/include/nrf_modem_os.h>
#include <nrfxlib/nrf_modem/include/nrf_modem_platform.h>

#include "gps.h"

#define	LC_MAX_READ_LENGTH	128
#define	AT_CMD_SIZE(x)		(sizeof(x) - 1)

#define	TCP_HOST	"machdep.com"
#define	TCP_PORT	80

extern struct nrf_uarte_softc uarte_sc;

static const char cind[] __unused = "AT+CIND?";
static const char subscribe[] = "AT+CEREG=5";
static const char lock_bands[] __unused =
    "AT\%XBANDLOCK=2,\"10000001000000001100\"";
static const char normal[] = "AT+CFUN=1";
static const char flight[] __unused = "AT+CFUN=4";
static const char gps_enable[] __unused = "AT+CFUN=31";
static const char lte_enable[] __unused = "AT+CFUN=21";
static const char lte_disable[] __unused = "AT+CFUN=20";
static const char edrx_req[] __unused = "AT+CEDRXS=1,4,\"1000\"";
static const char cgact[] __unused = "AT+CGACT=1,1";
static const char cgatt[] __unused = "AT+CGATT=1";
static const char cgdcont[] __unused = "AT+CGDCONT?";
static const char cgdcont_req[] __unused =
    "AT+CGDCONT=1,\"IP\",\"ibasis.iot\"";
static const char cgpaddr[] __unused = "AT+CGPADDR";
static const char cesq[] __unused = "AT+CESQ";
static const char cpsms[] __unused = "AT+CPSMS=";

static const char psm_req[] = "AT+CPSMS=1,,,\"00000110\",\"00000000\"";
/* Request eDRX to be disabled */
static const char edrx_disable[] = "AT+CEDRXS=3";

static const char magpio[] __unused = "AT\%XMAGPIO=1,0,0,1,1,1574,1577";
static const char coex0[] __unused = "AT\%XCOEX0=1,1,1570,1580";

/*
 * %XSYSTEMMODE=<M1_support>,<NB1_support>,<GNSS_support>,<LTE_preference>
 */

static const char systm_mode[] __unused = "AT%XSYSTEMMODE?";
static const char nbiot_gps[] __unused = "AT%XSYSTEMMODE=0,1,1,0";
static const char catm1_gps[] __unused = "AT%XSYSTEMMODE=1,0,1,0";

static char buffer[LC_MAX_READ_LENGTH];
static int buffer_fill;
static int ready_to_send;

#define	NRF_MODEM_OS_SHMEM_CTRL_ADDR	0x20010000
#define	NRF_MODEM_OS_SHMEM_CTRL_SIZE	NRF_MODEM_SHMEM_CTRL_SIZE
#define	NRF_MODEM_OS_SHMEM_TX_ADDR	0x20011000
#define	NRF_MODEM_OS_SHMEM_TX_SIZE	0x5000
#define	NRF_MODEM_OS_SHMEM_RX_ADDR	0x20016000
#define	NRF_MODEM_OS_SHMEM_RX_SIZE	0x5000

CTASSERT(NRF_MODEM_OS_SHMEM_CTRL_SIZE <= 0x1000);

static const nrf_modem_init_params_t init_params = {
	.ipc_irq_prio = NRF_MODEM_NETWORK_IRQ_PRIORITY,
	.shmem.ctrl = {
		.base = NRF_MODEM_OS_SHMEM_CTRL_ADDR,
		.size = NRF_MODEM_OS_SHMEM_CTRL_SIZE,
	},
	.shmem.tx = {
		.base = NRF_MODEM_OS_SHMEM_TX_ADDR,
		.size = NRF_MODEM_OS_SHMEM_TX_SIZE,
	},
	.shmem.rx = {
		.base = NRF_MODEM_OS_SHMEM_RX_ADDR,
		.size = NRF_MODEM_OS_SHMEM_RX_SIZE,
	},
#if 0
	.shmem.trace = {
		.base = NRF_MODEM_OS_TRACE_ADDRESS,
		.size =	NRF_MODEM_OS_TRACE_SIZE,
	},
#endif
};

static int
at_send(int fd, const char *cmd, size_t size)
{
	int len;

	len = nrf_send(fd, cmd, size, 0);
	if (len != size) {
		printf("send failed\n");
		return (-1);
	}

	return (0);
}

static int
at_recv(int fd, char *buf, int bufsize)
{
	int len;

	len = nrf_recv(fd, buf, bufsize, 0);

	return (len);
}

static int
at_cmd(int fd, const char *cmd, size_t size)
{
	char buffer[LC_MAX_READ_LENGTH];
	int len;

	printf("send: %s\n", cmd);

	if (at_send(fd, cmd, size) == 0) {
		len = at_recv(fd, buffer, LC_MAX_READ_LENGTH);
		if (len)
			printf("recv: %s\n", buffer);
	}

	return (0);
}

static void
lte_signal(int fd)
{
	char buf[LC_MAX_READ_LENGTH];
	float rsrq;
	int rsrp;
	int len;
	char *t, *p;

	/* Extended signal quality */
	at_send(fd, cesq, AT_CMD_SIZE(cesq));
	len = at_recv(fd, buf, LC_MAX_READ_LENGTH);
	if (len) {
		printf("recv: %s\n", buf);

		t = (char *)buf;

		p = strsep(&t, ",");	/* +CESQ: rxlev */
		p = strsep(&t, ",");	/* ber */
		p = strsep(&t, ",");	/* rscp */
		p = strsep(&t, ",");	/* echo */
		p = strsep(&t, ",");	/* rsrq */

		rsrq = 20 - atoi(p) / 2;

		p = strsep(&t, ",");

		rsrp = 140 - atoi(p) + 1;

		printf("LTE signal quality: rsrq -%.2f dB rsrp -%d dBm\n",
		    rsrq, rsrp);
	}
}

static void __unused
lte_at_client(void *arg)
{
	int fd;
	int len;

	fd = nrf_socket(NRF_AF_LTE, NRF_SOCK_DGRAM, NRF_PROTO_AT);
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
		mdx_usleep(10000);
	}
}

static void
connect_to_server(void)
{
	struct nrf_addrinfo *server_addr;
	struct nrf_sockaddr_in local_addr;
	struct nrf_sockaddr_in *s;
	uint8_t *ip;
	int err;
	int fd;

	fd = nrf_socket(NRF_AF_INET, NRF_SOCK_STREAM, NRF_IPPROTO_TCP);
	if (fd < 0)
		panic("failed to create socket");

	err = nrf_getaddrinfo(TCP_HOST, NULL, NULL, &server_addr);
	if (err != 0)
		panic("getaddrinfo failed with error %d\n", err);

	s = (struct nrf_sockaddr_in *)server_addr->ai_addr;
	ip = (uint8_t *)&(s->sin_addr.s_addr);
	printf("Server IP address: %d.%d.%d.%d\n",
	    ip[0], ip[1], ip[2], ip[3]);

	s->sin_port = nrf_htons(TCP_PORT);
	s->sin_len = sizeof(struct nrf_sockaddr_in);

	bzero(&local_addr, sizeof(struct nrf_sockaddr_in));
	local_addr.sin_family = NRF_AF_INET;
	local_addr.sin_port = nrf_htons(0);
	local_addr.sin_addr.s_addr = 0;
	local_addr.sin_len = sizeof(struct nrf_sockaddr_in);

	err = nrf_bind(fd, (struct nrf_sockaddr *)&local_addr,
	    sizeof(local_addr));
	if (err != 0)
		panic("Bind failed: %d\n", err);

	printf("Connecting to server...\n");
	err = nrf_connect(fd, s,
	    sizeof(struct nrf_sockaddr_in));
	if (err != 0)
		panic("TCP connect failed: err %d\n", err);

	printf("Successfully connected to the server\n");
}

static int __unused
check_ipaddr(char *buf)
{
	char *t;
	char *p;

	t = (char *)buf;

	printf("%s: %s\n", __func__, buf);

	p = strsep(&t, ",");
	if (p == NULL || strcmp(p, "+CGDCONT: 0") != 0)
		return (0);

	p = strsep(&t, ",");
	if (p == NULL || strcmp(p, "\"IP\"") != 0)
		return (0);

	p = strsep(&t, ",");
	if (p == NULL || strcmp(p, "\"\"") == 0)
		return (0);

	printf("APN: %s\n", p);

	p = strsep(&t, ",");
	if (p == NULL || strcmp(p, "\"\"") == 0)
		return (0);

	printf("IP: %s\n", p);

	/* Success */

	return (1);
}

static int
lte_wait(int fd)
{
	char buf[LC_MAX_READ_LENGTH];
	int len;
	char *t;
	char *p;

	printf("Awaiting registration in the LTE-M network...\n");

	while (1) {
		len = at_recv(fd, buf, LC_MAX_READ_LENGTH);
		if (len) {
			printf("recv: %s\n", buf);
			t = (char *)buf;
			p = strsep(&t, ",");
			if (p != NULL) {
				/* Check network registration status. */

				if (strcmp(p, "+CEREG: 3") == 0) {
					printf("Registration denied\n");
					return (-1);
				}

				if (strcmp(p, "+CEREG: 5") == 0) {
					printf("Registered, roaming.\n");
					break;
				}

				if (strcmp(p, "+CEREG: 1") == 0) {
					printf("Registered, home network.\n");
					break;
				}
			}
		}

		mdx_usleep(1000000);
	}

	lte_signal(fd);

	return (0);
}

static int
lte_connect(void)
{
	int fd;

	fd = nrf_socket(NRF_AF_LTE, NRF_SOCK_DGRAM, NRF_PROTO_AT);
	if (fd < 0) {
		printf("failed to create socket\n");
		return (-1);
	}

	printf("AT lte socket %d\n", fd);

	/* Switch to the flight mode. */
	at_cmd(fd, flight, AT_CMD_SIZE(flight));

	/* Read current system mode. */
	at_cmd(fd, systm_mode, AT_CMD_SIZE(systm_mode));

	/* Set new system mode */
	at_cmd(fd, catm1_gps, AT_CMD_SIZE(catm1_gps));

	/* GPS: nrf9160-DK only. */
	at_cmd(fd, magpio, AT_CMD_SIZE(magpio));
	at_cmd(fd, coex0, AT_CMD_SIZE(coex0));

	/* Switch to power saving mode as required for GPS to operate. */
	at_cmd(fd, psm_req, AT_CMD_SIZE(psm_req));

	at_cmd(fd, cind, AT_CMD_SIZE(cind));
	at_cmd(fd, edrx_req, AT_CMD_SIZE(edrx_req));

	/* Lock bands 3,4,13,20. */
	at_cmd(fd, lock_bands, AT_CMD_SIZE(lock_bands));

	/* Subscribe for events. */
	at_send(fd, subscribe, AT_CMD_SIZE(subscribe));

	/* Switch to normal mode. */
	at_cmd(fd, normal, AT_CMD_SIZE(normal));

	if (lte_wait(fd) == 0) {
		printf("LTE connected\n");
		connect_to_server();
	} else
		printf("Failed to connect to LTE\n");

	mdx_usleep(500000);

	/* Switch to GPS mode. */
	at_cmd(fd, lte_disable, AT_CMD_SIZE(lte_disable));

	mdx_usleep(500000);

	at_cmd(fd, edrx_disable, AT_CMD_SIZE(edrx_disable));

	mdx_usleep(500000);

	/* Switch to GPS mode. */
	at_cmd(fd, gps_enable, AT_CMD_SIZE(gps_enable));

	mdx_usleep(500000);

	return (0);
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
main(void)
{
	mdx_device_t uart;
	int error;

	uart = mdx_device_lookup_by_name("nrf_uarte", 0);
	if (!uart)
		panic("uart dev not found");
	nrf_uarte_register_callback(uart, nrf_input, NULL);

	nrf_modem_init(&init_params, NORMAL_MODE);

	printf("nrf_modem library initialized\n");

	buffer_fill = 0;
	ready_to_send = 0;

	lte_connect();

	error = gps_init();
	if (error)
		printf("Can't initialize GPS\n");
	else {
		printf("GPS initialized\n");
		gps_test();
	}

	while (1)
		mdx_usleep(1000000);

	return (0);
}
