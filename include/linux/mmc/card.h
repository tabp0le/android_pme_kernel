/*
 *  linux/include/linux/mmc/card.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Card driver specific definitions.
 */
#ifndef LINUX_MMC_CARD_H
#define LINUX_MMC_CARD_H

#include <linux/device.h>
#include <linux/mmc/core.h>
#include <linux/mmc/mmc.h>
#include <linux/mod_devicetable.h>
#include <linux/notifier.h>

#define MMC_CARD_CMDQ_BLK_SIZE 512

struct mmc_cid {
	unsigned int		manfid;
	char			prod_name[8];
	unsigned char		prv;
	unsigned int		serial;
	unsigned short		oemid;
	unsigned short		year;
	unsigned char		hwrev;
	unsigned char		fwrev;
	unsigned char		month;
};

struct mmc_csd {
	unsigned char		structure;
	unsigned char		mmca_vsn;
	unsigned short		cmdclass;
	unsigned short		tacc_clks;
	unsigned int		tacc_ns;
	unsigned int		c_size;
	unsigned int		r2w_factor;
	unsigned int		max_dtr;
	unsigned int		erase_size;		
	unsigned int		read_blkbits;
	unsigned int		write_blkbits;
	unsigned int		capacity;
	unsigned int		read_partial:1,
				read_misalign:1,
				write_partial:1,
				write_misalign:1,
				dsr_imp:1;
};

struct mmc_ext_csd {
	u8			rev;
	u8			erase_group_def;
	u8			sec_feature_support;
	u8			rel_sectors;
	u8			rel_param;
	bool			enhanced_rpmb_supported;
	u8			part_config;
	u8			cache_ctrl;
	u8			rst_n_function;
	u8			max_packed_writes;
	u8			max_packed_reads;
	u8			packed_event_en;
	unsigned int		part_time;		
	unsigned int		sa_timeout;		
	unsigned int		generic_cmd6_time;	
	unsigned int            power_off_longtime;     
	u8			power_off_notification;	
	unsigned int		hs_max_dtr;
	unsigned int		hs200_max_dtr;
#define MMC_HIGH_26_MAX_DTR	26000000
#define MMC_HIGH_52_MAX_DTR	52000000
#define MMC_HIGH_DDR_MAX_DTR	52000000
#define MMC_HS200_MAX_DTR	200000000
	unsigned int		sectors;
	unsigned int		hc_erase_size;		
	unsigned int		hc_erase_timeout;	
	unsigned int		sec_trim_mult;	
	unsigned int		sec_erase_mult;	
	unsigned int		trim_timeout;		
	bool			partition_setting_completed;	
	unsigned long long	enhanced_area_offset;	
	unsigned int		enhanced_area_size;	
	unsigned int		cache_size;		
	bool			hpi_en;			
	bool			hpi;			
	unsigned int		hpi_cmd;		
	bool			bkops;		
	u8			bkops_en;	
	unsigned int            data_sector_size;       
	unsigned int            data_tag_unit_size;     
	unsigned int		boot_ro_lock;		
	bool			boot_ro_lockable;
	u8			raw_ext_csd_cmdq;	
	u8			raw_ext_csd_cache_ctrl;	
	u8			raw_exception_status;	
	u8			raw_partition_support;	
	u8			raw_rpmb_size_mult;	
	u8			raw_erased_mem_count;	
	u8			raw_ext_csd_bus_width;	
	u8			strobe_support;		
#define MMC_STROBE_SUPPORT	(1 << 0)
	u8			raw_ext_csd_hs_timing;	
	u8			raw_ext_csd_structure;	
	u8			raw_card_type;		
	u8			raw_drive_strength;	
	u8			out_of_int_time;	
	u8			raw_pwr_cl_52_195;	
	u8			raw_pwr_cl_26_195;	
	u8			raw_pwr_cl_52_360;	
	u8			raw_pwr_cl_26_360;	
	u8			raw_s_a_timeout;	
	u8			raw_hc_erase_gap_size;	
	u8			raw_erase_timeout_mult;	
	u8			raw_hc_erase_grp_size;	
	u8			raw_sec_trim_mult;	
	u8			raw_sec_erase_mult;	
	u8			raw_sec_feature_support;
	u8			raw_trim_mult;		
	u8			raw_pwr_cl_200_195;	
	u8			raw_pwr_cl_200_360;	
	u8			raw_pwr_cl_ddr_52_195;	
	u8			raw_pwr_cl_ddr_52_360;	
	u8			raw_pwr_cl_ddr_200_360;	
	u8			cache_flush_policy;	
	u8			raw_bkops_status;	
	u8			raw_sectors[4];		
	u8			cmdq_depth;		
	u8			cmdq_support;		
	u8			barrier_support;	
	u8			barrier_en;

	u8			fw_version;		

    bool			ffu_mode_op;	
	unsigned int            feature_support;
#define MMC_DISCARD_FEATURE	BIT(0)                  
};

struct sd_scr {
	unsigned char		sda_vsn;
	unsigned char		sda_spec3;
	unsigned char		bus_widths;
#define SD_SCR_BUS_WIDTH_1	(1<<0)
#define SD_SCR_BUS_WIDTH_4	(1<<2)
	unsigned char		cmds;
#define SD_SCR_CMD20_SUPPORT   (1<<0)
#define SD_SCR_CMD23_SUPPORT   (1<<1)
};

struct sd_ssr {
	unsigned int		au;			
	unsigned int		erase_timeout;		
	unsigned int		erase_offset;		
};

struct sd_switch_caps {
	unsigned int		hs_max_dtr;
	unsigned int		uhs_max_dtr;
#define HIGH_SPEED_MAX_DTR	50000000
#define UHS_SDR104_MAX_DTR	208000000
#define UHS_SDR50_MAX_DTR	100000000
#define UHS_DDR50_MAX_DTR	50000000
#define UHS_SDR25_MAX_DTR	UHS_DDR50_MAX_DTR
#define UHS_SDR12_MAX_DTR	25000000
	unsigned int		sd3_bus_mode;
#define UHS_SDR12_BUS_SPEED	0
#define HIGH_SPEED_BUS_SPEED	1
#define UHS_SDR25_BUS_SPEED	1
#define UHS_SDR50_BUS_SPEED	2
#define UHS_SDR104_BUS_SPEED	3
#define UHS_DDR50_BUS_SPEED	4

#define SD_MODE_HIGH_SPEED	(1 << HIGH_SPEED_BUS_SPEED)
#define SD_MODE_UHS_SDR12	(1 << UHS_SDR12_BUS_SPEED)
#define SD_MODE_UHS_SDR25	(1 << UHS_SDR25_BUS_SPEED)
#define SD_MODE_UHS_SDR50	(1 << UHS_SDR50_BUS_SPEED)
#define SD_MODE_UHS_SDR104	(1 << UHS_SDR104_BUS_SPEED)
#define SD_MODE_UHS_DDR50	(1 << UHS_DDR50_BUS_SPEED)
	unsigned int		sd3_drv_type;
#define SD_DRIVER_TYPE_B	0x01
#define SD_DRIVER_TYPE_A	0x02
#define SD_DRIVER_TYPE_C	0x04
#define SD_DRIVER_TYPE_D	0x08
	unsigned int		sd3_curr_limit;
#define SD_SET_CURRENT_LIMIT_200	0
#define SD_SET_CURRENT_LIMIT_400	1
#define SD_SET_CURRENT_LIMIT_600	2
#define SD_SET_CURRENT_LIMIT_800	3
#define SD_SET_CURRENT_NO_CHANGE	(-1)

#define SD_MAX_CURRENT_200	(1 << SD_SET_CURRENT_LIMIT_200)
#define SD_MAX_CURRENT_400	(1 << SD_SET_CURRENT_LIMIT_400)
#define SD_MAX_CURRENT_600	(1 << SD_SET_CURRENT_LIMIT_600)
#define SD_MAX_CURRENT_800	(1 << SD_SET_CURRENT_LIMIT_800)
};

struct sdio_cccr {
	unsigned int		sdio_vsn;
	unsigned int		sd_vsn;
	unsigned int		multi_block:1,
				low_speed:1,
				wide_bus:1,
				high_power:1,
				high_speed:1,
				disable_cd:1,
				async_intr_sup:1;
};

struct sdio_cis {
	unsigned short		vendor;
	unsigned short		device;
	unsigned short		blksize;
	unsigned int		max_dtr;
};

struct mmc_host;
struct mmc_ios;
struct sdio_func;
struct sdio_func_tuple;

#define SDIO_MAX_FUNCS		7

enum mmc_blk_status {
	MMC_BLK_SUCCESS = 0,
	MMC_BLK_PARTIAL,
	MMC_BLK_CMD_ERR,
	MMC_BLK_RETRY,
	MMC_BLK_ABORT,
	MMC_BLK_DATA_ERR,
	MMC_BLK_ECC_ERR,
	MMC_BLK_NOMEDIUM,
	MMC_BLK_NEW_REQUEST,
};

enum mmc_packed_stop_reasons {
	EXCEEDS_SEGMENTS = 0,
	EXCEEDS_SECTORS,
	WRONG_DATA_DIR,
	FLUSH_OR_DISCARD,
	EMPTY_QUEUE,
	REL_WRITE,
	THRESHOLD,
	LARGE_SEC_ALIGN,
	RANDOM,
	FUA,
	MAX_REASONS,
};

struct mmc_wr_pack_stats {
	u32 *packing_events;
	u32 pack_stop_reason[MAX_REASONS];
	spinlock_t lock;
	bool enabled;
	bool print_in_read;
};

#define MMC_NUM_BOOT_PARTITION	2
#define MMC_NUM_GP_PARTITION	4
#define MMC_NUM_PHY_PARTITION	7
#define MAX_MMC_PART_NAME_LEN	20

struct mmc_part {
	unsigned int	size;	
	unsigned int	part_cfg;	
	char	name[MAX_MMC_PART_NAME_LEN];
	bool	force_ro;	
	unsigned int	area_type;
#define MMC_BLK_DATA_AREA_MAIN	(1<<0)
#define MMC_BLK_DATA_AREA_BOOT	(1<<1)
#define MMC_BLK_DATA_AREA_GP	(1<<2)
#define MMC_BLK_DATA_AREA_RPMB	(1<<3)
};

enum {
	MMC_BKOPS_NO_OP,
	MMC_BKOPS_NOT_CRITICAL,
	MMC_BKOPS_PERF_IMPACT,
	MMC_BKOPS_CRITICAL,
	MMC_BKOPS_NUM_SEVERITY_LEVELS,
};

struct mmc_bkops_stats {
	spinlock_t	lock;
	unsigned int	manual_start;
	unsigned int	hpi;
	unsigned int	auto_start;
	unsigned int	auto_stop;
	unsigned int	level[MMC_BKOPS_NUM_SEVERITY_LEVELS];
	bool		enabled;
};

struct mmc_bkops_info {
	struct mmc_bkops_stats stats;
	bool needs_check;
	bool needs_manual;
};

enum mmc_pon_type {
	MMC_LONG_PON = 1,
	MMC_SHRT_PON,
};

#define MMC_QUIRK_CMDQ_DELAY_BEFORE_DCMD 6 

struct mmc_card {
	struct mmc_host		*host;		
	struct device		dev;		
	u32			ocr;		
	unsigned long		clk_scaling_lowest;	
	unsigned long		clk_scaling_highest;	
	unsigned int		rca;		
	unsigned int		type;		
#define MMC_TYPE_MMC		0		
#define MMC_TYPE_SD		1		
#define MMC_TYPE_SDIO		2		
#define MMC_TYPE_SD_COMBO	3		
#define MMC_TYPE_NA		0xFF		
	unsigned int		state;		
#define MMC_STATE_PRESENT	(1<<0)		
#define MMC_STATE_READONLY	(1<<1)		
#define MMC_STATE_BLOCKADDR	(1<<2)		
#define MMC_CARD_SDXC		(1<<3)		
#define MMC_CARD_REMOVED	(1<<4)		
#define MMC_STATE_DOING_BKOPS	(1<<5)		
#define MMC_STATE_SUSPENDED	(1<<6)		
#define MMC_STATE_CMDQ		(1<<12)         
#define MMC_STATE_AUTO_BKOPS	(1<<13)		
	unsigned int		quirks; 	
#define MMC_QUIRK_LENIENT_FN0	(1<<0)		
#define MMC_QUIRK_BLKSZ_FOR_BYTE_MODE (1<<1)	
						
#define MMC_QUIRK_NONSTD_SDIO	(1<<2)		
						
#define MMC_QUIRK_BROKEN_CLK_GATING (1<<3)	
#define MMC_QUIRK_NONSTD_FUNC_IF (1<<4)		
#define MMC_QUIRK_DISABLE_CD	(1<<5)		
#define MMC_QUIRK_INAND_CMD38	(1<<6)		
#define MMC_QUIRK_BLK_NO_CMD23	(1<<7)		
#define MMC_QUIRK_BROKEN_BYTE_MODE_512 (1<<8)	
						
#define MMC_QUIRK_LONG_READ_TIME (1<<9)		
#define MMC_QUIRK_SEC_ERASE_TRIM_BROKEN (1<<10)	
#define MMC_QUIRK_BROKEN_IRQ_POLLING	(1<<11)	
						
#define MMC_QUIRK_INAND_DATA_TIMEOUT  (1<<12)   
#define MMC_QUIRK_BROKEN_HPI (1 << 13)		
						
#define MMC_QUIRK_CACHE_DISABLE (1 << 14)	
#define MMC_QUIRK_QCA6574_SETTINGS (1 << 15)	
#define MMC_QUIRK_QCA9377_SETTINGS (1 << 16)	

#define MMC_QUIRK_CMDQ_EMPTY_BEFORE_DCMD (1 << 17)

	unsigned int		erase_size;	
 	unsigned int		erase_shift;	
 	unsigned int		pref_erase;	
 	u8			erased_byte;	

	u32			raw_cid[4];	
	u32			raw_csd[4];	
	u32			raw_scr[2];	
	struct mmc_cid		cid;		
	struct mmc_csd		csd;		
	struct mmc_ext_csd	ext_csd;	
	struct sd_scr		scr;		
	struct sd_ssr		ssr;		
	struct sd_switch_caps	sw_caps;	

	unsigned int		sdio_funcs;	
	struct sdio_cccr	cccr;		
	struct sdio_cis		cis;		
	struct sdio_func	*sdio_func[SDIO_MAX_FUNCS]; 
	struct sdio_func	*sdio_single_irq; 
	unsigned		num_info;	
	const char		**info;		
	struct sdio_func_tuple	*tuples;	

	unsigned int		sd_bus_speed;	
	unsigned int		mmc_avail_type;	
	int                     force_remove;   

	struct dentry		*debugfs_root;
	struct mmc_part	part[MMC_NUM_PHY_PARTITION]; 
	unsigned int    nr_parts;
	unsigned int	part_curr;

	struct mmc_wr_pack_stats wr_pack_stats; 
	struct notifier_block        reboot_notify;
	enum mmc_pon_type pon_type;
	u8 *cached_ext_csd;
	bool cmdq_init;
	struct mmc_bkops_info bkops;
	unsigned char   speed_class;		
};

static inline void mmc_part_add(struct mmc_card *card, unsigned int size,
			unsigned int part_cfg, char *name, int idx, bool ro,
			int area_type)
{
	card->part[card->nr_parts].size = size;
	card->part[card->nr_parts].part_cfg = part_cfg;
	sprintf(card->part[card->nr_parts].name, name, idx);
	card->part[card->nr_parts].force_ro = ro;
	card->part[card->nr_parts].area_type = area_type;
	card->nr_parts++;
}

static inline bool mmc_large_sector(struct mmc_card *card)
{
	return card->ext_csd.data_sector_size == 4096;
}


struct mmc_fixup {
	
	const char *name;

	
	u64 rev_start, rev_end;

	unsigned int manfid;
	unsigned short oemid;

	
	u16 cis_vendor, cis_device;

	
	unsigned int ext_csd_rev;

	void (*vendor_fixup)(struct mmc_card *card, int data);
	int data;
};

#define CID_MANFID_SANDISK	0x2
#define CID_MANFID_SANDISK_2	0x45
#define CID_MANFID_TOSHIBA	0x11
#define CID_MANFID_MICRON	0x13
#define CID_MANFID_SAMSUNG	0x15
#define CID_MANFID_KINGSTON	0x70
#define CID_MANFID_HYNIX        0x90
#define CID_MANFID_NUMONYX_MICRON 0xfe

#define CID_MANFID_ANY (-1u)
#define CID_OEMID_ANY ((unsigned short) -1)
#define CID_NAME_ANY (NULL)
#define EXT_CSD_REV_ANY (-1u)

#define END_FIXUP { NULL }

enum mmc_version_ext_csd_rev {
	MMC_V4_0,
	MMC_V4_1,
	MMC_V4_2,
	MMC_V4_41 = 5,
	MMC_V4_5,
	MMC_V4_51 = MMC_V4_5,
	MMC_V5_0,
	MMC_V5_01 = MMC_V5_0,
	MMC_V5_1
};

#define _FIXUP_EXT(_name, _manfid, _oemid, _rev_start, _rev_end,	\
		   _cis_vendor, _cis_device,				\
		   _fixup, _data, _ext_csd_rev)				\
	{						   \
		.name = (_name),			   \
		.manfid = (_manfid),			   \
		.oemid = (_oemid),			   \
		.rev_start = (_rev_start),		   \
		.rev_end = (_rev_end),			   \
		.cis_vendor = (_cis_vendor),		   \
		.cis_device = (_cis_device),		   \
		.vendor_fixup = (_fixup),		   \
		.data = (_data),			   \
		.ext_csd_rev = (_ext_csd_rev),		   \
	 }

#define MMC_FIXUP_REV(_name, _manfid, _oemid, _rev_start, _rev_end,	\
		      _fixup, _data, _ext_csd_rev)			\
	_FIXUP_EXT(_name, _manfid,					\
		   _oemid, _rev_start, _rev_end,			\
		   SDIO_ANY_ID, SDIO_ANY_ID,				\
		   _fixup, _data, _ext_csd_rev)				\

#define MMC_FIXUP(_name, _manfid, _oemid, _fixup, _data) \
	MMC_FIXUP_REV(_name, _manfid, _oemid, 0, -1ull, _fixup, _data,	\
		      EXT_CSD_REV_ANY)

#define MMC_FIXUP_EXT_CSD_REV(_name, _manfid, _oemid, _fixup, _data,	\
			      _ext_csd_rev)				\
	MMC_FIXUP_REV(_name, _manfid, _oemid, 0, -1ull, _fixup, _data,	\
		      _ext_csd_rev)

#define SDIO_FIXUP(_vendor, _device, _fixup, _data)			\
	_FIXUP_EXT(CID_NAME_ANY, CID_MANFID_ANY,			\
		    CID_OEMID_ANY, 0, -1ull,				\
		   _vendor, _device,					\
		   _fixup, _data, EXT_CSD_REV_ANY)			\

#define cid_rev(hwrev, fwrev, year, month)	\
	(((u64) hwrev) << 40 |                  \
	 ((u64) fwrev) << 32 |                  \
	 ((u64) year) << 16 |                   \
	 ((u64) month))

#define cid_rev_card(card)		  \
	cid_rev(card->cid.hwrev,	  \
		    card->cid.fwrev,      \
		    card->cid.year,	  \
		    card->cid.month)


static inline void __maybe_unused add_quirk(struct mmc_card *card, int data)
{
	card->quirks |= data;
}

static inline void __maybe_unused remove_quirk(struct mmc_card *card, int data)
{
	card->quirks &= ~data;
}

#define mmc_card_mmc(c)		((c)->type == MMC_TYPE_MMC)
#define mmc_card_sd(c)		((c)->type == MMC_TYPE_SD)
#define mmc_card_sdio(c)	((c)->type == MMC_TYPE_SDIO)

#define mmc_card_present(c)	((c)->state & MMC_STATE_PRESENT)
#define mmc_card_readonly(c)	((c)->state & MMC_STATE_READONLY)
#define mmc_card_blockaddr(c)	((c)->state & MMC_STATE_BLOCKADDR)
#define mmc_card_ext_capacity(c) ((c)->state & MMC_CARD_SDXC)
#define mmc_card_removed(c)	((c) && ((c)->state & MMC_CARD_REMOVED))
#define mmc_card_doing_bkops(c)	((c)->state & MMC_STATE_DOING_BKOPS)
#define mmc_card_suspended(c)	((c)->state & MMC_STATE_SUSPENDED)
#define mmc_card_cmdq(c)       ((c)->state & MMC_STATE_CMDQ)
#define mmc_card_doing_auto_bkops(c)	((c)->state & MMC_STATE_AUTO_BKOPS)

#define mmc_card_set_present(c)	((c)->state |= MMC_STATE_PRESENT)
#define mmc_card_set_readonly(c) ((c)->state |= MMC_STATE_READONLY)
#define mmc_card_set_blockaddr(c) ((c)->state |= MMC_STATE_BLOCKADDR)
#define mmc_card_set_ext_capacity(c) ((c)->state |= MMC_CARD_SDXC)
#define mmc_card_set_removed(c) ((c)->state |= MMC_CARD_REMOVED)
#define mmc_card_set_doing_bkops(c)	((c)->state |= MMC_STATE_DOING_BKOPS)
#define mmc_card_clr_doing_bkops(c)	((c)->state &= ~MMC_STATE_DOING_BKOPS)
#define mmc_card_set_suspended(c) ((c)->state |= MMC_STATE_SUSPENDED)
#define mmc_card_clr_suspended(c) ((c)->state &= ~MMC_STATE_SUSPENDED)
#define mmc_card_set_cmdq(c)           ((c)->state |= MMC_STATE_CMDQ)
#define mmc_card_clr_cmdq(c)           ((c)->state &= ~MMC_STATE_CMDQ)
#define mmc_card_set_auto_bkops(c)	((c)->state |= MMC_STATE_AUTO_BKOPS)
#define mmc_card_clr_auto_bkops(c)	((c)->state &= ~MMC_STATE_AUTO_BKOPS)

#define mmc_card_strobe(c) (((c)->ext_csd).strobe_support & MMC_STROBE_SUPPORT)


static inline void __maybe_unused add_quirk_mmc(struct mmc_card *card, int data)
{
	if (mmc_card_mmc(card))
		card->quirks |= data;
}

static inline void __maybe_unused remove_quirk_mmc(struct mmc_card *card,
						   int data)
{
	if (mmc_card_mmc(card))
		card->quirks &= ~data;
}


static inline void __maybe_unused add_quirk_sd(struct mmc_card *card, int data)
{
	if (mmc_card_sd(card))
		card->quirks |= data;
}

static inline void __maybe_unused remove_quirk_sd(struct mmc_card *card,
						   int data)
{
	if (mmc_card_sd(card))
		card->quirks &= ~data;
}

static inline int mmc_card_lenient_fn0(const struct mmc_card *c)
{
	return c->quirks & MMC_QUIRK_LENIENT_FN0;
}

static inline int mmc_blksz_for_byte_mode(const struct mmc_card *c)
{
	return c->quirks & MMC_QUIRK_BLKSZ_FOR_BYTE_MODE;
}

static inline int mmc_card_disable_cd(const struct mmc_card *c)
{
	return c->quirks & MMC_QUIRK_DISABLE_CD;
}

static inline int mmc_card_nonstd_func_interface(const struct mmc_card *c)
{
	return c->quirks & MMC_QUIRK_NONSTD_FUNC_IF;
}

static inline int mmc_card_broken_byte_mode_512(const struct mmc_card *c)
{
	return c->quirks & MMC_QUIRK_BROKEN_BYTE_MODE_512;
}

static inline int mmc_card_long_read_time(const struct mmc_card *c)
{
	return c->quirks & MMC_QUIRK_LONG_READ_TIME;
}

static inline int mmc_card_broken_irq_polling(const struct mmc_card *c)
{
	return c->quirks & MMC_QUIRK_BROKEN_IRQ_POLLING;
}

static inline bool mmc_card_support_auto_bkops(const struct mmc_card *c)
{
	return c->ext_csd.rev >= MMC_V5_1;
}

static inline bool mmc_card_configured_manual_bkops(const struct mmc_card *c)
{
	return c->ext_csd.bkops_en & EXT_CSD_BKOPS_MANUAL_EN;
}

static inline bool mmc_card_configured_auto_bkops(const struct mmc_card *c)
{
	return c->ext_csd.bkops_en & EXT_CSD_BKOPS_AUTO_EN;
}

static inline bool mmc_enable_qca6574_settings(const struct mmc_card *c)
{
	return c->quirks & MMC_QUIRK_QCA6574_SETTINGS;
}

static inline bool mmc_enable_qca9377_settings(const struct mmc_card *c)
{
	return c->quirks & MMC_QUIRK_QCA9377_SETTINGS;
}

#define mmc_card_name(c)	((c)->cid.prod_name)
#define mmc_card_id(c)		(dev_name(&(c)->dev))

#define mmc_dev_to_card(d)	container_of(d, struct mmc_card, dev)

#define mmc_list_to_card(l)	container_of(l, struct mmc_card, node)
#define mmc_get_drvdata(c)	dev_get_drvdata(&(c)->dev)
#define mmc_set_drvdata(c,d)	dev_set_drvdata(&(c)->dev, d)

struct mmc_driver {
	struct device_driver drv;
	int (*probe)(struct mmc_card *);
	void (*remove)(struct mmc_card *);
	int (*suspend)(struct mmc_card *);
	int (*resume)(struct mmc_card *);
	void (*shutdown)(struct mmc_card *);
};

extern int mmc_register_driver(struct mmc_driver *);
extern void mmc_unregister_driver(struct mmc_driver *);

extern void mmc_fixup_device(struct mmc_card *card,
			     const struct mmc_fixup *table);
extern struct mmc_wr_pack_stats *mmc_blk_get_packed_statistics(
			struct mmc_card *card);
extern void mmc_blk_init_packed_statistics(struct mmc_card *card);
extern int mmc_send_pon(struct mmc_card *card);
extern void mmc_blk_cmdq_req_done(struct mmc_request *mrq);
#endif 
