#ifndef PLATFORM_INFO_H_
#define PLATFORM_INFO_H_

#include <openamp/remoteproc.h>
#include <openamp/virtio.h>
#include <openamp/rpmsg.h>
#include <riscv-clic.h>
#include <rtdef.h>

#if defined __cplusplus
extern "C" {
#endif

/* memory attributes: not used  */
#define DEVICE_SHARED		0x00000001U /* device, shareable */
#define DEVICE_NONSHARED	0x00000010U /* device, non shareable */
#define NORM_NSHARED_NCACHE	0x00000008U /* Non cacheable  non shareable */
#define NORM_SHARED_NCACHE	0x0000000CU /* Non cacheable shareable */
#define PRIV_RW_USER_RW		(0x00000003U<<8U) /* Full Access */

/* Interrupt vectors */
#define IPC_IRQ_VECT_ID		IPC_AP2AUD_IRQn
#define IPC_BASE_ADDR		0xc088a000 /* IPC base address*/
#define IPC_CHN_BITMASK		0x0000003 /* IPC channel bit mask for IPC from/to */
#define IPC_CHN_TX_BITMASK	0x1
#define IPC_TX_DONE_CHN_OFFSET	0x4

#define SHARED_MEM_SIZE		0xfc000
/* not used */
#define SHARED_BUF_OFFSET	0x0
#define SHARED_MEM_PA		0x30200000

#define RT_HEAP_START		0x30000000
#define RT_HEAP_END		0x30200000

#define RCPU_RUNTIME_MEM_SNAPSHOT_BASE	0x30300000

struct remoteproc_priv {
	const char *poll_dev_name;
	const char *poll_dev_bus_name;
	struct metal_device *poll_dev;
	struct metal_io_region *poll_io;
#ifndef RPMSG_NO_IPI
	unsigned int ipi_chn_mask; /**< IPC channel mask */
	/* atomic_int ipi_nokick; */
	rt_sem_t sem; 
#endif /* !RPMSG_NO_IPI */
};

/**
 * platform_init - initialize the platform
 *
 * It will initialize the platform.
 *
 * @argc: number of arguments
 * @argv: array of the input arguements
 * @platform: pointer to store the platform data pointer
 *
 * return 0 for success or negative value for failure
 */
int platform_init(int argc, char *argv[], void **platform);

/**
 * platform_create_rpmsg_vdev - create rpmsg vdev
 *
 * It will create rpmsg virtio device, and returns the rpmsg virtio
 * device pointer.
 *
 * @platform: pointer to the private data
 * @vdev_index: index of the virtio device, there can more than one vdev
 *              on the platform.
 * @role: virtio master or virtio slave of the vdev
 * @rst_cb: virtio device reset callback
 * @ns_bind_cb: rpmsg name service bind callback
 *
 * return pointer to the rpmsg virtio device
 */
struct rpmsg_device *
platform_create_rpmsg_vdev(void *platform, unsigned int vdev_index,
			   unsigned int role,
			   void (*rst_cb)(struct virtio_device *vdev),
			   rpmsg_ns_bind_cb ns_bind_cb);

/**
 * platform_poll - platform poll function
 *
 * @platform: pointer to the platform
 *
 * return negative value for errors, otherwise 0.
 */
int platform_poll(void *platform);

/**
 * platform_release_rpmsg_vdev - release rpmsg virtio device
 *
 * @rpdev: pointer to the rpmsg device
 */
void platform_release_rpmsg_vdev(struct rpmsg_device *rpdev);

/**
 * platform_cleanup - clean up the platform resource
 *
 * @platform: pointer to the platform
 */
void platform_cleanup(void *platform);

#if defined __cplusplus
}
#endif

#endif /* PLATFORM_INFO_H_ */
