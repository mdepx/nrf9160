/*-
 * Copyright (c) 2024 Ruslan Bukin <br@bsdpad.com>
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

#include <lib/mbedtls/include/mbedtls/bignum.h>

#include "math.h"

int get_random_number(uint8_t *out, int size);

static uint64_t
rand(void)
{
	uint64_t num;

	get_random_number((uint8_t *)&num, 8);

	printf("%s: %lu\n", __func__, num);

	return (num);
}

static uint64_t
gcd(uint64_t a, uint64_t b)
{

	return (b ? gcd (b, a % b) : a);
}

static int
decompose_64bit(uint64_t pq, uint64_t *p, uint64_t *q0)
{
	uint64_t g;
	uint64_t x, y;
	uint64_t a, b, c;
	uint64_t t, z;
	int iter;
	int i;
	int lim;
	int j, q;

	iter = 0;
	g = 0;

	for (i = 0; i < 3 || iter < 1000; i++) {
		q = ((rand() & 15) + 17) % pq;
		x = rand() % (pq - 1) + 1;
		y = x;
		lim = 1 << (i + 18);
		for (j = 1; j < lim; j++) {
			++iter;
			a = x;
			b = x;
			c = q;
			while (b) {
				if (b & 1) {
					c += a;
					if (c >= pq)
						c -= pq;
				}
				a += a;
				if (a >= pq)
					a -= pq;
				b >>= 1;
			}
			x = c;
			z = x < y ? pq + x - y : x - y;
			g = gcd(z, pq);
			if (g != 1)
				break;
			if (!(j & (j - 1)))
				y = x;
		}
		if (g > 1 && g < pq)
			break;
	}

	*p = g;
	*q0 = pq / g;

	if (*p > *q0) {
		t = *p;
		*p = *q0;
		*q0 = t;
	}

	return (0);
}

int
math_test(void)
{
	uint64_t pq;
	uint64_t p, q;
	int error;

	pq = 2341224282031054919ULL;

	printf("%s: decomposing %lld ...\n", __func__, pq);

	error = decompose_64bit(pq, &p, &q);
	if (error)
		return (error);

	printf("%s: p %lld q %lld\n", __func__, p, q);

	if ((p * q) == pq)
		return (0);

	return (-1);
}
