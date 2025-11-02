/*
 * Copyright (c) 2025, Spacemit Corporation
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * base.h — Defines basic data structures and interfaces
 * required by the code.
 */

#ifndef __RT_BASE_H__
#define __RT_BASE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <rtdef.h>
#include <rtm.h>
#include <rtthread.h>
#include <rtservice.h>
#include <ipc/waitqueue.h>

//C lib
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>	//for off_t
#include <errno.h>

/* list interface */
void ec_rt_list_delete(rt_list_t *n);

typedef rt_list_t list_head;

#define INIT_LIST_HEAD(l)    rt_list_init(l)

#define list_empty(l)        rt_list_isempty(l)

#define list_del_init(l)     rt_list_remove(l)

#define list_for_each_entry(pos, head, member) \
		rt_list_for_each_entry(pos, head, member)

#define list_for_each_entry_safe(pos, n, head, member) \
		rt_list_for_each_entry_safe(pos, n, head, member)

#define list_entry(node, type, member) \
		rt_list_entry(node, type, member)

#define list_add_tail(node, head) \
		rt_list_insert_before(head, node)

#define list_del(l)        ec_rt_list_delete(l)

#define list_for_each_entry_from(pos, head, member)  \
	for (pos = rt_list_entry((head)->next, typeof(*pos), member); \
		 &pos->member != (head) && pos != (head); \
		 pos = rt_list_entry(pos->member.next, typeof(*pos), member))

/* mutex interface */
typedef rt_mutex_t mutex;

rt_mutex_t ec_mutex_init(rt_mutex_t rt_mutex);

#define mutex_lock(rt_mutex)    rt_mutex_take(rt_mutex, RT_WAITING_FOREVER)

#define mutex_unlock(rt_mutex)  rt_mutex_release(rt_mutex)

#define ec_rt_lock_interruptible(rt_mutex) mutex_lock(rt_mutex)

/* semaphore interface */
typedef rt_sem_t semaphore;

rt_sem_t ec_sema_init(rt_uint32_t val);

#define down(rt_semaphore) rt_sem_take(rt_semaphore, RT_WAITING_FOREVER)

#define down_interruptible(rt_semaphore) down(rt_semaphore)

#define down_trylock(rt_semaphore) rt_sem_trytake(rt_semaphore)

#define up(rt_semaphore) rt_sem_release(rt_semaphore)

/* thread interface */
typedef rt_thread_t task_struct;

/* wait queue interface */
void ec_rt_wqueue_wakeup_all(rt_wqueue_t *queue, void *key);

#define wait_queue_head_t    rt_wqueue_t

#define init_waitqueue_head(q)   rt_wqueue_init(q)

#define wake_up(q)     rt_wqueue_wakeup(q, NULL)

#define wake_up_all(q)     ec_rt_wqueue_wakeup_all(q, NULL)

#define wake_up_interruptible(q)     rt_wqueue_wakeup(q, NULL)

#define wait_event(queue, condition)	\
do {					\
	if (condition) {		\
		break;			\
	}				\
	rt_wqueue_wait(queue, 0, RT_WAITING_FOREVER);	\
} while (1)

/*
#define wait_event_interruptible(queue, condition) \
		wait_event(queue, condition)
*/
#define wait_event_interruptible(queue, condition)	\
({							\
	int __ret = 0;					\
	wait_event(queue, condition);			\
	__ret;						\
})

/* unlikely and likely macros */
#ifndef unlikely
#define unlikely(x)   __builtin_expect(!!(x), 0)
#endif

#ifndef likely
#define likely(x)     __builtin_expect(!!(x), 1)
#endif

/* min & max */
#ifndef min
#define min(x, y)  ((x) < (y) ? (x) : (y))
#endif

#ifndef max
#define max(x, y)  ((x) < (y) ? (y) : (x))
#endif

/* HZ */
#define HZ RT_TICK_PER_SECOND

/* true/false */
#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

/* IS_ERR/ */
#define EC_MAX_ERRNO 255
extern uint8_t err_ptr_pool[EC_MAX_ERRNO + 1];

static inline void *err_to_ptr(int err)
{
	if (err >= 0 || err <= -EC_MAX_ERRNO - 1) {
		return NULL;
	}
	return (void *)(&err_ptr_pool[(-err) & EC_MAX_ERRNO]);
}

static inline int ptr_to_err(const void *ptr)
{
	const uint8_t *p = (const uint8_t *)ptr;
	if (p < &err_ptr_pool[0] || p >= &err_ptr_pool[EC_MAX_ERRNO + 1]) {
		return 0;
	}
	return -(p - &err_ptr_pool[0]);
}

static inline int ptr_is_err(const void *ptr)
{
	const uint8_t *p = (const uint8_t *)ptr;
	return (p >= &err_ptr_pool[0] && p < &err_ptr_pool[EC_MAX_ERRNO + 1]);
}

/* error no */
#ifndef EOVERFLOW
#define EOVERFLOW	139
#endif

#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT	93
#endif

#ifndef ENOBUFS
#define ENOBUFS		105
#endif

#ifndef EPROTO
#define EPROTO		71
#endif

/* ethernet */
#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#ifndef ETH_HLEN
#define ETH_HLEN 14
#endif

#ifndef ETH_ZLEN
#define ETH_ZLEN 60
#endif

#define ARCH_DMA_MINALIGN 64

#define ECAT_BUF_SIZE RT_ALIGN(1514, ARCH_DMA_MINALIGN)

/* do_div */
uint32_t ec_u64_do_div(uint64_t *n, uint32_t base);

/* get thread priority */
rt_uint8_t rt_thread_get_current_priority(void);

rt_bool_t kthread_should_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* __RT_BASE_H__ */
