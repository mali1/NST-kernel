#
# Makefile for the OMAP3EP framebuffer driver (for Linux Kernel 2.6)
#

#EXTRA_CFLAGS += -DDEBUG=1

obj-$(CONFIG_FB_OMAP3EP) += omap3epfb.o

objs-y := omap3epfb-dev.o subfq.o pmic.o pmic-tps65180-i2c.o pmic-tps65185-i2c.o omap3epfb-user.o

objs-$(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS) += omap3ep-dss.o  halDSS.o \
						pmic-auo-i2c.o \
						dsi_pll_drv.o


objs-$(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_GPMC) += gpmc_config.o

omap3epfb-objs := $(objs-y)
