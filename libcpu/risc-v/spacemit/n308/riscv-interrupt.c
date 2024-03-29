/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <spacemit_sdk_soc.h>

/**
 * \defgroup  NMSIS_Core_IntExcNMI_Handling   Interrupt and Exception and NMI Handling
 * \brief Functions for interrupt, exception and nmi handle available in system_<device>.c.
 * \details
 * Nuclei provide a template for interrupt, exception and NMI handling. Silicon Vendor could adapat according
 * to their requirement. Silicon vendor could implement interface for different exception code and
 * replace current implementation.
 *
 * @{
 */
/** \brief Max exception handler number, don't include the NMI(0xFFF) one */
#define MAX_SYSTEM_EXCEPTION_NUM        16

static struct rt_irq_desc isr_irq_table[SOC_INT_MAX];

/**
 * \brief      Exception Handler Function Typedef
 * \note
 * This typedef is only used internal in this system_<Device>.c file.
 * It is used to do type conversion for registered exception handler before calling it.
 */
typedef void (*EXC_HANDLER)(unsigned long cause, unsigned long sp);

/**
 * \brief      Store the exception handlers for each exception ID
 * \note
 * - This SystemExceptionHandlers are used to store all the handlers for all
 * the exception codes Nuclei N/NX core provided.
 * - Exception code 0 - 11, totally 12 exceptions are mapped to SystemExceptionHandlers[0:11]
 * - Exception for NMI is also re-routed to exception handling(exception code 0xFFF) in startup code configuration, the handler itself is mapped to SystemExceptionHandlers[MAX_SYSTEM_EXCEPTION_NUM]
 */
static unsigned long SystemExceptionHandlers[MAX_SYSTEM_EXCEPTION_NUM + 1];

/**
 * \brief      Dump Exception Frame
 * \details
 * This function provided feature to dump exception frame stored in stack.
 * \param [in]  sp    stackpoint
 * \param [in]  mode  privileged mode to decide whether to dump msubm CSR
 */
static void Exception_DumpFrame(unsigned long sp, rt_uint8_t mode)
{
    EXC_Frame_Type *exc_frame = (EXC_Frame_Type *)sp;

#ifndef __riscv_32e
    rt_kprintf("ra: 0x%lx, tp: 0x%lx, t0: 0x%lx, t1: 0x%lx, t2: 0x%lx, t3: 0x%lx, t4: 0x%lx, t5: 0x%lx, t6: 0x%lx\n" \
           "a0: 0x%lx, a1: 0x%lx, a2: 0x%lx, a3: 0x%lx, a4: 0x%lx, a5: 0x%lx, a6: 0x%lx, a7: 0x%lx\n" \
           "cause: 0x%lx, epc: 0x%lx\n", exc_frame->ra, exc_frame->tp, exc_frame->t0, \
           exc_frame->t1, exc_frame->t2, exc_frame->t3, exc_frame->t4, exc_frame->t5, exc_frame->t6, \
           exc_frame->a0, exc_frame->a1, exc_frame->a2, exc_frame->a3, exc_frame->a4, exc_frame->a5, \
           exc_frame->a6, exc_frame->a7, exc_frame->cause, exc_frame->epc);
#else
    rt_kprintf("ra: 0x%lx, tp: 0x%lx, t0: 0x%lx, t1: 0x%lx, t2: 0x%lx\n" \
           "a0: 0x%lx, a1: 0x%lx, a2: 0x%lx, a3: 0x%lx, a4: 0x%lx, a5: 0x%lx\n" \
           "cause: 0x%lx, epc: 0x%lx\n", exc_frame->ra, exc_frame->tp, exc_frame->t0, \
           exc_frame->t1, exc_frame->t2, exc_frame->a0, exc_frame->a1, exc_frame->a2, exc_frame->a3, \
           exc_frame->a4, exc_frame->a5, exc_frame->cause, exc_frame->epc);
#endif

    if (PRV_M == mode) {
        /* msubm is exclusive to machine mode */
        rt_kprintf("msubm: 0x%lx\n", exc_frame->msubm);
    }
}

/**
 * \brief      System Default Exception Handler
 * \details
 * This function provides a default exception and NMI handler for all exception ids.
 * By default, It will just print some information for debug, Vendor can customize it according to its requirements.
 * \param [in]  mcause    code indicating the reason that caused the trap in machine mode
 * \param [in]  sp        stack pointer
 */
static void system_default_exception_handler(unsigned long mcause, unsigned long sp)
{
    /* TODO: Uncomment this if you have implement printf function */
    rt_kprintf("MCAUSE : 0x%lx\r\n", mcause);
    rt_kprintf("MDCAUSE: 0x%lx\r\n", __RV_CSR_READ(CSR_MDCAUSE));
    rt_kprintf("MEPC   : 0x%lx\r\n", __RV_CSR_READ(CSR_MEPC));
    rt_kprintf("MTVAL  : 0x%lx\r\n", __RV_CSR_READ(CSR_MTVAL));
    rt_kprintf("HARTID : %u\r\n", (unsigned int)__get_hart_id());

    Exception_DumpFrame(sp, PRV_M);

    while (1);
}
/**
 * \brief      Initialize all the default core exception handlers
 * \details
 * The core exception handler for each exception id will be initialized to \ref system_default_exception_handler.
 * \note
 * Called in \ref _init function, used to initialize default exception handlers for all exception IDs
 * SystemExceptionHandlers contains NMI, but SystemExceptionHandlers_S not, because NMI can't be delegated to S-mode.
 */
void Exception_Init(void)
{
    for (int i = 0; i < MAX_SYSTEM_EXCEPTION_NUM; i++) {
        SystemExceptionHandlers[i] = (unsigned long)system_default_exception_handler;
    }
    SystemExceptionHandlers[MAX_SYSTEM_EXCEPTION_NUM] = (unsigned long)system_default_exception_handler;
}

/**
 * \brief       Register an exception handler for exception code EXCn
 * \details
 * - For EXCn < \ref MAX_SYSTEM_EXCEPTION_NUM, it will be registered into SystemExceptionHandlers[EXCn-1].
 * - For EXCn == NMI_EXCn, it will be registered into SystemExceptionHandlers[MAX_SYSTEM_EXCEPTION_NUM].
 * \param [in]  EXCn    See \ref EXCn_Type
 * \param [in]  exc_handler     The exception handler for this exception code EXCn
 */
void Exception_Register_EXC(rt_uint32_t EXCn, unsigned long exc_handler)
{
    if (EXCn < MAX_SYSTEM_EXCEPTION_NUM) {
        SystemExceptionHandlers[EXCn] = exc_handler;
    } else if (EXCn == NMI_EXCn) {
        SystemExceptionHandlers[MAX_SYSTEM_EXCEPTION_NUM] = exc_handler;
    }
}

/**
 * \brief       Get current exception handler for exception code EXCn
 * \details
 * - For EXCn < \ref MAX_SYSTEM_EXCEPTION_NUM, it will return SystemExceptionHandlers[EXCn-1].
 * - For EXCn == NMI_EXCn, it will return SystemExceptionHandlers[MAX_SYSTEM_EXCEPTION_NUM].
 * \param [in]  EXCn    See \ref EXCn_Type
 * \return  Current exception handler for exception code EXCn, if not found, return 0.
 */
unsigned long Exception_Get_EXC(rt_uint32_t EXCn)
{
    if (EXCn < MAX_SYSTEM_EXCEPTION_NUM) {
        return SystemExceptionHandlers[EXCn];
    } else if (EXCn == NMI_EXCn) {
        return SystemExceptionHandlers[MAX_SYSTEM_EXCEPTION_NUM];
    } else {
        return 0;
    }
}

/**
 * \brief      Common NMI and Exception handler entry
 * \details
 * This function provided a command entry for NMI and exception. Silicon Vendor could modify
 * this template implementation according to requirement.
 * \param [in]  mcause    code indicating the reason that caused the trap in machine mode
 * \param [in]  sp        stack pointer
 * \remarks
 * - RISCV provided common entry for all types of exception. This is proposed code template
 *   for exception entry function, Silicon Vendor could modify the implementation.
 * - For the core_exception_handler template, we provided exception register function \ref Exception_Register_EXC
 *   which can help developer to register your exception handler for specific exception number.
 */
rt_uint32_t core_exception_handler(unsigned long mcause, unsigned long sp)
{
    rt_uint32_t EXCn = (rt_uint32_t)(mcause & 0X00000fff);
    EXC_HANDLER exc_handler;

    if (EXCn < MAX_SYSTEM_EXCEPTION_NUM) {
        exc_handler = (EXC_HANDLER)SystemExceptionHandlers[EXCn];
    } else if (EXCn == NMI_EXCn) {
        exc_handler = (EXC_HANDLER)SystemExceptionHandlers[MAX_SYSTEM_EXCEPTION_NUM];
    } else {
        exc_handler = (EXC_HANDLER)system_default_exception_handler;
    }
    if (exc_handler != NULL) {
        exc_handler(mcause, sp);
    }
    return 0;
}

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

    for (idx = SOC_EXTERNAL_MAP_TO_ECLIC_IRQn_OFFSET; idx < SOC_INT_MAX; idx++)
    {
        isr_irq_table[idx].handler = (rt_isr_handler_t)rt_hw_interrupt_handle;
        isr_irq_table[idx].param = RT_NULL;

	ECLIC_SetLevelIRQ(idx, configKERENL_INTERRUPT_PRIORITY);
	ECLIC_SetShvIRQ(idx, ECLIC_NON_VECTOR_INTERRUPT);
	ECLIC_DisableIRQ(idx);
	ECLIC_ClearPendingIRQ(idx);
	ECLIC_SetTrigIRQ(idx, ECLIC_LEVEL_TRIGGER); /* level trigger */
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
	ECLIC_DisableIRQ(vector);
}

void rt_hw_interrupt_umask(int vector)
{
	ECLIC_EnableIRQ(vector);
}

rt_uint32_t rt_hw_interrupt_is_enabled(int vector)
{
	return ECLIC_GetEnableIRQ(vector);
}

rt_uint32_t rt_hw_interrupt_is_pending(int vector)
{
       return ECLIC_GetPendingIRQ(vector);	
}

void rt_hw_interrupt_clear_pending(int vector)
{
	ECLIC_ClearPendingIRQ(vector);
}

void rt_hw_interrupt_set_pending(int vector)
{
	ECLIC_SetPendingIRQ(vector);
}

void Spacemit_External_IRQHandler(void)
{
	CSR_MCAUSE_Type mcause = (CSR_MCAUSE_Type)__RV_CSR_READ(CSR_MCAUSE) ;

	if (mcause.b.interrupt == 1) {
		rt_interrupt_enter();
   		isr_irq_table[mcause.b.exccode].handler(mcause.b.exccode,
				isr_irq_table[mcause.b.exccode].param);
		rt_interrupt_leave();
	}
}

#ifdef BSP_USING_PM

static unsigned int ecli_save_reg[SOC_INT_MAX];

void rt_hw_eclic_save(void)
{
    int idx = 0;

    for (idx = SOC_EXTERNAL_MAP_TO_ECLIC_IRQn_OFFSET; idx < SOC_INT_MAX; idx++)
    {
	    ecli_save_reg[idx] = ECLIC->CTRL[idx].INTIE;
    }

    /* disable all the irqs */
    for (idx = 0; idx < SOC_INT_MAX; idx++)
    {
	    rt_hw_interrupt_mask(idx);
	    rt_hw_interrupt_clear_pending(idx);
    }
}

void rt_hw_eclic_restore(void)
{
    int idx = 0;

    for (idx = SOC_EXTERNAL_MAP_TO_ECLIC_IRQn_OFFSET; idx < SOC_INT_MAX; idx++)
    {
	ECLIC_SetLevelIRQ(idx, configKERENL_INTERRUPT_PRIORITY);
	ECLIC_SetShvIRQ(idx, ECLIC_NON_VECTOR_INTERRUPT);
	ECLIC_SetTrigIRQ(idx, ECLIC_LEVEL_TRIGGER); /* level trigger */

	ECLIC->CTRL[idx].INTIE = ecli_save_reg[idx];
    }

}
#endif
