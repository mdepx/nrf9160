/*-
 * Copyright (c) 2018-2024 Ruslan Bukin <br@bsdpad.com>
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
#include <nrfxlib/nrf_modem/include/nrf_modem_at.h>

#include "gps.h"
#include "lcd.h"
#include "ntp.h"

#define	AT_RESPONSE_LEN		128
#define	LC_MAX_READ_LENGTH	128
#define	AT_CMD_SIZE(x)		(sizeof(x) - 1)

#define	TCP_HOST	"machdep.com"
#define	TCP_PORT	80

extern struct nrf_uarte_softc uarte_sc;

static const char cind[] __unused = "AT+CIND?";
static const char subscribe[] = "AT+CEREG=5";
static const char lock_bands[] __unused =
    "AT%%XBANDLOCK=2,\"10000001000000001100\"";
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

/* Power Save Mode.*/
static const char psm_req[] = "AT+CPSMS=1,,,\"00000110\",\"00000000\"";

/* Request eDRX to be disabled */
static const char edrx_disable[] = "AT+CEDRXS=3";

static const char magpio[] __unused = "AT%%XMAGPIO=1,0,0,1,1,1574,1577";
static const char coex0[] __unused = "AT%%XCOEX0=1,1,1565,1586";

/*
 * %XSYSTEMMODE=<M1_support>,<NB1_support>,<GNSS_support>,<LTE_preference>
 */

static const char systm_mode[] __unused = "AT%%XSYSTEMMODE?";
static const char nbiot_gps[] __unused = "AT%%XSYSTEMMODE=0,1,1,0";
static const char catm1_gps[] __unused = "AT%%XSYSTEMMODE=1,0,1,0";

static char buffer[LC_MAX_READ_LENGTH];
static int buffer_fill;
static int ready_to_send;

#define	NRF_MODEM_OS_SHMEM_CTRL_ADDR	0x20010000
#define	NRF_MODEM_OS_SHMEM_CTRL_SIZE	NRF_MODEM_SHMEM_CTRL_SIZE
#define	NRF_MODEM_OS_SHMEM_TX_ADDR	0x20011000
#define	NRF_MODEM_OS_SHMEM_TX_SIZE	0x7800
#define	NRF_MODEM_OS_SHMEM_RX_ADDR	0x20018800
#define	NRF_MODEM_OS_SHMEM_RX_SIZE	0x7800

CTASSERT(NRF_MODEM_OS_SHMEM_CTRL_SIZE <= 0x1000);

enum {
	LTE_CEREG_NOT_REGISTERED	= 0,
	LTE_CEREG_REGISTERED_HOME	= 1,
	LTE_CEREG_SEARCHING		= 2,
	LTE_CEREG_REGISTRATION_DENIED	= 3,
	LTE_CEREG_UNKNOWN		= 4,
	LTE_CEREG_REGISTERED_ROAMING	= 5,
	LTE_CEREG_UICC_FAIL		= 90
};

static void
nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info)
{

	printf("%s: implement me\n", __func__);
}

static void nrf_modem_lib_dfu_handler(uint32_t dfu_res)
{

	printf("%s: implement me\n", __func__);
}

static struct nrf_modem_init_params init_params = {
	.ipc_irq_prio = 0,
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
	.fault_handler = nrf_modem_fault_handler,
	.dfu_handler = nrf_modem_lib_dfu_handler,
};

static char buf[AT_RESPONSE_LEN];

static void
lte_signal_quality_decode(const char *at_s)
{
	float rsrq;
	int rsrp;
	char *p;
	char *t;

	strncpy((char *)buf, at_s, AT_RESPONSE_LEN);

	t = buf;

	p = strsep(&t, ",");	/* +CESQ: rxlev */
	p = strsep(&t, ",");	/* ber */
	p = strsep(&t, ",");	/* rscp */
	p = strsep(&t, ",");	/* echo */
	p = strsep(&t, ",");	/* rsrq */

	rsrq = 20 - atoi(p) / 2;
	p = strsep(&t, ",");
	rsrp = 140 - atoi(p) + 1;

	printf("LTE signal quality: rsrq -%.2f dB rsrp -%d dBm\n", rsrq, rsrp);
}

static int
lte_signal(void)
{

	/* Extended signal quality */
	int err;
	err = nrf_modem_at_cmd_async(lte_signal_quality_decode, cesq);
	if (err) {
		printf("%s: could not get signal quality err %d\n",
		    __func__, err);
		return (-1);
	}

	return (0);
}

#define	PORT_MAX_SIZE	5 /* 0xFFFF = 65535 */
#define	PDN_ID_MAX_SIZE	2 /* 0..10 */

static struct nrf_addrinfo hints = {
	.ai_flags = NRF_AI_PDNSERV,
};

static void
connect_to_server(void)
{
	struct nrf_addrinfo *server_addr;
	struct nrf_sockaddr_in local_addr;
	struct nrf_sockaddr_in *s;
	uint8_t *ip;
	int pdn_id;
	int err;
	int fd;

	pdn_id = 0;

	fd = nrf_socket(NRF_AF_INET, NRF_SOCK_STREAM, NRF_IPPROTO_TCP);
	if (fd < 0)
		panic("failed to create socket");

	nrf_setsockopt(fd, NRF_SOL_SOCKET, NRF_SO_BINDTOPDN, &pdn_id,
	    sizeof(pdn_id));

	err = nrf_getaddrinfo(TCP_HOST, "pdn0", &hints, &server_addr);
	if (err != 0)
		panic("getaddrinfo failed with error %d\n", err);

	s = (struct nrf_sockaddr_in *)server_addr->ai_addr;
	ip = (uint8_t *)&(s->sin_addr.s_addr);
	printf("Server IP address: %d.%d.%d.%d\n",
	    ip[0], ip[1], ip[2], ip[3]);

	s->sin_port = nrf_htons(TCP_PORT);

	bzero(&local_addr, sizeof(struct nrf_sockaddr_in));
	local_addr.sin_family = NRF_AF_INET;
	local_addr.sin_port = nrf_htons(0);
	local_addr.sin_addr.s_addr = 0;

	err = nrf_bind(fd, (struct nrf_sockaddr *)&local_addr,
	    sizeof(local_addr));
	if (err != 0)
		panic("Bind failed: %d\n", err);

	printf("Connecting to server...\n");

	err = nrf_connect(fd, (struct nrf_sockaddr *)s,
	    sizeof(struct nrf_sockaddr_in));
	if (err != 0)
		panic("TCP connect failed: err %d\n", err);

	printf("Successfully connected to the server\n");

	nrf_close(fd);
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
lte_wait(void)
{
	uint32_t cell_id;
	uint16_t status;
	int err;

	printf("Awaiting registration in the LTE-M network...\n");

	cell_id = 0;

	while (1) {
		err = nrf_modem_at_scanf("AT+CEREG?",
			"+CEREG: "
			"%*u,"          /* <n> */
			"%u,"           /* <stat> */
			"%*[^,],"       /* <tac> */
			"\"%x\",",      /* <ci> */
			&status, &cell_id);
		printf("%s: err %d, status %d, cell_id %x\n",
		    __func__, err, status, cell_id);

		switch (status) {
		case LTE_CEREG_NOT_REGISTERED:
			printf("Not registered\n");
			break;
		case LTE_CEREG_REGISTERED_HOME:
			printf("Registered, home network.\n");
			return (0);
		case LTE_CEREG_SEARCHING:
			printf("Searching for a network...\n");
			break;
		case LTE_CEREG_REGISTRATION_DENIED:
			printf("Registration denied\n");
			return (1);
		case LTE_CEREG_UNKNOWN:
			printf("Status is unknown\n");
			break;
		case LTE_CEREG_REGISTERED_ROAMING:
			printf("Registered, roaming.\n");
			return (0);
		case LTE_CEREG_UICC_FAIL:
			printf("SIM card failure.\n");
			return (1);
		}

		mdx_usleep(1000000);
	}

	return (0);
}

static int
lte_at(const char *cmd)
{
	int err;

	err = nrf_modem_at_printf(cmd);
	if (err)
		printf("%s: Could not issue cmd %s, err %d\n", __func__,
		    cmd, err);

	return (err);
}

static int
lte_connect(void)
{

	/* Switch to the flight mode. */
	lte_at(flight);

	/* Read current system mode. */
	lte_at(systm_mode);

	/* Set new system mode */
	lte_at(catm1_gps);

	/* GPS: NRF9160-DK only. */
	lte_at(magpio);

	/* GPS: NRF9160-DK and Thingy91 onboard antennas. */
	lte_at(coex0);

	/* Switch to power saving mode as required for GPS to operate. */
	lte_at(psm_req);

	lte_at(cind);
	lte_at(edrx_req);

	/* Lock bands 3,4,13,20. */
	lte_at(lock_bands);

	/* Subscribe for events. */
	lte_at(subscribe);

	/* Switch to normal mode. */
	lte_at(normal);

	if (lte_wait() == 0) {
		printf("LTE connected\n");
		lte_signal();
		ntp_main();
		connect_to_server();
	} else
		printf("Failed to connect to LTE\n");

	mdx_usleep(500000);

	/* Switch to GPS mode. */
	lte_at(lte_disable);

	mdx_usleep(500000);

	lte_at(edrx_disable);

	mdx_usleep(500000);

	/* Switch to GPS mode. */
	lte_at(gps_enable);

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

static void
callback(const char *notif)
{

	printf("%s: %s\n", __func__, notif);
}

int
main(void)
{
	mdx_device_t uart;
	char * version;
	int error;

#if 0
	lcd_init();
#endif

	uart = mdx_device_lookup_by_name("nrf_uarte", 0);
	if (!uart)
		panic("uart dev not found");
	nrf_uarte_register_callback(uart, nrf_input, NULL);

	error = nrf_modem_init(&init_params);
	if (error)
		panic("could not initialize nrf_modem library, error %d\n",
		    error);

	printf("nrf_modem library initialized.\n");

	nrf_modem_at_notif_handler_set(callback);

	version = nrf_modem_build_version();
	printf("nrf_modem version: %s\n", version);

	buffer_fill = 0;
	ready_to_send = 0;

	lte_connect();

	error = gps_init();
	if (error)
		printf("Can't initialize GPS\n");
	else
		printf("GPS initialized\n");

	while (1)
		mdx_usleep(1000000);

	return (0);
}
