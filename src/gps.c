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

#include <sys/mbuf.h>
#include <net/if.h>
#include <net/netinet/in.h>
#include <arpa/inet.h>

#include <arm/arm/nvic.h>
#include <arm/nordicsemi/nrf9160.h>

#include <nrfxlib/bsdlib/include/nrf_socket.h>
#include <nrfxlib/bsdlib/include/bsd.h>
#include <nrfxlib/bsdlib/include/bsd_os.h>

#include "gps.h"
#include "ftoa.h"

static int socket;

static void
print_stats(nrf_gnss_pvt_data_frame_t *pvt)
{
	nrf_gnss_sv_t *sv;
	int i;

	for (i = 0; i < NRF_GNSS_MAX_SATELLITES; i++) {
		sv = &pvt->sv[i];

		if (sv->sv > 0 && sv->sv <= 32) {
			printf("sv %d signal %d cn0 %d elev %d az %d flag %x\n",
			    sv->sv, sv->signal, sv->cn0, sv->elevation,
			    sv->azimuth, sv->flags);

			if (pvt->sv[i].flags & NRF_GNSS_SV_FLAG_USED_IN_FIX)
			    printf("sat in fix %d\n", pvt->sv[i].sv);
                        if (pvt->sv[i].flags & NRF_GNSS_SV_FLAG_UNHEALTHY)
			    printf("sat unhealthy %d\n", pvt->sv[i].sv);
		}

	}
}

static void __unused
test_thr(void *arg)
{
	char buf[100];
	float a;
	float b;
	int i;

	double d;

	while (1) {
		d = 0.125;
		ftoa(d, buf, 10);
		printf("d is: %s\n", buf);

		b = 0;
		a = 0.1;
		for (i = 0; i < 1000000; i++)
			b += a;

		ftoa(b, buf, 10);

		critical_enter();
		printf("result0 %s\n", buf);
		critical_exit();
	}
}

static void __unused
fpu_test(void)
{
	struct thread *td;
	char buf[100];
	float a;
	float b;
	int i;

	td = mdx_thread_create("test", 1, 10000, 4096, test_thr, NULL);
	mdx_sched_add(td);

	while (1) {
		b = 0;
		a = 0.5;
		for (i = 0; i < 1000000; i++)
			b += a;

		ftoa(b, buf, 10);

		critical_enter();
		printf("result1 %s\n", buf);
		critical_exit();
	}
}

int
gps_init(void)
{
	nrf_gnss_fix_retry_t fix_retry;
	nrf_gnss_fix_interval_t fix_interval;
	nrf_gnss_nmea_mask_t nmea_mask;
	nrf_gnss_delete_mask_t delete_mask;
	int error;

	socket = nrf_socket(NRF_AF_LOCAL, NRF_SOCK_DGRAM, NRF_PROTO_GNSS);
	if (socket < 0) {
		printf("can't make GPS socket\n");
		return (-1);
	}

	fix_retry = 0;
	fix_interval = 1;
	nmea_mask = NRF_GNSS_NMEA_GGA_MASK;
#if 0
	nmea_mask |= NRF_GNSS_NMEA_GSV_MASK |
		NRF_GNSS_NMEA_GSA_MASK |
		NRF_GNSS_NMEA_GLL_MASK |
		NRF_GNSS_NMEA_GGA_MASK |
		NRF_GNSS_NMEA_RMC_MASK;
#endif

	delete_mask = 0;

	error = nrf_setsockopt(socket,
				NRF_SOL_GNSS,
				NRF_SO_GNSS_FIX_RETRY,
				&fix_retry, sizeof(fix_retry));
	if (error) {
		printf("%s: Can't set fix retry: error %d\n", __func__, error);
		return (-1);
	}

	error = nrf_setsockopt(socket,
				NRF_SOL_GNSS,
				NRF_SO_GNSS_FIX_INTERVAL,
				&fix_interval,
				sizeof(fix_interval));
	if (error) {
		printf("%s: Can't set fix interval: error %d\n",
		    __func__, error);
		return (-1);
	}

	error = nrf_setsockopt(socket,
				NRF_SOL_GNSS,
				NRF_SO_GNSS_NMEA_MASK,
				&nmea_mask,
				sizeof(nmea_mask));
	if (error) {
		printf("%s: Can't set NMEA mask: error %d\n", __func__, error);
		return (-1);
	}

	error = nrf_setsockopt(socket,
				NRF_SOL_GNSS,
				NRF_SO_GNSS_START,
				&delete_mask,
				sizeof(delete_mask));
	if (error) {
		printf("%s: Can't set delete mask: error %d\n",
		    __func__, error);
		return (-1);
	}

	return (0);
}

int
gps_test(void)
{
	int len;
	nrf_gnss_data_frame_t raw_gps_data;
	nrf_gnss_pvt_data_frame_t *pvt;
	bool blocked;
	bool data_avail;
	char lat[32];
	char lon[32];

	blocked = false;
	data_avail = false;

	while (1) {
		len = nrf_recv(socket, &raw_gps_data,
		    sizeof(nrf_gnss_data_frame_t), 0);
		if (len <= 0)
			break;

		switch (raw_gps_data.data_id) {
		case NRF_GNSS_PVT_DATA_ID:
			pvt = &raw_gps_data.pvt;
			print_stats(pvt);
			if (pvt->flags &
			    NRF_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME) {
				blocked = true;
				printf("GPS blocked\n");
				break;
			}
			blocked = false;

			if (pvt->flags & NRF_GNSS_PVT_FLAG_DEADLINE_MISSED) {
				printf("pvt deadline missed\n");
				break;
			}

			if (pvt->flags & NRF_GNSS_PVT_FLAG_FIX_VALID_BIT) {
				ftoa(pvt->latitude, lat, -1);
				ftoa(pvt->longitude, lon, -1);
				printf("pvt data: latitude %s longitude %s\n",
				    lat, lon);
				data_avail = true;
			}

			break;
		case NRF_GNSS_NMEA_DATA_ID:
			if (blocked == false && data_avail == true) {
				printf("nmea data: %s\n", raw_gps_data.nmea);
				data_avail = false;
			}
			break;
		case NRF_GNSS_AGPS_DATA_ID:
			printf("agps data id\n");
			break;
		default:
			printf("unknown id %d\n", raw_gps_data.data_id);
			break;
		}
	}

	return (0);
}
