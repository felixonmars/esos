#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#include <drivers/spi.h>
#include <drivers/dma.h>
#include <ipc/workqueue.h>
#include "k1x_spi.h"

#ifdef RT_USING_SPI
#ifdef RT_USING_FINSH

static int rt_hw_spi_flash_init(void)
{
	struct rt_spi_device *spi_device = RT_NULL;

	spi_device = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
	if(RT_NULL == spi_device)
	{
		rt_kprintf("Failed to malloc the spi device.");
		return -RT_ENOMEM;
	}
	if (RT_EOK != rt_spi_bus_attach_device(spi_device, "rspi00", "rspi0", RT_NULL))
	{
		rt_kprintf("Failed to attach the spi device.");
		return -RT_ERROR;
	}

	return RT_EOK;
}

static int test_routine(struct rt_spi_device *dev)
{
	int result = 0;
	rt_uint8_t send[8] = {0x9F,0x11,0x22,0x00,0x55,0x66,0x77,0x88};
	rt_uint8_t *tmp;
	rt_uint8_t recv[8] = {0};

	tmp = (rt_uint8_t *)rt_malloc(sizeof(rt_uint8_t) * 1024);

	// get jedec id
	if (rt_spi_send_then_recv(dev, send, 1, recv, 3) == RT_EOK)
	{
		rt_kprintf("jedec id: %02x %02x %02x\n", recv[0], recv[1], recv[2]);
	}
	else
	{
		result = -RT_ERROR;
		rt_kprintf("read jedec id failed\n");
		return result;
	}

	// erase
	send[0] = 0x20;
	send[1] = 0x00;
	send[2] = 0x00;
	send[3] = 0x00;
	if (rt_spi_transfer(dev, send, RT_NULL, 4) == RT_EOK)
	{
		result = RT_EOK;
	}
	else
	{
		rt_kprintf("erase error\n");
		result = -RT_ERROR;
	}
	rt_thread_mdelay(200);

	//write enable
	send[0] = 0x06;
	if (rt_spi_transfer(dev, send, RT_NULL, 1))
	{
		result = RT_EOK;
	}
	else
	{
		rt_kprintf("write enable error\n");
		result = -RT_ERROR;
	}

	rt_memset(tmp, 0x55, sizeof(rt_uint8_t) * 1024);

	// write
	tmp[0] = 0x02;
	tmp[1] = 0x00;
	tmp[2] = 0x00;
	tmp[3] = 0x00;

	if (rt_spi_transfer(dev, tmp, RT_NULL, 200))
	{
		result = RT_EOK;
	}
	else
	{
		rt_kprintf("write data error\n");
		result = -RT_ERROR;
	}

	// write disable
	tmp[0] = 0x04;
	rt_spi_transfer(dev, tmp, RT_NULL, 1);

	//read
	rt_memset(tmp, 0, sizeof(rt_uint8_t) * 1024);
	send[0] = 0x03;
	send[1] = 0x00;
	send[2] = 0x00;
	send[3] = 0x00;
	if (rt_spi_send_then_recv(dev, send, 4, tmp, 256) == RT_EOK)
	{
		result = RT_EOK;
	}
	else
	{
		rt_kprintf("read data error\n");
		result = -RT_ERROR;
	}

	// check result
	int sum = 0;
	for (int i = 0; i < 256; i++)
		if (tmp[i] == 0x55)
			sum++;

	if (sum == 196) {
		rt_kprintf("test ok!\n");
		result = RT_EOK;
	} else {
		rt_kprintf("data check failed\n");
		result = -RT_ERROR;
	}

	return result;

}

int msh_spi_test(int argc, char **argv)
{
	rt_hw_spi_flash_init();
	struct rt_spi_device *dev;

	dev = (struct rt_spi_device *)rt_device_find("rspi00");
	if (!dev)
	{
		rt_kprintf("get rspi00 device failed\n");
		return -RT_EIO;
	}

	dev->config.mode = RT_SPI_MODE_3;
	dev->config.data_width = 8;
	if (argv[1][0] == '1') {
		// multiple freq test
		struct k1x_spi *spacemit_spi = rt_container_of(dev->bus, struct k1x_spi, bus);
		rt_uint32_t freq_list[] = {812500, 1000000, 1625000, 3250000, 6500000,
			13000000, 26000000, 52000000};
		for (int i = 0; i < sizeof(freq_list) / sizeof(freq_list[0]); i++) {
			clk_set_rate(spacemit_spi->clk, freq_list[i]);
			if (test_routine(dev) != RT_EOK)
				break;
			if (i == sizeof(freq_list) / sizeof(freq_list[0]) - 1)
				rt_kprintf("multi freq ok!\n");
		}
	} else {
		test_routine(dev);
	}

	return RT_EOK;
}

#include <finsh.h>

MSH_CMD_EXPORT_ALIAS(msh_spi_test, spi_test, e.g.: spi_test 0: fixed freq; spi_test 1: various freq);
#endif
#endif
