/*-
 * Copyright (c) 2019 Ruslan Bukin <br@bsdpad.com>
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

#include <arm/arm/nvic.h>
#include <arm/nordicsemi/nrf9160.h>

#include <nrfxlib/bsdlib/include/bsd_os.h>

#define	BSD_OS_DEBUG
#undef	BSD_OS_DEBUG

#ifdef	BSD_OS_DEBUG
#define	dprintf(fmt, ...)	printf(fmt, ##__VA_ARGS__)
#else
#define	dprintf(fmt, ...)
#endif

extern struct arm_nvic_softc nvic_sc;

void
bsd_os_init(void)
{

	dprintf("%s\n", __func__);

	arm_nvic_set_prio(&nvic_sc, ID_EGU1, 6);
	arm_nvic_enable_intr(&nvic_sc, ID_EGU1);

	arm_nvic_set_prio(&nvic_sc, ID_EGU2, 6);
	arm_nvic_enable_intr(&nvic_sc, ID_EGU2);
}

int32_t
bsd_os_timedwait(uint32_t context, int32_t * p_timeout)
{

	dprintf("%s: %p\n", __func__, p_timeout);

	return (0);
}

void
bsd_os_errno_set(int errno_val)
{

	dprintf("%s\n", __func__);
}

void
bsd_os_application_irq_clear(void)
{

	dprintf("%s\n", __func__);
	arm_nvic_clear_pending(&nvic_sc, ID_EGU1);
}

void
bsd_os_application_irq_set(void)
{

	dprintf("%s\n", __func__);
	arm_nvic_set_pending(&nvic_sc, ID_EGU1);
}

void
bsd_os_trace_irq_set(void)
{

	dprintf("%s\n", __func__);
	arm_nvic_set_pending(&nvic_sc, ID_EGU2);
}

void
bsd_os_trace_irq_clear(void)
{

	dprintf("%s\n", __func__);
	arm_nvic_clear_pending(&nvic_sc, ID_EGU2);
}

int32_t
bsd_os_trace_put(const uint8_t * const p_buffer, uint32_t buf_len)
{

	dprintf("%s\n", __func__);

	return (0);
}
