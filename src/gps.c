/*-
 * Copyright (c) 2020-2021 Ruslan Bukin <br@bsdpad.com>
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

#include <ftoa/ftoa.h>

#include <nrfxlib/nrf_modem/include/nrf_modem_gnss.h>
#include "gps.h"

/* GNSS delete mask */
#define	GNSS_DEL_EPHEMERIDES		(1 << 0)
#define	GNSS_DEL_ALMANAC		(1 << 1)
#define	GNSS_DEL_IONOSPHERIC		(1 << 2)
#define	GNSS_DEL_LGF			(1 << 3) /* Last good fix */
#define	GNSS_DEL_GPS_TOW		(1 << 4) /* Time of week */
#define	GNSS_DEL_GPS_WN			(1 << 5) /* Week number */
#define	GNSS_DEL_LEAP_SECOND		(1 << 6)
#define	GNSS_DEL_LOCAL_CLOCK_FOD	(1 << 7) /* Frequency offset data */

static void gnss_event_handler(int event);

static void
print_stats(struct nrf_modem_gnss_pvt_data_frame *pvt)
{
	struct nrf_modem_gnss_sv *sv;
	int i;

	for (i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; i++) {
		sv = &pvt->sv[i];

		if (sv->sv > 0 && sv->sv <= 32) {
			printf("sv %d signal %d cn0 %d elev %d az %d flag %x\n",
			    sv->sv, sv->signal, sv->cn0, sv->elevation,
			    sv->azimuth, sv->flags);

			if (pvt->sv[i].flags &
			    NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX)
				printf("sat in fix %d\n", pvt->sv[i].sv);
                        if (pvt->sv[i].flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY)
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
	d = 0.125;
	ftoa(d, buf, 10);
	printf("d is: %s\n", buf);

	while (1) {
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
	int fix_retry;
	int fix_interval;
	int nmea_mask;
	int error;

	nrf_modem_gnss_event_handler_set(gnss_event_handler);

	/* Continuous tracking. */
	fix_retry = 0;
	fix_interval = 1;

	nmea_mask = NRF_MODEM_GNSS_NMEA_GGA_MASK;
#if 0
	nmea_mask |= NRF_MODEM_GNSS_NMEA_GSV_MASK |
		NRF_MODEM_GNSS_NMEA_GSA_MASK |
		NRF_MODEM_GNSS_NMEA_GLL_MASK |
		NRF_MODEM_GNSS_NMEA_GGA_MASK |
		NRF_MODEM_GNSS_NMEA_RMC_MASK;
#endif

	error = nrf_modem_gnss_fix_retry_set(fix_retry);
	if (error) {
		printf("%s: Can't set fix retry: error %d\n", __func__, error);
		return (-1);
	}

	error = nrf_modem_gnss_fix_interval_set(fix_interval);
	if (error) {
		printf("%s: Can't set fix interval: error %d\n",
		    __func__, error);
		return (-1);
	}

	error = nrf_modem_gnss_nmea_mask_set(nmea_mask);
	if (error) {
		printf("%s: Can't set NMEA mask: error %d\n", __func__, error);
		return (-1);
	}

	nrf_modem_gnss_start();

	return (0);
}

static void
gnss_event_handler(int event)
{
	struct nrf_modem_gnss_nmea_data_frame nmea_data;
	struct nrf_modem_gnss_pvt_data_frame pvt;
	bool blocked;
	bool data_avail;
	char lat[32];
	char lon[32];

	blocked = false;
	data_avail = false;

	switch (event) {
	case NRF_MODEM_GNSS_EVT_PVT:
		nrf_modem_gnss_read(&pvt, sizeof(pvt),
		    NRF_MODEM_GNSS_DATA_PVT);
		print_stats(&pvt);
		if (pvt.flags &
		    NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME) {
			blocked = true;
			printf("GPS blocked\n");
			break;
		}
		blocked = false;

		if (pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_DEADLINE_MISSED) {
			printf("pvt deadline missed\n");
			break;
		}

		if (pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
			ftoa(pvt.latitude, lat, -1);
			ftoa(pvt.longitude, lon, -1);
			printf("pvt data: latitude %s longitude %s\n",
			    lat, lon);
			data_avail = true;
		}

		break;
	case NRF_MODEM_GNSS_EVT_NMEA:
		if (blocked == false && data_avail == true) {
			printf("nmea data: %s\n", nmea_data.nmea_str);
			data_avail = false;
		}
		break;
	case NRF_MODEM_GNSS_EVT_AGPS_REQ:
		printf("agps data id\n");
		break;
	default:
		printf("unknown event id %d\n", event);
		break;
	}
}
