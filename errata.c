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
#include <arm/nordicsemi/nrf9160.h>
#include "errata.h"

extern struct arm_nvic_softc nvic_sc;
extern struct power_softc power_sc;

static bool
errata_6(void)
{

	if (*(volatile uint32_t *)0x00ff0130 == 0x9)
		if (*(volatile uint32_t *)0x00ff0134 == 0x1)
			return (true);

	if (*(volatile uint32_t *)0x00ff0134 == 0x2)
		return (true);

	return (false);
}

static bool
errata_14(void)
{

	if (*(volatile uint32_t *)0x00ff0130 == 0x9)
		if (*(volatile uint32_t *)0x00ff0134 == 0x1)
			return (true);

	return (false);
}

static bool
errata_15(void)
{

	if (*(volatile uint32_t *)0x00ff0130 == 0x9)
		if (*(volatile uint32_t *)0x00ff0134 == 0x2)
			return (true);

	return (false);
}

void
errata_init(void)
{

	if (errata_6())
		power_reset_events(&power_sc);

	if (errata_14()) {
		/* ldo workaround */
		*(volatile uint32_t *)0x50004A38 = 1;
		*(volatile uint32_t *)0x50004578 = 1;
	}

	if (errata_15()) {
		*(volatile uint32_t *)0x50004A38 = 0;
		*(volatile uint32_t *)0x50004578 = 1;
	}
}
