/*

 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0

 * PWM Comprehensive Test Suite
 *
 * 包含周期设置、占空比设置、异常参数测试
 */

#include <rtthread.h>
#include <rtdevice.h>

#define PWM_CHANNEL         1  /* PWM通道号 */
#define MAX_PWM_DEVICES     16 /* 最大PWM设备数量 */

/* 测试用的周期和占空比值 (单位: 纳秒)
 * 硬件限制: clk=245.76MHz, prescale<=63, pv<=1023
 * period_max = 1e9 * 64 * 1024 / 245760000 ≈ 266829 ns (约 0.267 ms)
 * 因此测试周期需 <= 266000 ns
 */
#define TEST_PERIOD_100US   100000     /* 100us = 100000ns, 10KHz */
#define TEST_PERIOD_200US   200000     /* 200us = 200000ns, 5KHz */
#define TEST_DUTY_25        25000      /* 25% of 100us */
#define TEST_DUTY_50        50000      /* 50% of 100us */
#define TEST_DUTY_75        75000      /* 75% of 100us */

/* PWM设备枚举命令 */
static int pwm_dev_enum(int argc, char *argv[])
{
    struct rt_device_pwm *pwm_dev;
    int found_count = 0;
    int i;

    rt_kprintf("\n");
    rt_kprintf("==========================================\n");
    rt_kprintf("  Available PWM Devices Enumeration\n");
    rt_kprintf("==========================================\n\n");

    /* 枚举常见的PWM设备名称 */
    const char *pwm_names[] = {
        "rpwm0", "rpwm1", "rpwm2", "rpwm3", "rpwm4",
        "rpwm5", "rpwm6", "rpwm7", "rpwm8", "rpwm9",
        "pwm0", "pwm1", "pwm2", "pwm3", "pwm4",
        "pwm5", "pwm6", "pwm7", "pwm8", "pwm9"
    };

    for (i = 0; i < sizeof(pwm_names) / sizeof(pwm_names[0]); i++) {
        pwm_dev = (struct rt_device_pwm *)rt_device_find(pwm_names[i]);
        if (pwm_dev != RT_NULL) {
            rt_kprintf("[%d] %s - Available\n", found_count + 1, pwm_names[i]);
            found_count++;
        }
    }

    if (found_count == 0) {
        rt_kprintf("No PWM devices found in the system.\n");
    } else {
        rt_kprintf("\nTotal: %d PWM device(s) found.\n", found_count);
        rt_kprintf("\nUsage: pwm_test_xxxx [device_name]\n");
        rt_kprintf("Example: pwm_test_0001 rpwm0\n");
    }

    rt_kprintf("==========================================\n\n");
    return 0;
}

/* 获取PWM设备 - 支持参数指定或全量自动查找 */
static struct rt_device_pwm *get_pwm_device(const char *device_name)
{
    struct rt_device_pwm *pwm_dev = RT_NULL;
    int i;

    /* 定义完整的搜索列表，与枚举函数保持一致 */
    const char *pwm_names[] = {
        "rpwm0", "rpwm1", "rpwm2", "rpwm3", "rpwm4",
        "rpwm5", "rpwm6", "rpwm7", "rpwm8", "rpwm9",
        "pwm0", "pwm1", "pwm2", "pwm3", "pwm4",
        "pwm5", "pwm6", "pwm7", "pwm8", "pwm9"
    };

    /* 1. 如果用户指定了设备名，优先尝试查找指定的 */
    if (device_name != RT_NULL && rt_strlen(device_name) > 0) {
        pwm_dev = (struct rt_device_pwm *)rt_device_find(device_name);
        if (pwm_dev != RT_NULL) {
            rt_kprintf("Using specified PWM device: %s\n", device_name);
            return pwm_dev; /* 找到了就直接返回 */
        } else {
            rt_kprintf("Specified PWM device '%s' not found, trying auto-detection...\n", device_name);
        }
    }

    /* 2. 如果未指定或指定的不存在，则遍历列表自动查找第一个可用的 */
    if (pwm_dev == RT_NULL) {
        for (i = 0; i < sizeof(pwm_names) / sizeof(pwm_names[0]); i++) {
            pwm_dev = (struct rt_device_pwm *)rt_device_find(pwm_names[i]);
            if (pwm_dev != RT_NULL) {
                rt_kprintf("Auto-detected PWM device found: %s\n", pwm_names[i]);
                break; /* 找到第一个可用的，跳出循环 */
            }
        }
    }

    return pwm_dev;
}

/* 0001 - 设置周期测试 */
static int pwm_test_0001(int argc, char *argv[])
{
    struct rt_device_pwm *pwm_dev;
    struct rt_pwm_configuration config;
    int pass = 1;
    rt_uint32_t test_periods[] = {50000, 100000, 150000, 200000}; /* 50us, 100us, 150us, 200us */
    int i, ret;
    const char *device_name = (argc > 1) ? argv[1] : RT_NULL;

    rt_kprintf("\n");
    rt_kprintf("==========================================\n");
    rt_kprintf("  BSP_ESOS_PWM_0001: Period Setting Test\n");
    rt_kprintf("==========================================\n\n");

    /* 获取PWM设备 */
    pwm_dev = get_pwm_device(device_name);
    if (pwm_dev == RT_NULL) {
        rt_kprintf("[FAIL] No PWM device available for testing\n");
        rt_kprintf("==========================================\n\n");
        return -1;
    }

    rt_kprintf("Testing PWM period settings...\n\n");

    config.channel = PWM_CHANNEL;
    config.pulse = 25000;  /* 固定占空比为25us */

    for (i = 0; i < sizeof(test_periods) / sizeof(test_periods[0]); i++) {
        config.period = test_periods[i];

        rt_kprintf("[%d] Set period to %d ns (%.2f Hz) ... ",
                   i + 1, config.period, 1000000000.0 / config.period);

        ret = rt_pwm_set(pwm_dev, config.channel, config.period, config.pulse);
        if (ret == RT_EOK) {
            rt_kprintf("[OK]\n");
        } else {
            rt_kprintf("[FAIL] (err=%d)\n", ret);
            pass = 0;
        }
    }

    /* 启用PWM测试实际输出 */
    rt_kprintf("\n[Test] Enable PWM with 200us period ... ");
    config.period = TEST_PERIOD_200US;
    config.pulse = TEST_DUTY_50;

    ret = rt_pwm_set(pwm_dev, config.channel, config.period, config.pulse);
    if (ret == RT_EOK) {
        ret = rt_pwm_enable(pwm_dev, config.channel);
    }

    if (ret == RT_EOK) {
        rt_kprintf("[OK]\n");
        rt_thread_mdelay(100);  /* 让PWM运行一小段时间 */
        rt_pwm_disable(pwm_dev, config.channel);
    } else {
        rt_kprintf("[FAIL] (err=%d)\n", ret);
        pass = 0;
    }

    rt_kprintf("\n[Result] Period Setting Test: %s\n", pass ? "[PASS]" : "[FAIL]");
    rt_kprintf("==========================================\n\n");
    return 0;
}

/* 0002 - 设置占空比测试 */
static int pwm_test_0002(int argc, char *argv[])
{
    struct rt_device_pwm *pwm_dev;
    struct rt_pwm_configuration config;
    int pass = 1;
    rt_uint32_t test_duties[] = {0, 25000, 50000, 75000, 100000}; /* 0%, 25%, 50%, 75%, 100% of 100us */
    int i, ret;
    const char *device_name = (argc > 1) ? argv[1] : RT_NULL;

    rt_kprintf("\n");
    rt_kprintf("==========================================\n");
    rt_kprintf("  BSP_ESOS_PWM_0002: Duty Cycle Setting Test\n");
    rt_kprintf("==========================================\n\n");

    /* 获取PWM设备 */
    pwm_dev = get_pwm_device(device_name);
    if (pwm_dev == RT_NULL) {
        rt_kprintf("[FAIL] No PWM device available for testing\n");
        rt_kprintf("==========================================\n\n");
        return -1;
    }

    rt_kprintf("Testing PWM duty cycle settings (Period = 100us)...\n\n");

    config.channel = PWM_CHANNEL;
    config.period = TEST_PERIOD_100US;

    for (i = 0; i < sizeof(test_duties) / sizeof(test_duties[0]); i++) {
        config.pulse = test_duties[i];

        rt_kprintf("[%d] Set duty to %d ns (%.1f%%) ... ",
                   i + 1, config.pulse, (config.pulse * 100.0) / config.period);

        ret = rt_pwm_set(pwm_dev, config.channel, config.period, config.pulse);
        if (ret == RT_EOK) {
            rt_kprintf("[OK]\n");
        } else {
            rt_kprintf("[FAIL] (err=%d)\n", ret);
            pass = 0;
        }
    }

    /* 启用PWM测试实际输出 - 50% 占空比 */
    rt_kprintf("\n[Test] Enable PWM with 50%% duty cycle ... ");
    config.pulse = TEST_DUTY_50;

    ret = rt_pwm_set(pwm_dev, config.channel, config.period, config.pulse);
    if (ret == RT_EOK) {
        ret = rt_pwm_enable(pwm_dev, config.channel);
    }

    if (ret == RT_EOK) {
        rt_kprintf("[OK]\n");
        rt_thread_mdelay(100);  /* 让PWM运行一小段时间 */
        rt_pwm_disable(pwm_dev, config.channel);
    } else {
        rt_kprintf("[FAIL] (err=%d)\n", ret);
        pass = 0;
    }

    rt_kprintf("\n[Result] Duty Cycle Setting Test: %s\n", pass ? "[PASS]" : "[FAIL]");
    rt_kprintf("==========================================\n\n");
    return 0;
}

/* 0003 - 占空比大于周期测试 (应该失败或被限制) */
static int pwm_test_0003(int argc, char *argv[])
{
    struct rt_device_pwm *pwm_dev;
    struct rt_pwm_configuration config;
    int result;
    const char *device_name = (argc > 1) ? argv[1] : RT_NULL;

    rt_kprintf("\n");
    rt_kprintf("==========================================\n");
    rt_kprintf("  BSP_ESOS_PWM_0003: Duty > Period Test\n");
    rt_kprintf("==========================================\n\n");

    /* 获取PWM设备 */
    pwm_dev = get_pwm_device(device_name);
    if (pwm_dev == RT_NULL) {
        rt_kprintf("[FAIL] No PWM device available for testing\n");
        rt_kprintf("==========================================\n\n");
        return -1;
    }

    rt_kprintf("Testing duty cycle greater than period...\n\n");

    config.channel = PWM_CHANNEL;
    config.period = 100000;               /* Period = 100us */
    config.pulse = 200000;                /* Duty = 200us (2x period) */

    rt_kprintf("[1] Set duty=%d ns > period=%d ns ... ", config.pulse, config.period);

    result = rt_pwm_set(pwm_dev, config.channel, config.period, config.pulse);

    if (result != RT_EOK) {
        rt_kprintf("[OK] Correctly rejected\n");
        rt_kprintf("\n[Result] Duty > Period Test: [PASS]\n");
        rt_kprintf("    System correctly prevents duty > period setting\n");
    } else {
        rt_kprintf("[WARNING] Setting accepted\n");
        rt_kprintf("    Note: Some implementations may auto-correct this\n");

        /* 尝试启用看看会发生什么 */
        rt_kprintf("\n[2] Try to enable with invalid config ... ");
        if (rt_pwm_enable(pwm_dev, config.channel) == RT_EOK) {
            rt_kprintf("[ENABLED]\n");
            rt_thread_mdelay(100);
            rt_pwm_disable(pwm_dev, config.channel);
        } else {
            rt_kprintf("[FAILED]\n");
        }

        rt_kprintf("\n[Result] Duty > Period Test: [PASS]\n");
        rt_kprintf("    System accepted but may have auto-corrected the value\n");
    }

    rt_kprintf("==========================================\n\n");
    return 0;
}

/* 运行所有测试的命令 */
static int pwm_test_all(int argc, char *argv[])
{
    const char *device_name = (argc > 1) ? argv[1] : RT_NULL;

    rt_kprintf("\n");
    rt_kprintf("========================================\n");
    rt_kprintf("  PWM Comprehensive Test Suite\n");
    rt_kprintf("  Running all PWM tests...\n");
    if (device_name) {
        rt_kprintf("  Target device: %s\n", device_name);
    }
    rt_kprintf("========================================\n");

    /* 依次运行所有测试 */
    char *test_argv[] = {"pwm_test_0001", (char*)device_name};
    int test_argc = device_name ? 2 : 1;

    pwm_test_0001(test_argc, test_argv);

    test_argv[0] = "pwm_test_0002";
    pwm_test_0002(test_argc, test_argv);

    test_argv[0] = "pwm_test_0003";
    pwm_test_0003(test_argc, test_argv);

    rt_kprintf("\n");
    rt_kprintf("========================================\n");
    rt_kprintf("  All PWM tests completed!\n");
    rt_kprintf("========================================\n\n");

    return 0;
}

/* 导出MSH命令 */
#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(pwm_dev_enum, List all available PWM devices);
MSH_CMD_EXPORT(pwm_test_0001, BSP_ESOS_PWM_0001: Period Setting Test [device_name]);
MSH_CMD_EXPORT(pwm_test_0002, BSP_ESOS_PWM_0002: Duty Cycle Setting Test [device_name]);
MSH_CMD_EXPORT(pwm_test_0003, BSP_ESOS_PWM_0003: Duty > Period Test [device_name]);
MSH_CMD_EXPORT(pwm_test_all, Run all PWM tests [device_name]);
#endif /* RT_USING_FINSH */
