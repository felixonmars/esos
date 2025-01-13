#include <rthw.h>
#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rtservice.h>

static struct rt_mutex con_mutex;
static rt_list_t mbox_cons = RT_LIST_OBJECT_INIT(mbox_cons);

#undef ENODEV
#define ENODEV		19
#undef EPROBE_DEFER 
#define EPROBE_DEFER    517     /* Driver requests probe retry */
#undef EBUSY
#define EBUSY		16
#undef ENOTSUPP
#define ENOTSUPP        524     /* Operation is not supported */
#undef ETIME
#define ETIME		62
#undef ENOBUFS
#define ENOBUFS		105	

static int add_to_rbuf(struct mbox_chan *chan, void *mssg)
{
	int idx;
	unsigned long flags;

	flags = rt_spin_lock_irqsave(&chan->lock);

	/* See if there is any space left */
	if (chan->msg_count == MBOX_TX_QUEUE_LEN) {
		rt_spin_unlock_irqrestore(&chan->lock, flags);
		return -ENOBUFS;
	}

	idx = chan->msg_free;
	chan->msg_data[idx] = mssg;
	chan->msg_count++;

	if (idx == MBOX_TX_QUEUE_LEN - 1)
		chan->msg_free = 0;
	else
		chan->msg_free++;

	rt_spin_unlock_irqrestore(&chan->lock, flags);

	return idx;
}

static void msg_submit(struct mbox_chan *chan)
{
	rt_tick_t timeout = 0;
	unsigned count, idx;
	unsigned long flags;
 	void *data;
 	int err = -EBUSY;

	flags = rt_spin_lock_irqsave(&chan->lock);

	if (!chan->msg_count || chan->active_req)
		goto exit;

	count = chan->msg_count;
	idx = chan->msg_free;
	if (idx >= count)
		idx -= count;
	else
		idx += MBOX_TX_QUEUE_LEN - count;

	data = chan->msg_data[idx];

	if (chan->cl->tx_prepare)
		chan->cl->tx_prepare(chan->cl, data);
	/* Try to submit a message to the MBOX controller */
	err = chan->mbox->ops->send_data(chan, data);
	if (!err) {
		chan->active_req = data;
		chan->msg_count--;
	}
exit:
	rt_spin_unlock_irqrestore(&chan->lock, flags);

	if (!err && (chan->txdone_method & TXDONE_BY_POLL)) {
		/* kick start the timer immediately to avoid delays */
		flags = rt_spin_lock_irqsave(&chan->mbox->poll_hrt_lock);
		/* hrtimer_start(&chan->mbox->poll_hrt, 0, HRTIMER_MODE_REL); */
		
		rt_timer_control(&(chan->mbox->poll_hrt),
				RT_TIMER_CTRL_SET_TIME,
				&timeout);
		
		rt_timer_start(&(chan->mbox->poll_hrt));

		rt_spin_unlock_irqrestore(&chan->mbox->poll_hrt_lock, flags);
	}
}

static void tx_tick(struct mbox_chan *chan, int r)
{
	unsigned long flags;
	void *mssg;

	flags = rt_spin_lock_irqsave(&chan->lock);
	mssg = chan->active_req;
	chan->active_req = NULL;
	rt_spin_unlock_irqrestore(&chan->lock, flags);

	/* Submit next message */
	msg_submit(chan);

	if (!mssg)
		return;

	/* Notify the client */
	if (chan->cl->tx_done)
		chan->cl->tx_done(chan->cl, mssg, r);

	if (r != -ETIME && chan->cl->tx_block)
		rt_completion_done(&chan->tx_complete);
}

static void txdone_timer(void *parameters)
{
	struct mbox_controller *mbox = parameters;
	bool txdone, resched = false;
	int i;
	rt_tick_t args;
	unsigned long flags;

	for (i = 0; i < mbox->num_chans; i++) {
		struct mbox_chan *chan = &mbox->chans[i];

		if (chan->active_req && chan->cl) {
			txdone = chan->mbox->ops->last_tx_done(chan);
			if (txdone)
				tx_tick(chan, 0);
			else
				resched = true;
		}
	}

	if (resched) {
		flags = rt_spin_lock_irqsave(&mbox->poll_hrt_lock);
#if 0
		if (!hrtimer_is_queued(hrtimer))
			hrtimer_forward_now(hrtimer, ms_to_ktime(mbox->txpoll_period));
#else
		rt_timer_control(&mbox->poll_hrt, RT_TIMER_CTRL_GET_STATE, (void *)&args);
		if (args != RT_TIMER_FLAG_ACTIVATED) {
			args = rt_tick_from_millisecond(mbox->txpoll_period);
			rt_timer_control(&(mbox->poll_hrt),
					RT_TIMER_CTRL_SET_TIME,
					&args);
			rt_timer_start(&(mbox->poll_hrt));
		}

#endif
		rt_spin_unlock_irqrestore(&mbox->poll_hrt_lock, flags);
	}
}

/**
 * mbox_chan_received_data - A way for controller driver to push data
 *                              received from remote to the upper layer.
 * @chan: Pointer to the mailbox channel on which RX happened.
 * @mssg: Client specific message typecasted as void *
 *
 * After startup and before shutdown any data received on the chan
 * is passed on to the API via atomic mbox_chan_received_data().
 * The controller should ACK the RX only after this call returns.
 */
void mbox_chan_received_data(struct mbox_chan *chan, void *mssg)
{
	/* No buffering the received data */
	if (chan->cl->rx_callback)
		chan->cl->rx_callback(chan->cl, mssg);
}

/**
 * mbox_chan_txdone - A way for controller driver to notify the
 *                      framework that the last TX has completed.
 * @chan: Pointer to the mailbox chan on which TX happened.
 * @r: Status of last TX - OK or ERROR
 *
 * The controller that has IRQ for TX ACK calls this atomic API
 * to tick the TX state machine. It works only if txdone_irq
 * is set by the controller.
 */
void mbox_chan_txdone(struct mbox_chan *chan, int r)
{
	if (!(chan->txdone_method & TXDONE_BY_IRQ)) {
		rt_kprintf("Controller can't run the TX ticker\n");
		return;
	}

	tx_tick(chan, r);
}

/**
 * mbox_client_txdone - The way for a client to run the TX state machine.
 * @chan: Mailbox channel assigned to this client.
 * @r: Success status of last transmission.
 *
 * The client/protocol had received some 'ACK' packet and it notifies
 * the API that the last packet was sent successfully. This only works
 * if the controller can't sense TX-Done.
 */
void mbox_client_txdone(struct mbox_chan *chan, int r)
{
	if (!(chan->txdone_method & TXDONE_BY_ACK)) {
		rt_kprintf("Client can't run the TX ticker\n");
		return;
	}

	tx_tick(chan, r);
}

/**
 * mbox_client_peek_data - A way for client driver to pull data
 *                      received from remote by the controller.
 * @chan: Mailbox channel assigned to this client.
 *
 * A poke to controller driver for any received data.
 * The data is actually passed onto client via the
 * mbox_chan_received_data()
 * The call can be made from atomic context, so the controller's
 * implementation of peek_data() must not sleep.
 *
 * Return: True, if controller has, and is going to push after this,
 *          some data.
 *         False, if controller doesn't have any data to be read.
 */
bool mbox_client_peek_data(struct mbox_chan *chan)
{
	if (chan->mbox->ops->peek_data)
		return chan->mbox->ops->peek_data(chan);

	return false;
}

/**
 * mbox_send_message -  For client to submit a message to be
 *                              sent to the remote.
 * @chan: Mailbox channel assigned to this client.
 * @mssg: Client specific message typecasted.
 *
 * For client to submit data to the controller destined for a remote
 * processor. If the client had set 'tx_block', the call will return
 * either when the remote receives the data or when 'tx_tout' millisecs
 * run out.
 *  In non-blocking mode, the requests are buffered by the API and a
 * non-negative token is returned for each queued request. If the request
 * is not queued, a negative token is returned. Upon failure or successful
 * TX, the API calls 'tx_done' from atomic context, from which the client
 * could submit yet another request.
 * The pointer to message should be preserved until it is sent
 * over the chan, i.e, tx_done() is made.
 * This function could be called from atomic context as it simply
 * queues the data and returns a token against the request.
 *
 * Return: Non-negative integer for successful submission (non-blocking mode)
 *      or transmission over chan (blocking mode).
 *      Negative value denotes failure.
 */
int mbox_send_message(struct mbox_chan *chan, void *mssg)
{
	int t;

	if (!chan || !chan->cl)
		return -RT_EINVAL;

	t = add_to_rbuf(chan, mssg);
	if (t < 0) {
		rt_kprintf("Try increasing MBOX_TX_QUEUE_LEN\n");
		return t;
	}

	msg_submit(chan);

	if (chan->cl->tx_block) {
		unsigned long wait;
		int ret;

		if (!chan->cl->tx_tout) /* wait forever */
			/* wait = msecs_to_jiffies(3600000); */
			wait = RT_WAITING_FOREVER; 
		else
			wait = rt_tick_from_millisecond(chan->cl->tx_tout);
		
		ret = rt_completion_wait(&chan->tx_complete, wait);
		if (ret == 0) {
			t = -ETIME;
			tx_tick(chan, t);
		}
	}

	return t;
}

/**
 * mbox_flush - flush a mailbox channel
 * @chan: mailbox channel to flush
 * @timeout: time, in milliseconds, to allow the flush operation to succeed
 *
 * Mailbox controllers that need to work in atomic context can implement the
 * ->flush() callback to busy loop until a transmission has been completed.
 * The implementation must call mbox_chan_txdone() upon success. Clients can
 * call the mbox_flush() function at any time after mbox_send_message() to
 * flush the transmission. After the function returns success, the mailbox
 * transmission is guaranteed to have completed.
 *
 * Returns: 0 on success or a negative error code on failure.
 */
int mbox_flush(struct mbox_chan *chan, unsigned long timeout)
{
	int ret;

	if (!chan->mbox->ops->flush)
		return -ENOTSUPP;

	ret = chan->mbox->ops->flush(chan, timeout);
	if (ret < 0)
		tx_tick(chan, ret);

	return ret;
}

static int __mbox_bind_client(struct mbox_chan *chan, struct mbox_client *cl)
{
	int ret;
	unsigned long flags;

	if (chan->cl /* || !try_module_get(chan->mbox->dev->driver->owner) */) {
		rt_kprintf("%s: mailbox not free\n", __func__);
		return -EBUSY;
	}

	flags = rt_spin_lock_irqsave(&chan->lock);
	chan->msg_free = 0;
	chan->msg_count = 0;
	chan->active_req = NULL;
	chan->cl = cl;
	rt_completion_init(&chan->tx_complete);

	if (chan->txdone_method == TXDONE_BY_POLL && cl->knows_txdone)
		chan->txdone_method = TXDONE_BY_ACK;

	rt_spin_unlock_irqrestore(&chan->lock, flags);

	if (chan->mbox->ops->startup) {
		ret = chan->mbox->ops->startup(chan);

		if (ret) {
			rt_kprintf("Unable to startup the chan (%d)\n", ret);
			mbox_free_channel(chan);
			return ret;
		}
	}

	return 0;
}

/**
 * mbox_bind_client - Request a mailbox channel.
 * @chan: The mailbox channel to bind the client to.
 * @cl: Identity of the client requesting the channel.
 *
 * The Client specifies its requirements and capabilities while asking for
 * a mailbox channel. It can't be called from atomic context.
 * The channel is exclusively allocated and can't be used by another
 * client before the owner calls mbox_free_channel.
 * After assignment, any packet received on this channel will be
 * handed over to the client via the 'rx_callback'.
 * The framework holds reference to the client, so the mbox_client
 * structure shouldn't be modified until the mbox_free_channel returns.
 *
 * Return: 0 if the channel was assigned to the client successfully.
 *         <0 for request failure.
 */
int mbox_bind_client(struct mbox_chan *chan, struct mbox_client *cl)
{
	int ret;

	rt_mutex_take(&con_mutex, RT_WAITING_FOREVER);
	ret = __mbox_bind_client(chan, cl);
	rt_mutex_release(&con_mutex);

	return ret;
}

/**
 * mbox_request_channel - Request a mailbox channel.
 * @cl: Identity of the client requesting the channel.
 * @index: Index of mailbox specifier in 'mboxes' property.
 *
 * The Client specifies its requirements and capabilities while asking for
 * a mailbox channel. It can't be called from atomic context.
 * The channel is exclusively allocated and can't be used by another
 * client before the owner calls mbox_free_channel.
 * After assignment, any packet received on this channel will be
 * handed over to the client via the 'rx_callback'.
 * The framework holds reference to the client, so the mbox_client
 * structure shouldn't be modified until the mbox_free_channel returns.
 *
 * Return: Pointer to the channel assigned to the client if successful.
 *              ERR_PTR for request failure.
 */
struct mbox_chan *mbox_request_channel(struct mbox_client *cl, int index)
{
	struct dtb_node *dev = cl->dev;
	struct mbox_controller *mbox;
	struct fdt_phandle_args spec;
	struct mbox_chan *chan;
	int ret;

	if (!dev) {
		rt_kprintf("%s: No owner device node\n", __func__);
		return ERR_PTR(-ENODEV);
	}

	rt_mutex_take(&con_mutex, RT_WAITING_FOREVER);

	if (dtb_node_parse_phandle_with_args(dev, "mboxes",
				"#mbox-cells", index, &spec)) {
		rt_kprintf("%s: can't parse \"mboxes\" property\n", __func__);
		rt_mutex_release(&con_mutex);
		return ERR_PTR(-ENODEV);
	}

	chan = ERR_PTR(-EPROBE_DEFER);
	rt_list_for_each_entry(mbox, &mbox_cons, node)
		if (mbox->dev == spec.np) {
			chan = mbox->of_xlate(mbox, &spec);
			if (!IS_ERR(chan))
				break;
		}

	dtb_node_put(spec.np);

	if (IS_ERR(chan)) {
		rt_mutex_release(&con_mutex);
		return chan;
	}

	ret = __mbox_bind_client(chan, cl);
	if (ret)
		chan = ERR_PTR(ret);

	rt_mutex_release(&con_mutex);

	return chan;
}

struct mbox_chan *mbox_request_channel_byname(struct mbox_client *cl,
                                              const char *name)
{
	struct dtb_node *np = cl->dev;
	int prop_size;
	const char *mbox_name;
	int index = 0;

	if (!np) {
		rt_kprintf("%s() currently only supports DT\n", __func__);
		return ERR_PTR(-RT_EINVAL);
	}

	if (!dtb_node_get_property(np, "mbox-names", NULL)) {
                rt_kprintf("%s() requires an \"mbox-names\" property\n", __func__);
		return ERR_PTR(-RT_EINVAL);
	}

	for_each_property_string(np, "mbox-names", mbox_name, prop_size) {
		if (!rt_strncmp(name, mbox_name, rt_strlen(name)))
			return mbox_request_channel(cl, index);
		index++;
	}

        rt_kprintf("%s() could not locate channel named \"%s\"\n", __func__, name);
	return ERR_PTR(-RT_EINVAL);
}

/**
 * mbox_free_channel - The client relinquishes control of a mailbox
 *                      channel by this call.
 * @chan: The mailbox channel to be freed.
 */
void mbox_free_channel(struct mbox_chan *chan)
{
	unsigned long flags;

	if (!chan || !chan->cl)
		return;

	if (chan->mbox->ops->shutdown)
		chan->mbox->ops->shutdown(chan);

	/* The queued TX requests are simply aborted, no callbacks are made */
	flags = rt_spin_lock_irqsave(&chan->lock);
	chan->cl = NULL;
	chan->active_req = NULL;
	if (chan->txdone_method == TXDONE_BY_ACK)
		chan->txdone_method = TXDONE_BY_POLL;

	/* module_put(chan->mbox->dev->driver->owner); */
	rt_spin_unlock_irqrestore(&chan->lock, flags);
}

static struct mbox_chan *
of_mbox_index_xlate(struct mbox_controller *mbox,
                    const struct fdt_phandle_args *sp)
{
	int ind = sp->args[0];

	if (ind >= mbox->num_chans)
		return ERR_PTR(-RT_EINVAL);

	return &mbox->chans[ind];
}

/**
 * mbox_controller_register - Register the mailbox controller
 * @mbox:       Pointer to the mailbox controller.
 *
 * The controller driver registers its communication channels
 */
int mbox_controller_register(struct mbox_controller *mbox)
{
	int i, txdone;

	/* Sanity check */
	if (!mbox || !mbox->dev || !mbox->ops || !mbox->num_chans)
		return -RT_EINVAL;

	if (mbox->txdone_irq)
		txdone = TXDONE_BY_IRQ;
	else if (mbox->txdone_poll)
		txdone = TXDONE_BY_POLL;
	else /* It has to be ACK then */
		txdone = TXDONE_BY_ACK;

	if (txdone == TXDONE_BY_POLL) {

		if (!mbox->ops->last_tx_done) {
			rt_kprintf("last_tx_done method is absent\n");
			return -RT_EINVAL;
		}

#if 0
		hrtimer_init(&mbox->poll_hrt, CLOCK_MONOTONIC,
				HRTIMER_MODE_REL);
		mbox->poll_hrt.function = txdone_hrtimer;
#else
		/* initialize thread timer */
		rt_timer_init(&mbox->poll_hrt,
				mbox->dev->name,
				txdone_timer,
				mbox,
				0,
				RT_TIMER_FLAG_ONE_SHOT);
#endif
		rt_spin_lock_init(&mbox->poll_hrt_lock);
	}

	for (i = 0; i < mbox->num_chans; i++) {
		struct mbox_chan *chan = &mbox->chans[i];

		chan->cl = RT_NULL;
		chan->mbox = mbox;
		chan->txdone_method = txdone;
		rt_spin_lock_init(&chan->lock);
	}

	if (!mbox->of_xlate)
		mbox->of_xlate = of_mbox_index_xlate;

	rt_mutex_take(&con_mutex, RT_WAITING_FOREVER);
	rt_list_insert_after(&mbox_cons, &mbox->node);
	rt_mutex_release(&con_mutex);

	return 0;
}

/**
 * mbox_controller_unregister - Unregister the mailbox controller
 * @mbox:       Pointer to the mailbox controller.
 */
void mbox_controller_unregister(struct mbox_controller *mbox)
{
	int i;

	if (!mbox)
		return;

	rt_mutex_take(&con_mutex, RT_WAITING_FOREVER);

	rt_list_remove(&mbox->node);

	for (i = 0; i < mbox->num_chans; i++)
		mbox_free_channel(&mbox->chans[i]);

	if (mbox->txdone_poll)
		rt_timer_detach(&mbox->poll_hrt);

	rt_mutex_release(&con_mutex);
}

static int  __component_mailbox_init(void)
{
	rt_mutex_init(&con_mutex, "con_mbox_mutex", RT_IPC_FLAG_PRIO);

	return 0;
}

INIT_BOARD_EXPORT(__component_mailbox_init);
