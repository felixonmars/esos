/*
 * Copyright (c) 2025, Spacemit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Spacemit K3 Generic PHY driver - RT-Thread port
 *
 */

#include "genphy.h"

struct mii_dev *mdio_alloc(void)
{
	struct mii_dev *mdio;

	mdio = rt_malloc(sizeof(struct mii_dev));
	if (!mdio)
		return RT_NULL;

	rt_memset(mdio, 0, sizeof(struct mii_dev));

	rt_memset(mdio->name, 0, MDIO_NAME_LEN);

	for (int i = 0; i < PHY_MAX_ADDR; i++)
		mdio->phymap[i] = RT_NULL;

	mdio->phy_mask = 0;

	return mdio;
}

void mdio_free(struct mii_dev *mdio)
{
	if (mdio)
		rt_free(mdio);
}

rt_int32_t phy_read(struct phy_device *phydev, rt_int32_t devad, rt_int32_t regnum)
{
	struct mii_dev *bus = phydev->bus;

	if (!bus || !bus->read) {
		rt_kprintf("%s: No bus configured\n", __func__);
		return -RT_EINVAL;
	}

	return bus->read(bus, phydev->addr, devad, regnum);
}

rt_int32_t phy_write(struct phy_device *phydev, rt_int32_t devad,
		rt_int32_t regnum, rt_uint16_t val)
{
	struct mii_dev *bus = phydev->bus;

	if (!bus || !bus->write) {
		rt_kprintf("%s: No bus configured\n", __func__);
		return -RT_EINVAL;
	}

	return bus->write(bus, phydev->addr, devad, regnum, val);
}

rt_int32_t phy_modify(struct phy_device *phydev, rt_int32_t devad,
		rt_int32_t regnum, rt_uint16_t mask, rt_uint16_t set)
{
	rt_int32_t val, old, new;

	val = phy_read(phydev, devad, regnum);
	if (val < 0)
		return val;

	old = (rt_uint16_t)val;

	new = (old & ~mask) | (set & mask);
	if (new == old)
		return 0;

	return phy_write(phydev, devad, regnum, new);
}

void phy_dump_registers(struct phy_device *phydev)
{
	rt_int32_t val;
	int reg;

	if (!phydev) {
		rt_kprintf("%s: invalid argument\n", __func__);
		return;
	}

	rt_kprintf("Dump PHY registers (addr=%d):\n", phydev->addr);

	for (reg = 0; reg < 32; reg++) {
		val = phy_read(phydev, MDIO_DEVAD_NONE, reg);
		if (val < 0)
			rt_kprintf("  Reg %02d: <read error %d>\n", reg, val);
		else
			rt_kprintf("  Reg %02d: 0x%04x\n", reg, (rt_uint16_t)val);
	}
}

/* Generic PHY support and helper functions */

/**
 * genphy_config_advert - sanitize and advertise auto-negotiation parameters
 * @phydev: target phy_device struct
 *
 * Description: Writes MII_ADVERTISE with the appropriate values,
 *   after sanitizing the values to make sure we only advertise
 *   what is supported.	 Returns < 0 on error, 0 if the PHY's advertisement
 *   hasn't changed, and > 0 if it has changed.
 */
static rt_int32_t genphy_config_advert(struct phy_device *phydev)
{
	rt_uint32_t advertise;
	rt_int32_t oldadv, adv, bmsr;
	rt_int32_t err, changed = 0;

	/* Only allow advertising what this PHY supports */
	phydev->advertising &= phydev->supported;
	advertise = phydev->advertising;

	/* Setup standard advertisement */
	adv = phy_read(phydev, MDIO_DEVAD_NONE, MII_ADVERTISE);
	oldadv = adv;

	if (adv < 0)
		return adv;

	adv &= ~(ADVERTISE_ALL | ADVERTISE_100BASE4 | ADVERTISE_PAUSE_CAP |
		 ADVERTISE_PAUSE_ASYM);
	if (advertise & ADVERTISED_10baseT_Half)
		adv |= ADVERTISE_10HALF;
	if (advertise & ADVERTISED_10baseT_Full)
		adv |= ADVERTISE_10FULL;
	if (advertise & ADVERTISED_100baseT_Half)
		adv |= ADVERTISE_100HALF;
	if (advertise & ADVERTISED_100baseT_Full)
		adv |= ADVERTISE_100FULL;
	if (advertise & ADVERTISED_Pause)
		adv |= ADVERTISE_PAUSE_CAP;
	if (advertise & ADVERTISED_Asym_Pause)
		adv |= ADVERTISE_PAUSE_ASYM;
	if (advertise & ADVERTISED_1000baseX_Half)
		adv |= ADVERTISE_1000XHALF;
	if (advertise & ADVERTISED_1000baseX_Full)
		adv |= ADVERTISE_1000XFULL;

	if (adv != oldadv) {
		err = phy_write(phydev, MDIO_DEVAD_NONE, MII_ADVERTISE, adv);

		if (err < 0)
			return err;
		changed = 1;
	}

	bmsr = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);
	if (bmsr < 0)
		return bmsr;

	/* Per 802.3-2008, Section 22.2.4.2.16 Extended status all
	 * 1000Mbits/sec capable PHYs shall have the BMSR_ESTATEN bit set to a
	 * logical 1.
	 */
	if (!(bmsr & BMSR_ESTATEN))
		return changed;

	/* Configure gigabit if it's supported */
	adv = phy_read(phydev, MDIO_DEVAD_NONE, MII_CTRL1000);
	oldadv = adv;

	if (adv < 0)
		return adv;

	adv &= ~(ADVERTISE_1000FULL | ADVERTISE_1000HALF);

	if (phydev->supported & (SUPPORTED_1000baseT_Half |
				SUPPORTED_1000baseT_Full)) {
		if (advertise & SUPPORTED_1000baseT_Half)
			adv |= ADVERTISE_1000HALF;
		if (advertise & SUPPORTED_1000baseT_Full)
			adv |= ADVERTISE_1000FULL;
	}

	if (adv != oldadv)
		changed = 1;

	err = phy_write(phydev, MDIO_DEVAD_NONE, MII_CTRL1000, adv);
	if (err < 0)
		return err;

	return changed;
}

/**
 * genphy_setup_forced - configures/forces speed/duplex from @phydev
 * @phydev: target phy_device struct
 *
 * Description: Configures MII_BMCR to force speed/duplex
 *   to the values in phydev. Assumes that the values are valid.
 */
static rt_int32_t genphy_setup_forced(struct phy_device *phydev)
{
	rt_int32_t err;
	rt_int32_t ctl = BMCR_ANRESTART;

	phydev->pause = 0;
	phydev->asym_pause = 0;

	if (phydev->speed == SPEED_1000)
		ctl |= BMCR_SPEED1000;
	else if (phydev->speed == SPEED_100)
		ctl |= BMCR_SPEED100;

	if (phydev->duplex == DUPLEX_FULL)
		ctl |= BMCR_FULLDPLX;

	err = phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, ctl);

	return err;
}

/**
 * genphy_restart_aneg - Enable and Restart Autonegotiation
 * @phydev: target phy_device struct
 */
rt_int32_t genphy_restart_aneg(struct phy_device *phydev)
{
	rt_int32_t ctl;

	ctl = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMCR);

	if (ctl < 0)
		return ctl;

	ctl |= (BMCR_ANENABLE | BMCR_ANRESTART);

	/* Don't isolate the PHY if we're negotiating */
	ctl &= ~(BMCR_ISOLATE);

	ctl = phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, ctl);

	return ctl;
}

rt_int32_t genphy_config_aneg(struct phy_device *phydev)
{
	rt_int32_t result;

	if (phydev->autoneg != AUTONEG_ENABLE)
		return genphy_setup_forced(phydev);

	result = genphy_config_advert(phydev);

	if (result < 0) /* error */
		return result;

	if (result == 0) {
		/*
		 * Advertisment hasn't changed, but maybe aneg was never on to
		 * begin with?	Or maybe phy was isolated?
		 */
		rt_int32_t ctl = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMCR);

		if (ctl < 0)
			return ctl;

		if (!(ctl & BMCR_ANENABLE) || (ctl & BMCR_ISOLATE))
			result = 1; /* do restart aneg */
	}

	/*
	 * Only restart aneg if we are advertising something different
	 * than we were before.
	 */
	if (result > 0)
		result = genphy_restart_aneg(phydev);

	return result;
}

/**
 * genphy_update_link - update link status in @phydev
 * @phydev: target phy_device struct
 *
 * Description: Update the value in phydev->link to reflect the
 *   current link value.  In order to do this, we need to read
 *   the status register twice, keeping the second value.
 */
rt_int32_t genphy_update_link(struct phy_device *phydev)
{
	rt_uint32_t mii_reg;

	/*
	 * Wait if the link is up, and autonegotiation is in progress
	 * (ie - we're capable and it's not done)
	 */
	mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);

	/*
	 * If we already saw the link up, and it hasn't gone down, then
	 * we don't need to wait for autoneg again
	 */
	if (phydev->link && mii_reg & BMSR_LSTATUS)
		return 0;

	if ((phydev->autoneg == AUTONEG_ENABLE) &&
	    !(mii_reg & BMSR_ANEGCOMPLETE)) {
		rt_int32_t i = 0;

		rt_kprintf("Waiting for PHY auto negotiation to complete");
		while (!(mii_reg & BMSR_ANEGCOMPLETE)) {
			/*
			 * Timeout reached ?
			 */
			if (i > (PHY_ANEG_TIMEOUT / 50)) {
				rt_kprintf(" TIMEOUT !\n");
				phydev->link = 0;
				return -RT_ETIMEOUT;
			}

			if ((i++ % 10) == 0)
				rt_kprintf(".");

			mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);
			rt_thread_mdelay(50);	/* 50 ms */
		}
		rt_kprintf(" done\n");
		phydev->link = 1;
	} else {
		/* Read the link a second time to clear the latched state */
		mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);

		phydev->link = !!(mii_reg & BMSR_LSTATUS);
	}

	return 0;
}

/*
 * Generic function which updates the speed and duplex.	 If
 * autonegotiation is enabled, it uses the AND of the link
 * partner's advertised capabilities and our advertised
 * capabilities.  If autonegotiation is disabled, we use the
 * appropriate bits in the control register.
 *
 * Stolen from Linux's mii.c and phy_device.c
 */
rt_int32_t genphy_parse_link(struct phy_device *phydev)
{
	rt_int32_t mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);

	/* We're using autonegotiation */
	if (phydev->autoneg == AUTONEG_ENABLE) {
		rt_uint32_t lpa = 0;
		rt_int32_t gblpa = 0;
		rt_uint32_t estatus = 0;

		/* Check for gigabit capability */
		if (phydev->supported & (SUPPORTED_1000baseT_Full |
					SUPPORTED_1000baseT_Half)) {
			/* We want a list of states supported by
			 * both PHYs in the link
			 */
			gblpa = phy_read(phydev, MDIO_DEVAD_NONE, MII_STAT1000);
			if (gblpa < 0) {
				rt_kprintf("Could not read MII_STAT1000. ");
				rt_kprintf("Ignoring gigabit capability\n");
				gblpa = 0;
			}
			gblpa &= phy_read(phydev,
					MDIO_DEVAD_NONE, MII_CTRL1000) << 2;
		}

		/* Set the baseline so we only have to set them
		 * if they're different
		 */
		phydev->speed = SPEED_10;
		phydev->duplex = DUPLEX_HALF;

		/* Check the gigabit fields */
		if (gblpa & (PHY_1000BTSR_1000FD | PHY_1000BTSR_1000HD)) {
			phydev->speed = SPEED_1000;

			if (gblpa & PHY_1000BTSR_1000FD)
				phydev->duplex = DUPLEX_FULL;

			/* We're done! */
			return 0;
		}

		lpa = phy_read(phydev, MDIO_DEVAD_NONE, MII_ADVERTISE);
		lpa &= phy_read(phydev, MDIO_DEVAD_NONE, MII_LPA);

		if (lpa & (LPA_100FULL | LPA_100HALF)) {
			phydev->speed = SPEED_100;

			if (lpa & LPA_100FULL)
				phydev->duplex = DUPLEX_FULL;

		} else if (lpa & LPA_10FULL) {
			phydev->duplex = DUPLEX_FULL;
		}

		/*
		 * Extended status may indicate that the PHY supports
		 * 1000BASE-T/X even though the 1000BASE-T registers
		 * are missing. In this case we can't tell whether the
		 * peer also supports it, so we only check extended
		 * status if the 1000BASE-T registers are actually
		 * missing.
		 */
		if ((mii_reg & BMSR_ESTATEN) && !(mii_reg & BMSR_ERCAP))
			estatus = phy_read(phydev, MDIO_DEVAD_NONE,
					   MII_ESTATUS);

		if (estatus & (ESTATUS_1000_XFULL | ESTATUS_1000_XHALF |
				ESTATUS_1000_TFULL | ESTATUS_1000_THALF)) {
			phydev->speed = SPEED_1000;
			if (estatus & (ESTATUS_1000_XFULL | ESTATUS_1000_TFULL))
				phydev->duplex = DUPLEX_FULL;
		}

	} else {
		rt_uint32_t bmcr = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMCR);

		phydev->speed = SPEED_10;
		phydev->duplex = DUPLEX_HALF;

		if (bmcr & BMCR_FULLDPLX)
			phydev->duplex = DUPLEX_FULL;

		if (bmcr & BMCR_SPEED1000)
			phydev->speed = SPEED_1000;
		else if (bmcr & BMCR_SPEED100)
			phydev->speed = SPEED_100;
	}

	return 0;
}

static rt_int32_t phy_read_id(struct phy_device *phydev, rt_uint32_t *phy_id)
{
	rt_int32_t id1, id2;

	id1 = phy_read(phydev, 0, MII_PHYSID1);
	if (id1 < 0)
		return -RT_ERROR;

	id2 = phy_read(phydev, 0, MII_PHYSID2);
	if (id2 < 0)
		return -RT_ERROR;

	*phy_id = ((rt_uint32_t)id1 << 16) | (id2 & 0xffff);
	return 0;
}

static struct phy_device *phy_device_create(struct mii_dev *bus, int addr, rt_uint32_t phy_id)
{
	struct phy_device *phydev = rt_calloc(1, sizeof(struct phy_device));

	if (!phydev)
		return RT_NULL;

	phydev->bus = bus;
	phydev->addr = addr;
	phydev->phy_id = phy_id;
	phydev->autoneg = 1;
	phydev->supported = 0xffffffff;
	phydev->advertising = 0xffffffff;
	phydev->link = 0;
	phydev->speed = 0;
	phydev->duplex = -1;
	phydev->iface = PHY_INTERFACE_MODE_NA;

	if (addr >= 0 && addr < PHY_MAX_ADDR && phy_id != PHY_FIXED_ID)
		bus->phymap[addr] = phydev;
	return phydev;
}

rt_int32_t phy_reset(struct phy_device *phydev)
{
	rt_int32_t reg;
	rt_int32_t timeout = 100;
	rt_int32_t devad = MDIO_DEVAD_NONE;

	if (phydev->flags & PHY_FLAG_BROKEN_RESET)
		return 0;

	if (phy_write(phydev, devad, MII_BMCR, BMCR_RESET) < 0) {
		rt_kprintf("PHY reset failed\n");
		return -1;
	}
	/*
	 * Poll the control register for the reset bit to go to 0 (it is
	 * auto-clearing).  This should happen within 0.5 seconds per the
	 * IEEE spec.
	 */
	reg = phy_read(phydev, devad, MII_BMCR);
	while ((reg & BMCR_RESET) && timeout--) {
		reg = phy_read(phydev, devad, MII_BMCR);

		if (reg < 0) {
			rt_kprintf("PHY status read failed\n");
			return -1;
		}
		rt_thread_mdelay(5);
	}

	if (reg & BMCR_RESET) {
		rt_kprintf("PHY reset timed out\n");
		return -1;
	}

	return 0;
}

static int phy_wait_link(struct phy_device *phydev)
{
	int val;
	int timeout = 1000;

	do {
		val = phy_read(phydev, 0, MII_BMSR);
		if (val < 0)
			return val;

		if (val & BMSR_LSTATUS)
			return 0;

		rt_thread_mdelay(5);
	} while (--timeout);

	return -RT_ETIMEOUT;
}

static void genphy_update_link_noblock(struct phy_device *phydev)
{
	rt_uint32_t bmsr1, bmsr2;

	bmsr1 = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);
	if (bmsr1 < 0)
		return;

	/* Read the link a second time to clear the latched state */
	bmsr2 = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);
	if (bmsr2 < 0)
		return;

	phydev->link = !!(bmsr2 & BMSR_LSTATUS);
}

static void phy_monitor_thread(void *priv_data)
{
	struct phy_device *phydev = (struct phy_device *) priv_data;
	rt_uint32_t old_link, old_speed, old_duplex;

	if (!phydev->link_change_cb)
		return;

	phydev->monitor_running = true;

	while (phydev->monitor_running) {
		/* Store the previous state */
		old_link = phydev->link;
		old_speed = phydev->speed;
		old_duplex = phydev->duplex;

		/* Keep previous state if loopback is enabled because some PHYs
		 * report that Link is Down when loopback is enabled.
		 */
		if (!phydev->loopback_enabled) {
			/* Update the current state */
			genphy_update_link_noblock(phydev);
			genphy_parse_link(phydev);
		}

		/* Check if any of the link, speed, or duplex values have changed */
		if (phydev->link != old_link ||
			phydev->speed != old_speed ||
			phydev->duplex != old_duplex) {

			/* Call the link change callback */
			phydev->link_change_cb(phydev->priv);
		}

		/* Delay for 1 second before checking again */
		rt_thread_mdelay(1000);
	}
}

struct phy_device *phy_connect(struct mii_dev *bus, int addr, void *priv, phy_interface_t iface)
{
	struct phy_device *phydev = RT_NULL;
	rt_uint32_t phy_id = 0;

	if (!bus || !bus->read || !bus->write)
		return RT_NULL;

	if (addr < 0) {
		for (addr = 0; addr < PHY_MAX_ADDR; addr++) {
			struct phy_device temp = { .bus = bus, .addr = addr };

			if (phy_read_id(&temp, &phy_id) == 0 &&
			    (phy_id & PHY_ID_MASK) != PHY_ID_MASK) {
				phydev = phy_device_create(bus, addr, phy_id);
				break;
			}
		}
	} else {
		struct phy_device temp = { .bus = bus, .addr = addr };

		if (phy_read_id(&temp, &phy_id) == 0 &&
		    (phy_id & PHY_ID_MASK) != PHY_ID_MASK) {
			phydev = phy_device_create(bus, addr, phy_id);
		} else {
			rt_kprintf("Could not get PHY in addr: %d\n", addr);
		}
	}
	if (!phydev) {
		rt_kprintf("PHY device crate failed!\n");
		return RT_NULL;
	}

	phydev->priv = priv;
	phydev->iface = iface;

	if (phy_reset(phydev) != 0)
		rt_kprintf("PHY reset failed\n");

	return phydev;
}

rt_err_t phy_set_supported(struct phy_device *phydev, rt_uint32_t max_speed)
{
	/* The default values for phydev->supported are provided by the PHY
	 * driver "features" member, we want to reset to sane defaults first
	 * before supporting higher speeds.
	 */
	phydev->supported &= PHY_DEFAULT_FEATURES;

	switch (max_speed) {
	default:
		return -RT_EINVAL;
	case SPEED_1000:
		phydev->supported |= PHY_1000BT_FEATURES;
		/* fall through */
	case SPEED_100:
		phydev->supported |= PHY_100BT_FEATURES;
		/* fall through */
	case SPEED_10:
		phydev->supported |= PHY_10BT_FEATURES;
	}

	return RT_EOK;
}

rt_err_t phy_config(struct phy_device *phydev)
{
	rt_int32_t val;
	rt_uint32_t features;

	features = (SUPPORTED_TP | SUPPORTED_MII |
		    SUPPORTED_AUI | SUPPORTED_FIBRE |
		    SUPPORTED_BNC);

	/* Do we support autonegotiation? */
	val = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);

	if (val < 0)
		return val;

	if (val & BMSR_ANEGCAPABLE)
		features |= SUPPORTED_Autoneg;

	if (val & BMSR_100FULL)
		features |= SUPPORTED_100baseT_Full;
	if (val & BMSR_100HALF)
		features |= SUPPORTED_100baseT_Half;
	if (val & BMSR_10FULL)
		features |= SUPPORTED_10baseT_Full;
	if (val & BMSR_10HALF)
		features |= SUPPORTED_10baseT_Half;

	if (val & BMSR_ESTATEN) {
		val = phy_read(phydev, MDIO_DEVAD_NONE, MII_ESTATUS);

		if (val < 0)
			return val;

		if (val & ESTATUS_1000_TFULL)
			features |= SUPPORTED_1000baseT_Full;
		if (val & ESTATUS_1000_THALF)
			features |= SUPPORTED_1000baseT_Half;
		if (val & ESTATUS_1000_XFULL)
			features |= SUPPORTED_1000baseX_Full;
		if (val & ESTATUS_1000_XHALF)
			features |= SUPPORTED_1000baseX_Half;
	}

	phydev->supported &= features;
	phydev->advertising &= features;

	genphy_config_aneg(phydev);

	return RT_EOK;
}

rt_err_t phy_startup(struct phy_device *phydev)
{
	rt_err_t ret;

	ret = genphy_update_link(phydev);
	if (ret)
		return ret;

	ret = genphy_parse_link(phydev);
	if (ret)
		return ret;

	return RT_EOK;
}

rt_err_t phy_shutdown(struct phy_device *phydev)
{
	if (phydev->link_monitor_thread) {
		rt_thread_delete(phydev->link_monitor_thread);
		phydev->monitor_running = false;
	}

	if (phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, BMCR_PDOWN) < 0) {
		rt_kprintf("Unable to put PHY into power-down mode\n");
		return -1;
	}

	if (!phydev)
		rt_free(phydev);
	/* Dummy: do nothing */
	return RT_EOK;
}

/**
 * mii_bmcr_encode_fixed - encode fixed speed/duplex settings to a BMCR value
 * @speed: a SPEED_* value
 * @duplex: a DUPLEX_* value
 *
 * Encode the speed and duplex to a BMCR value. 2500, 1000, 100 and 10 Mbps are
 * supported. 2500Mbps is encoded to 1000Mbps. Other speeds are encoded as 10
 * Mbps. Unknown duplex values are encoded to half-duplex.
 */
static inline rt_uint16_t mii_bmcr_encode_fixed(int speed, int duplex)
{
	rt_uint16_t bmcr;

	switch (speed) {
	case SPEED_2500:
	case SPEED_1000:
		bmcr = BMCR_SPEED1000;
		break;

	case SPEED_100:
		bmcr = BMCR_SPEED100;
		break;

	case SPEED_10:
	default:
		bmcr = BMCR_SPEED10;
		break;
	}

	if (duplex == DUPLEX_FULL)
		bmcr |= BMCR_FULLDPLX;

	return bmcr;
}

typedef rt_bool_t (*phy_cond_cb_t)(rt_int32_t val);

rt_int32_t phy_read_poll_timeout(struct phy_device *phydev, rt_uint32_t reg,
			rt_uint16_t *val, phy_cond_cb_t cond, rt_int32_t delay_ms,
			rt_int32_t timeout_ms, rt_bool_t busy_wait)
{
	rt_int32_t ret = -RT_ETIMEOUT;
	rt_tick_t start = rt_tick_get();
	rt_tick_t timeout_ticks = timeout_ms > 0 ? rt_tick_from_millisecond(timeout_ms) : 0;

	for (;;) {
		ret = phy_read(phydev, MDIO_DEVAD_NONE, reg);
		if (ret < 0)
			break;

		if (val)
			*val = (rt_uint16_t)ret;

		if (cond && cond(*val)) {
			ret = RT_EOK;
			break;
		}

		if (rt_tick_get() - start >= timeout_ticks) {
			ret = -RT_ETIMEOUT;
			break;
		}

		if (!busy_wait && delay_ms > 0)
			rt_thread_mdelay(delay_ms);
	}

	if (ret != RT_EOK)
		rt_kprintf("%s failed: %d\n", __func__, ret);

	return ret;
}

static rt_bool_t link_up(rt_int32_t v)
{
	return v & BMSR_LSTATUS;
}

rt_int32_t genphy_loopback(struct phy_device *phydev, rt_bool_t enable)
{
	rt_uint16_t ctl;
	rt_int32_t ret, val;

	if (enable) {
		/* Set loopback mode */
		ctl = BMCR_LOOPBACK;

		/* Set fixed speed and duplex */
		ctl |= mii_bmcr_encode_fixed(phydev->speed, phydev->duplex);

		/* Write to BMCR to enable loopback mode */
		ret = phy_modify(phydev, MDIO_DEVAD_NONE, MII_BMCR, ~0, ctl);
		if (ret < 0) {
			rt_kprintf("Failed to enable PHY loopback mode\n");
			return ret;
		}

		/* Poll BMSR to wait for link status */
		ret = phy_read_poll_timeout(phydev, MII_BMSR, &ctl, link_up, 5, 500, RT_TRUE);
		if (ret) {
			rt_kprintf("Failed to detect link status in PHY loopback mode\n");
			return ret;
		}
	} else {
		/* Clear loopback mode bit */
		ret = phy_modify(phydev, MDIO_DEVAD_NONE, MII_BMCR, BMCR_LOOPBACK, 0);
		if (ret < 0) {
			rt_kprintf("Failed to disable PHY loopback mode\n");
			return ret;
		}

		/* Re-enable autonegotiation */
		ret = genphy_config_aneg(phydev);
		if (ret < 0) {
			rt_kprintf("Failed to re-enable autonegotiation\n");
			return ret;
		}
	}

	return RT_EOK;
}

rt_err_t phy_loopback(struct phy_device *phydev, rt_bool_t enable)
{
	rt_err_t ret;

	if (phydev->loopback_enabled == enable)
		return RT_EOK;

	ret = genphy_loopback(phydev, enable);
	if (ret)
		return -RT_ERROR;

	phydev->loopback_enabled = enable;

	return RT_EOK;
}

rt_err_t phy_set_link_notify(struct phy_device *phydev,
			     rt_err_t (*notify)(void *netdev))
{
	rt_err_t ret;

	if (!phydev)
		return -RT_EINVAL;

	phydev->link_change_cb = notify;

	phydev->link_monitor_thread = rt_thread_create(
		"phy_monitor", phy_monitor_thread, phydev, 4096, 20, 10);

	if (!phydev->link_monitor_thread) {
		rt_kprintf("Failed to create link monitor thread\n");
		return -RT_ERROR;
	}

	ret = rt_thread_startup(phydev->link_monitor_thread);
	if (ret < 0) {
		rt_kprintf("Failed to startup link monitor thread\n");
		return ret;
	}

	rt_kprintf("Success to startup link monitor thread\n");
	return RT_EOK;
}
