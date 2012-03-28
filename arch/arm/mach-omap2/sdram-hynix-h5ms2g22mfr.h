/*
 * SDRC register values for the Hynix H5MS2G22MFR
 *
 * Copyright (C) 2008 Texas Instruments, Inc.
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ARCH_ARM_MACH_OMAP2_SDRAM_HYNIX_H5MS2G22MFR
#define ARCH_ARM_MACH_OMAP2_SDRAM_HYNIX_H5MS2G22MFR

#include <mach/sdrc.h>

/* Hynix H5MS2G22MFR */
/* XXX Using ARE = 0x1 (no autorefresh burst) -- can this be changed? */
static struct omap_sdrc_params h5ms2g22mfr_sdrc_params[] = {
	[0] = {
		.rate	     = 166000000,
		.actim_ctrla = 0x7a9db4c6,
		.actim_ctrlb = 0x00011118,
		.rfr_ctrl    = 0x0004e201,
		.mr	     = 0x00000032,
	},
	[1] = {
		.rate	     = 165941176,
		.actim_ctrla = 0x7a9db4c6,
		.actim_ctrlb = 0x00011217,
		.rfr_ctrl    = 0x0004e201,
		.mr	     = 0x00000032,
	},
	[2] = {
		.rate	     = 83000000,
		.actim_ctrla = 0x41512283,
		.actim_ctrlb = 0x0001110c,
		.rfr_ctrl    = 0x00025801,
		.mr	     = 0x00000032,
	},
	[3] = {
		.rate	     = 82970588,
		.actim_ctrla = 0x41512283,
		.actim_ctrlb = 0x0001110c,
		.rfr_ctrl    = 0x00025801,
		.mr	     = 0x00000032,
	},
	[4] = {
		.rate	     = 0
	},
};

#endif
