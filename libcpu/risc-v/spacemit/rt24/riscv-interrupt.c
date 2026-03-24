#include <rthw.h>
#include <rtthread.h>
#include <riscv-plic.h>
#include <spacemit_sdk_soc.h>

#define SOC_INT_MAX		128

static struct rt_irq_desc isr_irq_table[SOC_INT_MAX];

/**
 * Temporary interrupt entry function
 *
 * @param mcause Machine Cause Register
 * @return RT_NULL
 */
void rt_hw_interrupt_handle(int mcause, void *param)
{
	while (1);
}

/**
 * Interrupt entry function initialization
 */
void rt_hw_interrupt_init(void)
{
    int idx = 0;

    /* set the plic threshold */
    __plic_set_threshold(0);

    for (; idx < SOC_INT_MAX; idx++)
    {
        isr_irq_table[idx].handler = /* (rt_isr_handler_t)rt_hw_interrupt_handle */ RT_NULL;
        isr_irq_table[idx].param = RT_NULL;

	__plic_set_priority(idx, 1);
    }
}

/**
 * Break Entry Function Binding
 *
 * @param vector  interrupt number
 * @param handler Break-in function requiring binding
 * @param param   NULL
 * @param name    NULL
 * @return old handler
 */
rt_isr_handler_t rt_hw_interrupt_install(int vector, rt_isr_handler_t handler,
        void *param, const char *name)
{
    rt_isr_handler_t old_handler = RT_NULL;

    if(vector < SOC_INT_MAX)
    {
        old_handler = isr_irq_table[vector].handler;
        if (handler != RT_NULL)
        {
            isr_irq_table[vector].handler = (rt_isr_handler_t)handler;
            isr_irq_table[vector].param = param;
#ifdef RT_USING_INTERRUPT_INFO
            rt_snprintf(isr_irq_table[vector].name, RT_NAME_MAX - 1, "%s", name);
            isr_irq_table[vector].counter = 0;
#endif
        }
    }

    return old_handler;
}

void rt_hw_interrupt_mask(int vector)
{
	__plic_irq_disable(vector);
}

void rt_hw_interrupt_umask(int vector)
{
	__plic_irq_enable(vector);
}

rt_uint32_t rt_hw_interrupt_is_enabled(int vector)
{
	return __plic_irq_is_enabled(vector);
}

rt_uint32_t rt_hw_interrupt_is_pending(int vector)
{
	return 0;
}

void rt_hw_interrupt_clear_pending(int vector)
{
	__plic_clr_pending(vector);
}

void rt_hw_interrupt_set_pending(int vector)
{
	__plic_set_pending(vector);
}

void rt_hw_irq_isr(void)
{
	unsigned int vector = __plic_irq_claim();

	if (isr_irq_table[vector].handler)
		isr_irq_table[vector].handler(vector, isr_irq_table[vector].param);

	__plic_irq_complete(vector);
}

#ifdef BSP_USING_PM

static unsigned int ecli_save_reg[SOC_INT_MAX];

void rt_hw_eclic_save(void)
{
    int idx = 0;

    for (idx = 0; idx < SOC_INT_MAX; idx++)
    {
	    ecli_save_reg[idx] = rt_hw_interrupt_is_enabled(idx);
	    if (ecli_save_reg[idx]) {
		__plic_set_priority(idx, 0);
		rt_hw_interrupt_mask(idx);
	    }
    }
}

void rt_hw_eclic_restore(void)
{
    int idx = 0;

    /* set the plic threshold */
    __plic_set_threshold(0);

    for (; idx < SOC_INT_MAX; idx++)
    {
	if (ecli_save_reg[idx]) {
		__plic_set_priority(idx, 1);
		rt_hw_interrupt_umask(idx);
	}
    }
}
#endif
