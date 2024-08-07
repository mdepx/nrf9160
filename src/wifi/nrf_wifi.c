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

#include "nrf_wifi.h"
#include "config.h"

#include "nrfxlib/nrf_wifi/os_if/inc/osal_ops.h"
#include "nrfxlib/nrf_wifi/os_if/inc/osal_api.h"

static void *
mdx_shim_mem_alloc(size_t size)
{

	return (NULL);
}

static void *
mdx_shim_mem_zalloc(size_t size)
{

	return (NULL);
}

static void
mdx_shim_mem_free(void *buf)
{

}

static void *
mdx_shim_mem_cpy(void *dest, const void *src, size_t count)
{

	return memcpy(dest, src, count);
}

static void *
mdx_shim_mem_set(void *start, int val, size_t size)
{

	return memset(start, val, size);
}

static int
mdx_shim_mem_cmp(const void *addr1, const void *addr2, size_t size)
{

	return memcmp(addr1, addr2, size);
}

const struct nrf_wifi_osal_ops nrf_wifi_os_mdx_ops = {
	.mem_alloc = mdx_shim_mem_alloc,
	.mem_zalloc = mdx_shim_mem_zalloc,
	.mem_free = mdx_shim_mem_free,
	.mem_cpy = mdx_shim_mem_cpy,
	.mem_set = mdx_shim_mem_set,
	.mem_cmp = mdx_shim_mem_cmp,

#ifdef notyet
	.qspi_read_reg32 = mdx_shim_qspi_read_reg32,
	.qspi_write_reg32 = mdx_shim_qspi_write_reg32,
	.qspi_cpy_from = mdx_shim_qspi_cpy_from,
	.qspi_cpy_to = mdx_shim_qspi_cpy_to,

	.spinlock_alloc = mdx_shim_spinlock_alloc,
	.spinlock_free = mdx_shim_spinlock_free,
	.spinlock_init = mdx_shim_spinlock_init,
	.spinlock_take = mdx_shim_spinlock_take,
	.spinlock_rel = mdx_shim_spinlock_rel,

	.spinlock_irq_take = mdx_shim_spinlock_irq_take,
	.spinlock_irq_rel = mdx_shim_spinlock_irq_rel,

	.log_dbg = mdx_shim_pr_dbg,
	.log_info = mdx_shim_pr_info,
	.log_err = mdx_shim_pr_err,

	.llist_node_alloc = mdx_shim_llist_node_alloc,
	.llist_node_free = mdx_shim_llist_node_free,
	.llist_node_data_get = mdx_shim_llist_node_data_get,
	.llist_node_data_set = mdx_shim_llist_node_data_set,

	.llist_alloc = mdx_shim_llist_alloc,
	.llist_free = mdx_shim_llist_free,
	.llist_init = mdx_shim_llist_init,
	.llist_add_node_tail = mdx_shim_llist_add_node_tail,
	.llist_add_node_head = mdx_shim_llist_add_node_head,
	.llist_get_node_head = mdx_shim_llist_get_node_head,
	.llist_get_node_nxt = mdx_shim_llist_get_node_nxt,
	.llist_del_node = mdx_shim_llist_del_node,
	.llist_len = mdx_shim_llist_len,

	.nbuf_alloc = mdx_shim_nbuf_alloc,
	.nbuf_free = mdx_shim_nbuf_free,
	.nbuf_headroom_res = mdx_shim_nbuf_headroom_res,
	.nbuf_headroom_get = mdx_shim_nbuf_headroom_get,
	.nbuf_data_size = mdx_shim_nbuf_data_size,
	.nbuf_data_get = mdx_shim_nbuf_data_get,
	.nbuf_data_put = mdx_shim_nbuf_data_put,
	.nbuf_data_push = mdx_shim_nbuf_data_push,
	.nbuf_data_pull = mdx_shim_nbuf_data_pull,
	.nbuf_get_priority = mdx_shim_nbuf_get_priority,
	.nbuf_get_chksum_done = mdx_shim_nbuf_get_chksum_done,
	.nbuf_set_chksum_done = mdx_shim_nbuf_set_chksum_done,

	.tasklet_alloc = mdx_shim_work_alloc,
	.tasklet_free = mdx_shim_work_free,
	.tasklet_init = mdx_shim_work_init,
	.tasklet_schedule = mdx_shim_work_schedule,
	.tasklet_kill = mdx_shim_work_kill,

	.sleep_ms = k_msleep,
	.delay_us = k_usleep,
	.time_get_curr_us = mdx_shim_time_get_curr_us,
	.time_elapsed_us = mdx_shim_time_elapsed_us,

	.bus_qspi_init = mdx_shim_bus_qspi_init,
	.bus_qspi_deinit = mdx_shim_bus_qspi_deinit,
	.bus_qspi_dev_add = mdx_shim_bus_qspi_dev_add,
	.bus_qspi_dev_rem = mdx_shim_bus_qspi_dev_rem,
	.bus_qspi_dev_init = mdx_shim_bus_qspi_dev_init,
	.bus_qspi_dev_deinit = mdx_shim_bus_qspi_dev_deinit,
	.bus_qspi_dev_intr_reg = mdx_shim_bus_qspi_intr_reg,
	.bus_qspi_dev_intr_unreg = mdx_shim_bus_qspi_intr_unreg,
	.bus_qspi_dev_host_map_get = mdx_shim_bus_qspi_dev_host_map_get,

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	.timer_alloc = mdx_shim_timer_alloc,
	.timer_init = mdx_shim_timer_init,
	.timer_free = mdx_shim_timer_free,
	.timer_schedule = mdx_shim_timer_schedule,
	.timer_kill = mdx_shim_timer_kill,

	.bus_qspi_ps_sleep = mdx_shim_bus_qspi_ps_sleep,
	.bus_qspi_ps_wake = mdx_shim_bus_qspi_ps_wake,
	.bus_qspi_ps_status = mdx_shim_bus_qspi_ps_status,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	.assert = mdx_shim_assert,
	.strlen = mdx_shim_strlen,
#endif
};

void
mdx_nrf_wifi_init(void)
{

	nrf_wifi_osal_init(&nrf_wifi_os_mdx_ops);
}
