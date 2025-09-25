/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#include <drivers/i2c.h>
#include <drivers/i2c-bit-ops.h>
#include <drivers/i2c_dev.h>
#include "i2c-k1.h"

#define I2C_SMBUS_BLOCK_MAX	32    /* As specified in SMBus standard */
#define USEC_PER_SEC		1000000L

static inline struct spacemit_i2c_dev *to_spacemit_i2c_dev(struct rt_i2c_bus_device *dev)
{
	return rt_container_of(dev, struct spacemit_i2c_dev, dev);
}

static inline rt_uint32_t spacemit_i2c_read_reg(struct spacemit_i2c_dev *spacemit_i2c, int reg)
{
	return readl(spacemit_i2c->mapbase + reg);
}

static inline void
spacemit_i2c_write_reg(struct spacemit_i2c_dev *spacemit_i2c, int reg, rt_uint32_t val)
{
	writel(val, spacemit_i2c->mapbase + reg);
}

static void spacemit_i2c_enable(struct spacemit_i2c_dev *spacemit_i2c)
{
	spacemit_i2c_write_reg(spacemit_i2c, REG_CR,
	spacemit_i2c_read_reg(spacemit_i2c, REG_CR) | CR_IUE);
}

static void spacemit_i2c_disable(struct spacemit_i2c_dev *spacemit_i2c)
{
	spacemit_i2c->i2c_ctrl_reg_value = spacemit_i2c_read_reg(spacemit_i2c, REG_CR) & ~CR_IUE;
	spacemit_i2c_write_reg(spacemit_i2c, REG_CR, spacemit_i2c->i2c_ctrl_reg_value);
}

static void spacemit_i2c_flush_fifo_buffer(struct spacemit_i2c_dev *spacemit_i2c)
{
	/* flush REG_WFIFO_WPTR and REG_WFIFO_RPTR */
	spacemit_i2c_write_reg(spacemit_i2c, REG_WFIFO_WPTR, 0);
	spacemit_i2c_write_reg(spacemit_i2c, REG_WFIFO_RPTR, 0);

	/* flush REG_RFIFO_WPTR and REG_RFIFO_RPTR */
	spacemit_i2c_write_reg(spacemit_i2c, REG_RFIFO_WPTR, 0);
	spacemit_i2c_write_reg(spacemit_i2c, REG_RFIFO_RPTR, 0);
}

static void spacemit_i2c_mdelay(rt_uint32_t ms)
{
	rt_tick_t tick_delay, now;

	if (ms < 10)
		ms = 10;
	tick_delay = rt_tick_from_millisecond(ms);
	now = rt_tick_get_millisecond();
	tick_delay += now;

	while (now < tick_delay)
		now = rt_tick_get_millisecond();
}

static void spacemit_i2c_controller_reset(struct spacemit_i2c_dev *spacemit_i2c)
{
	/* i2c controller reset */
	spacemit_i2c_write_reg(spacemit_i2c, REG_CR, CR_UR);

	/* udelay(5) */
	spacemit_i2c_mdelay(10);

	spacemit_i2c_write_reg(spacemit_i2c, REG_CR, 0);

	/* set load counter register */
	if (spacemit_i2c->i2c_lcr)
		spacemit_i2c_write_reg(spacemit_i2c, REG_LCR, spacemit_i2c->i2c_lcr);

	/* set wait counter register */
	if (spacemit_i2c->i2c_wcr)
		spacemit_i2c_write_reg(spacemit_i2c, REG_WCR, spacemit_i2c->i2c_wcr);
}

static void spacemit_i2c_bus_reset(struct spacemit_i2c_dev *spacemit_i2c)
{
	int clk_cnt = 0;
	rt_uint32_t bus_status;

	/* if bus is locked, reset unit. 0: locked */
	bus_status = spacemit_i2c_read_reg(spacemit_i2c, REG_BMR);
	if (!(bus_status & BMR_SDA) || !(bus_status & BMR_SCL)) {
		spacemit_i2c_controller_reset(spacemit_i2c);

		/* usleep_range(10, 20); */
		spacemit_i2c_mdelay(10);

		/* check scl status again */
		bus_status = spacemit_i2c_read_reg(spacemit_i2c, REG_BMR);
		if (!(bus_status & BMR_SCL))
			rt_kprintf("unit reset failed\n");
	}

	while (clk_cnt < 9) {
		/* check whether the SDA is still locked by slave */
		bus_status = spacemit_i2c_read_reg(spacemit_i2c, REG_BMR);
		if (bus_status & BMR_SDA)
			break;

		/* if still locked, send one clk to slave to request release */
		spacemit_i2c_write_reg(spacemit_i2c, REG_RST_CYC, 0x1);
		spacemit_i2c_write_reg(spacemit_i2c, REG_CR, CR_RSTREQ);

		/* usleep_range(20, 30); */
		spacemit_i2c_mdelay(20);

		clk_cnt++;
	}

	bus_status = spacemit_i2c_read_reg(spacemit_i2c, REG_BMR);
	if (clk_cnt >= 9 && !(bus_status & BMR_SDA))
		rt_kprintf("bus reset clk reaches the max 9-clocks\n");
	else
		rt_kprintf("bus reset, send clk: %d\n", clk_cnt);
}

static void spacemit_i2c_reset(struct spacemit_i2c_dev *spacemit_i2c)
{
	spacemit_i2c_controller_reset(spacemit_i2c);
}

static int spacemit_i2c_recover_bus_busy(struct spacemit_i2c_dev *spacemit_i2c)
{
	int timeout;
	int cnt, ret = 0;

	if (spacemit_i2c->high_mode)
		timeout = 1000; /* 1000us */
	else
		timeout = 1500; /* 1500us  */

	cnt = SPACEMIT_I2C_BUS_RECOVER_TIMEOUT / timeout;
	if (!(spacemit_i2c_read_reg(spacemit_i2c, REG_SR) & (SR_UB | SR_IBB)))
		return 0;

	/* wait unit and bus to recover idle */
	while (spacemit_i2c_read_reg(spacemit_i2c, REG_SR) & (SR_UB | SR_IBB)) {
		if (cnt-- <= 0)
			break;

		/* usleep_range(timeout / 2, timeout); */
		spacemit_i2c_mdelay(timeout / 2);

	}

	if (cnt <= 0) {
		/* reset controller */
		spacemit_i2c_reset(spacemit_i2c);
		ret = -RT_ERROR;
	}

	return ret;
}

static void spacemit_i2c_check_bus_release(struct spacemit_i2c_dev *spacemit_i2c)
{
	/* in case bus is not released after transfer completes */
	if (spacemit_i2c_read_reg(spacemit_i2c, REG_SR) & SR_EBB) {
		spacemit_i2c_bus_reset(spacemit_i2c);
		/* usleep_range(90, 150); */
		spacemit_i2c_mdelay(90);

	}
}

static void spacemit_i2c_unit_init(struct spacemit_i2c_dev *spacemit_i2c)
{
	rt_uint32_t cr_val = 0;

	/*
	 * Unmask interrupt bits for all xfer mode:
	 * bus error, arbitration loss detected.
	 * For transaction complete signal, we use master stop
	 * interrupt, so we don't need to unmask CR_TXDONEIE.
	 */
	cr_val |= CR_BEIE | CR_ALDIE;

	if (spacemit_i2c->xfer_mode == SPACEMIT_I2C_MODE_INTERRUPT)
		/*
		 * Unmask interrupt bits for interrupt xfer mode:
		 * DBR rx full.
		 * For tx empty interrupt CR_DTEIE, we only
		 * need to enable when trigger byte transfer to start
		 * data sending.
		 */
		cr_val |= CR_DRFIE;
	else if (spacemit_i2c->xfer_mode == SPACEMIT_I2C_MODE_FIFO)
		/* enable i2c FIFO mode*/
		cr_val |= CR_FIFOEN;

	/* set speed bits */
	if (spacemit_i2c->fast_mode)
		cr_val |= CR_MODE_FAST;
	if (spacemit_i2c->high_mode)
		cr_val |= CR_MODE_HIGH | CR_GPIOEN;

	/* disable response to general call */
	cr_val |= CR_GCD;

	/* enable SCL clock output */
	cr_val |= CR_SCLE;

	/* enable master stop detected */
	cr_val |= CR_MSDE | CR_MSDIE;

	spacemit_i2c_write_reg(spacemit_i2c, REG_CR, cr_val);
}

static void spacemit_i2c_trigger_byte_xfer(struct spacemit_i2c_dev *spacemit_i2c)
{
	rt_uint32_t cr_val = spacemit_i2c_read_reg(spacemit_i2c, REG_CR);

	/* send start pulse */
	cr_val &= ~CR_STOP;
	cr_val |= CR_START | CR_TB | CR_DTEIE;
	spacemit_i2c_write_reg(spacemit_i2c, REG_CR, cr_val);
}

static inline void
spacemit_i2c_clear_int_status(struct spacemit_i2c_dev *spacemit_i2c, rt_uint32_t mask)
{
	spacemit_i2c_write_reg(spacemit_i2c, REG_SR, mask & SPACEMIT_I2C_INT_STATUS_MASK);
}

static bool spacemit_i2c_is_last_byte_to_send(struct spacemit_i2c_dev *spacemit_i2c)
{
	return (spacemit_i2c->tx_cnt == spacemit_i2c->cur_msg->len
		&& spacemit_i2c->msg_idx == spacemit_i2c->num - 1) ? true : false;
}

static bool spacemit_i2c_is_last_byte_to_receive(struct spacemit_i2c_dev *spacemit_i2c)
{
	/*
	 * if the message length is received from slave device,
	 * should at least to read out the length byte from slave.
	 */
	if ((spacemit_i2c->cur_msg->flags & RT_I2C_M_RECV_LEN) &&
		!spacemit_i2c->smbus_rcv_len) {
		return false;
	} else {
		return (spacemit_i2c->rx_cnt == spacemit_i2c->cur_msg->len - 1 &&
			spacemit_i2c->msg_idx == spacemit_i2c->num - 1) ? true : false;
	}
}

static void spacemit_i2c_mark_rw_flag(struct spacemit_i2c_dev *spacemit_i2c)
{
	if (spacemit_i2c->cur_msg->flags & RT_I2C_RD) {
		spacemit_i2c->is_rx = true;
		spacemit_i2c->slave_addr_rw =
			((spacemit_i2c->cur_msg->addr & 0x7f) << 1) | 1;
	} else {
		spacemit_i2c->is_rx = false;
		spacemit_i2c->slave_addr_rw = (spacemit_i2c->cur_msg->addr & 0x7f) << 1;
	}
}

static void spacemit_i2c_byte_xfer_send_master_code(struct spacemit_i2c_dev *spacemit_i2c)
{
	rt_uint32_t cr_val = spacemit_i2c_read_reg(spacemit_i2c, REG_CR);

	spacemit_i2c->phase = SPACEMIT_I2C_XFER_MASTER_CODE;

	spacemit_i2c_write_reg(spacemit_i2c, REG_DBR, spacemit_i2c->master_code);

	cr_val &= ~(CR_STOP | CR_ALDIE);

	/* high mode: enable gpio to let I2C core generates SCL clock */
	cr_val |= CR_GPIOEN | CR_START | CR_TB | CR_DTEIE;
	spacemit_i2c_write_reg(spacemit_i2c, REG_CR, cr_val);
}

static void spacemit_i2c_byte_xfer_send_slave_addr(struct spacemit_i2c_dev *spacemit_i2c)
{
	spacemit_i2c->phase = SPACEMIT_I2C_XFER_SLAVE_ADDR;

	/* write slave address to DBR for interrupt mode */
	spacemit_i2c_write_reg(spacemit_i2c, REG_DBR, spacemit_i2c->slave_addr_rw);

	spacemit_i2c_trigger_byte_xfer(spacemit_i2c);
}

static int spacemit_i2c_byte_xfer(struct spacemit_i2c_dev *spacemit_i2c);
static int spacemit_i2c_byte_xfer_next_msg(struct spacemit_i2c_dev *spacemit_i2c);

static int spacemit_i2c_byte_xfer_body(struct spacemit_i2c_dev *spacemit_i2c)
{
	int ret = 0;
	rt_uint8_t  msglen = 0;
	rt_uint32_t cr_val = spacemit_i2c_read_reg(spacemit_i2c, REG_CR);

	cr_val &= ~(CR_TB | CR_ACKNAK | CR_STOP | CR_START);
	spacemit_i2c->phase = SPACEMIT_I2C_XFER_BODY;

	if (spacemit_i2c->i2c_status & SR_IRF) { /* i2c receive full */
		/* if current is transmit mode, ignore this signal */
		if (!spacemit_i2c->is_rx)
			return 0;

		/*
		 * if the message length is received from slave device,
		 * according to i2c spec, we should restrict the length size.
		 */
		if ((spacemit_i2c->cur_msg->flags & RT_I2C_M_RECV_LEN) &&
				!spacemit_i2c->smbus_rcv_len) {
			spacemit_i2c->smbus_rcv_len = true;
			msglen = (rt_uint8_t)spacemit_i2c_read_reg(spacemit_i2c, REG_DBR);
			if ((msglen == 0) ||
				(msglen > I2C_SMBUS_BLOCK_MAX)) {
				rt_kprintf("SMbus len out of range\n");
				*spacemit_i2c->msg_buf++ = 0;
				spacemit_i2c->rx_cnt = spacemit_i2c->cur_msg->len;
				cr_val |= CR_STOP | CR_ACKNAK;
				cr_val |= CR_ALDIE | CR_TB;
				spacemit_i2c_write_reg(spacemit_i2c, REG_CR, cr_val);

				return 0;
			} else {
				*spacemit_i2c->msg_buf++ = msglen;
				spacemit_i2c->cur_msg->len = msglen + 1;
				spacemit_i2c->rx_cnt++;
			}
		} else {
			if (spacemit_i2c->rx_cnt < spacemit_i2c->cur_msg->len) {
				*spacemit_i2c->msg_buf++ =
					spacemit_i2c_read_reg(spacemit_i2c, REG_DBR);
				spacemit_i2c->rx_cnt++;
			}
		}
		/* if transfer completes, ISR will handle it */
		if (spacemit_i2c->i2c_status & (SR_MSD | SR_ACKNAK))
			return 0;

		/* trigger next byte receive */
		if (spacemit_i2c->rx_cnt < spacemit_i2c->cur_msg->len) {
			/* send stop pulse for last byte of last msg */
			if (spacemit_i2c_is_last_byte_to_receive(spacemit_i2c))
				cr_val |= CR_STOP | CR_ACKNAK;

			cr_val |= CR_ALDIE | CR_TB;
			spacemit_i2c_write_reg(spacemit_i2c, REG_CR, cr_val);
		} else if (spacemit_i2c->msg_idx < spacemit_i2c->num - 1) {
			ret = spacemit_i2c_byte_xfer_next_msg(spacemit_i2c);
		} else {
			/*
			 * For this branch, we do nothing, here the receive
			 * transfer is already done, the master stop interrupt
			 * should be generated to complete this transaction.
			*/
		}
	} else if (spacemit_i2c->i2c_status & SR_ITE) { /* i2c transmit empty */
		/* MSD comes with ITE */
		if ((spacemit_i2c->i2c_status & SR_MSD) && (spacemit_i2c->msg_idx == spacemit_i2c->num - 1))
			/* only if stop signal come with the last msg, we do nothing */
			return ret;

		if (spacemit_i2c->i2c_status & SR_RWM) { /* receive mode */
			/* if current is transmit mode, ignore this signal */
			if (!spacemit_i2c->is_rx)
				return 0;

			if (spacemit_i2c_is_last_byte_to_receive(spacemit_i2c))
				cr_val |= CR_STOP | CR_ACKNAK;

			/* trigger next byte receive */
			cr_val |= CR_ALDIE | CR_TB;

			/*
			 * Mask transmit empty interrupt to avoid useless tx
			 * interrupt signal after switch to receive mode, the
			 * next expected is receive full interrupt signal.
			 */
			cr_val &= ~CR_DTEIE;
			spacemit_i2c_write_reg(spacemit_i2c, REG_CR, cr_val);
		} else { /* transmit mode */
			/* if current is receive mode, ignore this signal */
			if (spacemit_i2c->is_rx)
				return 0;

			if (spacemit_i2c->tx_cnt < spacemit_i2c->cur_msg->len) {
				spacemit_i2c_write_reg(spacemit_i2c, REG_DBR,
						*spacemit_i2c->msg_buf++);
				spacemit_i2c->tx_cnt++;

				/* send stop pulse for last byte of last msg */
				if (spacemit_i2c_is_last_byte_to_send(spacemit_i2c))
					cr_val |= CR_STOP;
				else if (spacemit_i2c->tx_cnt == spacemit_i2c->cur_msg->len) {
					cr_val |= CR_STOP;
					cr_val &= ~CR_DTEIE;
				}
				cr_val |= CR_ALDIE | CR_TB;
				spacemit_i2c_write_reg(spacemit_i2c, REG_CR, cr_val);
			} else if (spacemit_i2c->msg_idx < spacemit_i2c->num - 1) {
				ret = spacemit_i2c_byte_xfer_next_msg(spacemit_i2c);
			} else {
				/*
				 * For this branch, we do nothing, here the
				 * sending transfer is already done, the master
				 * stop interrupt should be generated to
				 * complete this transaction.
				*/
			}
		}
	}

	return ret;
}

static int spacemit_i2c_byte_xfer_next_msg(struct spacemit_i2c_dev *spacemit_i2c)
{
	if (spacemit_i2c->msg_idx == spacemit_i2c->num - 1)
		return 0;

	spacemit_i2c->msg_idx++;
	spacemit_i2c->cur_msg = spacemit_i2c->msgs + spacemit_i2c->msg_idx;
	spacemit_i2c->msg_buf = spacemit_i2c->cur_msg->buf;
	spacemit_i2c->rx_cnt = 0;
	spacemit_i2c->tx_cnt = 0;
	spacemit_i2c->i2c_err = 0;
	spacemit_i2c->i2c_status = 0;
	spacemit_i2c->smbus_rcv_len = false;
	spacemit_i2c->phase = SPACEMIT_I2C_XFER_IDLE;

	spacemit_i2c_mark_rw_flag(spacemit_i2c);

	return spacemit_i2c_byte_xfer(spacemit_i2c);
}

static void spacemit_i2c_fifo_xfer_fill_buffer(struct spacemit_i2c_dev *spacemit_i2c)
{
	unsigned long flags;
	int finish, count = 0, fill = 0;
	rt_uint32_t data = 0;
	rt_uint32_t data_buf[SPACEMIT_I2C_TX_FIFO_DEPTH * 2];
	int data_cnt = 0, i;

	while (spacemit_i2c->msg_idx < spacemit_i2c->num) {
		spacemit_i2c_mark_rw_flag(spacemit_i2c);

		if (spacemit_i2c->is_rx)
			finish = spacemit_i2c->rx_cnt;
		else
			finish = spacemit_i2c->tx_cnt;

		/* write master code to fifo buffer */
		if (spacemit_i2c->high_mode && spacemit_i2c->is_xfer_start) {
			data = spacemit_i2c->master_code;
			data |= WFIFO_CTRL_TB | WFIFO_CTRL_START;
			data_buf[data_cnt++] = data;

			fill += 2;
			count = min_t(i2c_size_t, spacemit_i2c->cur_msg->len - finish,
					SPACEMIT_I2C_TX_FIFO_DEPTH - fill);
		} else {
			fill += 1;
			count = min_t(i2c_size_t, spacemit_i2c->cur_msg->len - finish,
					SPACEMIT_I2C_TX_FIFO_DEPTH - fill);
		}

		spacemit_i2c->is_xfer_start = false;
		fill += count;
		data = spacemit_i2c->slave_addr_rw;
		data |= WFIFO_CTRL_TB | WFIFO_CTRL_START;

		/* write slave address to fifo buffer */
		data_buf[data_cnt++] = data;

		if (spacemit_i2c->is_rx) {
			spacemit_i2c->rx_cnt += count;

			if (spacemit_i2c->rx_cnt == spacemit_i2c->cur_msg->len &&
				spacemit_i2c->msg_idx == spacemit_i2c->num - 1)
				count -= 1;

			while (count > 0) {
				data = *spacemit_i2c->msg_buf | WFIFO_CTRL_TB;
				data_buf[data_cnt++] = data;
				spacemit_i2c->msg_buf++;
				count--;
			}

			if (spacemit_i2c->rx_cnt == spacemit_i2c->cur_msg->len &&
				spacemit_i2c->msg_idx == spacemit_i2c->num - 1) {
				data = *spacemit_i2c->msg_buf++;
				data = spacemit_i2c->slave_addr_rw | WFIFO_CTRL_TB |
					WFIFO_CTRL_STOP | WFIFO_CTRL_ACKNAK;
				data_buf[data_cnt++] = data;
			}
		} else {
			spacemit_i2c->tx_cnt += count;
			if (spacemit_i2c_is_last_byte_to_send(spacemit_i2c))
				count -= 1;

			while (count > 0) {
				data = *spacemit_i2c->msg_buf | WFIFO_CTRL_TB;
				data_buf[data_cnt++] = data;
				spacemit_i2c->msg_buf++;
				count--;
			}
			if (spacemit_i2c_is_last_byte_to_send(spacemit_i2c)) {
				data = *spacemit_i2c->msg_buf | WFIFO_CTRL_TB |
						WFIFO_CTRL_STOP;
				data_buf[data_cnt++] = data;
			}
		}

		if (spacemit_i2c->tx_cnt == spacemit_i2c->cur_msg->len ||
			spacemit_i2c->rx_cnt == spacemit_i2c->cur_msg->len) {
			spacemit_i2c->msg_idx++;
			if (spacemit_i2c->msg_idx == spacemit_i2c->num)
				break;

			spacemit_i2c->cur_msg = spacemit_i2c->msgs + spacemit_i2c->msg_idx;
			spacemit_i2c->msg_buf = spacemit_i2c->cur_msg->buf;
			spacemit_i2c->rx_cnt = 0;
			spacemit_i2c->tx_cnt = 0;
		}

		if (fill == SPACEMIT_I2C_TX_FIFO_DEPTH)
			break;
	}

	flags = rt_spin_lock_irqsave(&spacemit_i2c->fifo_lock);
	for (i = 0; i < data_cnt; i++)
		spacemit_i2c_write_reg(spacemit_i2c, REG_WFIFO, data_buf[i]);
	rt_spin_unlock_irqrestore(&spacemit_i2c->fifo_lock, flags);
}

static void spacemit_i2c_fifo_xfer_copy_buffer(struct spacemit_i2c_dev *spacemit_i2c)
{
	int idx = 0, cnt = 0;
	struct rt_i2c_msg *msg;

	/* copy the rx FIFO buffer to msg */
	while (idx < spacemit_i2c->num) {
		msg = spacemit_i2c->msgs + idx;
		if (msg->flags & RT_I2C_RD) {
			cnt = msg->len;
			while (cnt > 0) {
				*(msg->buf + msg->len - cnt)
					= spacemit_i2c_read_reg(spacemit_i2c, REG_RFIFO);
				cnt--;
			}
		}
		idx++;
	}
}

static int spacemit_i2c_fifo_xfer(struct spacemit_i2c_dev *spacemit_i2c)
{
	int ret = 0;
	unsigned long time_left;

	spacemit_i2c_fifo_xfer_fill_buffer(spacemit_i2c);

	time_left = rt_completion_wait(&spacemit_i2c->complete,
					spacemit_i2c->timeout);
	if (time_left == 0) {
		rt_kprintf("fifo transfer timeout\n");
		spacemit_i2c_bus_reset(spacemit_i2c);
		ret = -RT_ETIMEOUT;
		goto err_out;
	}

	if (spacemit_i2c->i2c_err) {
		ret = -1;
		spacemit_i2c_flush_fifo_buffer(spacemit_i2c);
		goto err_out;
	}

	spacemit_i2c_fifo_xfer_copy_buffer(spacemit_i2c);

err_out:
	return ret;
}

static int spacemit_i2c_byte_xfer(struct spacemit_i2c_dev *spacemit_i2c)
{
	int ret = 0;

	/* i2c error occurs */
	if (spacemit_i2c->i2c_err)
		return -1;

	if (spacemit_i2c->phase == SPACEMIT_I2C_XFER_IDLE) {
		if (spacemit_i2c->high_mode && spacemit_i2c->is_xfer_start)
			spacemit_i2c_byte_xfer_send_master_code(spacemit_i2c);
		else
			spacemit_i2c_byte_xfer_send_slave_addr(spacemit_i2c);

		spacemit_i2c->is_xfer_start = false;
	} else if (spacemit_i2c->phase == SPACEMIT_I2C_XFER_MASTER_CODE) {
		spacemit_i2c_byte_xfer_send_slave_addr(spacemit_i2c);
	} else {
		ret = spacemit_i2c_byte_xfer_body(spacemit_i2c);
	}

	return ret;
}

static int spacemit_i2c_handle_err(struct spacemit_i2c_dev *spacemit_i2c)
{
	if (spacemit_i2c->i2c_err) {
		rt_kprintf("i2c error status: 0x%08x\n",
				spacemit_i2c->i2c_status);
		if (spacemit_i2c->i2c_err & (SR_BED  | SR_ALD))
			spacemit_i2c_reset(spacemit_i2c);

		/* try transfer again */
		if (spacemit_i2c->i2c_err & (SR_RXOV | SR_ALD)) {
			spacemit_i2c_flush_fifo_buffer(spacemit_i2c);
			return -RT_EINVAL;
		}
		return -RT_EIO;
	}

	return 0;
}

static void spacemit_i2c_int_handler(int irq, void *devid)
{
	struct spacemit_i2c_dev *spacemit_i2c = devid;
	rt_uint32_t status, ctrl;
	int ret = 0;

	/* record i2c status */
	status = spacemit_i2c_read_reg(spacemit_i2c, REG_SR);
	spacemit_i2c->i2c_status = status;

	/* check if a valid interrupt status */
	if(!status) {
		/* nothing need be done */
		return;
	}

	/* bus error, rx overrun, arbitration lost */
	spacemit_i2c->i2c_err = status & (SR_BED | SR_RXOV | SR_ALD);

	/* clear interrupt status bits[31:18]*/
	spacemit_i2c_clear_int_status(spacemit_i2c, status);

	/* i2c error happens */
	if (spacemit_i2c->i2c_err)
		goto err_out;

	/* process interrupt mode */
	if (spacemit_i2c->xfer_mode == SPACEMIT_I2C_MODE_INTERRUPT)
		ret = spacemit_i2c_byte_xfer(spacemit_i2c);

err_out:
	/*
	 * send transaction complete signal:
	 * error happens, detect master stop
	 */
	if (spacemit_i2c->i2c_err || (ret < 0) || ((status & SR_MSD) && !(status & SR_ITE))) {
		/*
		 * Here the transaction is already done, we don't need any
		 * other interrupt signals from now, in case any interrupt
		 * happens before spacemit_i2c_xfer to disable irq and i2c unit,
		 * we mask all the interrupt signals and clear the interrupt
		 * status.
		*/
		ctrl = spacemit_i2c_read_reg(spacemit_i2c, REG_CR);
		ctrl &= ~SPACEMIT_I2C_INT_CTRL_MASK;
		spacemit_i2c_write_reg(spacemit_i2c, REG_CR, ctrl);

		spacemit_i2c_clear_int_status(spacemit_i2c, SPACEMIT_I2C_INT_STATUS_MASK);

		rt_completion_done(&spacemit_i2c->complete);
	} else if ((status & SR_MSD) && (status & SR_ITE)) {
		if (spacemit_i2c->tx_cnt > 1) {
			ctrl = spacemit_i2c_read_reg(spacemit_i2c, REG_CR);
			ctrl &= ~SPACEMIT_I2C_INT_CTRL_MASK;
			spacemit_i2c_write_reg(spacemit_i2c, REG_CR, ctrl);
			spacemit_i2c_clear_int_status(spacemit_i2c, SPACEMIT_I2C_INT_STATUS_MASK);
			rt_completion_done(&spacemit_i2c->complete);
		}
	}

	return;
}

static void spacemit_i2c_choose_xfer_mode(struct spacemit_i2c_dev *spacemit_i2c)
{
	unsigned long timeout;
	int idx = 0, cnt = 0, freq;
	bool block = false;

	/* scan msgs */
	if (spacemit_i2c->high_mode)
		cnt++;
	spacemit_i2c->rx_total = 0;
	while (idx < spacemit_i2c->num) {
		cnt += (spacemit_i2c->msgs + idx)->len + 1;
		if ((spacemit_i2c->msgs + idx)->flags & RT_I2C_RD)
			spacemit_i2c->rx_total += (spacemit_i2c->msgs + idx)->len;

		/*
		 * Some SMBus transactions require that
		 * we receive the transacttion length as the first read byte.
		 * force to use I2C_MODE_INTERRUPT
		 */
		if ((spacemit_i2c->msgs + idx)->flags & RT_I2C_M_RECV_LEN) {
			block = true;
			cnt += I2C_SMBUS_BLOCK_MAX + 2;
		}
		idx++;
	}
	if (cnt <= SPACEMIT_I2C_TX_FIFO_DEPTH)
			spacemit_i2c->xfer_mode = SPACEMIT_I2C_MODE_FIFO;

	if ((spacemit_i2c->intr_enable) || block) {
		spacemit_i2c->xfer_mode = SPACEMIT_I2C_MODE_INTERRUPT;

	} else {
		if (cnt <= SPACEMIT_I2C_TX_FIFO_DEPTH)
			spacemit_i2c->xfer_mode = SPACEMIT_I2C_MODE_FIFO;

		/* flush fifo buffer */
		spacemit_i2c_flush_fifo_buffer(spacemit_i2c);
	}
	/*
	 * if total message length is too large to over the allocated MDA
	 * total buf length, use interrupt mode. This may happens in the
	 * syzkaller test.
	 */
	if (cnt > (SPACEMIT_I2C_MAX_MSG_LEN * SPACEMIT_I2C_SCATTERLIST_SIZE) ||
		spacemit_i2c->rx_total > SPACEMIT_I2C_DMA_RX_BUF_LEN)
		spacemit_i2c->xfer_mode = SPACEMIT_I2C_MODE_INTERRUPT;

	/* calculate timeout */
	if (spacemit_i2c->high_mode)
		freq = 1500000;
	else if (spacemit_i2c->fast_mode)
		freq = 400000;
	else
		freq = 100000;

	timeout = cnt * 9 * USEC_PER_SEC / freq;

	if (spacemit_i2c->xfer_mode == SPACEMIT_I2C_MODE_INTERRUPT)
		timeout += (cnt - 1) * 220;

	if (spacemit_i2c->xfer_mode == SPACEMIT_I2C_MODE_INTERRUPT)
		spacemit_i2c->timeout = (timeout + 500000)/1000;
	else
		spacemit_i2c->timeout = (timeout + 100000)/1000;
}

static void spacemit_i2c_init_xfer_params(struct spacemit_i2c_dev *spacemit_i2c)
{
	/* initialize transfer parameters */
	spacemit_i2c->msg_idx = 0;
	spacemit_i2c->cur_msg = spacemit_i2c->msgs;
	spacemit_i2c->msg_buf = spacemit_i2c->cur_msg->buf;
	spacemit_i2c->rx_cnt = 0;
	spacemit_i2c->tx_cnt = 0;
	spacemit_i2c->i2c_err = 0;
	spacemit_i2c->i2c_status = 0;
	spacemit_i2c->phase = SPACEMIT_I2C_XFER_IDLE;

	/* only send master code once for high speed mode */
	spacemit_i2c->is_xfer_start = true;
}

static rt_size_t
spacemit_i2c_xfer(struct rt_i2c_bus_device *dev, struct rt_i2c_msg msgs[], rt_uint32_t num)
{
	int ret = 0, xfer_try = 0;
	bool clk_directly = false;
	struct spacemit_i2c_dev *spacemit_i2c = to_spacemit_i2c_dev(dev);

	rt_mutex_take(&spacemit_i2c->mtx, RT_WAITING_FOREVER);
	spacemit_i2c->msgs = msgs;
	spacemit_i2c->num = num;
	/* if unit keeps the last control status, don't need to do reset */
	if (spacemit_i2c_read_reg(spacemit_i2c, REG_CR) != spacemit_i2c->i2c_ctrl_reg_value)
	{
		/* i2c controller & bus reset */
		spacemit_i2c_reset(spacemit_i2c);
	}
	/* choose transfer mode */
	spacemit_i2c_choose_xfer_mode(spacemit_i2c);

	/* i2c unit init */
	spacemit_i2c_unit_init(spacemit_i2c);

	/* clear all interrupt status */
	spacemit_i2c_clear_int_status(spacemit_i2c, SPACEMIT_I2C_INT_STATUS_MASK);

	spacemit_i2c_init_xfer_params(spacemit_i2c);

	spacemit_i2c_mark_rw_flag(spacemit_i2c);

	//reinit_completion(&spacemit_i2c->complete);
	rt_completion_done(&spacemit_i2c->complete);
	rt_completion_init(&spacemit_i2c->complete);

	spacemit_i2c_enable(spacemit_i2c);
	rt_hw_interrupt_umask(spacemit_i2c->irq);

	/* i2c wait for bus busy */
	ret = spacemit_i2c_recover_bus_busy(spacemit_i2c);
	if (ret)
		goto err_recover;

	/* i2c msg transmit */
	if (spacemit_i2c->xfer_mode == SPACEMIT_I2C_MODE_INTERRUPT)
		ret = spacemit_i2c_byte_xfer(spacemit_i2c);
	else if (spacemit_i2c->xfer_mode == SPACEMIT_I2C_MODE_FIFO)
		ret = spacemit_i2c_fifo_xfer(spacemit_i2c);
	else
		ret = spacemit_i2c_byte_xfer(spacemit_i2c);

	if (ret < 0) {
		rt_kprintf("i2c transfer error\n");
		/* timeout error should not be overrided, and the transfer
		 * error will be confirmed by err handle function latter,
		 * the reset should be invalid argument error. */
		if (ret != -RT_ETIMEOUT)
			ret = -RT_EINVAL;
		goto err_xfer;
	}

	if (spacemit_i2c->xfer_mode == SPACEMIT_I2C_MODE_INTERRUPT) {
		ret = rt_completion_wait(&spacemit_i2c->complete,
							spacemit_i2c->timeout);
		if (ret != 0) {
			rt_kprintf("msg completion timeout\n");
			rt_hw_interrupt_mask(spacemit_i2c->irq);
			spacemit_i2c_bus_reset(spacemit_i2c);
			spacemit_i2c_reset(spacemit_i2c);
			ret = -RT_ETIMEOUT;
			goto timeout_xfex;
		}
	}

err_xfer:
	if (!ret)
		spacemit_i2c_check_bus_release(spacemit_i2c);

err_recover:
	rt_hw_interrupt_mask(spacemit_i2c->irq);

timeout_xfex:
	/* disable spacemit i2c */
	spacemit_i2c_disable(spacemit_i2c);

	/* process i2c error */
	if (spacemit_i2c->i2c_err)
		ret = spacemit_i2c_handle_err(spacemit_i2c);

	xfer_try++;
	/* retry i2c transfer 3 times for timeout and bus busy */
	if ((ret == -RT_ETIMEOUT || ret == -RT_EINVAL) &&
		xfer_try <= spacemit_i2c->drv_retries) {
		rt_kprintf("i2c transfer retry %d, ret %d mode %d err 0x%x\n",
				xfer_try, ret, spacemit_i2c->xfer_mode, spacemit_i2c->i2c_err);
		/* usleep_range(150, 200); */
		spacemit_i2c_mdelay(150);

		ret = 0;
		//goto xfer_retry;
	}

	if (clk_directly) {
		/* if clock is enabled directly, here disable it */
		clk_disable_unprepare(spacemit_i2c->clk);
	}
	rt_mutex_release(&spacemit_i2c->mtx);

	return ret < 0 ? ret : num;
}

static struct rt_i2c_bus_device_ops spacemit_i2c_ops = {
	.master_xfer	= spacemit_i2c_xfer,
};

static int
spacemit_i2c_parse_dt(struct dtb_node * dnode, struct spacemit_i2c_dev *spacemit_i2c)
{
	int property_size;
	rt_uint32_t u32_value;
	rt_uint8_t u8_value;
	rt_uint32_t * property_ptr;
	struct dtb_property *property_status = RT_NULL;

	/* enable fast speed mode */
	property_status = dtb_node_get_dtb_node_property(dnode, "spacemit,i2c-fast-mode", RT_NULL);
	if (property_status)
		spacemit_i2c->fast_mode = 1;
	else
		spacemit_i2c->fast_mode = 0;

	/* enable high speed mode */
	property_status = dtb_node_get_dtb_node_property(dnode, "spacemit,i2c-high-mode", RT_NULL);
	if (property_status)
		spacemit_i2c->high_mode = 1;
	else
		spacemit_i2c->high_mode = 0;

	if (spacemit_i2c->high_mode) {
		/* get master code for high speed mode */
		spacemit_i2c->master_code = 0x0e;
		for_each_property_byte(dnode, "spacemit,i2c-master-code", u8_value, property_ptr, property_size)
		{
			spacemit_i2c->master_code = u8_value;
		}
	}
	for_each_property_cell(dnode, "spacemit,i2c-clk-rate", u32_value, property_ptr, property_size)
	{
		spacemit_i2c->clk_rate = u32_value;
	}

	for_each_property_cell(dnode, "spacemit,i2c-lcr", u32_value, property_ptr, property_size)
	{
		spacemit_i2c->i2c_lcr = u32_value;
	}

	for_each_property_cell(dnode, "spacemit,i2c-wcr", u32_value, property_ptr, property_size)
	{
		spacemit_i2c->i2c_wcr = u32_value;
	}

	return 0;
}

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,k1x-ri2c0", "ri2c0"},
	{}
};

static int spacemit_i2c_probe(void)
{
	int i;
	struct spacemit_i2c_dev *spacemit_i2c;
	struct rt_i2c_bus_device *dev;
	struct dtb_node *compatible_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();
	int ret = 0;

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
			__compatible[i].compatible);
		if (compatible_node != RT_NULL) {
			/* check the status */
			if (!dtb_node_device_is_available(compatible_node))
				continue;
			spacemit_i2c = (struct spacemit_i2c_dev *)rt_calloc(1, sizeof(struct spacemit_i2c_dev));
			if (!spacemit_i2c) {
				rt_kprintf("%s:%d, calloc failed\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}
			memset(spacemit_i2c, 0, sizeof(struct spacemit_i2c_dev));
			dev = &spacemit_i2c->dev;
			spacemit_i2c->name = (char *)__compatible[i].data;

			ret = spacemit_i2c_parse_dt(compatible_node, spacemit_i2c);
			if (ret)
				goto err_out;

			spacemit_i2c->mapbase = (void *)dtb_node_get_addr_index(compatible_node, 0);
			if (spacemit_i2c->mapbase < 0) {
				rt_kprintf("get i2c mapbase failed\n");
				return -RT_ERROR;
			}

			spacemit_i2c->clk = of_clk_get(compatible_node, 0);
			if (IS_ERR(spacemit_i2c->clk)) {
				rt_kprintf("get i2c clk failed\n");
				return -RT_ERROR;
			}
			spacemit_i2c->rst = of_clk_get(compatible_node, 1);
			if (IS_ERR(spacemit_i2c->rst)) {
				rt_kprintf("get i2c rst failed\n");
				return -RT_ERROR;
			}
			clk_prepare_enable(spacemit_i2c->rst);
			clk_disable_unprepare(spacemit_i2c->rst);

			/* udelay(200); */
			spacemit_i2c_mdelay(10);

			clk_prepare_enable(spacemit_i2c->rst);
			rt_mutex_init(&spacemit_i2c->mtx, "i2c_mutex", RT_IPC_FLAG_PRIO);
			rt_completion_init(&spacemit_i2c->complete);

			rt_spin_lock_init(&spacemit_i2c->fifo_lock);

			spacemit_i2c->irq = dtb_node_irq_get(compatible_node, 0);
			if (spacemit_i2c->irq < 0) {
					rt_kprintf("get irq of i2c error\n");
					return -RT_ERROR;
			}
			spacemit_i2c->intr_enable = true;
			spacemit_i2c->xfer_mode = SPACEMIT_I2C_MODE_INTERRUPT;

			rt_hw_interrupt_install(spacemit_i2c->irq, spacemit_i2c_int_handler, (void *)spacemit_i2c, "ri2c0-irq");

			clk_set_rate(spacemit_i2c->clk, spacemit_i2c->clk_rate);
			clk_prepare_enable(spacemit_i2c->clk);

			dev->ops = &spacemit_i2c_ops;
			rt_i2c_bus_device_register(dev, spacemit_i2c->name);
		}
	}

err_out:
	return ret;
}
INIT_DEVICE_EXPORT(spacemit_i2c_probe);
