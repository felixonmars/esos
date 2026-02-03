#include <rtthread.h>
#include <drivers/regulator_dm.h>

#define THREAD_STACK_SIZE	1024
#define THREAD_PRIORITY     	25
#define THREAD_TIMESLICE    	10
#define SLEEP_INTERNAL		10

#define P1_REG			0xab
#define P1_REG_FASTBOOT		0xf1
#define LISTEN_ADDR		0xc087c000
#define FLAG_FASTBOOT		0x1
#define FLAG_FINISH		0x2

static rt_uint32_t i2c_addr = 0x0;

static struct rt_i2c_bus_device* reboot_get_i2c(void)
{
	struct dtb_node *dtb_head_node = get_dtb_node_head();
	struct rt_i2c_bus_device *i2c_bus_device;
	struct dtb_node *compatible_node;
	struct spacemit_pmic_chip *chip;
	struct rt_i2c_msg msgs[2];
	char *string, *strend;
	rt_int32_t size;

	compatible_node = dtb_node_find_compatible_node(dtb_head_node,
			"spacemit,k3-pwrgpio");
	if (compatible_node == RT_NULL) {
		rt_kprintf("reboot daemon: get dtb node failed\n");
		return RT_NULL;
	}
	dtb_node_read_u32_array(compatible_node, "slave_addr", &i2c_addr, 1);

	for_each_property_string_extend(compatible_node, "bind_driver", string, strend, size) {
		i2c_bus_device = rt_i2c_bus_device_find(string);
		if (i2c_bus_device == RT_NULL) {
			rt_kprintf("reboot daemon: the bind driver has not registered\n", __func__, __LINE__);
			return RT_NULL;
		}
	}
	return i2c_bus_device;

}
static void reboot_daemon(void *parameter)
{
	struct rt_i2c_bus_device *i2c_bus_device;
	volatile rt_uint8_t val = 0;
	struct rt_i2c_msg msgs[2];
	rt_uint8_t send_buf[2];

	rt_kprintf("reboot daemon: start\n");
	/* Wait until I2C is registered */
	while (1) {
		i2c_bus_device = reboot_get_i2c();
		if (i2c_bus_device != RT_NULL) {
			break;
		}
		rt_thread_mdelay(100);
	}

	/* Listening */
	/* Once kernel write to that listened address, we write P1's related register */
	while (1) {
		asm volatile("fence rw, rw");
		val = *((volatile rt_uint8_t*)LISTEN_ADDR);
		if (val == FLAG_FASTBOOT) {
			rt_kprintf("changing P1\n");
			send_buf[0] = P1_REG;
			send_buf[1] = P1_REG_FASTBOOT;
			msgs[0].addr = (rt_uint8_t)i2c_addr;
			msgs[0].flags = RT_I2C_WR;
			msgs[0].buf = send_buf;
			msgs[0].len = 2;

			if (rt_i2c_transfer(i2c_bus_device, msgs, 1) != 1) {
				rt_kprintf("reboot daemon: i2c transfer error\n");
				return;
			}
			*((volatile rt_uint8_t*)LISTEN_ADDR) = val | FLAG_FINISH;
			asm volatile("fence rw, rw");
			rt_kprintf("reboot daemon: successfully write P1, listen addr: 0x%0x\n", 
					*((volatile rt_uint8_t*)LISTEN_ADDR));
			break;
		}

		rt_thread_mdelay(10);
	}
}

int start_reboot_daemon(void)
{
	rt_thread_t tid;

	tid = rt_thread_create("rebootd",
			reboot_daemon,
			RT_NULL,
			THREAD_STACK_SIZE,
			THREAD_PRIORITY,
			THREAD_TIMESLICE);

	if (tid != RT_NULL)
		rt_thread_startup(tid);


	return 0;
}

INIT_APP_EXPORT(start_reboot_daemon);
