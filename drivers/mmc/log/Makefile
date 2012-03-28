#
# Makefile for MMC Logging driver
#

ifeq ($(CONFIG_MMC_DEBUG),y)
	EXTRA_CFLAGS		+= -DDEBUG
endif

obj-$(CONFIG_MMC_LOG)	+= mmc_log.o

