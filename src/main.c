/*-
 * Copyright (c) 2018-2020 Ruslan Bukin <br@bsdpad.com>
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

#include <nrfxlib/bsdlib/include/nrf_socket.h>
#include <nrfxlib/bsdlib/include/bsd.h>
#include <nrfxlib/bsdlib/include/bsd_os.h>

#include "board.h"

#define	LC_MAX_READ_LENGTH	128
#define	AT_CMD_SIZE(x)		(sizeof(x) - 1)

#define	TCP_HOST	"machdep.com"
#define	TCP_PORT	80

static const char cind[] __unused = "AT+CIND?";
static const char subscribe[] = "AT+CEREG=5";
static const char lock_bands[] __unused =
    "AT%XBANDLOCK=2,\"10000001000000001100\"";
static const char normal[] = "AT+CFUN=1";
static const char edrx_req[] __unused = "AT+CEDRXS=1,4,\"1000\"";
static const char cgact[] __unused = "AT+CGACT=1,1";
static const char cgatt[] __unused = "AT+CGATT=1";
static const char cgdcont[] __unused = "AT+CGDCONT?";
static const char cgdcont_req[] __unused =
    "AT+CGDCONT=1,\"IP\",\"ibasis.iot\"";
static const char cgpaddr[] __unused = "AT+CGPADDR";
static const char cesq[] __unused = "AT+CESQ";
static const char cpsms[] __unused = "AT+CPSMS=";

static const char nbiot[] __unused = "AT%XSYSTEMMODE=0,1,0,0";
static const char catm1[] __unused = "AT%XSYSTEMMODE=1,0,0,0";

static char buffer[LC_MAX_READ_LENGTH];
static int buffer_fill;
static int ready_to_send;

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

void
bsd_recoverable_error_handler(uint32_t error)
{

	printf("%s: error %d\n", __func__, error);
}

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

static void __unused
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
		mdx_tsleep(10000);
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

	fd = nrf_socket(NRF_AF_INET, NRF_SOCK_STREAM, 0);
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

	while (1)
		mdx_tsleep(1000000);
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
			if (p != NULL && strcmp(p, "+CEREG: 5") == 0)
				break;
		}

		mdx_tsleep(1000000);
	}

	/* Extended signal quality */
	at_send(fd, cesq, AT_CMD_SIZE(cesq));
	len = at_recv(fd, buf, LC_MAX_READ_LENGTH);
	if (len)
		printf("recv: %s\n", buf);

	return (0);
}

static void
lte_connect(void)
{
	int fd;

	fd = nrf_socket(NRF_AF_LTE, 0, NRF_PROTO_AT);
	if (fd < 0)
		printf("failed to create socket\n");

	at_cmd(fd, catm1, AT_CMD_SIZE(catm1));
	at_cmd(fd, cpsms, AT_CMD_SIZE(cpsms));
	at_cmd(fd, cind, AT_CMD_SIZE(cind));
	at_cmd(fd, edrx_req, AT_CMD_SIZE(edrx_req));

	/* Lock bands 3,4,13,20. */
	at_cmd(fd, lock_bands, AT_CMD_SIZE(lock_bands));

	/* Subscribe for events. */
	at_send(fd, subscribe, AT_CMD_SIZE(subscribe));

	/* Switch to normal mode. */
	at_send(fd, normal, AT_CMD_SIZE(normal));

	if (lte_wait(fd) == 0) {
		printf("LTE connected\n");
		connect_to_server();
	}
}

void
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

	bsd_init();

	printf("bsd library initialized\n");

	buffer_fill = 0;
	ready_to_send = 0;

	lte_connect();

	panic("lte_connect returned!\n");

	return (0);
}
