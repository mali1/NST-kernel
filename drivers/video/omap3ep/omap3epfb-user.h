/*
 * OMAP3EP FB user-space visible declarations.
 *
 * Copyright (C) 2009 Dimitar Dimitrov, Vladimir Krushkin, MM Solutions.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *	1. Redistributions of source code must retain the above copyright
 *	   notice, this list of conditions and the following disclaimer.
 *
 *	2. Redistributions in binary form must reproduce the above copyright
 *	   notice, this list of conditions and the following disclaimer in
 *	   the documentation and/or other materials provided with the
 *	   distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of MMS.
 *
 *
 *  API ChangeLog:
 *      01.00   Created.
 *      ...	Dark age
 *      01.08   Add support for double buffering/page flipping.
 *      01.09   Reintroduce support for timing statistics
 *      01.10   Exposed rotation via fixed parameters structure
 *      01.11   Add support for AUO panel
 *      01.12   Add support for run-time DSS parameter passing
 *      01.13   Internal driver change for update area handling
 *      02.00   Per pixel EPD controller
 *      02.01   Added VBORDER control
 *      02.02   Added DSS dummy lines to keep DATA_EN
*/

#ifndef OMAP3EPFB_USER_H
#define OMAP3EPFB_USER_H

#include <linux/types.h>
#include <linux/ioctl.h>

/*
 * Driver author must increase this when IOCTL semantics or structures
 * are changed. User-space application is expected to check this to ensure
 * it has been compiled with the correct header file.
 */

#define OMAP3EPFB_IOCTL_API_VERSION	0x0207

#define OMAP3EPFB_WVFID_GC		0
#define OMAP3EPFB_WVFID_GU		1
#define OMAP3EPFB_WVFID_DU		2
#define OMAP3EPFB_WVFID_A2		3
#define OMAP3EPFB_WVFID_GL		4
#define OMAP3EPFB_WVFID_X1		5
#define OMAP3EPFB_WVFID_X2		6
#define OMAP3EPFB_WVFID_NUM		(OMAP3EPFB_WVFID_X2 + 1)
#define OMAP3EPFB_WVFID_VU		128
#define OMAP3EPFB_WVFID_AUTO		256

/* Out-of-band flag in wvid field: STRICTLY FOR PRIVATE USE! */
#define OMAP3EPFB_OOB_VSID		16 	/* Virtual screen ID.*/

/* Threshold for BW for the source image*/
#define OMAP3EPFB_THRESHOLD_GRAY          0   /* shows the image via gamma correction */
#define OMAP3EPFB_THRESHOLD_BW_DEFAULT    12  /* the normal threshold if is required B&W image */
#define OMAP3EPFB_THRESHOLD_WHITE         16  /* pseudo threshold - complete white image */
#define OMAP3EPFB_THRESHOLD_BLACK         17  /* pseudo threshold - complete black image */
/* from 1 to 17 are values indicating the BlackWhite Threshold - small number less black */


/*
 * User requests area updates using this structure.
 *
 * A few notes about the update (window) descriptor returned by the driver
 * upon successful update area request. There is no update-area-close or
 * acknowledge by the user space application, so the validity time is
 * explained below.
 *
 * One part of the descriptor's bits are assigned to the window buffer index
 * used internally by the driver for access to the active windows array.
 * The other bits are assigned the consecutive request number for this
 * particular index. Driver tends to use the lower indexes more often,
 * so even in the worst case scenario with frequent requests routed to
 * the same index and maximum number of windows exceeding 256, we still
 * must make more than 2^20 requests before getting the same update
 * descriptor.
 *
 * WARNING: In case of overlapping the driver returns the number of
 * milliseconds after which the area will become available for update.
 * The request, however, is not queued. This means that after nleft_ms
 * milliseconds the user-space application might again be rejected if
 * another application is faster and queues an update request.
 */
struct omap3epfb_update_area {
	/* input parameters, provided by the user-space application */
	unsigned int x0;        /* area's horizontal position */
	unsigned int y0;        /* area's vertical position */
	unsigned int x1;        /* area last horizontal pixel */
	unsigned int y1;        /* area last vertical pixel */
	unsigned int wvfid;     /* waveform ID */
        unsigned int threshold; /* threshold for BW images */
};

#define OMAP3EPFB_PANELTYPE_EINK         0
#define OMAP3EPFB_PANELTYPE_AUO          1

/*
 * EPD-related fixed/constant parameters.
 */
struct omap3epfb_epd_fixpar {
	int rotation;	       /* in degrees */
	int panel_type;        /* Panel which is managed (connect)*/
};

#define EPD_BORDER_FLOATING 0
#define EPD_BORDER_BLACK 1
#define EPD_BORDER_WHITE 2

#define FULL_QUEUE_PERIOD	500 /* ms */

/*
 * When missed-subframes counter for _current_ screen sequence
 * (par->num_missed_subframes) becomes greater than this value, then we
 * most probably have hit OMAPS00238228.
 */
#define SLEEPAWAKE_MISSED_THRESHOLD	15

/*
 * EPD-related user-configurable parameters.
 */
struct omap3epfb_epd_varpar {
	/* configurable parameters */
	unsigned int autowvf_gu_max_changed_pixels;
	long defio_timeout;
	unsigned int border_color;
	unsigned int emptysync;
};

/*
 * Average cumulative timing statistics gathered by the driver operation.
 *
 * The average timing statistics are gathered by a "dynamic mean" algorithm:
 * 	1. Execute a routine and time its duration.
 * 	2. Add the timing to t_us.
 * 	3. Increment proc_n.
 * 	4. If proc_n is bigger than THRESHOLD (e.g. 1024) then go to 6.
 * 	5. Exit.
 * 	6. t_us = t_us/2;
 * 	7. proc_n = proc_n/2;
 * 	8. Exit.
 */
struct omap3epfb_timing_statistics {
	unsigned int t_us;		/* processing time over ... */
	unsigned int proc_n;		/* number of processing calls */
};

/*
 * Debug statistics gathered by the driver operation. Note that driver might
 * have been built with statistics gathering disabled, in which case the
 * IOCTL will return an error.
 */
struct omap3epfb_statistics {
	/* Full-screen TM generation timing statistics. */
	struct omap3epfb_timing_statistics stm_proc_stats;
	/* Windowed TM generation timing statistics. */
	struct omap3epfb_timing_statistics wtm_proc_stats;
	/* Full-screen subframe generation timing statistics. */
	struct omap3epfb_timing_statistics ssubf_proc_stats;
	/* Partial/windowed subframe generation timing statistics. */
	struct omap3epfb_timing_statistics wsubf_proc_stats;

//!!!!! num_screen_seqs is not used DO NOT REMOVE IT
	unsigned int num_screen_seqs;	/* number of screen sequences so far */
	unsigned int num_area_updates;	/* number of area updates so far */
	unsigned int num_screen_updates; /* number of screen updates so far */
	unsigned int num_rejected_updates; /* number of rejected updates */
	unsigned int total_subframes;	/* total processed subframes so far */
//!!!!! total_missed_subf is not used DO NOT REMOVE IT
	unsigned int total_missed_subf; /* total missed subframes */
//!!!!! max_missed_subf is not used DO NOT REMOVE IT
	unsigned int max_missed_subf;	/* max missed per screen sequence */
	unsigned long pre_blit_latency; /* pre-blitter update latency in us */
	unsigned long full_latency; /* full update latency in us */
};


struct omap3epfb_bfb_update_area {
	/* input parameters, provided by the user-space application */
	unsigned int x0;        /* area's horizontal position */
	unsigned int y0;        /* area's vertical position */
	unsigned int x1;        /* area last horizontal pixel */
	unsigned int y1;        /* area last vertical pixel */
};


/* Epaper hardware states. */
enum omap3epfb_hwstate {
	OMAP3EPFB_HW_OK,
	OMAP3EPFB_HW_POWERUP_TIMEOUT,
	OMAP3EPFB_HW_POWERDOWN_TIMEOUT
};

/*Shared memory declarations */
#define OMAP3EPFB_SHARED_MEM_SIZE 	128

#ifndef OMAP3EPFB_SUBFQ_DEPTH
#define OMAP3EPFB_SUBFQ_DEPTH		4u
#else
  #if OMAP3EPFB_SUBFQ_DEPTH != 4u
    #error "Check DSP SN"
  #endif
#endif

typedef struct  {
	void *virt_arm;		/* ARM virtual address */
	void *virt_dsp;		/* ??? DSP virtual address */
}omap3epfb_subfbuf_rd_st;

typedef struct {
	int rai;
	unsigned int subframe_queue_depth; //must be power of 2
	omap3epfb_subfbuf_rd_st bufs[OMAP3EPFB_SUBFQ_DEPTH];
	omap3epfb_subfbuf_rd_st hdrs[OMAP3EPFB_SUBFQ_DEPTH];
	int temperature;
	int bfb_queue_depth;
	int bqwi;
	int bq_arm2dsp_last_req;
}omap3epfb_rd_st;

typedef struct {
	int vcom;
	int subf_type;
} dsp_subfbuf_hdrs_st;

typedef union {
	dsp_subfbuf_hdrs_st hdr;
	char dummy[128];
} dsp_subfbuf_hdrs_un;

typedef struct  {
	int last;		/* whether subframe is last in the sequence */
}omap3epfb_subfbuf_wr_st;

typedef struct {
	int wi;
	omap3epfb_subfbuf_wr_st bufs[OMAP3EPFB_SUBFQ_DEPTH];
	int last_subf_queued;
	int subframe_numbers;
	int bqri;
	int bq_dsp2arm_last_resp;
	int bq_dsp2arm_waveform_status;
	int dsp_running_flag;
}omap3epfb_wr_st;

typedef union {
	omap3epfb_rd_st rq;
	omap3epfb_wr_st wq;
	char dummy[OMAP3EPFB_SHARED_MEM_SIZE];
}omap3epqe_rdwr_un;//write from DSP - read from ARM

struct omap3epfb_sharedbuf {
	omap3epqe_rdwr_un *p;		/* virtual address */
	unsigned long phys;	/* physical (bus/dma) address */
	size_t size;		/* size of the region (???) */
};

#define EFFECT_ARRAY_SIZE	8

struct omap3epfb_area {
	int index;
	int effect_flags;
	struct omap3epfb_update_area effect_area;
};

/**/

struct omap3epfb_buffer {
	unsigned long phys;	/* physical (bus/dma) address */
	/*
	 * Page aligned address, that is to to store the lines
	 * before the phys address, must keep track to for free()
	 * calls.
	 */
	unsigned long phys_aligned;
	/*
	 * This is the exact address to be passed to the DSS. It is
	 * in the range 0 -> phys_aligned -> phys_lines -> phys -> 0xMAX
	 */
	unsigned long phys_lines;
	void __iomem *virt;	/* virtual address */
	size_t size;		/* size of the region (???) */
};

struct omap3epfb_subfbuf {
	struct omap3epfb_buffer hdr;
	struct omap3epfb_buffer sbf;
};

struct omap3epfb_buffaddrs {

	struct omap3epfb_buffer bfbmem;
	struct omap3epfb_buffer bfbquemem;
	struct omap3epfb_subfbuf subf[OMAP3EPFB_SUBFQ_DEPTH];


	struct {
		struct omap3epfb_sharedbuf rd;
		struct omap3epfb_sharedbuf wr;
		struct omap3epfb_sharedbuf stats;
		struct omap3epfb_sharedbuf gpmc;
		size_t size;
	}shmem;
};

#define DSP_PM_SLEEP 0x10000000UL
#define DSP_PM_AWAKE 0x20000000UL
#define EDP_ROTATE   0x30000000UL

/* Custom user OMAP3EP FB IOCTL-s */
#define OMAP3EPFB_IO_GET_API_VERSION	_IOR('O', 0x80, int)
#define OMAP3EPFB_IO_REQ_SCREEN_CLEAR	_IO('O', 0x81)
#define OMAP3EPFB_IO_REQ_SCREEN_UPDATE	_IOW('O', 0x82, int)
#define OMAP3EPFB_IO_REQ_AREA_UPDATE	_IOWR('O', 0x83, \
						struct omap3epfb_update_area)
#define OMAP3EPFB_IO_POLL_SCREEN_ACTIVE	_IO('O', 0x84)
#define OMAP3EPFB_IO_GET_EPDFIXPAR	_IOR('O', 0x85, \
						struct omap3epfb_epd_fixpar)
#define OMAP3EPFB_IO_GET_EPDVARPAR	_IOR('O', 0x86, \
						struct omap3epfb_epd_varpar)
#define OMAP3EPFB_IO_SET_EPDVARPAR	_IOW('O', 0x87, \
						struct omap3epfb_epd_varpar)
#define OMAP3EPFB_IO_GET_HWSTATE	_IOR('O', 0x88, int)
#define OMAP3EPFB_IO_GET_STATS		_IOR('O', 0x89, \
						struct omap3epfb_statistics)
#define OMAP3EPFB_IO_RESET_STATS	_IO('O', 0x8a)

#define OMAP3EPFB_IO_BFB_GET_ADDRS		_IOR('O', 0x8b, \
						struct omap3epfb_buffer)

#define OMAP3EPFB_IO_BFB_AREA_UPDATE	_IOWR('O', 0x8c, \
						struct omap3epfb_bfb_update_area)


/* Daemon-specific OMAP3EP FB IOCTL-s */
#define OMAP3EPFB_IO_GET_UPDATE_AREA	_IOR('O', 0x90, \
						struct omap3epfb_update_area)

#define OMAP3EPFB_IO_START_UPDATE	_IO('O', 0x91)


#define OMAP3EPFB_IO_DEQUEUE_SUBF	_IOR('O', 0x92, int)
#define OMAP3EPFB_IO_QUEUE_SUBF		_IOW('O', 0x93, int)

#define OMAP3EPFB_IO_GET_BUFFADDRS	_IOR('O', 0x94, \
						struct omap3epfb_buffaddrs)

#define OMAP3EPFB_IO_GET_TEMPERATURE	_IOR('O', 0x95, int)
#define OMAP3EPFB_IO_REGISTER_DEMON	_IO('O', 0x96)
#define OMAP3EPFB_IO_UNREGISTER_DEMON	_IO('O', 0x97)

#define OMAP3EPFB_IO_SET_DSS_TIMING     _IOW('O', 0x98,int[10])

#define OMAP3EPFB_IO_SET_ROTATE         _IOW('O', 0x99, int)

#define OMAP3EPFB_IO_SET_READY_DEMON	_IO('O', 0x9A)
#define OMAP3EPFB_IO_CHECK_READY_DEMON	_IOR('O', 0x9B, int)

#define OMAP3EPFB_IO_SET_REGION     	_IOW('R', 0x80, struct omap3epfb_area)
#define OMAP3EPFB_IO_GET_REGION     	_IOWR('R', 0x81, struct omap3epfb_area)
#define OMAP3EPFB_IO_RESET_REGION     	_IOWR('R', 0x82, int)
#define OMAP3EPFB_IO_FILL_REGION     	_IOW('R', 0x83, struct omap3epfb_area)

#endif  /* OMAP3EPFB_USER_H */
