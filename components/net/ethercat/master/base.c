/*
 * Copyright (c) 2025, Spacemit Corporation
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * base.c — Implements interfaces and helpers
 * declared in base.h.
 */
#include "base.h"
#include <rthw.h>
#include <rtdevice.h>
#include <rtservice.h>

uint8_t err_ptr_pool[EC_MAX_ERRNO + 1];

/**
 * @brief Remove a node from the list without reinitializing it.
 *
 * This function is an extension to the list interface, providing a variant
 * of node removal that does not reset the node's pointers. It is similar to
 * Linux kernel's list_del(), and is useful in cases where the node's
 * next/prev pointers need to be inspected after removal.
 *
 * @param n The node to remove from the list.
 */
void ec_rt_list_delete(rt_list_t *n)
{
	n->next->prev = n->prev;
	n->prev->next = n->next;
}

/**
 * Wake up all threads in the wait queue.
 *
 * Iterate through the wait queue and wake up each thread
 * whose wakeup condition is met.
 */
void ec_rt_wqueue_wakeup_all(rt_wqueue_t *queue, void *key)
{
	rt_base_t level;
	register int need_schedule = 0;

	rt_list_t *queue_list;
	struct rt_list_node *node, *next;
	struct rt_wqueue_node *entry;

	queue_list = &(queue->waiting_list);

	level = rt_hw_interrupt_disable();
	/* set wakeup flag in the queue */
	queue->flag = RT_WQ_FLAG_WAKEUP;

	if (!(rt_list_isempty(queue_list))) {
		rt_list_for_each_safe(node, next, queue_list) {
			entry = rt_list_entry(node, struct rt_wqueue_node, list);
			if (entry->wakeup(entry, key) == 0) {
				rt_thread_resume(entry->polling_thread);
				need_schedule = 1;
				rt_wqueue_remove(entry);
			}
		}
	}

	rt_hw_interrupt_enable(level);

	if (need_schedule)
		rt_schedule();
}

/**
 * @brief Divide a 64-bit unsigned integer by a 32-bit unsigned base.
 *
 * @param n Pointer to the 64-bit dividend; result (quotient) is written back to it.
 * @param base 32-bit divisor.
 * @return uint32_t The remainder of the division.
 */
uint32_t ec_u64_do_div(uint64_t *n, uint32_t base)
{
	uint64_t dividend = *n;
	uint64_t divisor  = base;
	uint64_t quotient = 0;
	uint64_t bit      = 1;
	uint64_t rem      = dividend;

	while ((divisor << 1) <= rem) {
		divisor <<= 1;
		bit     <<= 1;
	}

	while (bit) {
		if (rem >= divisor) {
			rem      -= divisor;
			quotient |= bit;
		}
		divisor >>= 1;
		bit     >>= 1;
	}

	*n = quotient;
	return (uint32_t)rem;
}

rt_bool_t kthread_should_stop(void)
{
	return RT_FALSE;
}

rt_uint8_t rt_thread_get_current_priority(void)
{
	rt_thread_t current_thread = rt_thread_self();
	if (current_thread != RT_NULL) {
		return current_thread->current_priority;
	}
	return RT_THREAD_PRIORITY_MAX;
}

rt_sem_t ec_sema_init(rt_uint32_t val)
{
	static int sem_counter;
	char sem_name[RT_NAME_MAX];

	rt_snprintf(sem_name, RT_NAME_MAX, "sem%d", sem_counter++);

	return rt_sem_create(sem_name, val, RT_IPC_FLAG_FIFO);
}

rt_mutex_t ec_mutex_init(rt_mutex_t rt_mutex)
{
	static int mutex_counter;
	char mutex_name[RT_NAME_MAX];

	rt_snprintf(mutex_name, RT_NAME_MAX, "ecmtx%d", mutex_counter++);

	return rt_mutex_create(mutex_name, RT_IPC_FLAG_PRIO);
}
