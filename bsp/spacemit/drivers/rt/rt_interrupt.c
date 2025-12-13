#include <rtthread.h>
#include <rtdevice.h>
#include <clint.h>
#include <spacemit_sdk_soc.h>

#ifdef RT_USING_FINSH

#define R_GPIO_INPUT    128  /* 小核输入测试引脚 (R-GPIO0) */
#define R_GPIO_OUTPUT   129  /* 小核输出测试引脚 (R-GPIO1) */

#define RT_TEST_NUMBER		500

static unsigned long long *timestamp;

static unsigned long long timestamp0, timestamp1;
static unsigned int buffer_count;

static void rt_irq_callback(void *args)
{
	timestamp1 = SysTimer_GetLoadValue();

	timestamp[buffer_count++] = timestamp1 - timestamp0;
}

static int rt_interrupt(void)
{
	int i = 0;
	unsigned long long all = 0, min = -1, max = 0;

	timestamp = rt_calloc(RT_TEST_NUMBER, sizeof(unsigned long long));
	if (!timestamp) {
		rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
		return -RT_ERROR;
	}

	/* 配置中断GPIO */
	rt_pin_mode(R_GPIO_INPUT, PIN_MODE_INPUT);
	rt_pin_attach_irq(R_GPIO_INPUT, PIN_IRQ_MODE_RISING,
			rt_irq_callback, (void *)(rt_ubase_t)R_GPIO_INPUT);
	rt_pin_irq_enable(R_GPIO_INPUT, PIN_IRQ_ENABLE);

	/* 配置触发GPIO */
	rt_pin_mode(R_GPIO_OUTPUT, PIN_MODE_OUTPUT);
	rt_pin_write(R_GPIO_OUTPUT, 0);

	for (i = 0; i < RT_TEST_NUMBER; ++i) {
		/* save the timestamp */
		timestamp0 = SysTimer_GetLoadValue();
		rt_pin_write(R_GPIO_OUTPUT, 1);
		rt_thread_delay(10);
		rt_pin_write(R_GPIO_OUTPUT, 0);
	}

	for (i = 0; i < RT_TEST_NUMBER; ++i) {
		all += timestamp[i];

		if (timestamp[i] < min)
			min = timestamp[i];

		if (timestamp[i] > max)
			max = timestamp[i];
	}

	all /= RT_TEST_NUMBER;

	rt_kprintf("Interrupt delay of esos Average:%lldns, min:%lldns, max:%lldns\n",
			all * 1000000000 / SOC_TIMER_FREQ,
			min * 1000000000 / SOC_TIMER_FREQ,
			max * 1000000000 / SOC_TIMER_FREQ);

	return 0;
}

MSH_CMD_EXPORT(rt_interrupt, "rt interrupt delay");
#endif
