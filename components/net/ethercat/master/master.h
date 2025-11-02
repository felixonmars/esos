/*****************************************************************************
 *
 *  Copyright (C) 2006-2024  Florian Pose, Ingenieurgemeinschaft IgH
 *
 *  This file is part of the IgH EtherCAT Master.
 *
 *  The IgH EtherCAT Master is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version 2, as
 *  published by the Free Software Foundation.
 *
 *  The IgH EtherCAT Master is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *  Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with the IgH EtherCAT Master; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  -------------------------------------------------------------------------
 *  Copyright (c) 2025, Spacemit Corporation
 *
 *  This file is the RT-Thread version developed from the original file.
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 ****************************************************************************/

/**
 * \file
 * EtherCAT master structure.
 */

/****************************************************************************/

#ifndef __EC_MASTER_H__
#define __EC_MASTER_H__

#include "base.h"
#include "device.h"
#include "domain.h"
#include "fsm_master.h"

/****************************************************************************/

/** Convenience macro for printing master-specific information to syslog.
 *
 * This will print the message in \a fmt with a prefixed "EtherCAT <INDEX>: ",
 * where INDEX is the master index.
 *
 * \param master EtherCAT master
 * \param fmt format string (like in printf())
 * \param args arguments (optional)
 */
#define EC_MASTER_INFO(master, fmt, args...) \
	rt_kprintf("EtherCAT %u: " fmt, master->index, ##args)

/** Convenience macro for printing master-specific errors to syslog.
 *
 * This will print the message in \a fmt with a prefixed "EtherCAT <INDEX>: ",
 * where INDEX is the master index.
 *
 * \param master EtherCAT master
 * \param fmt format string (like in printf())
 * \param args arguments (optional)
 */
#define EC_MASTER_ERR(master, fmt, args...) \
	rt_kprintf("EtherCAT ERROR %u: " fmt, master->index, ##args)

/** Convenience macro for printing master-specific warnings to syslog.
 *
 * This will print the message in \a fmt with a prefixed "EtherCAT <INDEX>: ",
 * where INDEX is the master index.
 *
 * \param master EtherCAT master
 * \param fmt format string (like in printf())
 * \param args arguments (optional)
 */
#define EC_MASTER_WARN(master, fmt, args...) \
	rt_kprintf("EtherCAT WARNING %u: " fmt, master->index, ##args)

/** Convenience macro for printing master-specific debug messages to syslog.
 *
 * This will print the message in \a fmt with a prefixed "EtherCAT <INDEX>: ",
 * where INDEX is the master index.
 *
 * \param master EtherCAT master
 * \param level Debug level. Master's debug level must be >= \a level for
 * output.
 * \param fmt format string (like in printf())
 * \param args arguments (optional)
 */
#define EC_MASTER_DBG(master, level, fmt, args...) \
	do { \
		if (master->debug_level >= level) { \
			rt_kprintf("EtherCAT DEBUG %u: " fmt, \
				   master->index, ##args); \
		} \
	} while (0)

/** Size of the external datagram ring.
 *
 * The external datagram ring is used for slave FSMs.
 */
#define EC_EXT_RING_SIZE 32

/** Maximum number of masters.
 */
#define EC_MAX_MASTERS 32

/****************************************************************************/

/** EtherCAT master phase.
 */
typedef enum {
	EC_ORPHANED, /**< Orphaned phase. The master has no Ethernet device
		       attached. */
	EC_IDLE, /**< Idle phase. An Ethernet device is attached, but the master
		   is not in use, yet. */
	EC_OPERATION /**< Operation phase. The master was requested by a realtime
		       application. */
} ec_master_phase_t;

/****************************************************************************/

/** Cyclic statistics.
 */
typedef struct {
	unsigned int timeouts; /**< datagram timeouts */
	unsigned int corrupted; /**< corrupted frames */
	unsigned int unmatched; /**< unmatched datagrams (received, but not
				  queued any longer) */
	unsigned long output_jiffies; /**< time of last output */
} ec_stats_t;

/****************************************************************************/

/** Device statistics.
 */
typedef struct {
	uint64_t tx_count; /**< Number of frames sent. */
	uint64_t last_tx_count; /**< Number of frames sent of last statistics cycle. */
	uint64_t rx_count; /**< Number of frames received. */
	uint64_t last_rx_count; /**< Number of frames received of last statistics
				  cycle. */
	uint64_t tx_bytes; /**< Number of bytes sent. */
	uint64_t last_tx_bytes; /**< Number of bytes sent of last statistics cycle. */
	uint64_t rx_bytes; /**< Number of bytes received. */
	uint64_t last_rx_bytes; /**< Number of bytes received of last statistics cycle.
				  */
	uint64_t last_loss; /**< Tx/Rx difference of last statistics cycle. */
	int32_t tx_frame_rates[EC_RATE_COUNT]; /**< Transmit rates in frames/s for
						 different statistics cycle periods.
						 */
	int32_t rx_frame_rates[EC_RATE_COUNT]; /**< Receive rates in frames/s for
						 different statistics cycle periods.
						 */
	int32_t tx_byte_rates[EC_RATE_COUNT]; /**< Transmit rates in byte/s for
						different statistics cycle periods. */
	int32_t rx_byte_rates[EC_RATE_COUNT]; /**< Receive rates in byte/s for
						different statistics cycle periods. */
	int32_t loss_rates[EC_RATE_COUNT]; /**< Frame loss rates for different
					     statistics cycle periods. */
	unsigned long jiffies; /**< Jiffies of last statistic cycle. */
} ec_device_stats_t;

/****************************************************************************/

#if EC_MAX_NUM_DEVICES < 1
#error Invalid number of devices
#endif

/****************************************************************************/

/** EtherCAT master.
 *
 * Manages slaves, domains and IO.
 */
struct ec_master {
	unsigned int index; /**< Index. */
	unsigned int reserved; /**< \a True, if the master is in use. */

	rt_device_t class_device; /**< Master class device. */

	semaphore master_sem; /**< Master semaphore. */

	ec_device_t devices[EC_MAX_NUM_DEVICES]; /**< EtherCAT devices. */
	const uint8_t *macs[EC_MAX_NUM_DEVICES]; /**< Device MAC addresses. */
#if EC_MAX_NUM_DEVICES > 1
	unsigned int num_devices; /**< Number of devices. Access this always via
				    ec_master_num_devices(), because it may be
				    optimized! */
#endif
	semaphore device_sem; /**< Device semaphore. */
	ec_device_stats_t device_stats; /**< Device statistics. */

	ec_fsm_master_t fsm; /**< Master state machine. */
	ec_datagram_t fsm_datagram; /**< Datagram used for state machines. */
	ec_master_phase_t phase; /**< Master phase. */
	unsigned int active; /**< Master has been activated. */
	unsigned int config_changed; /**< The configuration changed. */
	unsigned int injection_seq_fsm; /**< Datagram injection sequence number
					  for the FSM side. */
	unsigned int injection_seq_rt; /**< Datagram injection sequence number
					 for the realtime side. */

	ec_slave_t *slaves; /**< Array of slaves on the bus. */
	unsigned int slave_count; /**< Number of slaves on the bus. */

	/* Configuration applied by the application. */
	list_head configs; /**< List of slave configurations. */
	list_head domains; /**< List of domains. */

	uint64_t app_time; /**< Time of the last ecrt_master_sync() call. */
	uint64_t dc_ref_time; /**< Common reference timestamp for DC start times. */
	ec_datagram_t ref_sync_datagram; /**< Datagram used for synchronizing the
					   reference clock to the master clock. */
	ec_datagram_t sync_datagram; /**< Datagram used for DC drift
				       compensation. */
	ec_datagram_t sync_mon_datagram; /**< Datagram used for DC synchronisation
					   monitoring. */
	ec_slave_config_t *dc_ref_config; /**< Application-selected DC reference
					    clock slave config. */
	ec_slave_t *dc_ref_clock; /**< DC reference clock slave. */

	unsigned int scan_busy; /**< Current scan state. */
	unsigned int scan_index; /**< Index of slave currently scanned. */
	unsigned int allow_scan; /**< \a True, if slave scanning is allowed. */
	semaphore scan_sem; /**< Semaphore protecting the \a scan_busy
			      variable and the \a allow_scan flag. */
	wait_queue_head_t scan_queue; /**< Queue for processes that wait for
					slave scanning. */

	unsigned int config_busy; /**< State of slave configuration. */
	semaphore config_sem; /**< Semaphore protecting the \a config_busy
				variable and the allow_config flag. */
	wait_queue_head_t config_queue; /**< Queue for processes that wait for
					  slave configuration. */

	list_head datagram_queue; /**< Datagram queue. */
	uint8_t datagram_index; /**< Current datagram index. */

	list_head ext_datagram_queue; /**< Queue for non-application
					datagrams. */
	semaphore ext_queue_sem; /**< Semaphore protecting the \a
				   ext_datagram_queue. */

	ec_datagram_t ext_datagram_ring[EC_EXT_RING_SIZE]; /**< External datagram
							     ring. */
	unsigned int ext_ring_idx_rt; /**< Index in external datagram ring for RT
					side. */
	unsigned int ext_ring_idx_fsm; /**< Index in external datagram ring for
					 FSM side. */
	unsigned int send_interval; /**< Interval between two calls to
				      ecrt_master_send(). */
	size_t max_queue_size; /**< Maximum size of datagram queue */

	ec_slave_t *fsm_slave; /**< Slave that is queried next for FSM exec. */
	list_head fsm_exec_list; /**< Slave FSM execution list. */
	unsigned int fsm_exec_count; /**< Number of entries in execution list. */

	unsigned int debug_level; /**< Master debug level. */
	unsigned int run_on_cpu;  /**< bind kernel threads to this cpu */
	ec_stats_t stats; /**< Cyclic statistics. */

	task_struct thread; /**< Master thread. */

	mutex io_mutex;  /**< Mutex used in \a IDLE and \a OP phase. */

	void (*send_cb)(void *); /**< Current send datagrams callback. */
	void (*receive_cb)(void *); /**< Current receive datagrams callback. */
	void *cb_data; /**< Current callback data. */
	void (*app_send_cb)(void *); /**< Application's send datagrams
				       callback. */
	void (*app_receive_cb)(void *); /**< Application's receive datagrams
					  callback. */
	void *app_cb_data; /**< Application callback data. */

	list_head sii_requests; /**< SII write requests. */
	list_head emerg_reg_requests; /**< Emergency register access
					requests. */

	wait_queue_head_t request_queue; /**< Wait queue for external requests
					   from user space. */
};

/****************************************************************************/

// static funtions
void ec_master_init_static(void);

// master creation/deletion
int ec_master_init(ec_master_t *master, /**< EtherCAT master */
	unsigned int index, /**< master index */
	const uint8_t *main_mac, /**< MAC address of main device */
	const uint8_t *backup_mac, /**< MAC address of backup device */
	unsigned int debug_level, /**< Debug level (module parameter). */
	unsigned int run_on_cpu /**< bind created kernel threads to a cpu */
	);
void ec_master_clear(ec_master_t *);

/** Number of Ethernet devices.
 */
#if EC_MAX_NUM_DEVICES > 1
#define ec_master_num_devices(MASTER) ((MASTER)->num_devices)
#else
#define ec_master_num_devices(MASTER) 1
#endif

// phase transitions
int ec_master_enter_idle_phase(ec_master_t *);
void ec_master_leave_idle_phase(ec_master_t *);
int ec_master_enter_operation_phase(ec_master_t *);
void ec_master_leave_operation_phase(ec_master_t *);

// datagram IO
void ec_master_receive_datagrams(ec_master_t *, ec_device_t *,
		const uint8_t *, size_t);
void ec_master_queue_datagram(ec_master_t *, ec_datagram_t *);
void ec_master_queue_datagram_ext(ec_master_t *, ec_datagram_t *);

// misc.
void ec_master_set_send_interval(ec_master_t *, unsigned int);
void ec_master_attach_slave_configs(ec_master_t *);
ec_slave_t *ec_master_find_slave(ec_master_t *, uint16_t, uint16_t);
const ec_slave_t *ec_master_find_slave_const(const ec_master_t *, uint16_t,
		uint16_t);
void ec_master_output_stats(ec_master_t *);

void ec_master_clear_slaves(ec_master_t *);

unsigned int ec_master_config_count(const ec_master_t *);
ec_slave_config_t *ec_master_get_config(
		const ec_master_t *, unsigned int);
const ec_slave_config_t *ec_master_get_config_const(
		const ec_master_t *, unsigned int);
unsigned int ec_master_domain_count(const ec_master_t *);
ec_domain_t *ec_master_find_domain(ec_master_t *, unsigned int);
const ec_domain_t *ec_master_find_domain_const(const ec_master_t *,
		unsigned int);

int ec_master_debug_level(ec_master_t *, unsigned int);

ec_domain_t *ecrt_master_create_domain_err(ec_master_t *);
ec_slave_config_t *ecrt_master_slave_config_err(ec_master_t *, uint16_t,
		uint16_t, uint32_t, uint32_t);

void ec_master_calc_dc(ec_master_t *);
void ec_master_request_op(ec_master_t *);

void ec_master_internal_send_cb(void *);
void ec_master_internal_receive_cb(void *);

extern const unsigned int rate_intervals[EC_RATE_COUNT]; // see master.c

/****************************************************************************/

ec_domain_t *ecrt_master_create_domain(
	ec_master_t *master /**< master */
	);

int ecrt_master_activate(ec_master_t *master);
int ecrt_master_deactivate(ec_master_t *master);
int ecrt_master_send(ec_master_t *master);
int ecrt_master_send_ext(ec_master_t *master);
int ecrt_master_receive(ec_master_t *master);
void ecrt_master_callbacks(ec_master_t *master,
	void (*send_cb)(void *), void (*receive_cb)(void *), void *cb_data);

int ecrt_master(ec_master_t *master, ec_master_info_t *master_info);
int ecrt_master_scan_progress(ec_master_t *master,
	ec_master_scan_progress_t *progress);

int ecrt_master_get_slave(ec_master_t *master, uint16_t slave_position,
		ec_slave_info_t *slave_info);
ec_slave_config_t *ecrt_master_slave_config(ec_master_t *master,
		uint16_t alias, uint16_t position, uint32_t vendor_id,
		uint32_t product_code);

int ecrt_master_select_reference_clock(ec_master_t *master,
	ec_slave_config_t *sc);

int ecrt_master_state(const ec_master_t *master, ec_master_state_t *state);

int ecrt_master_link_state(const ec_master_t *master, unsigned int dev_idx,
	ec_master_link_state_t *state);

int ecrt_master_application_time(ec_master_t *master, uint64_t app_time);
int ecrt_master_sync_reference_clock(ec_master_t *master);

int ecrt_master_sync_reference_clock_to(
		ec_master_t *master,
		uint64_t sync_time
		);
int ecrt_master_sync_slave_clocks(ec_master_t *master);
int ecrt_master_reference_clock_time(const ec_master_t *master,
		uint32_t *time);

int ecrt_master_sync_monitor_queue(ec_master_t *master);
uint32_t ecrt_master_sync_monitor_process(const ec_master_t *master);

int ecrt_master_sdo_download(ec_master_t *master, uint16_t slave_position,
		uint16_t index, uint8_t subindex, const uint8_t *data,
		size_t data_size, uint32_t *abort_code);
int ecrt_master_sdo_download_complete(ec_master_t *master,
		uint16_t slave_position, uint16_t index, const uint8_t *data,
		size_t data_size, uint32_t *abort_code);
int ecrt_master_sdo_upload(ec_master_t *master, uint16_t slave_position,
		uint16_t index, uint8_t subindex, uint8_t *target,
		size_t target_size, size_t *result_size, uint32_t *abort_code);

int ecrt_master_write_idn(ec_master_t *master, uint16_t slave_position,
		uint8_t drive_no, uint16_t idn, const uint8_t *data, size_t data_size,
		uint16_t *error_code);

int ecrt_master_read_idn(ec_master_t *master, uint16_t slave_position,
		uint8_t drive_no, uint16_t idn, uint8_t *target, size_t target_size,
		size_t *result_size, uint16_t *error_code);

int ecrt_master_reset(ec_master_t *master);
#endif
