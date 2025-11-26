/*

 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0

 * GPIO Comprehensive Test Suite
 *
 * 包含输入、输出、上升沿、下降沿、双边沿中断测试
 * 覆盖大核(AP-GPIO)和小核(R-GPIO)
 */

#include <rtthread.h>
#include <rtdevice.h>

/* GPIO引脚定义 */
#define AP_GPIO_INPUT   57   /* 大核输入测试引脚 */
#define AP_GPIO_OUTPUT  58   /* 大核输出测试引脚 */
#define R_GPIO_INPUT    128  /* 小核输入测试引脚 (R-GPIO0) */
#define R_GPIO_OUTPUT   129  /* 小核输出测试引脚 (R-GPIO1) */

/* 中断测试相关变量 */
static volatile int irq_count_ap = 0;
static volatile int irq_count_r = 0;
static volatile int last_irq_gpio = -1;

/* 大核GPIO中断回调 */
static void ap_gpio_irq_callback(void *args)
{
    int gpio_num = (int)(rt_ubase_t)args;
    irq_count_ap++;
    last_irq_gpio = gpio_num;
    rt_kprintf("[IRQ] AP-GPIO %d interrupt! (count=%d)\n", gpio_num, irq_count_ap);
}

/* 小核GPIO中断回调 */
static void r_gpio_irq_callback(void *args)
{
    int gpio_num = (int)(rt_ubase_t)args;
    irq_count_r++;
    last_irq_gpio = gpio_num;
    rt_kprintf("[IRQ] R-GPIO %d interrupt! (count=%d)\n", gpio_num, irq_count_r);
}

/* 0001 - GPIO输入功能测试 */
static int gpio_test_0001(void)
{
    int val_ap, val_r;

    rt_kprintf("\n");
    rt_kprintf("==========================================\n");
    rt_kprintf("  BSP_RTTHREAD_GPIO_0001: Input Test\n");
    rt_kprintf("==========================================\n\n");

    /* 测试大核GPIO输入 */
    rt_kprintf("[1] Testing AP-GPIO %d (Big Core) Input\n", AP_GPIO_INPUT);
    rt_pin_mode(AP_GPIO_INPUT, PIN_MODE_INPUT);
    rt_thread_mdelay(100);

    val_ap = rt_pin_read(AP_GPIO_INPUT);
    rt_kprintf("    AP-GPIO %d value: %d ", AP_GPIO_INPUT, val_ap);
    if (val_ap == 0 || val_ap == 1) {
        rt_kprintf("[OK]\n");
    } else {
        rt_kprintf("[FAIL]\n");
    }

    /* 测试小核GPIO输入 */
    rt_kprintf("[2] Testing R-GPIO %d (Small Core) Input\n", R_GPIO_INPUT);
    rt_pin_mode(R_GPIO_INPUT, PIN_MODE_INPUT);
    rt_thread_mdelay(100);

    val_r = rt_pin_read(R_GPIO_INPUT);
    rt_kprintf("    R-GPIO %d value: %d ", R_GPIO_INPUT, val_r);
    if (val_r == 0 || val_r == 1) {
        rt_kprintf("[OK]\n");
    } else {
        rt_kprintf("[FAIL]\n");
    }

    rt_kprintf("\n[Result] GPIO Input Test: ");
    if ((val_ap == 0 || val_ap == 1) && (val_r == 0 || val_r == 1)) {
        rt_kprintf("[PASS]\n");
    } else {
        rt_kprintf("[FAIL]\n");
    }
    rt_kprintf("==========================================\n\n");

    return 0;
}
#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(gpio_test_0001, BSP_RTTHREAD_GPIO_0001: GPIO Input Test);
#endif

/* 0002 - GPIO输出功能测试 */
static int gpio_test_0002(void)
{
    int val_high_ap, val_low_ap, val_high_r, val_low_r;
    int pass = 1;

    rt_kprintf("\n");
    rt_kprintf("==========================================\n");
    rt_kprintf("  BSP_RTTHREAD_GPIO_0002: Output Test\n");
    rt_kprintf("==========================================\n\n");

    /* 测试大核GPIO输出 */
    rt_kprintf("[1] Testing AP-GPIO %d (Big Core) Output\n", AP_GPIO_OUTPUT);
    rt_pin_mode(AP_GPIO_OUTPUT, PIN_MODE_OUTPUT);

    /* 输出高电平 */
    rt_pin_write(AP_GPIO_OUTPUT, 1);
    rt_thread_mdelay(100);
    val_high_ap = rt_pin_read(AP_GPIO_OUTPUT);
    rt_kprintf("    Write HIGH, Read: %d ", val_high_ap);
    if (val_high_ap == 1) {
        rt_kprintf("[OK]\n");
    } else {
        rt_kprintf("[FAIL]\n");
        pass = 0;
    }

    /* 输出低电平 */
    rt_pin_write(AP_GPIO_OUTPUT, 0);
    rt_thread_mdelay(100);
    val_low_ap = rt_pin_read(AP_GPIO_OUTPUT);
    rt_kprintf("    Write LOW,  Read: %d ", val_low_ap);
    if (val_low_ap == 0) {
        rt_kprintf("[OK]\n");
    } else {
        rt_kprintf("[FAIL]\n");
        pass = 0;
    }

    /* 测试小核GPIO输出 */
    rt_kprintf("[2] Testing R-GPIO %d (Small Core) Output\n", R_GPIO_OUTPUT);
    rt_pin_mode(R_GPIO_OUTPUT, PIN_MODE_OUTPUT);

    /* 输出高电平 */
    rt_pin_write(R_GPIO_OUTPUT, 1);
    rt_thread_mdelay(100);
    val_high_r = rt_pin_read(R_GPIO_OUTPUT);
    rt_kprintf("    Write HIGH, Read: %d ", val_high_r);
    if (val_high_r == 1) {
        rt_kprintf("[OK]\n");
    } else {
        rt_kprintf("[FAIL]\n");
        pass = 0;
    }

    /* 输出低电平 */
    rt_pin_write(R_GPIO_OUTPUT, 0);
    rt_thread_mdelay(100);
    val_low_r = rt_pin_read(R_GPIO_OUTPUT);
    rt_kprintf("    Write LOW,  Read: %d ", val_low_r);
    if (val_low_r == 0) {
        rt_kprintf("[OK]\n");
    } else {
        rt_kprintf("[FAIL]\n");
        pass = 0;
    }

    rt_kprintf("\n[Result] GPIO Output Test: %s\n", pass ? "[PASS]" : "[FAIL]");
    rt_kprintf("==========================================\n\n");

    return 0;
}
#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(gpio_test_0002, BSP_RTTHREAD_GPIO_0002: GPIO Output Test);
#endif

/* 0003 - GPIO上升沿中断测试 */
static int gpio_test_0003(void)
{
    int i, pass = 1;

    rt_kprintf("\n");
    rt_kprintf("==========================================\n");
    rt_kprintf("  BSP_RTTHREAD_GPIO_0003: Rising Edge IRQ\n");
    rt_kprintf("==========================================\n\n");

    /* 测试大核GPIO上升沿中断 */
    rt_kprintf("[1] Testing AP-GPIO %d->%d (Big Core) Rising Edge\n",
               AP_GPIO_OUTPUT, AP_GPIO_INPUT);
    rt_kprintf("    Note: Connect GPIO %d output to GPIO %d input\n\n",
               AP_GPIO_OUTPUT, AP_GPIO_INPUT);

    /* 配置中断GPIO */
    rt_pin_mode(AP_GPIO_INPUT, PIN_MODE_INPUT);
    rt_pin_attach_irq(AP_GPIO_INPUT, PIN_IRQ_MODE_RISING,
                      ap_gpio_irq_callback, (void *)(rt_ubase_t)AP_GPIO_INPUT);
    rt_pin_irq_enable(AP_GPIO_INPUT, PIN_IRQ_ENABLE);

    /* 配置触发GPIO */
    rt_pin_mode(AP_GPIO_OUTPUT, PIN_MODE_OUTPUT);
    rt_pin_write(AP_GPIO_OUTPUT, 0);

    /* 重置计数 */
    irq_count_ap = 0;
    last_irq_gpio = -1;

    /* 测试3次上升沿 */
    for (i = 0; i < 3; i++) {
        last_irq_gpio = -1;
        rt_pin_write(AP_GPIO_OUTPUT, 0);
        rt_thread_mdelay(100);
        rt_pin_write(AP_GPIO_OUTPUT, 1);
        rt_thread_mdelay(100);

        rt_kprintf("    Test %d: ", i + 1);
        if (last_irq_gpio == AP_GPIO_INPUT) {
            rt_kprintf("[OK] Interrupt detected\n");
        } else {
            rt_kprintf("[FAIL] No interrupt\n");
            pass = 0;
        }
    }

    rt_kprintf("    AP-GPIO Total interrupts: %d\n\n", irq_count_ap);
    rt_pin_detach_irq(AP_GPIO_INPUT);

    /* 测试小核GPIO上升沿中断 */
    rt_kprintf("[2] Testing R-GPIO %d->%d (Small Core) Rising Edge\n",
               R_GPIO_OUTPUT, R_GPIO_INPUT);
    rt_kprintf("    Note: Connect GPIO %d output to GPIO %d input\n\n",
               R_GPIO_OUTPUT, R_GPIO_INPUT);

    /* 配置中断GPIO */
    rt_pin_mode(R_GPIO_INPUT, PIN_MODE_INPUT);
    rt_pin_attach_irq(R_GPIO_INPUT, PIN_IRQ_MODE_RISING,
                      r_gpio_irq_callback, (void *)(rt_ubase_t)R_GPIO_INPUT);
    rt_pin_irq_enable(R_GPIO_INPUT, PIN_IRQ_ENABLE);

    /* 配置触发GPIO */
    rt_pin_mode(R_GPIO_OUTPUT, PIN_MODE_OUTPUT);
    rt_pin_write(R_GPIO_OUTPUT, 0);

    /* 重置计数 */
    irq_count_r = 0;
    last_irq_gpio = -1;

    /* 测试3次上升沿 */
    for (i = 0; i < 3; i++) {
        last_irq_gpio = -1;
        rt_pin_write(R_GPIO_OUTPUT, 0);
        rt_thread_mdelay(100);
        rt_pin_write(R_GPIO_OUTPUT, 1);
        rt_thread_mdelay(100);

        rt_kprintf("    Test %d: ", i + 1);
        if (last_irq_gpio == R_GPIO_INPUT) {
            rt_kprintf("[OK] Interrupt detected\n");
        } else {
            rt_kprintf("[FAIL] No interrupt\n");
            pass = 0;
        }
    }

    rt_kprintf("    R-GPIO Total interrupts: %d\n\n", irq_count_r);
    rt_pin_detach_irq(R_GPIO_INPUT);

    rt_kprintf("[Result] Rising Edge IRQ Test: %s\n", pass ? "[PASS]" : "[FAIL]");
    rt_kprintf("Total interrupts: AP=%d, R=%d\n", irq_count_ap, irq_count_r);
    rt_kprintf("==========================================\n\n");

    return 0;
}
#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(gpio_test_0003, BSP_RTTHREAD_GPIO_0003: Rising Edge IRQ Test);
#endif

/* 0004 - GPIO下降沿中断测试 */
static int gpio_test_0004(void)
{
    int i, pass = 1;

    rt_kprintf("\n");
    rt_kprintf("==========================================\n");
    rt_kprintf("  BSP_RTTHREAD_GPIO_0004: Falling Edge IRQ\n");
    rt_kprintf("==========================================\n\n");

    /* 测试大核GPIO下降沿中断 */
    rt_kprintf("[1] Testing AP-GPIO %d->%d (Big Core) Falling Edge\n",
               AP_GPIO_OUTPUT, AP_GPIO_INPUT);

    /* 配置中断GPIO */
    rt_pin_mode(AP_GPIO_INPUT, PIN_MODE_INPUT);
    rt_pin_attach_irq(AP_GPIO_INPUT, PIN_IRQ_MODE_FALLING,
                      ap_gpio_irq_callback, (void *)(rt_ubase_t)AP_GPIO_INPUT);
    rt_pin_irq_enable(AP_GPIO_INPUT, PIN_IRQ_ENABLE);

    /* 配置触发GPIO */
    rt_pin_mode(AP_GPIO_OUTPUT, PIN_MODE_OUTPUT);
    rt_pin_write(AP_GPIO_OUTPUT, 1);

    /* 重置计数 */
    irq_count_ap = 0;
    last_irq_gpio = -1;

    /* 测试3次下降沿 */
    for (i = 0; i < 3; i++) {
        last_irq_gpio = -1;
        rt_pin_write(AP_GPIO_OUTPUT, 1);
        rt_thread_mdelay(100);
        rt_pin_write(AP_GPIO_OUTPUT, 0);
        rt_thread_mdelay(100);

        rt_kprintf("    Test %d: ", i + 1);
        if (last_irq_gpio == AP_GPIO_INPUT) {
            rt_kprintf("[OK] Interrupt detected\n");
        } else {
            rt_kprintf("[FAIL] No interrupt\n");
            pass = 0;
        }
    }

    rt_kprintf("    AP-GPIO Total interrupts: %d\n\n", irq_count_ap);
    rt_pin_detach_irq(AP_GPIO_INPUT);

    /* 测试小核GPIO下降沿中断 */
    rt_kprintf("[2] Testing R-GPIO %d->%d (Small Core) Falling Edge\n",
               R_GPIO_OUTPUT, R_GPIO_INPUT);

    /* 配置中断GPIO */
    rt_pin_mode(R_GPIO_INPUT, PIN_MODE_INPUT);
    rt_pin_attach_irq(R_GPIO_INPUT, PIN_IRQ_MODE_FALLING,
                      r_gpio_irq_callback, (void *)(rt_ubase_t)R_GPIO_INPUT);
    rt_pin_irq_enable(R_GPIO_INPUT, PIN_IRQ_ENABLE);

    /* 配置触发GPIO */
    rt_pin_mode(R_GPIO_OUTPUT, PIN_MODE_OUTPUT);
    rt_pin_write(R_GPIO_OUTPUT, 1);

    /* 重置计数 */
    irq_count_r = 0;
    last_irq_gpio = -1;

    /* 测试3次下降沿 */
    for (i = 0; i < 3; i++) {
        last_irq_gpio = -1;
        rt_pin_write(R_GPIO_OUTPUT, 1);
        rt_thread_mdelay(100);
        rt_pin_write(R_GPIO_OUTPUT, 0);
        rt_thread_mdelay(100);

        rt_kprintf("    Test %d: ", i + 1);
        if (last_irq_gpio == R_GPIO_INPUT) {
            rt_kprintf("[OK] Interrupt detected\n");
        } else {
            rt_kprintf("[FAIL] No interrupt\n");
            pass = 0;
        }
    }

    rt_kprintf("    R-GPIO Total interrupts: %d\n\n", irq_count_r);
    rt_pin_detach_irq(R_GPIO_INPUT);

    rt_kprintf("[Result] Falling Edge IRQ Test: %s\n", pass ? "[PASS]" : "[FAIL]");
    rt_kprintf("Total interrupts: AP=%d, R=%d\n", irq_count_ap, irq_count_r);
    rt_kprintf("==========================================\n\n");

    return 0;
}
#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(gpio_test_0004, BSP_RTTHREAD_GPIO_0004: Falling Edge IRQ Test);
#endif

/* 0005 - GPIO双边沿中断测试 */
static int gpio_test_0005(void)
{
    int i, pass = 1;

    rt_kprintf("\n");
    rt_kprintf("==========================================\n");
    rt_kprintf("  BSP_RTTHREAD_GPIO_0005: Both Edge IRQ\n");
    rt_kprintf("==========================================\n\n");

    /* 测试大核GPIO双边沿中断 */
    rt_kprintf("[1] Testing AP-GPIO %d->%d (Big Core) Both Edge\n",
               AP_GPIO_OUTPUT, AP_GPIO_INPUT);

    /* 配置中断GPIO */
    rt_pin_mode(AP_GPIO_INPUT, PIN_MODE_INPUT);
    rt_pin_attach_irq(AP_GPIO_INPUT, PIN_IRQ_MODE_RISING_FALLING,
                      ap_gpio_irq_callback, (void *)(rt_ubase_t)AP_GPIO_INPUT);
    rt_pin_irq_enable(AP_GPIO_INPUT, PIN_IRQ_ENABLE);

    /* 配置触发GPIO */
    rt_pin_mode(AP_GPIO_OUTPUT, PIN_MODE_OUTPUT);
    rt_pin_write(AP_GPIO_OUTPUT, 0);

    /* 重置计数 */
    irq_count_ap = 0;
    last_irq_gpio = -1;

    /* 测试3次双边沿 (每次产生上升沿+下降沿,共6次中断) */
    for (i = 0; i < 3; i++) {
        int count_before = irq_count_ap;

        /* 产生上升沿 */
        last_irq_gpio = -1;
        rt_pin_write(AP_GPIO_OUTPUT, 1);
        rt_thread_mdelay(100);

        /* 产生下降沿 */
        rt_pin_write(AP_GPIO_OUTPUT, 0);
        rt_thread_mdelay(100);

        rt_kprintf("    Test %d: ", i + 1);
        if (irq_count_ap == count_before + 2) {
            rt_kprintf("[OK] Both edges detected (count: %d->%d)\n",
                       count_before, irq_count_ap);
        } else {
            rt_kprintf("[FAIL] Expected 2 interrupts, got %d\n",
                       irq_count_ap - count_before);
            pass = 0;
        }
    }

    rt_kprintf("    AP-GPIO Total interrupts: %d (expected 6)\n\n", irq_count_ap);
    rt_pin_detach_irq(AP_GPIO_INPUT);

    /* 测试小核GPIO双边沿中断 */
    rt_kprintf("[2] Testing R-GPIO %d->%d (Small Core) Both Edge\n",
               R_GPIO_OUTPUT, R_GPIO_INPUT);

    /* 配置中断GPIO */
    rt_pin_mode(R_GPIO_INPUT, PIN_MODE_INPUT);
    rt_pin_attach_irq(R_GPIO_INPUT, PIN_IRQ_MODE_RISING_FALLING,
                      r_gpio_irq_callback, (void *)(rt_ubase_t)R_GPIO_INPUT);
    rt_pin_irq_enable(R_GPIO_INPUT, PIN_IRQ_ENABLE);

    /* 配置触发GPIO */
    rt_pin_mode(R_GPIO_OUTPUT, PIN_MODE_OUTPUT);
    rt_pin_write(R_GPIO_OUTPUT, 0);

    /* 重置计数 */
    irq_count_r = 0;
    last_irq_gpio = -1;

    /* 测试3次双边沿 */
    for (i = 0; i < 3; i++) {
        int count_before = irq_count_r;

        /* 产生上升沿 */
        last_irq_gpio = -1;
        rt_pin_write(R_GPIO_OUTPUT, 1);
        rt_thread_mdelay(100);

        /* 产生下降沿 */
        rt_pin_write(R_GPIO_OUTPUT, 0);
        rt_thread_mdelay(100);

        rt_kprintf("    Test %d: ", i + 1);
        if (irq_count_r == count_before + 2) {
            rt_kprintf("[OK] Both edges detected (count: %d->%d)\n",
                       count_before, irq_count_r);
        } else {
            rt_kprintf("[FAIL] Expected 2 interrupts, got %d\n",
                       irq_count_r - count_before);
            pass = 0;
        }
    }

    rt_kprintf("    R-GPIO Total interrupts: %d (expected 6)\n\n", irq_count_r);
    rt_pin_detach_irq(R_GPIO_INPUT);

    rt_kprintf("[Result] Both Edge IRQ Test: %s\n", pass ? "[PASS]" : "[FAIL]");
    rt_kprintf("Total interrupts: AP=%d, R=%d\n", irq_count_ap, irq_count_r);
    rt_kprintf("==========================================\n\n");

    return 0;
}
#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(gpio_test_0005, BSP_RTTHREAD_GPIO_0005: Both Edge IRQ Test);
#endif

/* 运行所有测试 */
static int gpio_test_all(void)
{
    rt_kprintf("\n\n");
    rt_kprintf("##################################################\n");
    rt_kprintf("#  GPIO Comprehensive Test Suite - Run All      #\n");
    rt_kprintf("##################################################\n");

    /* 依次执行所有测试 */
    gpio_test_0001();  /* 输入测试 */
    gpio_test_0002();  /* 输出测试 */
    gpio_test_0003();  /* 上升沿中断测试 */
    gpio_test_0004();  /* 下降沿中断测试 */
    gpio_test_0005();  /* 双边沿中断测试 */

    rt_kprintf("\n");
    rt_kprintf("##################################################\n");
    rt_kprintf("#  All GPIO Tests Completed                     #\n");
    rt_kprintf("##################################################\n\n");

    return 0;
}
#ifdef RT_USING_FINSH
MSH_CMD_EXPORT(gpio_test_all, Run all GPIO tests);
#endif
