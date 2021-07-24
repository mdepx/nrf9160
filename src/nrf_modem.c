/*-
 * Copyright (c) 2019-2020 Ruslan Bukin <br@bsdpad.com>
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
#include <sys/mutex.h>

#include <arm/arm/nvic.h>
#include <arm/nordicsemi/nrf9160.h>

#include <dev/intc/intc.h>

#include <nrfxlib/nrf_modem/include/nrf_modem_os.h>
#include <nrfxlib/nrf_modem/include/nrf_modem.h>
#include <nrfxlib/nrf_modem/include/nrf_errno.h>

#define	NRF_MODEM_DEBUG
#undef	NRF_MODEM_DEBUG

#ifdef	NRF_MODEM_DEBUG
#define	dprintf(fmt, ...)	printf(fmt, ##__VA_ARGS__)
#else
#define	dprintf(fmt, ...)
#endif

struct sleeping_thread {
	struct entry node;
	mdx_sem_t sem;
};

static struct mdx_mutex bsdos_mtx;
static struct entry sleeping_thread_list;
static mdx_device_t nvic;

static struct sleeping_thread *
td_first(void)
{
	struct sleeping_thread *td;

	if (list_empty(&sleeping_thread_list))
		return (NULL);

	td = CONTAINER_OF(sleeping_thread_list.next,
	    struct sleeping_thread, node);

	return (td);
}

static struct sleeping_thread *
td_next(struct sleeping_thread *td0)
{
	struct sleeping_thread *td;

	if (td0->node.next == &sleeping_thread_list)
		return (NULL);

	td = CONTAINER_OF(td0->node.next, struct sleeping_thread, node);

	return (td);
}

void
nrf_modem_recoverable_error_handler(uint32_t error)
{

	printf("%s: error %d\n", __func__, error);
}

static void
trace_proxy_intr(void *arg, int irq)
{

	nrf_modem_os_trace_irq_handler();
}

static void
rpc_proxy_intr(void *arg, int irq)
{
	struct sleeping_thread *td;

	dprintf(",");

	nrf_modem_os_application_irq_handler();

	for (td = td_first(); td != NULL; td = td_next(td))
		mdx_sem_post(&td->sem);
}

void
nrf_modem_os_init(void)
{

	dprintf("%s\n", __func__);

	mdx_mutex_init(&bsdos_mtx);
	list_init(&sleeping_thread_list);

	nvic = mdx_device_lookup_by_name("nvic", 0);
	if (!nvic)
		panic("could not find nvic device\n");

	mdx_intc_setup(nvic, ID_EGU1, rpc_proxy_intr, NULL);
	mdx_intc_set_prio(nvic, ID_EGU1, 6);
	mdx_intc_enable(nvic, ID_EGU1);

	mdx_intc_setup(nvic, ID_EGU2, trace_proxy_intr, NULL);
	mdx_intc_set_prio(nvic, ID_EGU2, 6);
	mdx_intc_enable(nvic, ID_EGU2);
}

int32_t
nrf_modem_os_timedwait(uint32_t context, int32_t * p_timeout)
{
	struct sleeping_thread td;
	int val;
	int err;
	int tmout;

	val = *p_timeout;
	if (val == 0) {
		mdx_thread_yield();
		return (NRF_ETIMEDOUT);
	}

	/*
	 * Note that rpc_proxy interrupt could fire right here.
	 * To handle that situation don't wait forever,
	 * just set some reasonable timeout.
	 */
	if (val < 0)
		tmout = 10000000; /* 10 sec. */
	else if (val > 0)
		tmout = val * 1000;

	mdx_sem_init(&td.sem, 0);

	critical_enter();
	list_append(&sleeping_thread_list, &td.node);
	critical_exit();

	dprintf("%s: %d\n", __func__, tmout);

	err = mdx_sem_timedwait(&td.sem, tmout);

	critical_enter();
	list_remove(&td.node);
	critical_exit();

	if (err == 0) {
		dprintf("%s: timeout\n", __func__);
		if (val == -1)
			return (0);
		return (NRF_ETIMEDOUT);
	}

	return (0);
}

void
nrf_modem_os_errno_set(int errno_val)
{

	dprintf("%s: %d\n", __func__, errno_val);
}

void
nrf_modem_os_application_irq_clear(void)
{

	dprintf("%s\n", __func__);
	mdx_intc_clear(nvic, ID_EGU1);
}

void
nrf_modem_os_application_irq_set(void)
{

	dprintf("%s\n", __func__);
	mdx_intc_set(nvic, ID_EGU1);
}

void
nrf_modem_os_trace_irq_set(void)
{

	dprintf("%s\n", __func__);
	mdx_intc_set(nvic, ID_EGU2);
}

void
nrf_modem_os_trace_irq_clear(void)
{

	dprintf("%s\n", __func__);
	mdx_intc_clear(nvic, ID_EGU2);
}

int32_t
nrf_modem_os_trace_put(const uint8_t * const p_buffer, uint32_t buf_len)
{

	dprintf("%s\n", __func__);

	return (0);
}

void *
nrf_modem_os_alloc(size_t bytes)
{
	void *addr;

	addr = malloc(bytes);

	return (addr);
}

void
nrf_modem_os_free(void *mem)
{

	free(mem);
}

void *
nrf_modem_os_shm_tx_alloc(size_t bytes)
{
	void *addr;

	addr = malloc(bytes);

	return (addr);
}

void
nrf_modem_os_shm_tx_free(void *mem)
{

	free(mem);
}

bool
nrf_modem_os_is_in_isr(void)
{

	/*
	 * TODO: should we have some flag set for intr thread?
	 * So we can use it here.
	 */

	return (curthread->td_critnest > 0);
}

void
nrf_modem_os_busywait(int32_t usec)
{

	udelay(usec);
}
