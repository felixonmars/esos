#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

#define RPMSG_NAME_SIZE	32
#define RPMSG_ADDR_ANY          0xFFFFFFFF

/**
 * struct rpmsg_endpoint_info - endpoint info representation
 * @name: name of service
 * @src: local address. To set to RPMSG_ADDR_ANY if not used.
 * @dst: destination address. To set to RPMSG_ADDR_ANY if not used.
 */
struct rpmsg_endpoint_info {
        char name[32];
        unsigned int src;
        unsigned int dst;
};

/**
 * Instantiate a new rmpsg char device endpoint.
 */
#define RPMSG_CREATE_EPT_IOCTL  _IOW(0xb5, 0x1, struct rpmsg_endpoint_info)

/**
 * Destroy a rpmsg char device endpoint created by the RPMSG_CREATE_EPT_IOCTL.
 */
#define RPMSG_DESTROY_EPT_IOCTL _IO(0xb5, 0x2)

/**
 * Instantiate a new local rpmsg service device.
 */
#define RPMSG_CREATE_DEV_IOCTL  _IOW(0xb5, 0x3, struct rpmsg_endpoint_info)

/**
 * Release a local rpmsg device.
 */
#define RPMSG_RELEASE_DEV_IOCTL _IOW(0xb5, 0x4, struct rpmsg_endpoint_info)

/**
 * Get the flow control state of the remote rpmsg char device.
 */
#define RPMSG_GET_OUTGOING_FLOWCONTROL _IOR(0xb5, 0x5, int)

/**
 * Set the flow control state of the local rpmsg char device.
 */
#define RPMSG_SET_INCOMING_FLOWCONTROL _IOR(0xb5, 0x6, int)

int main(void)
{
	int ret;
	int rpmsg_ctrl_fd, rpmsg_fd;
	char r[128];
	struct rpmsg_endpoint_info chinfo;

	rpmsg_ctrl_fd = open("/dev/rpmsg_ctrl0", O_RDWR);
	if (rpmsg_ctrl_fd < 0) {
		printf("Open device node:%s failed\n",  "/dev/rpmsg_ctrl1");
		return -1;
	}

	strncpy(chinfo.name, "rpmsg:demo0", RPMSG_NAME_SIZE);
	chinfo.src = 666;
	chinfo.dst = 888;

	ret = ioctl(rpmsg_ctrl_fd, RPMSG_CREATE_EPT_IOCTL, (void *)&chinfo);
	if (ret < 0) {
		printf("Create endpoint failed\n");
		return -1;
	}

	rpmsg_fd = open("/dev/rpmsg0", O_RDWR);
	if (rpmsg_fd < 0) {
		printf("Open device node: %s, failed\n", "/dev/rpmsg0");
		return -1;
	}

	while (1) {
		ret = write(rpmsg_fd, "Hello World", strlen("Hello World"));
		read(rpmsg_fd, r, 128);
		printf("%s----------%d, %s\n", __func__, __LINE__, r);
	}
}

