#
# Makefile for the kernel mmc device drivers.
#

ifeq ($(CONFIG_MMC_DEBUG),y)
	EXTRA_CFLAGS		+= -DDEBUG
endif

ifeq ($(CONFIG_MMC_LOG),y)
    obj-$(CONFIG_MMC)	+= log/
endif

obj-$(CONFIG_MMC)		+= core/
obj-$(CONFIG_MMC)		+= card/
obj-$(CONFIG_MMC)		+= host/

