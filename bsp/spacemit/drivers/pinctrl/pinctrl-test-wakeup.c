#include <rtthread.h>
#include <stdint.h>
#include <drivers/gpio/gpio.h>
#include <string.h>

#ifdef RT_USING_FINSH

struct value_operator {
	/* max 16 */
	int values[16];
	uint8_t num_value;
};

struct value_desc {
	struct value_operator *operator;
	int num_operator;
	int interval;
	/* ms */
	int per_operator_delay;
	int per_interval_delay;
};

static struct value_operator value_operator[3] = {
	{
		.values = {0, 1, 0},
		.num_value = sizeof(value_operator[0].values) / sizeof(value_operator[0].values[0]),
	},
	{
		.values = {1, 0, 1},
		.num_value = sizeof(value_operator[1].values) / sizeof(value_operator[1].values[0]),
	},
	{
		.values = {0, 1, 0, 1, 0, 1},
		.num_value = sizeof(value_operator[2].values) / sizeof(value_operator[2].values[0]),
	},
};

static struct value_desc value_desc = {
	.operator = value_operator,
	.num_operator = sizeof(value_operator) / sizeof(value_operator[0]),
	.per_operator_delay = 500,
	.per_interval_delay = 3000,
	.interval = 3,
};

static int arg_2_index(const char *arg_str)
{
	static const char* const arg_strs[] = {"rising", "failling", "all"};
	int i = sizeof(arg_strs) / sizeof(arg_strs[0]);

	for (--i; i >= 0; i--) {
		if (strcmp(arg_str, arg_strs[i]) == 0)
			return i;
	}

	return -RT_EINVAL;
}

static int start_wakeup(int pin, int wakeup_type)
{
	int i, j, count;
	struct value_operator *operator;

	if (wakeup_type >= value_desc.num_operator)
		return -RT_EINVAL;

	operator = value_desc.operator + wakeup_type;
	count = operator->num_value / value_desc.interval;

	for (i = 0; i < count; i++) {
		for (j = 0; j < operator->num_value; j++) {
			gpio_set_value(pin, operator->values[j]);
			rt_thread_mdelay(value_desc.per_operator_delay);
		}
		rt_thread_mdelay(value_desc.per_interval_delay);
	}

	return RT_EOK;
}

static int pmux_test_wakeup(int argc, void **argv)
{
	int ret, wakeup_type, pin;

	if (argc != 3)
		goto arg_fail;

	/*
	 * To simplely check the returned value,
	 * we can't use pin 0
	 */
	pin = atoi(argv[1]);
	if (!pin)
		goto arg_fail;

	wakeup_type = arg_2_index(argv[2]);
	if (wakeup_type == -1)
		goto arg_fail;

	ret = gpio_request(pin, "pmux wakeup test");
	if (ret) {
		rt_kprintf("%s:%d, request gpio failed\n", __func__, __LINE__);
		return -RT_EIO;
	}

	ret = gpio_direction_output(pin, 0);
	if (ret) {
		rt_kprintf("%s:%d, Failed to set gpio as output\n", __func__, __LINE__);
		/* now we don't have gpio_free API */
		/*gpio_free(pin);*/
		return -RT_EIO;
	}

	ret = start_wakeup(pin, wakeup_type);

	/* now we don't have gpio_free API */
	/*gpio_free(pin);*/

	return ret;

arg_fail:
	rt_kprintf("Usage:\npmux_test_wakeup <gpio_num> <rising|failling|all>\n");
	return -RT_EINVAL;
}

#include <finsh.h>
MSH_CMD_EXPORT_ALIAS(pmux_test_wakeup, pmux_test_wakeup, pinctrl driver test. e.g: pmux_test_wakeup());
#endif

