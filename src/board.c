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
#include <sys/of.h>
#include <sys/thread.h>
#include <sys/malloc.h>
#include <sys/console.h>

#include <arm/nordicsemi/nrf9160.h>

#include "board.h"

#define	EARLY_PRINTF
#undef	EARLY_PRINTF

static struct nrf_timer_softc *timer_sc;
static struct mdx_fl_zone mdx_fl_shm_tx_zone;

#ifdef EARLY_PRINTF
struct mdx_device uart;
#endif

void
udelay(uint32_t usec)
{

	nrf_timer_udelay(timer_sc, usec);
}

/*
 * nRF TX memory shared with modem. Must be below first 128k SRAM region.
 */
void *
shm_alloc(size_t size)
{
	void *ret;

	critical_enter();
	ret = mdx_fl_malloc(&mdx_fl_shm_tx_zone, size);
	critical_exit();

	return (ret);
}

void
shm_free(void *ptr)
{

	critical_enter();
	mdx_fl_free(&mdx_fl_shm_tx_zone, ptr);
	critical_exit();
}

void
board_init(void)
{
	mdx_device_t dev;

	/* Add some memory so OF could allocate devices and their softc. */
	malloc_init();
	malloc_add_region((void *)0x20030000, 0x10000);

	/* See comment provided for nrf_modem_init() argument. */
	mdx_fl_init(&mdx_fl_shm_tx_zone);
	mdx_fl_add_region(&mdx_fl_shm_tx_zone, (void *)0x20014800, 0x4000);

#ifdef EARLY_PRINTF
	nrf_uarte_init(&uart, 0x40008000, 29, 28);
	mdx_console_register_uart(&uart);
#endif

	mdx_of_install_dtbp((void *)0xf8000);
	mi_startup();

	/* Register console. */
	dev = mdx_device_lookup_by_name("nrf_uarte", 0);
	if (!dev)
		panic("uart dev not found");
	mdx_console_register_uart(dev);

	/* Enable the instruction cache. */
	dev = mdx_device_lookup_by_name("nrf_nvmc", 0);
	if (!dev)
		panic("nvmc dev not found");
	nrf_nvmc_icache_control(dev, true);

	dev = mdx_device_lookup_by_name("nrf_timer", 0);
	if (!dev)
		panic("timer dev not found");
	timer_sc = mdx_device_get_softc(dev);

	printf("mdepx initialized\n");
}
