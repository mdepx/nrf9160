/*-
 * Copyright (c) 2019-2024 Ruslan Bukin <br@bsdpad.com>
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

#include "nrfx_errors.h"
#include "nrfx_ipc.h"

#define	NRF_MODEM_DEBUG
#undef	NRF_MODEM_DEBUG

#ifdef	NRF_MODEM_DEBUG
#define	dprintf(fmt, ...)	printf(fmt, ##__VA_ARGS__)
#else
#define	dprintf(fmt, ...)
#endif

#define NRF_MODEM_OS_SEM_MAX NRF_MODEM_OS_NUM_SEM_REQUIRED
#define NRF_MODEM_OS_MTX_MAX NRF_MODEM_OS_NUM_MUTEX_REQUIRED

struct sleeping_thread {
	struct entry node;
	mdx_sem_t sem;
	uint32_t context;
};

static struct entry sleeping_thread_list;
static mdx_device_t nvic;
static mdx_sem_t nrf_modem_os_semaphores[NRF_MODEM_OS_SEM_MAX];
static mdx_mutex_t nrf_modem_os_mutexes[NRF_MODEM_OS_MTX_MAX];
static uint8_t sem_used = 0;
static uint8_t mutex_used = 0;

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
nrf_modem_os_event_notify(uint32_t context)
{
	struct sleeping_thread *td;

	for (td = td_first(); td != NULL; td = td_next(td)) {
		if ((td->context == context) || (context == 0))
			mdx_sem_post(&td->sem);
	}
}

void
nrf_modem_os_shutdown(void)
{
	struct sleeping_thread *td;

	printf("%s\n", __func__);

	for (td = td_first(); td != NULL; td = td_next(td))
		mdx_sem_post(&td->sem);
}


static void
ipc_intr(void *arg, int irq)
{

	nrfx_ipc_irq_handler();
}

void
nrf_modem_os_init(void)
{

	dprintf("%s\n", __func__);

	list_init(&sleeping_thread_list);

	nvic = mdx_device_lookup_by_name("nvic", 0);
	if (!nvic)
		panic("could not find nvic device\n");

	mdx_intc_setup(nvic, ID_IPC, ipc_intr, NULL);
}

/*
 * Return values for the nrf_modem_os_timedwait():
 *
 *  0 The thread is woken before the timeout expired.
 *  -NRF_EAGAIN The timeout expired.
 *  -NRF_ESHUTDOWN Modem is not initialized, or was shut down.
 */

int32_t
nrf_modem_os_timedwait(uint32_t context, int32_t * p_timeout)
{
	struct sleeping_thread td;
	int val;
	int err;
	int tmout;

	if (!nrf_modem_is_initialized())
		return (-NRF_ESHUTDOWN);

	val = *p_timeout;
	if (val == 0) {
		mdx_thread_yield();
		return (-NRF_EAGAIN);
	}

	if (val < 0)
		tmout = 500000;
	else
		tmout = val * 1000;

	memset(&td, 0, sizeof(struct sleeping_thread));
	mdx_sem_init(&td.sem, 0);
	td.context = context;

	critical_enter();
	list_append(&sleeping_thread_list, &td.node);
	critical_exit();

	dprintf("%s: %d\n", __func__, tmout);

	err = mdx_sem_timedwait(&td.sem, tmout);

	critical_enter();
	list_remove(&td.node);
	critical_exit();

	if (!nrf_modem_is_initialized())
		return (-NRF_ESHUTDOWN);

	if (err == 0) {
		if (val < 0) {
			printf(";");
			return (0);
		}

		dprintf("%s: timeout\n", __func__);
		return (-NRF_EAGAIN);
	}

	return (0);
}

void
nrf_modem_os_errno_set(int errno_val)
{

	dprintf("%s: %d\n", __func__, errno_val);
}

void *
nrf_modem_os_alloc(size_t bytes)
{
	void *addr;

	dprintf("%s: %d\n", __func__, bytes);

	addr = malloc(bytes);

	return (addr);
}

void
nrf_modem_os_free(void *mem)
{

	dprintf("%s\n", __func__);

	free(mem);
}

void *
nrf_modem_os_shm_tx_alloc(size_t bytes)
{
	void *addr;

	dprintf("%s: %d\n", __func__, bytes);

	addr = malloc(bytes);

	return (addr);
}

void
nrf_modem_os_shm_tx_free(void *mem)
{

	dprintf("%s\n", __func__);

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

int
nrf_modem_os_sem_init(void **arg, unsigned int initial_count,
    unsigned int limit)
{
	mdx_sem_t *sem;
	int i;

	sem = *arg;

	for (i = 0; i < NRF_MODEM_OS_SEM_MAX; i++)
		if (&nrf_modem_os_semaphores[i] == sem)
			goto reinit;

	KASSERT(sem_used < NRF_MODEM_OS_SEM_MAX, ("Max semaphores reached."));

	sem = &nrf_modem_os_semaphores[sem_used++];

	*arg = sem;

reinit:
	mdx_sem_init(sem, initial_count);

	return (0);
}

void
nrf_modem_os_sem_give(void *arg)
{
	mdx_sem_t *sem;

	sem = arg;

	mdx_sem_post(sem);
}

int
nrf_modem_os_sem_take(void *arg, int timeout)
{
	mdx_sem_t *sem;
	int err;

	sem = arg;

	err = mdx_sem_timedwait(sem, timeout == -1 ? 0 : timeout * 1000);
	if (err == 0)
		return (-NRF_EAGAIN);

	return (0);
}

void
nrf_modem_os_log(int level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	printf(fmt, ap);
	printf("\n");
	va_end(ap);
}

int
nrf_modem_os_sleep(uint32_t timeout)
{

	if (timeout == NRF_MODEM_OS_NO_WAIT || timeout == NRF_MODEM_OS_FOREVER)
		return (-NRF_EINVAL);

	mdx_usleep(timeout * 1000);

	return (0);
}

int
nrf_modem_os_mutex_init(void **arg)
{
	mdx_mutex_t *mtx;
	int i;

	mtx = *arg;

	for (i = 0; i < NRF_MODEM_OS_MTX_MAX; i++)
		if (&nrf_modem_os_mutexes[i] == mtx)
			goto reinit;

	KASSERT(mutex_used < NRF_MODEM_OS_MTX_MAX, ("Max mutex reached."));

	mtx = &nrf_modem_os_mutexes[mutex_used++];

	*arg = mtx;

reinit:
	mdx_mutex_init(mtx);

	return (0);
}

int
nrf_modem_os_mutex_lock(void *mtx, int timeout)
{

	if (timeout == -1)
		mdx_mutex_lock(mtx);
	else
		mdx_mutex_timedlock(mtx, timeout * 1000);

	return (0);
}

int
nrf_modem_os_mutex_unlock(void *mtx)
{

	mdx_mutex_unlock(mtx);

	return (0);
}
