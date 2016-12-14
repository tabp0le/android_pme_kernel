/*
 * Copyright (c) 2014-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/clk/msm-clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/usb/phy.h>
#include <linux/usb/msm_hsusb.h>

#define PHY_CLK_SCHEME_SEL BIT(0)

#define QUSB2PHY_PLL_PWR_CTL		0x18
#define REF_BUF_EN			BIT(0)
#define REXT_EN				BIT(1)
#define PLL_BYPASSNL			BIT(2)
#define REXT_TRIM_0			BIT(4)

#define QUSB2PHY_PLL_AUTOPGM_CTL1	0x1C
#define PLL_RESET_N_CNT_5		0x5
#define PLL_RESET_N			BIT(4)
#define PLL_AUTOPGM_EN			BIT(7)

#define QUSB2PHY_PORT_QUICKCHARGE1	0x70
#define IDP_SRC_EN			BIT(3)

#define QUSB2PHY_PORT_QUICKCHARGE2	0x74
#define QUSB2PHY_PORT_INT_STATUS	0xF0

#define QUSB2PHY_PLL_STATUS	0x38
#define QUSB2PHY_PLL_LOCK	BIT(5)

#define QUSB2PHY_PORT_QC1	0x70
#define VDM_SRC_EN		BIT(4)
#define VDP_SRC_EN		BIT(2)

#define QUSB2PHY_PORT_QC2	0x74
#define RDM_UP_EN		BIT(1)
#define RDP_UP_EN		BIT(3)
#define RPUM_LOW_EN		BIT(4)
#define RPUP_LOW_EN		BIT(5)

#define QUSB2PHY_PORT_POWERDOWN		0xB4
#define CLAMP_N_EN			BIT(5)
#define FREEZIO_N			BIT(1)
#define POWER_DOWN			BIT(0)

#define QUSB2PHY_PORT_UTMI_CTRL1	0xC0
#define SUSPEND_N			BIT(5)
#define TERM_SELECT			BIT(4)
#define XCVR_SELECT_FS			BIT(2)
#define OP_MODE_NON_DRIVE		BIT(0)

#define QUSB2PHY_PORT_UTMI_CTRL2	0xC4
#define UTMI_ULPI_SEL			BIT(7)
#define UTMI_TEST_MUX_SEL		BIT(6)

#define QUSB2PHY_PLL_TEST		0x04
#define CLK_REF_SEL			BIT(7)

#define QUSB2PHY_PORT_TUNE1             0x80
#define QUSB2PHY_PORT_TUNE2             0x84
#define QUSB2PHY_PORT_TUNE3             0x88
#define QUSB2PHY_PORT_TUNE4             0x8C

#define TUNE2_DEFAULT_HIGH_NIBBLE	0xB
#define TUNE2_DEFAULT_LOW_NIBBLE	0x3

#define TUNE2_HIGH_NIBBLE_VAL(val, pos, mask)	((val >> pos) & mask)

#define QUSB2PHY_PORT_INTR_CTRL         0xBC
#define CHG_DET_INTR_EN                 BIT(4)
#define DMSE_INTR_HIGH_SEL              BIT(3)
#define DMSE_INTR_EN                    BIT(2)
#define DPSE_INTR_HIGH_SEL              BIT(1)
#define DPSE_INTR_EN                    BIT(0)

#define QUSB2PHY_PORT_UTMI_STATUS	0xF4
#define LINESTATE_DP			BIT(0)
#define LINESTATE_DM			BIT(1)

#define HS_PHY_CTRL_REG			0x10
#define UTMI_OTG_VBUS_VALID             BIT(20)
#define SW_SESSVLD_SEL                  BIT(28)

#define QUSB2PHY_1P8_VOL_MIN           1800000 
#define QUSB2PHY_1P8_VOL_MAX           1800000 
#define QUSB2PHY_1P8_HPM_LOAD          30000   

#define QUSB2PHY_3P3_VOL_MIN		3075000 
#define QUSB2PHY_3P3_VOL_MAX		3200000 
#define QUSB2PHY_3P3_HPM_LOAD		30000	

#define QUSB2PHY_REFCLK_ENABLE		BIT(0)

unsigned int tune1;
module_param(tune1, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tune1, "QUSB PHY TUNE1");

unsigned int tune2;
module_param(tune2, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tune2, "QUSB PHY TUNE2");

unsigned int tune3;
module_param(tune3, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tune3, "QUSB PHY TUNE3");

unsigned int tune4;
module_param(tune4, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tune4, "QUSB PHY TUNE4");

struct qusb_phy {
	struct usb_phy		phy;
	void __iomem		*base;
	void __iomem		*qscratch_base;
	void __iomem		*tune2_efuse_reg;
	void __iomem		*ref_clk_base;
	void __iomem		*tcsr_phy_clk_scheme_sel;

	struct clk		*ref_clk_src;
	struct clk		*ref_clk;
	struct clk		*cfg_ahb_clk;
	struct clk		*phy_reset;

	struct regulator	*vdd;
	struct regulator	*vdda33;
	struct regulator	*vdda18;
	int			vdd_levels[3]; 
	int			init_seq_len;
	int			*qusb_phy_init_seq;

	u32			tune2_val;
	int			tune2_efuse_bit_pos;
	int			tune2_efuse_num_of_bits;

	bool			power_enabled;
	bool			clocks_enabled;
	bool			cable_connected;
	bool			suspended;
	bool			ulpi_mode;
	bool			rm_pulldown;
	bool			dpdm_pulsing_enabled;

	
	void __iomem		*emu_phy_base;
	bool			emulation;
	int			*emu_init_seq;
	int			emu_init_seq_len;
	int			*phy_pll_reset_seq;
	int			phy_pll_reset_seq_len;
	int			*emu_dcm_reset_seq;
	int			emu_dcm_reset_seq_len;
	spinlock_t		pulse_lock;
};

static void qusb_phy_enable_clocks(struct qusb_phy *qphy, bool on)
{
	dev_dbg(qphy->phy.dev, "%s(): clocks_enabled:%d on:%d\n",
			__func__, qphy->clocks_enabled, on);

	if (!qphy->clocks_enabled && on) {
		clk_prepare_enable(qphy->ref_clk_src);
		clk_prepare_enable(qphy->ref_clk);
		clk_prepare_enable(qphy->cfg_ahb_clk);
		qphy->clocks_enabled = true;
	}

	if (qphy->clocks_enabled && !on) {
		clk_disable_unprepare(qphy->ref_clk);
		clk_disable_unprepare(qphy->ref_clk_src);
		clk_disable_unprepare(qphy->cfg_ahb_clk);
		qphy->clocks_enabled = false;
	}

	dev_dbg(qphy->phy.dev, "%s(): clocks_enabled:%d\n", __func__,
						qphy->clocks_enabled);
}

static int qusb_phy_config_vdd(struct qusb_phy *qphy, int high)
{
	int min, ret;

	min = high ? 1 : 0; 
	ret = regulator_set_voltage(qphy->vdd, qphy->vdd_levels[min],
						qphy->vdd_levels[2]);
	if (ret) {
		dev_err(qphy->phy.dev, "unable to set voltage for qusb vdd\n");
		return ret;
	}

	dev_dbg(qphy->phy.dev, "min_vol:%d max_vol:%d\n",
			qphy->vdd_levels[min], qphy->vdd_levels[2]);
	return ret;
}

static int qusb_phy_enable_power(struct qusb_phy *qphy, bool on,
						bool toggle_vdd)
{
	int ret = 0;
	static bool L24_keep = false; 

	dev_dbg(qphy->phy.dev, "%s turn %s regulators. power_enabled:%d\n",
			__func__, on ? "on" : "off", qphy->power_enabled);

	if (toggle_vdd && qphy->power_enabled == on) {
		dev_dbg(qphy->phy.dev, "PHYs' regulators are already ON.\n");
		return 0;
	}

	if (!on)
		goto disable_vdda33;

	if (toggle_vdd) {
		ret = qusb_phy_config_vdd(qphy, true);
		if (ret) {
			dev_err(qphy->phy.dev, "Unable to config VDD:%d\n",
								ret);
			goto err_vdd;
		}

		ret = regulator_enable(qphy->vdd);
		if (ret) {
			dev_err(qphy->phy.dev, "Unable to enable VDD\n");
			goto unconfig_vdd;
		}
	}

	ret = regulator_set_optimum_mode(qphy->vdda18, QUSB2PHY_1P8_HPM_LOAD);
	if (ret < 0) {
		dev_err(qphy->phy.dev, "Unable to set HPM of vdda18:%d\n", ret);
		goto disable_vdd;
	}

	ret = regulator_set_voltage(qphy->vdda18, QUSB2PHY_1P8_VOL_MIN,
						QUSB2PHY_1P8_VOL_MAX);
	if (ret) {
		dev_err(qphy->phy.dev,
				"Unable to set voltage for vdda18:%d\n", ret);
		goto put_vdda18_lpm;
	}

	if (!L24_keep) {
		ret = regulator_enable(qphy->vdda18);
		if (ret) {
			dev_err(qphy->phy.dev, "Unable to enable vdda18:%d\n", ret);
			goto unset_vdda18;
		}
	}

	ret = regulator_set_optimum_mode(qphy->vdda33, QUSB2PHY_3P3_HPM_LOAD);
	if (ret < 0) {
		dev_err(qphy->phy.dev, "Unable to set HPM of vdda33:%d\n", ret);
		goto disable_vdda18;
	}

	ret = regulator_set_voltage(qphy->vdda33, QUSB2PHY_3P3_VOL_MIN,
						QUSB2PHY_3P3_VOL_MAX);
	if (ret) {
		dev_err(qphy->phy.dev,
				"Unable to set voltage for vdda33:%d\n", ret);
		goto put_vdda33_lpm;
	}
	if (!L24_keep) {
		ret = regulator_enable(qphy->vdda33);
		if (ret) {
			dev_err(qphy->phy.dev, "Unable to enable vdda33:%d\n", ret);
			goto unset_vdd33;
		}
		L24_keep = true;
	}

	if (toggle_vdd)
		qphy->power_enabled = true;

	pr_debug("%s(): QUSB PHY's regulators are turned ON.\n", __func__);
	return ret;

disable_vdda33:

unset_vdd33:

put_vdda33_lpm:
	ret = regulator_set_optimum_mode(qphy->vdda33, 0);
	if (ret < 0)
		dev_err(qphy->phy.dev, "Unable to set (0) HPM of vdda33\n");

disable_vdda18:
unset_vdda18:
put_vdda18_lpm:
	ret = regulator_set_optimum_mode(qphy->vdda18, 0);
	if (ret < 0)
		dev_err(qphy->phy.dev, "Unable to set LPM of vdda18\n");

disable_vdd:
	if (toggle_vdd) {
		ret = regulator_disable(qphy->vdd);
		if (ret)
			dev_err(qphy->phy.dev, "Unable to disable vdd:%d\n",
								ret);

unconfig_vdd:
		ret = qusb_phy_config_vdd(qphy, false);
		if (ret)
			dev_err(qphy->phy.dev, "Unable unconfig VDD:%d\n",
								ret);
	}
err_vdd:
	if (toggle_vdd)
		qphy->power_enabled = false;
	dev_dbg(qphy->phy.dev, "QUSB PHY's regulators are turned OFF.\n");
	return ret;
}

#define PHY_PULSE_TIME_USEC		250
static int qusb_phy_update_dpdm(struct usb_phy *phy, int value)
{
	struct qusb_phy *qphy = container_of(phy, struct qusb_phy, phy);
	unsigned long flags;
	int ret = 0;
	u32 reg;

	dev_dbg(phy->dev, "%s value:%d rm_pulldown:%d pulsing enabled %d\n",
			__func__, value, qphy->rm_pulldown,
			qphy->dpdm_pulsing_enabled);

	switch (value) {
	case POWER_SUPPLY_DP_DM_DPF_DMF:
		dev_dbg(phy->dev, "POWER_SUPPLY_DP_DM_DPF_DMF\n");
		if (!qphy->rm_pulldown) {
			ret = qusb_phy_enable_power(qphy, true, false);
			if (ret >= 0) {
				qphy->rm_pulldown = true;
				dev_dbg(phy->dev, "DP_DM_F: rm_pulldown:%d\n",
						qphy->rm_pulldown);
			}
		}

		
		if (qphy->dpdm_pulsing_enabled && qphy->rm_pulldown) {
			dev_dbg(phy->dev, "clearing qc1 and qc2 registers.\n");
			ret = clk_prepare_enable(qphy->cfg_ahb_clk);
			if (ret)
				goto clk_error;

			
			writel_relaxed(0x00, qphy->base + QUSB2PHY_PORT_QC1);
			writel_relaxed(0x00, qphy->base + QUSB2PHY_PORT_QC2);
			
			mb();
			clk_disable_unprepare(qphy->cfg_ahb_clk);
		}
		break;

	case POWER_SUPPLY_DP_DM_DPR_DMR:
		dev_dbg(phy->dev, "POWER_SUPPLY_DP_DM_DPR_DMR\n");
		if (qphy->rm_pulldown) {
			dev_dbg(phy->dev, "clearing qc1 and qc2 registers.\n");
			if (qphy->dpdm_pulsing_enabled) {
				ret = clk_prepare_enable(qphy->cfg_ahb_clk);
				if (ret)
					goto clk_error;

				
				writel_relaxed(0x00,
						qphy->base + QUSB2PHY_PORT_QC1);
				writel_relaxed(0x00,
						qphy->base + QUSB2PHY_PORT_QC2);
				
				mb();
				clk_disable_unprepare(qphy->cfg_ahb_clk);
			}

			ret = qusb_phy_enable_power(qphy, false, false);
			if (ret >= 0) {
				qphy->rm_pulldown = false;
				dev_dbg(phy->dev, "DP_DM_R: rm_pulldown:%d\n",
						qphy->rm_pulldown);
			}
		}
		break;

	case POWER_SUPPLY_DP_DM_DP0P6_DMF:
		if (!qphy->dpdm_pulsing_enabled)
			break;

		dev_dbg(phy->dev, "POWER_SUPPLY_DP_DM_DP0P6_DMF\n");
		ret = clk_prepare_enable(qphy->cfg_ahb_clk);
		if (ret)
			goto clk_error;

		
		writel_relaxed(VDP_SRC_EN, qphy->base + QUSB2PHY_PORT_QC1);
		
		mb();
		clk_disable_unprepare(qphy->cfg_ahb_clk);
		break;

	case POWER_SUPPLY_DP_DM_DP0P6_DM3P3:
		if (!qphy->dpdm_pulsing_enabled)
			break;

		dev_dbg(phy->dev, "POWER_SUPPLY_DP_DM_DP0PHVDCP_36_DM3P3\n");
		ret = clk_prepare_enable(qphy->cfg_ahb_clk);
		if (ret)
			goto clk_error;

		
		writel_relaxed(VDP_SRC_EN, qphy->base + QUSB2PHY_PORT_QC1);
		
		writel_relaxed(RPUM_LOW_EN | RDM_UP_EN,
				qphy->base + QUSB2PHY_PORT_QC2);
		
		mb();
		clk_disable_unprepare(qphy->cfg_ahb_clk);
		break;

	case POWER_SUPPLY_DP_DM_DP_PULSE:
		if (!qphy->dpdm_pulsing_enabled)
			break;

		dev_dbg(phy->dev, "POWER_SUPPLY_DP_DM_DP_PULSE\n");
		ret = clk_prepare_enable(qphy->cfg_ahb_clk);
		if (ret)
			goto clk_error;

		spin_lock_irqsave(&qphy->pulse_lock, flags);
		
		reg = readl_relaxed(qphy->base + QUSB2PHY_PORT_QC2);
		reg |= (RDP_UP_EN | RPUP_LOW_EN);
		writel_relaxed(reg, qphy->base + QUSB2PHY_PORT_QC2);

		
		mb();

		udelay(PHY_PULSE_TIME_USEC);

		 
		reg = readl_relaxed(qphy->base + QUSB2PHY_PORT_QC1);
		reg |= VDP_SRC_EN;
		writel_relaxed(reg, qphy->base + QUSB2PHY_PORT_QC1);

		reg = readl_relaxed(qphy->base + QUSB2PHY_PORT_QC2);
		reg &= ~(RDP_UP_EN | RPUP_LOW_EN);
		writel_relaxed(reg, qphy->base + QUSB2PHY_PORT_QC2);
		
		mb();
		spin_unlock_irqrestore(&qphy->pulse_lock, flags);
		usleep_range(2000, 3000);
		clk_disable_unprepare(qphy->cfg_ahb_clk);
		break;

	case POWER_SUPPLY_DP_DM_DM_PULSE:
		if (!qphy->dpdm_pulsing_enabled)
			break;

		dev_dbg(phy->dev, "POWER_SUPPLY_DP_DM_DM_PULSE\n");
		ret = clk_prepare_enable(qphy->cfg_ahb_clk);
		if (ret)
			goto clk_error;

		spin_lock_irqsave(&qphy->pulse_lock, flags);
		
		reg = readl_relaxed(qphy->base + QUSB2PHY_PORT_QC1);
		reg |= VDM_SRC_EN;
		writel_relaxed(reg, qphy->base + QUSB2PHY_PORT_QC1);

		reg = readl_relaxed(qphy->base + QUSB2PHY_PORT_QC2);
		reg &= ~(RDM_UP_EN | RPUM_LOW_EN);
		writel_relaxed(reg, qphy->base + QUSB2PHY_PORT_QC2);

		
		mb();

		udelay(PHY_PULSE_TIME_USEC);

		
		reg = readl_relaxed(qphy->base + QUSB2PHY_PORT_QC2);
		reg |= (RPUM_LOW_EN | RDM_UP_EN);
		writel_relaxed(reg, qphy->base + QUSB2PHY_PORT_QC2);

		reg = readl_relaxed(qphy->base + QUSB2PHY_PORT_QC1);
		reg &= ~VDM_SRC_EN;
		writel_relaxed(reg, qphy->base + QUSB2PHY_PORT_QC1);

		
		mb();
		spin_unlock_irqrestore(&qphy->pulse_lock, flags);

		usleep_range(2000, 3000);
		clk_disable_unprepare(qphy->cfg_ahb_clk);
		break;
	default:
		ret = -EINVAL;
		dev_err(phy->dev, "Invalid power supply property(%d)\n", value);
		break;
	}

clk_error:
	return ret;
}

static void __maybe_unused qusb_phy_get_tune2_param(struct qusb_phy *qphy)
{
	u8 num_of_bits;
	u32 bit_mask = 1;

	pr_debug("%s(): num_of_bit s:%d bit_pos:%d\n", __func__,
				qphy->tune2_efuse_num_of_bits,
				qphy->tune2_efuse_bit_pos);

	
	if (qphy->tune2_efuse_num_of_bits) {
		num_of_bits = qphy->tune2_efuse_num_of_bits;
		bit_mask = (bit_mask << num_of_bits) - 1;
	}

	qphy->tune2_val = readl_relaxed(qphy->tune2_efuse_reg);
	pr_debug("%s(): bit_mask:%d efuse based tune2 value:%d\n",
				__func__, bit_mask, qphy->tune2_val);

	qphy->tune2_val = TUNE2_HIGH_NIBBLE_VAL(qphy->tune2_val,
				qphy->tune2_efuse_bit_pos, bit_mask);

	if (!qphy->tune2_val)
		qphy->tune2_val = TUNE2_DEFAULT_HIGH_NIBBLE;

	
	qphy->tune2_val = ((qphy->tune2_val << 0x4) |
					TUNE2_DEFAULT_LOW_NIBBLE);
}

static void qusb_phy_write_seq(void __iomem *base, u32 *seq, int cnt,
		unsigned long delay)
{
	int i;

	pr_debug("Seq count:%d\n", cnt);
	for (i = 0; i < cnt; i = i+2) {
		pr_debug("write 0x%02x to 0x%02x\n", seq[i], seq[i+1]);
		writel_relaxed(seq[i], base + seq[i+1]);
		if (delay)
			usleep_range(delay, (delay + 2000));
	}
}

static int qusb_phy_init(struct usb_phy *phy)
{
	struct qusb_phy *qphy = container_of(phy, struct qusb_phy, phy);
	int ret, reset_val = 0;
	bool is_se_clk = true;

	dev_dbg(phy->dev, "%s\n", __func__);

	ret = qusb_phy_enable_power(qphy, true, true);
	if (ret)
		return ret;

	qusb_phy_enable_clocks(qphy, true);

	if (qphy->ref_clk_base) {
		writel_relaxed((readl_relaxed(qphy->ref_clk_base) &
					~QUSB2PHY_REFCLK_ENABLE),
					qphy->ref_clk_base);
		
		wmb();
	}

	
	clk_reset(qphy->phy_reset, CLK_RESET_ASSERT);
	usleep_range(100, 150);
	clk_reset(qphy->phy_reset, CLK_RESET_DEASSERT);

	if (qphy->emulation) {
		if (qphy->emu_init_seq)
			qusb_phy_write_seq(qphy->emu_phy_base,
				qphy->emu_init_seq, qphy->emu_init_seq_len, 0);

		if (qphy->qusb_phy_init_seq)
			qusb_phy_write_seq(qphy->base, qphy->qusb_phy_init_seq,
					qphy->init_seq_len, 0);

		
		usleep_range(5000, 7000);

		if (qphy->phy_pll_reset_seq)
			qusb_phy_write_seq(qphy->base, qphy->phy_pll_reset_seq,
					qphy->phy_pll_reset_seq_len, 10000);

		if (qphy->emu_dcm_reset_seq)
			qusb_phy_write_seq(qphy->emu_phy_base,
					qphy->emu_dcm_reset_seq,
					qphy->emu_dcm_reset_seq_len, 10000);

		return 0;
	}

	
	writel_relaxed(CLAMP_N_EN | FREEZIO_N | POWER_DOWN,
			qphy->base + QUSB2PHY_PORT_POWERDOWN);

	
	if (qphy->ulpi_mode)
		writel_relaxed(0x0, qphy->base + QUSB2PHY_PORT_UTMI_CTRL2);

	
	reset_val = readl_relaxed(qphy->base + QUSB2PHY_PLL_TEST);

	if (qphy->qusb_phy_init_seq)
		qusb_phy_write_seq(qphy->base, qphy->qusb_phy_init_seq,
				qphy->init_seq_len, 0);


	
	if (tune1) {
		pr_debug("%s(): (modparam) TUNE1 val:0x%02x\n",
						__func__, tune1);
		writel_relaxed(tune1,
				qphy->base + QUSB2PHY_PORT_TUNE1);
	}

	
	if (tune2) {
		pr_debug("%s(): (modparam) TUNE2 val:0x%02x\n",
						__func__, tune2);
		writel_relaxed(tune2,
				qphy->base + QUSB2PHY_PORT_TUNE2);
	}

	
	if (tune3) {
		pr_debug("%s(): (modparam) TUNE3:0x%02x\n",
						__func__, tune3);
		writel_relaxed(tune3,
				qphy->base + QUSB2PHY_PORT_TUNE3);
	}

	
	if (tune4) {
		pr_debug("%s(): (modparam) TUNE4:0x%02x\n",
						__func__, tune4);
		writel_relaxed(tune4,
				qphy->base + QUSB2PHY_PORT_TUNE4);
	}

	
	wmb();

	
	writel_relaxed(CLAMP_N_EN | FREEZIO_N,
		qphy->base + QUSB2PHY_PORT_POWERDOWN);

	
	wmb();

	
	usleep_range(150, 160);

	if (qphy->tcsr_phy_clk_scheme_sel) {
		ret = readl_relaxed(qphy->tcsr_phy_clk_scheme_sel);
		if (ret & PHY_CLK_SCHEME_SEL) {
			pr_debug("%s:select single-ended clk src\n",
				__func__);
			is_se_clk = true;
		} else {
			pr_debug("%s:select differential clk src\n",
				__func__);
			is_se_clk = false;
		}
	}

	if (!is_se_clk)
		reset_val &= ~CLK_REF_SEL;
	else
		reset_val |= CLK_REF_SEL;

	
	if (!is_se_clk && qphy->ref_clk_base)
		writel_relaxed((readl_relaxed(qphy->ref_clk_base) |
					QUSB2PHY_REFCLK_ENABLE),
					qphy->ref_clk_base);
	else
		writel_relaxed(reset_val, qphy->base + QUSB2PHY_PLL_TEST);

	
	wmb();

	
	usleep_range(100, 110);

	if (!(readb_relaxed(qphy->base + QUSB2PHY_PLL_STATUS) &
					QUSB2PHY_PLL_LOCK)) {
		dev_err(phy->dev, "QUSB PHY PLL LOCK fails:%x\n",
			readb_relaxed(qphy->base + QUSB2PHY_PLL_STATUS));
		WARN_ON(1);
	}

	return 0;
}

static void qusb_phy_shutdown(struct usb_phy *phy)
{
	struct qusb_phy *qphy = container_of(phy, struct qusb_phy, phy);

	dev_dbg(phy->dev, "%s\n", __func__);

	qusb_phy_enable_clocks(qphy, true);

	
	writel_relaxed(CLAMP_N_EN | FREEZIO_N | POWER_DOWN,
			qphy->base + QUSB2PHY_PORT_POWERDOWN);
	wmb();

	qusb_phy_enable_clocks(qphy, false);
}

static int qusb_phy_linestate_with_idp_src(struct usb_phy *phy)
{
	struct qusb_phy *qphy = container_of(phy, struct qusb_phy, phy);
	u8 int_status, ret;

	
	writel_relaxed(CLAMP_N_EN | FREEZIO_N | POWER_DOWN,
			qphy->base + QUSB2PHY_PORT_POWERDOWN);

	
	writel_relaxed(TERM_SELECT | XCVR_SELECT_FS | OP_MODE_NON_DRIVE |
			SUSPEND_N, qphy->base + QUSB2PHY_PORT_UTMI_CTRL1);

	
	writel_relaxed(UTMI_ULPI_SEL | UTMI_TEST_MUX_SEL,
			qphy->base + QUSB2PHY_PORT_UTMI_CTRL2);

	writel_relaxed(PLL_RESET_N_CNT_5,
			qphy->base + QUSB2PHY_PLL_AUTOPGM_CTL1);

	
	writel_relaxed(CLAMP_N_EN | FREEZIO_N,
			qphy->base + QUSB2PHY_PORT_POWERDOWN);

	writel_relaxed(REF_BUF_EN | REXT_EN | PLL_BYPASSNL | REXT_TRIM_0,
			qphy->base + QUSB2PHY_PLL_PWR_CTL);

	usleep_range(5, 1000);

	writel_relaxed(PLL_RESET_N | PLL_RESET_N_CNT_5,
			qphy->base + QUSB2PHY_PLL_AUTOPGM_CTL1);
	usleep_range(50, 1000);

	writel_relaxed(0x00, qphy->base + QUSB2PHY_PORT_QUICKCHARGE1);
	writel_relaxed(0x00, qphy->base + QUSB2PHY_PORT_QUICKCHARGE2);

	
	writel_relaxed(0x1F, qphy->base + QUSB2PHY_PORT_INTR_CTRL);
	
	writel_relaxed(IDP_SRC_EN, qphy->base + QUSB2PHY_PORT_QUICKCHARGE1);

	usleep_range(1000, 2000);
	int_status = readl_relaxed(qphy->base + QUSB2PHY_PORT_INT_STATUS);

	
	writel_relaxed(CLAMP_N_EN | FREEZIO_N | POWER_DOWN,
			qphy->base + QUSB2PHY_PORT_POWERDOWN);  

	writel_relaxed(PLL_AUTOPGM_EN | PLL_RESET_N | PLL_RESET_N_CNT_5,
			qphy->base + QUSB2PHY_PLL_AUTOPGM_CTL1);

	writel_relaxed(UTMI_ULPI_SEL, qphy->base + QUSB2PHY_PORT_UTMI_CTRL2);

	writel_relaxed(TERM_SELECT, qphy->base + QUSB2PHY_PORT_UTMI_CTRL1);

	writel_relaxed(CLAMP_N_EN | FREEZIO_N,
			qphy->base + QUSB2PHY_PORT_POWERDOWN);

	int_status = int_status & 0x5;

	ret = (int_status >> 2) | ((int_status & 0x1) << 1);
	pr_debug("%s: int_status:%x, dpdm:%x\n", __func__, int_status, ret);

	return ret;
}

static int qusb_phy_set_suspend(struct usb_phy *phy, int suspend)
{
	struct qusb_phy *qphy = container_of(phy, struct qusb_phy, phy);
	u32 linestate = 0, intr_mask = 0;

	if (qphy->suspended && suspend) {
		dev_dbg(phy->dev, "%s: USB PHY is already suspended\n",
			__func__);
		return 0;
	}

	if (suspend) {
		
		if (qphy->cable_connected ||
			(qphy->phy.flags & PHY_HOST_MODE)) {
			
			writel_relaxed(0x00,
				qphy->base + QUSB2PHY_PORT_INTR_CTRL);

			linestate = readl_relaxed(qphy->base +
					QUSB2PHY_PORT_UTMI_STATUS);

			intr_mask = DPSE_INTR_EN | DMSE_INTR_EN;
			if (!(linestate & LINESTATE_DP)) 
				intr_mask |= DPSE_INTR_HIGH_SEL;
			if (!(linestate & LINESTATE_DM)) 
				intr_mask |= DMSE_INTR_HIGH_SEL;

			writel_relaxed(intr_mask,
				qphy->base + QUSB2PHY_PORT_INTR_CTRL);

			qusb_phy_enable_clocks(qphy, false);
		} else { 
			
			writel_relaxed(0x00,
				qphy->base + QUSB2PHY_PORT_INTR_CTRL);
			writel_relaxed(TERM_SELECT | XCVR_SELECT_FS |
				OP_MODE_NON_DRIVE,
				qphy->base + QUSB2PHY_PORT_UTMI_CTRL1);
			writel_relaxed(UTMI_ULPI_SEL | UTMI_TEST_MUX_SEL,
				qphy->base + QUSB2PHY_PORT_UTMI_CTRL2);


			qusb_phy_enable_clocks(qphy, false);
			
		}
		qphy->suspended = true;
	} else {
		
		if (qphy->cable_connected ||
			(qphy->phy.flags & PHY_HOST_MODE)) {
			qusb_phy_enable_clocks(qphy, true);
			
			writel_relaxed(0x00,
				qphy->base + QUSB2PHY_PORT_INTR_CTRL);
		} else {
			qusb_phy_enable_power(qphy, true, true);
			qusb_phy_enable_clocks(qphy, true);
		}
		qphy->suspended = false;
	}

	return 0;
}

static void qusb_write_readback(void *base, u32 offset,
					const u32 mask, u32 val)
{
	u32 write_val, tmp = readl_relaxed(base + offset);
	tmp &= ~mask; 
	write_val = tmp | val;

	writel_relaxed(write_val, base + offset);

	/* Read back to see if val was written */
	tmp = readl_relaxed(base + offset);
	tmp &= mask; 

	if (tmp != val)
		pr_err("%s: write: %x to QSCRATCH: %x FAILED\n",
			__func__, val, offset);
}

static int qusb_phy_notify_connect(struct usb_phy *phy,
					enum usb_device_speed speed)
{
	struct qusb_phy *qphy = container_of(phy, struct qusb_phy, phy);

	qphy->cable_connected = true;

	dev_dbg(phy->dev, " cable_connected=%d\n", qphy->cable_connected);

	
	qusb_write_readback(qphy->qscratch_base, HS_PHY_CTRL_REG,
				UTMI_OTG_VBUS_VALID,
				UTMI_OTG_VBUS_VALID);

	
	qusb_write_readback(qphy->qscratch_base, HS_PHY_CTRL_REG,
				SW_SESSVLD_SEL, SW_SESSVLD_SEL);

	dev_dbg(phy->dev, "QUSB2 phy connect notification\n");
	return 0;
}

static int qusb_phy_notify_disconnect(struct usb_phy *phy,
					enum usb_device_speed speed)
{
	struct qusb_phy *qphy = container_of(phy, struct qusb_phy, phy);

	qphy->cable_connected = false;

	dev_dbg(phy->dev, " cable_connected=%d\n", qphy->cable_connected);

	
	qusb_write_readback(qphy->qscratch_base, HS_PHY_CTRL_REG,
				UTMI_OTG_VBUS_VALID, 0);

	
	qusb_write_readback(qphy->qscratch_base, HS_PHY_CTRL_REG,
				SW_SESSVLD_SEL, 0);

	dev_dbg(phy->dev, "QUSB2 phy disconnect notification\n");
	return 0;
}

static int qusb_phy_probe(struct platform_device *pdev)
{
	struct qusb_phy *qphy;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret = 0, size = 0;
	const char *phy_type;
	bool hold_phy_reset;

	qphy = devm_kzalloc(dev, sizeof(*qphy), GFP_KERNEL);
	if (!qphy)
		return -ENOMEM;

	qphy->phy.dev = dev;
	spin_lock_init(&qphy->pulse_lock);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
							"qusb_phy_base");
	qphy->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(qphy->base))
		return PTR_ERR(qphy->base);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
							"qscratch_base");
	if (res) {
		qphy->qscratch_base = devm_ioremap_resource(dev, res);
		if (IS_ERR(qphy->qscratch_base)) {
			dev_dbg(dev, "couldn't ioremap qscratch_base\n");
			qphy->qscratch_base = NULL;
		}
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
							"emu_phy_base");
	if (res) {
		qphy->emu_phy_base = devm_ioremap_resource(dev, res);
		if (IS_ERR(qphy->emu_phy_base)) {
			dev_dbg(dev, "couldn't ioremap emu_phy_base\n");
			qphy->emu_phy_base = NULL;
		}
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
							"tune2_efuse_addr");
	if (res) {
		qphy->tune2_efuse_reg = devm_ioremap_nocache(dev, res->start,
							resource_size(res));
		if (!IS_ERR_OR_NULL(qphy->tune2_efuse_reg)) {
			ret = of_property_read_u32(dev->of_node,
					"qcom,tune2-efuse-bit-pos",
					&qphy->tune2_efuse_bit_pos);
			if (!ret) {
				ret = of_property_read_u32(dev->of_node,
						"qcom,tune2-efuse-num-bits",
						&qphy->tune2_efuse_num_of_bits);
			}

			if (ret) {
				dev_err(dev, "DT Value for tune2 efuse is invalid.\n");
				return -EINVAL;
			}
		}
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
							"ref_clk_addr");
	if (res) {
		qphy->ref_clk_base = devm_ioremap_nocache(dev,
				res->start, resource_size(res));
		if (IS_ERR(qphy->ref_clk_base))
			dev_dbg(dev, "ref_clk_address is not available.\n");
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
			"tcsr_phy_clk_scheme_sel");
	if (res) {
		qphy->tcsr_phy_clk_scheme_sel = devm_ioremap_nocache(dev,
				res->start, resource_size(res));
		if (IS_ERR(qphy->tcsr_phy_clk_scheme_sel))
			dev_dbg(dev, "err reading tcsr_phy_clk_scheme_sel\n");
	}

	qphy->dpdm_pulsing_enabled = of_property_read_bool(dev->of_node,
					"qcom,enable-dpdm-pulsing");

	qphy->ref_clk_src = devm_clk_get(dev, "ref_clk_src");
	if (IS_ERR(qphy->ref_clk_src))
		dev_dbg(dev, "clk get failed for ref_clk_src\n");

	qphy->ref_clk = devm_clk_get(dev, "ref_clk");
	if (IS_ERR(qphy->ref_clk))
		dev_dbg(dev, "clk get failed for ref_clk\n");
	else
		clk_set_rate(qphy->ref_clk, 19200000);

	qphy->cfg_ahb_clk = devm_clk_get(dev, "cfg_ahb_clk");
	if (IS_ERR(qphy->cfg_ahb_clk))
		return PTR_ERR(qphy->cfg_ahb_clk);

	qphy->phy_reset = devm_clk_get(dev, "phy_reset");
	if (IS_ERR(qphy->phy_reset))
		return PTR_ERR(qphy->phy_reset);

	qphy->emulation = of_property_read_bool(dev->of_node,
					"qcom,emulation");

	of_get_property(dev->of_node, "qcom,emu-init-seq", &size);
	if (size) {
		qphy->emu_init_seq = devm_kzalloc(dev,
						size, GFP_KERNEL);
		if (qphy->emu_init_seq) {
			qphy->emu_init_seq_len =
				(size / sizeof(*qphy->emu_init_seq));
			if (qphy->emu_init_seq_len % 2) {
				dev_err(dev, "invalid emu_init_seq_len\n");
				return -EINVAL;
			}

			of_property_read_u32_array(dev->of_node,
				"qcom,qemu-init-seq",
				qphy->emu_init_seq,
				qphy->emu_init_seq_len);
		} else {
			dev_dbg(dev, "error allocating memory for emu_init_seq\n");
		}
	}

	of_get_property(dev->of_node, "qcom,phy-pll-reset-seq", &size);
	if (size) {
		qphy->phy_pll_reset_seq = devm_kzalloc(dev,
						size, GFP_KERNEL);
		if (qphy->phy_pll_reset_seq) {
			qphy->phy_pll_reset_seq_len =
				(size / sizeof(*qphy->phy_pll_reset_seq));
			if (qphy->phy_pll_reset_seq_len % 2) {
				dev_err(dev, "invalid phy_pll_reset_seq_len\n");
				return -EINVAL;
			}

			of_property_read_u32_array(dev->of_node,
				"qcom,phy-pll-reset-seq",
				qphy->phy_pll_reset_seq,
				qphy->phy_pll_reset_seq_len);
		} else {
			dev_dbg(dev, "error allocating memory for phy_pll_reset_seq\n");
		}
	}

	of_get_property(dev->of_node, "qcom,emu-dcm-reset-seq", &size);
	if (size) {
		qphy->emu_dcm_reset_seq = devm_kzalloc(dev,
						size, GFP_KERNEL);
		if (qphy->emu_dcm_reset_seq) {
			qphy->emu_dcm_reset_seq_len =
				(size / sizeof(*qphy->emu_dcm_reset_seq));
			if (qphy->emu_dcm_reset_seq_len % 2) {
				dev_err(dev, "invalid emu_dcm_reset_seq_len\n");
				return -EINVAL;
			}

			of_property_read_u32_array(dev->of_node,
				"qcom,emu-dcm-reset-seq",
				qphy->emu_dcm_reset_seq,
				qphy->emu_dcm_reset_seq_len);
		} else {
			dev_dbg(dev, "error allocating memory for emu_dcm_reset_seq\n");
		}
	}

	of_get_property(dev->of_node, "qcom,qusb-phy-init-seq", &size);
	if (size) {
		qphy->qusb_phy_init_seq = devm_kzalloc(dev,
						size, GFP_KERNEL);
		if (qphy->qusb_phy_init_seq) {
			qphy->init_seq_len =
				(size / sizeof(*qphy->qusb_phy_init_seq));
			if (qphy->init_seq_len % 2) {
				dev_err(dev, "invalid init_seq_len\n");
				return -EINVAL;
			}

			of_property_read_u32_array(dev->of_node,
				"qcom,qusb-phy-init-seq",
				qphy->qusb_phy_init_seq,
				qphy->init_seq_len);
		} else {
			dev_err(dev, "error allocating memory for phy_init_seq\n");
		}
	}

	qphy->ulpi_mode = false;
	ret = of_property_read_string(dev->of_node, "phy_type", &phy_type);

	if (!ret) {
		if (!strcasecmp(phy_type, "ulpi"))
			qphy->ulpi_mode = true;
	} else {
		dev_err(dev, "error reading phy_type property\n");
		return ret;
	}

	hold_phy_reset = of_property_read_bool(dev->of_node, "qcom,hold-reset");
	ret = of_property_read_u32_array(dev->of_node, "qcom,vdd-voltage-level",
					 (u32 *) qphy->vdd_levels,
					 ARRAY_SIZE(qphy->vdd_levels));
	if (ret) {
		dev_err(dev, "error reading qcom,vdd-voltage-level property\n");
		return ret;
	}

	qphy->vdd = devm_regulator_get(dev, "vdd");
	if (IS_ERR(qphy->vdd)) {
		dev_err(dev, "unable to get vdd supply\n");
		return PTR_ERR(qphy->vdd);
	}

	qphy->vdda33 = devm_regulator_get(dev, "vdda33");
	if (IS_ERR(qphy->vdda33)) {
		dev_err(dev, "unable to get vdda33 supply\n");
		return PTR_ERR(qphy->vdda33);
	}

	qphy->vdda18 = devm_regulator_get(dev, "vdda18");
	if (IS_ERR(qphy->vdda18)) {
		dev_err(dev, "unable to get vdda18 supply\n");
		return PTR_ERR(qphy->vdda18);
	}

	platform_set_drvdata(pdev, qphy);

	qphy->phy.label			= "msm-qusb-phy";
	qphy->phy.init			= qusb_phy_init;
	qphy->phy.set_suspend           = qusb_phy_set_suspend;
	qphy->phy.shutdown		= qusb_phy_shutdown;
	qphy->phy.change_dpdm		= qusb_phy_update_dpdm;
	qphy->phy.type			= USB_PHY_TYPE_USB2;
	qphy->phy.dpdm_with_idp_src	= qusb_phy_linestate_with_idp_src;

	if (qphy->qscratch_base) {
		qphy->phy.notify_connect        = qusb_phy_notify_connect;
		qphy->phy.notify_disconnect     = qusb_phy_notify_disconnect;
	}

	if (hold_phy_reset)
		clk_reset(qphy->phy_reset, CLK_RESET_ASSERT);

	ret = usb_add_phy_dev(&qphy->phy);
	return ret;
}

static int qusb_phy_remove(struct platform_device *pdev)
{
	struct qusb_phy *qphy = platform_get_drvdata(pdev);

	usb_remove_phy(&qphy->phy);

	if (qphy->clocks_enabled) {
		clk_disable_unprepare(qphy->cfg_ahb_clk);
		clk_disable_unprepare(qphy->ref_clk);
		clk_disable_unprepare(qphy->ref_clk_src);
		qphy->clocks_enabled = false;
	}

	qusb_phy_enable_power(qphy, false, true);

	return 0;
}

static const struct of_device_id qusb_phy_id_table[] = {
	{ .compatible = "qcom,qusb2phy", },
	{ },
};
MODULE_DEVICE_TABLE(of, qusb_phy_id_table);

static struct platform_driver qusb_phy_driver = {
	.probe		= qusb_phy_probe,
	.remove		= qusb_phy_remove,
	.driver = {
		.name	= "msm-qusb-phy",
		.of_match_table = of_match_ptr(qusb_phy_id_table),
	},
};

module_platform_driver(qusb_phy_driver);

MODULE_DESCRIPTION("MSM QUSB2 PHY driver");
MODULE_LICENSE("GPL v2");
