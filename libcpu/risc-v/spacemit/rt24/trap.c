/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024/01/11     flyingcys    The first version
 */

#include <rthw.h>
#include <rtthread.h>
#include <riscv_encoding.h>


struct exception_stack_frame
{
    rt_uint64_t x1;
    rt_uint64_t x2;
    rt_uint64_t x3;
    rt_uint64_t x4;
    rt_uint64_t x5;
    rt_uint64_t x6;
    rt_uint64_t x7;
    rt_uint64_t x8;
    rt_uint64_t x9;
    rt_uint64_t x10;
    rt_uint64_t x11;
    rt_uint64_t x12;
    rt_uint64_t x13;
    rt_uint64_t x14;
    rt_uint64_t x15;
    rt_uint64_t x16;
    rt_uint64_t x17;
    rt_uint64_t x18;
    rt_uint64_t x19;
    rt_uint64_t x20;
    rt_uint64_t x21;
    rt_uint64_t x22;
    rt_uint64_t x23;
    rt_uint64_t x24;
    rt_uint64_t x25;
    rt_uint64_t x26;
    rt_uint64_t x27;
    rt_uint64_t x28;
    rt_uint64_t x29;
    rt_uint64_t x30;
    rt_uint64_t x31;
};

static void print_stack_frame(rt_uint64_t * sp)
{
    struct exception_stack_frame * esf = (struct exception_stack_frame *)(sp+1);

    rt_kprintf("\n=================================================================\n");
    rt_kprintf("x1 (ra   : Return address                ) ==> 0x%08x%08x\n", esf->x1 >> 32  , esf->x1 & RT_UINT32_MAX);
    rt_kprintf("x2 (sp   : Stack pointer                 ) ==> 0x%08x%08x\n", esf->x2 >> 32  , esf->x2 & RT_UINT32_MAX);
    rt_kprintf("x3 (gp   : Global pointer                ) ==> 0x%08x%08x\n", esf->x3 >> 32  , esf->x3 & RT_UINT32_MAX);
    rt_kprintf("x4 (tp   : Thread pointer                ) ==> 0x%08x%08x\n", esf->x4 >> 32  , esf->x4 & RT_UINT32_MAX);
    rt_kprintf("x5 (t0   : Temporary                     ) ==> 0x%08x%08x\n", esf->x5 >> 32  , esf->x5 & RT_UINT32_MAX);
    rt_kprintf("x6 (t1   : Temporary                     ) ==> 0x%08x%08x\n", esf->x6 >> 32  , esf->x6 & RT_UINT32_MAX);
    rt_kprintf("x7 (t2   : Temporary                     ) ==> 0x%08x%08x\n", esf->x7 >> 32  , esf->x7 & RT_UINT32_MAX);
    rt_kprintf("x8 (s0/fp: Save register,frame pointer   ) ==> 0x%08x%08x\n", esf->x8 >> 32  , esf->x8 & RT_UINT32_MAX);
    rt_kprintf("x9 (s1   : Save register                 ) ==> 0x%08x%08x\n", esf->x9 >> 32  , esf->x9 & RT_UINT32_MAX);
    rt_kprintf("x10(a0   : Function argument,return value) ==> 0x%08x%08x\n", esf->x10 >> 32 , esf->x10 & RT_UINT32_MAX);
    rt_kprintf("x11(a1   : Function argument,return value) ==> 0x%08x%08x\n", esf->x11 >> 32 , esf->x11 & RT_UINT32_MAX);
    rt_kprintf("x12(a2   : Function argument             ) ==> 0x%08x%08x\n", esf->x12 >> 32 , esf->x12 & RT_UINT32_MAX);
    rt_kprintf("x13(a3   : Function argument             ) ==> 0x%08x%08x\n", esf->x13 >> 32 , esf->x13 & RT_UINT32_MAX);
    rt_kprintf("x14(a4   : Function argument             ) ==> 0x%08x%08x\n", esf->x14 >> 32 , esf->x14 & RT_UINT32_MAX);
    rt_kprintf("x15(a5   : Function argument             ) ==> 0x%08x%08x\n", esf->x15 >> 32 , esf->x15 & RT_UINT32_MAX);
    rt_kprintf("x16(a6   : Function argument             ) ==> 0x%08x%08x\n", esf->x16 >> 32 , esf->x16 & RT_UINT32_MAX);
    rt_kprintf("x17(a7   : Function argument             ) ==> 0x%08x%08x\n", esf->x17 >> 32 , esf->x17 & RT_UINT32_MAX);
    rt_kprintf("x18(s2   : Save register                 ) ==> 0x%08x%08x\n", esf->x18 >> 32 , esf->x18 & RT_UINT32_MAX);
    rt_kprintf("x19(s3   : Save register                 ) ==> 0x%08x%08x\n", esf->x19 >> 32 , esf->x19 & RT_UINT32_MAX);
    rt_kprintf("x20(s4   : Save register                 ) ==> 0x%08x%08x\n", esf->x20 >> 32 , esf->x20 & RT_UINT32_MAX);
    rt_kprintf("x21(s5   : Save register                 ) ==> 0x%08x%08x\n", esf->x21 >> 32 , esf->x21 & RT_UINT32_MAX);
    rt_kprintf("x22(s6   : Save register                 ) ==> 0x%08x%08x\n", esf->x22 >> 32 , esf->x22 & RT_UINT32_MAX);
    rt_kprintf("x23(s7   : Save register                 ) ==> 0x%08x%08x\n", esf->x23 >> 32 , esf->x23 & RT_UINT32_MAX);
    rt_kprintf("x24(s8   : Save register                 ) ==> 0x%08x%08x\n", esf->x24 >> 32 , esf->x24 & RT_UINT32_MAX);
    rt_kprintf("x25(s9   : Save register                 ) ==> 0x%08x%08x\n", esf->x25 >> 32 , esf->x25 & RT_UINT32_MAX);
    rt_kprintf("x26(s10  : Save register                 ) ==> 0x%08x%08x\n", esf->x26 >> 32 , esf->x26 & RT_UINT32_MAX);
    rt_kprintf("x27(s11  : Save register                 ) ==> 0x%08x%08x\n", esf->x27 >> 32 , esf->x27 & RT_UINT32_MAX);
    rt_kprintf("x28(t3   : Temporary                     ) ==> 0x%08x%08x\n", esf->x28 >> 32 , esf->x28 & RT_UINT32_MAX);
    rt_kprintf("x29(t4   : Temporary                     ) ==> 0x%08x%08x\n", esf->x29 >> 32 , esf->x29 & RT_UINT32_MAX);
    rt_kprintf("x30(t5   : Temporary                     ) ==> 0x%08x%08x\n", esf->x30 >> 32 , esf->x30 & RT_UINT32_MAX);
    rt_kprintf("x31(t6   : Temporary                     ) ==> 0x%08x%08x\n", esf->x31 >> 32 , esf->x31 & RT_UINT32_MAX);
    rt_kprintf("=================================================================\n");
}

RT_WEAK void rt_hw_soft_irq_isr(void)
{

}

extern void rt_hw_tick_isr(void);
extern void rt_hw_irq_isr(void);

RT_WEAK rt_size_t handle_trap(rt_size_t cause, rt_size_t epc, rt_size_t *sp)
{
    if (cause & (1UL << (__riscv_xlen - 1)))          //interrupt
    {
        if ((cause & 0x1f) == IRQ_M_SOFT)
        {
            rt_hw_soft_irq_isr();
        }
        else if ((cause & 0x1f) == IRQ_M_TIMER || (cause & 0x1f) == IRQ_S_TIMER)
        {
            rt_hw_tick_isr();
        }
        else if ((cause & 0x1f) == IRQ_M_EXT)
        {
            rt_hw_irq_isr();
        }
    }
    else
    {
        rt_thread_t tid;
#if defined(RT_USING_FINSH) && defined(MSH_USING_BUILT_IN_COMMANDS)
        extern long list_thread();
#endif

        rt_hw_interrupt_disable();

        tid = rt_thread_self();
        rt_kprintf("\nException:\n");
        switch (cause)
        {
            case CAUSE_MISALIGNED_FETCH:
                rt_kprintf("Instruction address misaligned");
                break;
            case CAUSE_FAULT_FETCH:
                rt_kprintf("Instruction access fault");
                break;
            case CAUSE_ILLEGAL_INSTRUCTION:
                rt_kprintf("Illegal instruction");
                break;
            case CAUSE_BREAKPOINT:
                rt_kprintf("Breakpoint");
                break;
            case CAUSE_MISALIGNED_LOAD:
                rt_kprintf("Load address misaligned");
                break;
            case CAUSE_FAULT_LOAD:
                rt_kprintf("Load access fault");
                break;
            case CAUSE_MISALIGNED_STORE:
                rt_kprintf("Store address misaligned");
                break;
            case CAUSE_FAULT_STORE:
                rt_kprintf("Store access fault");
                break;
            case CAUSE_USER_ECALL:
                rt_kprintf("Environment call from U-mode");
                break;
            case CAUSE_SUPERVISOR_ECALL:
                rt_kprintf("Environment call from S-mode");
                break;
            case CAUSE_HYPERVISOR_ECALL:
                rt_kprintf("Environment call from H-mode");
                break;
            case CAUSE_MACHINE_ECALL:
                rt_kprintf("Environment call from M-mode");
                break;
            default:
                rt_kprintf("Uknown exception : %08lX", cause);
                break;
        }
        rt_kprintf("\n");
        print_stack_frame(sp);
        rt_kprintf("exception pc => 0x%08x\n", epc);
        rt_kprintf("current thread: %.*s\n", RT_NAME_MAX, tid->name);
#if defined(RT_USING_FINSH) && defined(MSH_USING_BUILT_IN_COMMANDS)
        list_thread();
#endif
        while(1);
    }

    return epc;
}
