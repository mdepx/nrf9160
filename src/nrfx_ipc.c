/*-
 * Copyright (c) 2020 Ruslan Bukin <br@bsdpad.com>
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

#include <dev/intc/intc.h>

#include <arm/nordicsemi/nrf9160.h>

#include "nrfx_errors.h"
#include "nrfx_ipc.h"

struct ipc_control_block {
	nrfx_ipc_handler_t handler;
	void *p_context;
	int state;
};

static struct ipc_control_block m_ipc_cb;

static void
cb(void *arg, int ev_id)
{

	m_ipc_cb.handler((1 << ev_id), m_ipc_cb.p_context);
}

void
nrfx_ipc_config_load(nrfx_ipc_config_t const * p_config)
{
	mdx_device_t dev;
	int i;

	dev = mdx_device_lookup_by_name("nrf_ipc", 0);

	for (i = 0; i < IPC_CONF_NUM; i++)
		nrf_ipc_configure_send(dev, i, p_config->send_task_config[i]);
	for (i = 0; i < IPC_CONF_NUM; i++)
		nrf_ipc_configure_recv(dev, i,
		    p_config->receive_event_config[i], cb, NULL);

	nrf_ipc_inten_chanmask(dev, p_config->receive_events_enabled, true);
}

void
nrfx_ipc_receive_event_disable(uint8_t event_index)
{
	mdx_device_t dev;

	dev = mdx_device_lookup_by_name("nrf_ipc", 0);
	if (dev == NULL)
		panic("IPC device not found. Check your DTB\n");

	nrf_ipc_inten(dev, event_index, false);
}

nrfx_err_t
nrfx_ipc_init(uint8_t irq_priority, nrfx_ipc_handler_t handler,
    void *p_context)
{
	mdx_device_t dev;
	mdx_device_t nvic;

	nvic = mdx_device_lookup_by_name("nvic", 0);
	if (nvic == NULL)
		panic("NVIC not found. Check your DTB\n");

	dev = mdx_device_lookup_by_name("nrf_ipc", 0);
	if (dev == NULL)
		panic("IPC device not found. Check your DTB\n");

	m_ipc_cb.handler = handler;
	m_ipc_cb.p_context = p_context;

	return (NRFX_SUCCESS);
}

void
nrfx_ipc_uninit(void)
{
	mdx_device_t dev;
	int i;

	dev = mdx_device_lookup_by_name("nrf_ipc", 0);

	for (i = 0; i < IPC_CONF_NUM; i++)
		nrf_ipc_configure_send(dev, i, 0);
	for (i = 0; i < IPC_CONF_NUM; i++)
		nrf_ipc_configure_recv(dev, i, 0, NULL, NULL);

	nrf_ipc_inten_chanmask(dev, 0xffffffff, false);
}
