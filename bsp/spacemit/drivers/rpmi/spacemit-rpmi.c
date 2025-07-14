#include <rthw.h>
#include <rtthread.h>
#include <dtb_head.h>
#include "spacemit-rpmi.h"

extern struct spacemit_rpmi_func rpmi_hsm_func;
extern struct spacemit_rpmi_func rpmi_clk_func;
extern struct spacemit_rpmi_func rpmi_voltage_func;

static struct dtb_compatible_array __k1_compatible_sub[] = {
	{ .compatible = "k3-os0-rpmi-clock", .data = (void *)&rpmi_clk_func },
	{ .compatible = "k3-os0-rpmi-voltage", .data = (void *)&rpmi_voltage_func },
	{ .compatible = "k3-os0-rpmi-hsm", .data = (void *)&rpmi_hsm_func },
	{},
};

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,k3-os0-rpmi", .data = __k1_compatible_sub },
	{}
};

static rt_int32_t spacemit_rpmi_get_config_from_dt(struct dtb_node *node, struct spacemit_rpmi_config *config)
{
	const void* prop_data;
	rt_int32_t prop_len;

	/* get the configuration */
	prop_data = dtb_node_get_property(node, "shmem-base", &prop_len);
	if (!prop_data) {
		rt_kprintf("%s:%d, get shmem-base failed\n", __func__, __LINE__);
		return -RT_EINVAL;
	}

	config->shmem_base = fdt32_to_cpu(*(uint32_t*)prop_data);

	/* k3 memory base */
	config->shmem_base |= 0x100000000;

	prop_data = dtb_node_get_property(node, "shmem-size", &prop_len);
	if (!prop_data) {
		rt_kprintf("%s:%d, get shmem-size failed\n", __func__, __LINE__);
		return -RT_EINVAL;
	}

	config->shmem_size = fdt32_to_cpu(*(uint32_t*)prop_data);

	prop_data = dtb_node_get_property(node, "slot-size", &prop_len);
	if (!prop_data) {
		rt_kprintf("%s:%d, get slot-size failed\n", __func__, __LINE__);
		return -RT_EINVAL;
	}

	config->slot_size = fdt32_to_cpu(*(uint32_t*)prop_data);

	prop_data = dtb_node_get_property(node, "a2p-queue-size", &prop_len);
	if (!prop_data) {
		rt_kprintf("%s:%d, get a2p-queue-size failed\n", __func__, __LINE__);
		return -RT_EINVAL;
	}

	config->a2p_queue_size = fdt32_to_cpu(*(uint32_t*)prop_data);

	prop_data = dtb_node_get_property(node, "p2a-queue-size", &prop_len);
	if (!prop_data) {
		rt_kprintf("%s:%d, get a2p-queue-size failed\n", __func__, __LINE__);
		return -RT_EINVAL;
	}
	config->p2a_queue_size = fdt32_to_cpu(*(uint32_t*)prop_data);

	return 0;

}

static void rpmi_rx_callback(struct mbox_client *cl, void *data)
{
	struct spacemit_rpmi_priv *priv = rt_container_of(cl, struct spacemit_rpmi_priv, client);

	rt_sem_release(priv->sem);
}

static void spacemit_rpmi_poll(void *priv)
{
	struct spacemit_rpmi_priv *ppriv = (struct spacemit_rpmi_priv *)priv;

	while(1) {
		/* wait the tick or 20ms polling */
		rt_sem_take(ppriv->sem, RT_WAITING_FOREVER);

		/* Use proper API function as suggested by compiler */
		rpmi_context_process_a2p_request(ppriv->cntx);
		
		rpmi_context_process_all_events(ppriv->cntx);
	}
}

static rt_int32_t spacemit_rpmi_create_foundation(char *name, struct spacemit_rpmi_priv *priv, rt_int32_t number_service)
{
	char *tmp;
	char *string;
	rt_int32_t size;
	struct rpmi_shmem* shmem = NULL;
	struct rpmi_transport* transport = NULL;

	/* Create shared memory instance */
	tmp = rt_calloc(1, 64);
	rt_snprintf(tmp, 64, "%s_shmem", name);


	shmem = rpmi_shmem_create(tmp, priv->config.shmem_base, priv->config.shmem_size, &rpmi_shmem_simple_noncoherent_ops, NULL);
	if (!shmem) {
		rt_kprintf("%s: create shmem failed\n", name);
		return -RT_EINVAL;
	}

	/* Create transport using shared memory - using configuration from DTS */
	tmp = rt_calloc(1, 64);
	rt_snprintf(tmp, 64, "%s_transport", name);
	transport = rpmi_transport_shmem_create(tmp, priv->config.slot_size, priv->config.a2p_queue_size,
			priv->config.p2a_queue_size,
			shmem);
	if (!transport) {
		rt_kprintf("%s: create tranport failed\n", name);
		return -RT_EINVAL;
	}

	/* Create RPMI context */
	tmp = rt_calloc(1, 64);
	rt_snprintf(tmp, 64, "%s_cntx", name);
	priv->cntx = rpmi_context_create(tmp, transport, number_service, RPMI_PRIVILEGE_M_MODE, 0, NULL);
	if (!priv->cntx) {
		rt_kprintf("%s: create cntx failed\n", name);
		return -RT_EINVAL;
	}
	
	/* get the mailbox */
	for_each_property_string(priv->node, "mbox-names", string, size) {
		priv->client.dev = priv->node;
		priv->client.tx_block = true;
		priv->client.rx_callback = rpmi_rx_callback;
		priv->chan = mbox_request_channel_byname(&priv->client, string);
	}

	/* create the sem and the thread */
	priv->sem = rt_sem_create(name, 0, RT_IPC_FLAG_FIFO);

	/* create the rpmsg poll thread */
	priv->tid = rt_thread_create(name,
			spacemit_rpmi_poll,
			(void *)priv,
			2048,
			RT_THREAD_PRIORITY_MAX / 3,
			20);
	if (!priv->tid) {
		rt_kprintf("Failed to create adma service\n");
		return -RT_EINVAL;
	}

	rt_thread_startup(priv->tid);

	return 0;
}

rt_int32_t rt_hw_rpmi_init(void)
{
	rt_int32_t i, j, ret, c= 0;
	struct spacemit_rpmi_priv *priv;
	struct dtb_node *compatible_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
			__compatible[i].compatible);

		if (compatible_node != RT_NULL) {

			/* check the status */
			if (!dtb_node_device_is_available(compatible_node))
				continue;

			priv = (struct spacemit_rpmi_priv *)rt_calloc(1, sizeof(struct spacemit_rpmi_priv));
			if (priv == RT_NULL) {
				rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}

			priv->node = compatible_node;

			ret = spacemit_rpmi_get_config_from_dt(compatible_node, &priv->config);
			if (ret < 0) {
				rt_kprintf("%s:%d, get config error\n", __func__, __LINE__);
				return -RT_EINVAL;
			}

			/* find the sub numbers of services */
			j = 0;
			struct dtb_compatible_array *sub_node = (struct dtb_compatible_array *)__compatible[i].data;

			while (sub_node[j].compatible) {
				compatible_node = dtb_node_find_compatible_node(dtb_head_node, sub_node[j].compatible);

				/* check the status */
				if (!dtb_node_device_is_available(compatible_node))
					break;
				++j;
			}

			/* create the base cntx */
			ret = spacemit_rpmi_create_foundation(__compatible[i].compatible, priv, j + 1 + 1 /* hsm with syssusp and base */);
			if (ret < 0) {
				rt_kprintf("%s:%d, create foundation failed\n", __func__, __LINE__);
				return -RT_EINVAL;
			}

			for (c = 0; c <= j; ++c) {
				compatible_node = dtb_node_find_compatible_node(dtb_head_node, sub_node[c].compatible);

				/* check the status */
				if (!dtb_node_device_is_available(compatible_node))
					break;

				struct spacemit_rpmi_func *func = (struct spacemit_rpmi_func *)sub_node[c].data;

				ret = func->rmpi_get_configuration(compatible_node, (void *)&priv->config, sub_node[c].compatible);
				if (ret < 0) {
					rt_kprintf("%s, get the configuration failed\n", sub_node[j].compatible, __func__, __LINE__);
					return -RT_EINVAL;
				}

				ret = func->rpmi_register_service((void *)&priv->config, priv->cntx);
				if (ret < 0) {
					rt_kprintf("%s, register service failed\n", sub_node[j].compatible);
					return -RT_EINVAL;
				}
			}
		}
	}

	return 0;
}
INIT_COMPONENT_EXPORT(rt_hw_rpmi_init);
