/*-
 * Copyright (c) 2025 Ruslan Bukin <br@bsdpad.com>
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
#include <sys/endian.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/tick.h>

#include <time.h>

#include <nrfxlib/nrf_modem/include/nrf_socket.h>
#include <lib/httpc/httpc.h>
#include "http.h"

typedef struct {
	size_t position;
	FILE *output;
} httpc_dump_t;

static struct nrf_addrinfo hints __unused = {
	.ai_flags = NRF_AI_PDNSERV,
};

static uint64_t time_started;

int
httpc_open(httpc_options_t *os, void **socket, void *opts, const char * domain,
    unsigned short port, int use_ssl)
{
	struct nrf_addrinfo *server_addr;
	struct nrf_sockaddr_in local_addr;
	struct nrf_sockaddr_in *s;
	uint8_t *ip;
	int pdn_id;
	int error;
	int fd;

	printf("%s: domain '%s' port %d use_ssl %d\n", __func__, domain, port,
	    use_ssl);

	pdn_id = 0;

	fd = nrf_socket(NRF_AF_INET, NRF_SOCK_STREAM, NRF_IPPROTO_TCP);
	if (fd < 0)
		panic("failed to create socket");

	nrf_setsockopt(fd, NRF_SOL_SOCKET, NRF_SO_BINDTOPDN, &pdn_id,
	    sizeof(pdn_id));

	error = nrf_getaddrinfo(domain, "pdn0", &hints, &server_addr);
	if (error != 0)
		panic("getaddrinfo failed with error %d\n", error);

	s = (struct nrf_sockaddr_in *)server_addr->ai_addr;
	ip = (uint8_t *)&(s->sin_addr.s_addr);
	printf("Server IP address: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

	s->sin_port = nrf_htons(port);

	bzero(&local_addr, sizeof(struct nrf_sockaddr_in));
	local_addr.sin_family = NRF_AF_INET;
	local_addr.sin_port = nrf_htons(0);
	local_addr.sin_addr.s_addr = 0;

	error = nrf_bind(fd, (struct nrf_sockaddr *)&local_addr,
	    sizeof(local_addr));
	if (error != 0)
		panic("Bind failed: %d\n", error);

	printf("Connecting to server...\n");

	error = nrf_connect(fd, (struct nrf_sockaddr *)s,
	    sizeof(struct nrf_sockaddr_in));
	if (error != 0)
		panic("TCP connect failed: error %d\n", error);

	printf("Successfully connected to the server\n");

	*socket = (void *)fd;

	return (0);
}

int
httpc_close(httpc_options_t *os, void *socket)
{

	printf("%s\n", __func__);

	return (0);
}

static void *
httpc_allocator(void *arena, void *ptr, size_t oldsz, size_t newsz)
{

	printf("%s\n", __func__);

	return (NULL);
}

int
httpc_read(httpc_options_t *os, void *socket, unsigned char *buf,
    size_t *length)
{
	int error;
	int fd;

	fd = (int)socket;

	error = nrf_recv(fd, buf, *length, 0);
	if (error >= 0) {
		*length = error;
		return (0);
	}

	printf("%s: error %d\n", __func__, error);

	return (error);
}

int
httpc_write(httpc_options_t *os, void *socket, const unsigned char *buf,
    size_t *length)
{
	int error;
	int fd;
	//int len;

	fd = (int)socket;

	//len = strlen(buf);

	printf("%s: sending len %d data %s\n", __func__, *length, buf);

	error = nrf_send(fd, buf, *length, 0);

	printf("%s: error %d\n", __func__, error);

	if (error > 0)
		return (0);

	return (error);
}

int
httpc_sleep(httpc_options_t *os, unsigned long milliseconds)
{

	printf("%s\n", __func__);

	return (0);
}

int
httpc_time(httpc_options_t *os, unsigned long *millisecond)
{

	printf("%s\n", __func__);

	return (0);
}

int
httpc_logger(httpc_options_t *os, void *file, const char *fmt, va_list ap)
{

	printf("%s\n", __func__);

	return (0);
}

static httpc_options_t a __unused = {
	.allocator	= httpc_allocator,
	.open		= httpc_open,
	.close		= httpc_close,
	.read		= httpc_read,
	.write		= httpc_write,
	.sleep		= httpc_sleep,
	.time		= httpc_time,
	.logger		= httpc_logger,
	.arena		= NULL,
	.socketopts	= NULL,
	.logfile	= NULL,
};

static int
httpc_dump_cb(void *param, unsigned char *buf, size_t length, size_t position)
{
	uint32_t delta;
	uint32_t speed;
	uint32_t kbit;

	delta = mdx_uptime() / 1000000;
	//printf("delta %d, time_started %d\n", delta, time_started);
	delta -= time_started;

	kbit = position * 8; /* bit */
	kbit /= 1000;

	speed = kbit / delta;

	printf("speed %d kbit/s, position %d\n", speed, position);

	return (0);
}

int
httpc_main(void)
{

	httpc_dump_t d = { .position = 0, .output = NULL, };

	time_started = mdx_uptime() / 1000000;

	httpc_get(&a, "http://xdma.org/static/128mb", httpc_dump_cb, &d);

	return (0);
}
