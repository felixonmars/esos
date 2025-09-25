/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rtconfig.h>
#include "spacemit_rproc.h"

/* Place resource table in special ELF section */
#define __section_t(S)          __attribute__((__section__(#S)))
#define __resource              __section_t(.resource_table)

/* VirtIO rpmsg device id */
#define VIRTIO_ID_RPMSG_             7

/* Remote supports Name Service announcement */
#define VIRTIO_RPMSG_F_NS		0
#define VIRTIO_F_ACCESS_PLATFORM	1

#define NUM_VRINGS                  0x02
#define VRING_ALIGN                 0x1000
#define RING_TX                     FW_RSC_U32_ADDR_ANY
#define RING_RX                     FW_RSC_U32_ADDR_ANY
#define VRING_SIZE                  256

#define NUM_TABLE_ENTRIES           1

struct remote_resource_table __resource resources[1][1] = {
	{
		{
			/* Version */
			1,

			/* NUmber of table entries */
			NUM_TABLE_ENTRIES,

			/* reserved fields */
			{ 0, 0, },

			/* Offsets of rsc entries */
			{
				 offsetof(struct remote_resource_table, rpmsg_vdev),
			},

			/* Virtio device entry */
			{
#if defined(SOC_SPACEMIT_K1_X)
				RSC_VDEV, VIRTIO_ID_RPMSG_, 0, (1 << VIRTIO_RPMSG_F_NS), (1 << VIRTIO_F_ACCESS_PLATFORM), 0, 0,
				NUM_VRINGS, {0, 0},
#elif defined(SOC_SPACEMIT_K3)
				RSC_VDEV, VIRTIO_ID_RPMSG_, 0, (1 << VIRTIO_RPMSG_F_NS), 0, 0, 0,
				NUM_VRINGS, {0, 0},
#endif
			},

			/* Vring rsc entry - part of vdev rsc entry */
			{ RING_TX, VRING_ALIGN, VRING_SIZE, 0, 0 },

			{ RING_RX, VRING_ALIGN, VRING_SIZE, 1, 0 },
		},
	},
};

static struct dtb_compatible_array __compatible[] = {
#if defined(SOC_SPACEMIT_K1_X)
	{ .compatible = "spacemit,k1x-rproc0" },
#elif defined(SOC_SPACEMIT_K3)
	{ .compatible = "spacemit,k3-rproc0" },
#endif
	{}
};

struct spacemit_rproc {
	struct dtb_node *node;
	metal_phys_addr_t shmem_pa_base;
	rt_uint32_t shmem_buf_offset;
	rt_uint32_t shmem_size;
	rt_uint32_t procid, rscid, vdevid;
	struct rpmsg_virtio_shm_pool shpool;

	rt_event_t event;
	struct remoteproc *rproc;
	struct remoteproc_priv *priv;
	struct rpmsg_device *rpmsgdev;
};

static void *get_resource_table (rt_int32_t proc_id, rt_int32_t rsc_id, rt_int32_t *len)
{
	*len = sizeof(resources[proc_id][rsc_id]);
	return &resources[proc_id][rsc_id];
}

static void rproc_rx_callback(struct mbox_client *cl, void *data)
{
	struct remoteproc_priv *mb = rt_container_of(cl, struct remoteproc_priv, client);

	rt_event_send(mb->event, mb->evtype);
}

static struct remoteproc *spacemit_proc_init(struct remoteproc *rproc,
            const struct remoteproc_ops *ops, void *arg)
{
	struct remoteproc_priv *prproc = arg;

	if (!rproc || !prproc || !ops)
		return NULL;

	rproc->priv = prproc;
	rproc->ops = ops;

	return rproc;
}

static void spacemit_proc_remove(struct remoteproc *rproc)
{
	if (!rproc)
		return;

	/* TODO */
}

static void * spacemit_proc_mmap(struct remoteproc *rproc, metal_phys_addr_t *pa,
			metal_phys_addr_t *da, size_t size,
			rt_uint32_t attribute, struct metal_io_region **io)
{
	struct remoteproc_mem *mem;
	metal_phys_addr_t lpa, lda;
	struct metal_io_region *tmpio;

	lpa = *pa;
	lda = *da;

	if (lpa == METAL_BAD_PHYS && lda == METAL_BAD_PHYS)
		return NULL;

	if (lpa == METAL_BAD_PHYS)
		lpa = lda;

	if (lda == METAL_BAD_PHYS)
		lda = lpa;

	if (!attribute)
		attribute = NORM_SHARED_NCACHE | PRIV_RW_USER_RW;

	mem = metal_allocate_memory(sizeof(*mem));
	if (!mem)
		return NULL;

	tmpio = metal_allocate_memory(sizeof(*tmpio));
	if (!tmpio) {
		metal_free_memory(mem);
		return NULL;
	}

	remoteproc_init_mem(mem, NULL, lpa, lda, size, tmpio);
	/* va is the same as pa in this platform */

	metal_io_init(tmpio, (void *)lpa, &mem->pa, size,
		      sizeof(metal_phys_addr_t) << 3, attribute, NULL);

	remoteproc_add_mem(rproc, mem);
	*pa = lpa;
	*da = lda;
	if (io)
		*io = tmpio;

	return metal_io_phys_to_virt(tmpio, mem->pa);
}

static int spacemit_proc_notify(struct remoteproc *rproc, uint32_t id)
{
	char c = 'c';
	struct remoteproc_priv *prproc;

	if (!rproc)
		return -1;

	prproc = rproc->priv;

	mbox_send_message(prproc[id].chan, &c);

	return 0;
}
/* processor operations from n308 to x60. It defines
 * notification operation and remote processor managementi operations. */
struct remoteproc_ops spacemit_proc_ops = {
	.init = spacemit_proc_init,
	.remove = spacemit_proc_remove,
	.mmap = spacemit_proc_mmap,
	.notify = spacemit_proc_notify,
	.start = NULL,
	.stop = NULL,
	.shutdown = NULL,
};

static struct remoteproc *platform_create_proc(struct spacemit_rproc *rproc)
{
	rt_int32_t ret, i = 0;
	char *string;
	rt_int32_t size;
	void *rsc_table;
	rt_int32_t rsc_size;
	struct remoteproc *_rproc;
	metal_phys_addr_t pa;

	rsc_table = get_resource_table(rproc->procid, rproc->rscid, &rsc_size);

	/* alloc memory of rproc */
	_rproc = rt_calloc(1, sizeof(struct remoteproc));
	if (!_rproc) {
		rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
		return NULL;
	}

	rproc->event = rt_event_create("proc_tick", RT_IPC_FLAG_FIFO);

	/* request the mailbox */
	for_each_property_string(rproc->node, "mbox-names", string, size) {
		++i;
	}

	rproc->priv = rt_calloc(i, sizeof(struct remoteproc_priv));
	if (!rproc->priv) {
		rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
		return NULL;
	}

	i = 0;
	for_each_property_string(rproc->node, "mbox-names", string, size) {

		rproc->priv[i].event = rproc->event;
		rproc->priv[i].evtype = 1 << i;
		rproc->priv[i].client.dev = rproc->node;
		rproc->priv[i].client.tx_block = true;
		rproc->priv[i].client.rx_callback = rproc_rx_callback;

		rproc->priv[i].chan = mbox_request_channel_byname(&rproc->priv[i].client, string);
		++i;
	}

	/* Initialize remoteproc instance */
	if (!remoteproc_init(_rproc, &spacemit_proc_ops, rproc->priv))
		return NULL;

	/*
	 * Mmap shared memories
	 * Or shall we constraint that they will be set as carved out
	 * in the resource table?
	 */
	/* mmap resource table */
	pa = (metal_phys_addr_t)rsc_table;
	(void *)remoteproc_mmap(_rproc, &pa,
				NULL, rsc_size,
				NORM_NSHARED_NCACHE | PRIV_RW_USER_RW,
				&_rproc->rsc_io);
	/* mmap shared memory */
	pa = rproc->shmem_pa_base;
	(void *)remoteproc_mmap(_rproc, &pa,
				NULL, rproc->shmem_size,
				NORM_NSHARED_NCACHE | PRIV_RW_USER_RW,
				NULL);

	/* parse resource table to remoteproc */
	ret = remoteproc_set_rsc_table(_rproc, rsc_table, rsc_size);
	if (ret) {
		metal_err("Failed to intialize remoteproc\r\n");
		remoteproc_remove(_rproc);
		return NULL;
	}

	metal_dbg("Initialize remoteproc successfully.\r\n");

	return _rproc;
}

struct rpmsg_device *
platform_create_rpmsg_vdev(struct spacemit_rproc *proc, rt_uint32_t vdev_index,
		rt_uint32_t role,
		void (*rst_cb)(struct virtio_device *vdev),
		rpmsg_ns_bind_cb ns_bind_cb)
{
	void *shbuf;
	struct remoteproc *rproc = proc->rproc;
	struct rpmsg_virtio_device *rpmsg_vdev;
	struct virtio_device *vdev;
	struct metal_io_region *shbuf_io;
	rt_int32_t ret;

	rpmsg_vdev = metal_allocate_memory(sizeof(*rpmsg_vdev));
	if (!rpmsg_vdev)
		return NULL;

	shbuf_io = remoteproc_get_io_with_pa(rproc, proc->shmem_pa_base);
	if (!shbuf_io)
		return NULL;

	shbuf = metal_io_phys_to_virt(shbuf_io,
			proc->shmem_pa_base + proc->shmem_buf_offset);

	metal_dbg("creating remoteproc virtio\r\n");

	/* TODO: can we have a wrapper for the following two functions? */
	vdev = remoteproc_create_virtio(rproc, vdev_index, role, rst_cb);
	if (!vdev) {
		metal_err("failed remoteproc_create_virtio\r\n");
		goto err1;
	}

	metal_dbg("initializing rpmsg shared buffer pool\r\n");

	/* Only RPMsg virtio master needs to initialize the shared buffers pool */
	rpmsg_virtio_init_shm_pool(&proc->shpool, shbuf,
			(proc->shmem_size - proc->shmem_buf_offset));

	metal_dbg("initializing rpmsg vdev\r\n");

	/* RPMsg virtio slave can set shared buffers pool argument to NULL */
	ret =  rpmsg_init_vdev(rpmsg_vdev, vdev, ns_bind_cb, shbuf_io, &proc->shpool);
	if (ret) {
		metal_err("failed rpmsg_init_vdev\r\n");
		goto err2;
	}

	metal_dbg("initializing rpmsg vdev\r\n");

	return rpmsg_virtio_get_rpmsg_device(rpmsg_vdev);
err2:
	remoteproc_remove_virtio(rproc, vdev);
err1:
	metal_free_memory(rpmsg_vdev);

	return NULL;
}

extern rt_int32_t init_system(void);
struct rpmsg_device *rpdev;

static void spacemit_platform_poll(void *priv)
{
	rt_int32_t ret;
	rt_uint32_t e;
	struct spacemit_rproc *sproc = (struct spacemit_rproc *)priv;
	struct remoteproc *rproc = sproc->rproc;

	/* create remote proc device */
	sproc->rpmsgdev = platform_create_rpmsg_vdev(sproc, sproc->vdevid, VIRTIO_DEV_DEVICE, NULL, NULL);
	if (!sproc->rpmsgdev) {
		rt_kprintf("%s:%d, create rpmsg vdev failed\n", __func__, __LINE__);
		return;
	}

	rpdev = sproc->rpmsgdev;

	while(1) {
		ret = rt_event_recv(sproc->event,
				3, /* channel 0 & channel 1 */
				RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
				RT_WAITING_FOREVER, &e);

		ret = remoteproc_get_notification(rproc, RSC_NOTIFY_ID_ANY);
		if (ret)
			return;
	}
}

static rt_int32_t spacemit_rproc_probe(void)
{
	rt_int32_t i, irq;
	rt_thread_t tid;
	uint32_t val[2] = {0, 0};
	struct spacemit_rproc *rproc;
	rt_int32_t property_size;
	rt_uint32_t u32_value;
	rt_uint32_t *u32_ptr;
	struct dtb_node *compatible_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();

	init_system();

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
			__compatible[i].compatible);
		if (compatible_node != RT_NULL) {
			/* check the status */
			if (!dtb_node_device_is_available(compatible_node))
				continue;

			rproc = rt_calloc(1, sizeof(struct spacemit_rproc));
			if (!rproc) {
				rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}

			rproc->node = compatible_node;

			/* get the shmem_pa_base */
			dtb_node_read_u32_array(compatible_node, "shared_mem_pa_base", val, 2);
			rproc->shmem_pa_base = (((rt_uint64_t)val[0]) << 32) | val[1];

			/* get the shmem buf offset */
			for_each_property_cell(compatible_node, "shared_mem_buf_offset",
					u32_value, u32_ptr, property_size) {
				rproc->shmem_buf_offset = u32_value;
			}

			/* get the shmem size */
			for_each_property_cell(compatible_node, "shared_mem_size",
					u32_value, u32_ptr, property_size) {
				rproc->shmem_size = u32_value;
			}
	
			/* get the proc id */
			for_each_property_cell(compatible_node, "proc_id",
					u32_value, u32_ptr, property_size) {
				rproc->procid = u32_value;
			}
	
			/* get the rsc id */
			for_each_property_cell(compatible_node, "rsc_id",
					u32_value, u32_ptr, property_size) {
				rproc->rscid = u32_value;
			}

			/* get the vdev id */
			for_each_property_cell(compatible_node, "vdev_id",
					u32_value, u32_ptr, property_size) {
				rproc->vdevid = u32_value;
			}

			/* create remote proc instanse */
			rproc->rproc = platform_create_proc(rproc);

			/* create the rpmsg poll thread */
			tid = rt_thread_create(__compatible[i].compatible,
					spacemit_platform_poll,
					(void *)rproc,
					2048,
					RT_THREAD_PRIORITY_MAX / 3,
					20);
			if (!tid) {
				rt_kprintf("Failed to create adma service\n");
				return -RT_EINVAL;
			}

			rt_thread_startup(tid);

#ifdef SOC_SPACEMIT_K1_X
			/* eable the mailbox irq */
			irq = dtb_node_irq_get(compatible_node, 0);

			rt_hw_interrupt_umask(irq);
#endif
		}
	}

	return 0;
}
INIT_COMPONENT_EXPORT(spacemit_rproc_probe);
