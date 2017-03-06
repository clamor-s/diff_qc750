
DHDCFLAGS = -DFPNUM='"57"' -DDEBUG_LEVEL1 -DLINUX

ifeq ($(CONFIG_MRVLSD8787_PROC_DEBUG),y)
	DHDCFLAGS += -DPROC_DEBUG
	export CONFIG_PROC_DEBUG
endif

ifeq ($(CONFIG_MRVLSD8787_STA_SUPPORT),y)
	DHDCFLAGS += -DSTA_SUPPORT
ifeq ($(CONFIG_MRVLSD8787_REASSOCIATION),y)
	DHDCFLAGS += -DREASSOCIATION
endif
endif

ifeq ($(CONFIG_MRVLSD8787_UAP_SUPPORT),y)
	DHDCFLAGS += -DUAP_SUPPORT
endif

ifeq ($(CONFIG_MRVLSD8787_MFG_CMD_SUPPORT),y)
	DHDCFLAGS += -DMFG_CMD_SUPPORT
endif

ifeq ($(CONFIG_MRVLSD8787_BIG_ENDIAN),y)
	DHDCFLAGS += -DBIG_ENDIAN_SUPPORT
endif

ifeq ($(CONFIG_MRVLSD8787_SDIO_MULTI_PORT_TX_AGGR),y)
	DHDCFLAGS += -DSDIO_MULTI_PORT_TX_AGGR
endif

ifeq ($(CONFIG_MRVLSD8787_SDIO_MULTI_PORT_RX_AGGR),y)
	DHDCFLAGS += -DSDIO_MULTI_PORT_RX_AGGR
endif


ifeq ($(CONFIG_MRVLSD8787_SDIO_SUSPEND_RESUME),y)
	DHDCFLAGS += -DSDIO_SUSPEND_RESUME
endif

MLANOBJS =	mlinux/moal_main.o \
		mlinux/moal_ioctl.o \
		mlinux/moal_shim.o

ifeq ($(CONFIG_MRVLSD8787_STA_SUPPORT),y)
MLANOBJS += mlinux/moal_priv.o \
            mlinux/moal_wext.o
endif

ifeq ($(CONFIG_MRVLSD8787_UAP_SUPPORT),y)
MLANOBJS += mlinux/moal_uap.o
MLANOBJS += mlinux/moal_uap_priv.o
MLANOBJS += mlinux/moal_uap_wext.o
endif

ifdef CONFIG_PROC_FS
MLANOBJS += mlinux/moal_proc.o
ifeq ($(CONFIG_MRVLSD8787_PROC_DEBUG),y)
MLANOBJS += mlinux/moal_debug.o
endif
endif

MLANOBJS += mlinux/moal_sdio_mmc.o

MLANOBJS +=	mlan/mlan_shim.o mlan/mlan_init.o \
		mlan/mlan_txrx.o \
		mlan/mlan_cmdevt.o mlan/mlan_misc.o

MLANOBJS += mlan/mlan_wmm.o
MLANOBJS += mlan/mlan_sdio.o
MLANOBJS += mlan/mlan_11n_aggr.o
MLANOBJS += mlan/mlan_11n_rxreorder.o
MLANOBJS += mlan/mlan_11n.o
ifeq ($(CONFIG_MRVLSD8787_STA_SUPPORT),y)
MLANOBJS += mlan/mlan_11d.o
MLANOBJS += mlan/mlan_11h.o
MLANOBJS += mlan/mlan_meas.o
MLANOBJS += mlan/mlan_cfp.o \
			mlan/mlan_scan.o \
			mlan/mlan_sta_ioctl.o \
			mlan/mlan_sta_rx.o \
			mlan/mlan_sta_tx.o \
			mlan/mlan_sta_event.o \
			mlan/mlan_sta_cmd.o \
			mlan/mlan_sta_cmdresp.o \
			mlan/mlan_join.o
endif

ifeq ($(CONFIG_MRVLSD8787_UAP_SUPPORT),y)
MLANOBJS += mlan/mlan_uap_ioctl.o
MLANOBJS += mlan/mlan_uap_cmdevent.o
MLANOBJS += mlan/mlan_uap_txrx.o
endif

obj-$(CONFIG_MRVLSD8787_MLAN) := wlan.o
wlan-objs := $(MLANOBJS)

EXTRA_CFLAGS = $(DHDCFLAGS)
EXTRA_LDFLAGS += --strip-debug
