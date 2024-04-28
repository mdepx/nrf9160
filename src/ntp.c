/*-
 * Copyright (c) 2024 Ruslan Bukin <br@bsdpad.com>
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
#include <sys/callout.h>

#include <nrfxlib/nrf_modem/include/nrf_socket.h>
#include <arm/nordicsemi/nrf9160.h>
#include "ntp.h"

#define	UDP_HOST	"time1.google.com"
#define	UDP_PORT	123
#define	NTP_TO_UNIX	2208988800ULL

static struct nrf_addrinfo hints = {
	.ai_flags = NRF_AI_PDNSERV,
};

static struct nrf_timeval timeout = {
	.tv_sec = 5,
	.tv_usec = 0,
};

struct ntp_msg {
	uint8_t flags;
	uint8_t stratum;
	uint8_t poll;
	int8_t precision;
	uint32_t rootdelay;
	uint32_t rootdisp;
	uint32_t refid;
	uint32_t reftime_sec;
	uint32_t reftime_frac;
	uint32_t org_sec;
	uint32_t org_frac;
	uint32_t rec_sec;
	uint32_t rec_frac;
	uint32_t xmt_sec;
	uint32_t xmt_frac;
};

static int __unused
mdx_recv_timeout(int fd, void *buf, size_t len, uint32_t timeout)
{
	struct nrf_pollfd fds;
	int error;

	fds.fd = fd;
	fds.events = NRF_POLLIN;
	fds.revents = 0;

	printf("%s: len %d, timeout %d\n", __func__, len, timeout);

	error = nrf_poll(&fds, 1, timeout * 1000);

	printf("%s: nrf_poll returned %d\n", __func__, error);

	if (error == 0) /* timeout */
		return (-1);

	if (error < 0) /* error */
		return (-2);

	error = 0;

	if (fds.revents & NRF_POLLIN)
		error = nrf_recv(fd, buf, len, 0);

	return (error);
}

static int
ntp_connect(int *fd0, struct nrf_addrinfo *server_addr)
{
	struct nrf_sockaddr_in local_addr;
	struct nrf_sockaddr_in *s;
	uint8_t *ip;
	int pdn_id;
	int error;
	int fd;

	pdn_id = 0;

	fd = nrf_socket(NRF_AF_INET, NRF_SOCK_DGRAM, NRF_IPPROTO_UDP);
	if (fd < 0) {
		printf("%s: failed to create socket\n", __func__);
		return (-1);
	}

	*fd0 = fd;

	/* nrf_fcntl(fd, NRF_F_SETFL, NRF_O_NONBLOCK); */

	error = nrf_setsockopt(fd, NRF_SOL_SOCKET, NRF_SO_BINDTOPDN, &pdn_id,
	    sizeof(pdn_id));
	if (error) {
		printf("%s: could not bind to PDN\n", __func__);
		return (error);
	}

	/* Set some reasonable timeout for recvfrom(). */
	error = nrf_setsockopt(fd, NRF_SOL_SOCKET, NRF_SO_SNDTIMEO, &timeout,
	    sizeof(timeout));
	if (error) {
		printf("%s: could not set sock opt\n", __func__);
		return (error);
	}

	error = nrf_setsockopt(fd, NRF_SOL_SOCKET, NRF_SO_RCVTIMEO, &timeout,
	    sizeof(timeout));
	if (error) {
		printf("%s: could not set sock opt\n", __func__);
		return (error);
	}

	s = (struct nrf_sockaddr_in *)server_addr->ai_addr;
	ip = (uint8_t *)&(s->sin_addr.s_addr);
	printf("%s: NTP Server IP address: %d.%d.%d.%d\n", __func__,
	    ip[0], ip[1], ip[2], ip[3]);

	s->sin_port = nrf_htons(UDP_PORT);

	bzero(&local_addr, sizeof(struct nrf_sockaddr_in));
	local_addr.sin_family = NRF_AF_INET;
	local_addr.sin_port = nrf_htons(0);
	local_addr.sin_addr.s_addr = 0;

	error = nrf_bind(fd, (struct nrf_sockaddr *)&local_addr,
	    sizeof(local_addr));
	if (error) {
		printf("%s: could not bind to socket\n", __func__);
		return (error);
	}

	printf("%s: Connecting to NTP server...\n", __func__);

	error = nrf_connect(fd, (struct nrf_sockaddr *)s,
	    sizeof(struct nrf_sockaddr_in));
	if (error) {
		printf("%s: could not connect to NTP server\n", __func__);
		return (error);
	}

	printf("Successfully connected to NTP server %s\n", UDP_HOST);

	return (0);
}

int
ntp_main(void)
{
	struct nrf_addrinfo *server_addr;
	nrf_socklen_t addrlen;
	struct ntp_msg ntp;
	int error;
	int len;
	int fd;

	error = nrf_getaddrinfo(UDP_HOST, "pdn0", &hints, &server_addr);
	if (error != 0) {
		printf("%s: could not resolve %s, error %d\n", __func__,
		    UDP_HOST, error);
		return (error);
	}

	error = ntp_connect(&fd, server_addr);
	if (error) {
		printf("%s: could not connect to NTP server\n", __func__);
		return (error);
	}

	memset(&ntp, 0, sizeof(struct ntp_msg));
	ntp.flags = 0x1b;

	addrlen = sizeof(struct nrf_sockaddr);

	len = nrf_send(fd, &ntp, sizeof(struct ntp_msg), 0);
	if (len <= 0) {
		printf("%s: could not send NTP query\n", __func__);
		nrf_close(fd);
		return (error);
	}

	printf("%s: Waiting for NTP reply\n", __func__);

	len = nrf_recvfrom(fd, &ntp, sizeof(struct ntp_msg), 0,
	    (struct nrf_sockaddr *)server_addr->ai_addr, &addrlen);
	if (len <= 0) {
		printf("%s: could not receive NTP response, error %d\n",
		    __func__, len);
		nrf_close(fd);
		return (error);
	}

	printf("%s: Successfully received NTP response, len %d\n",
	    __func__, len);

	nrf_close(fd);

	return (0);
}
