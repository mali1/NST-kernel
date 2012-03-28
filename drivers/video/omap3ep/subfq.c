/*
 * OMAP3EP FB: DSS and subframe queue management.
 *
 *      Copyright (C) 2009 Dimitar Dimitrov, Vladimir Krushkin, MM Solutions
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/omapfb.h>

#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>

#include <mach/sram.h>
#include <mach/vram.h>
#include <mach/dma.h>
#include <mach/resource.h>

#include "omap3epfb.h"

#include "pmic.h"
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
#include "omap3ep-dss.h"
#include <mach/omap-pm.h>
#define HEADER_SIZE		128u

#if defined(CONFIG_SMP)
  #error "Kernel configuration is not sane!"
#endif

#if (OMAP3EPFB_SUBFQ_DEPTH != 4) \
		&& (OMAP3EPFB_SUBFQ_DEPTH != 8) \
		&& (OMAP3EPFB_SUBFQ_DEPTH != 16)
   #error "Subframe queue depth is not sane!"
#endif

#define SUBFQ_INDEX_MASK	(OMAP3EPFB_SUBFQ_DEPTH-1)

#endif

/* ------------------------------------------------------------------------- */

#if defined(OMAP3EPFB_GATHER_STATISTICS)
static void omap3epfb_start_timestamp(struct omap3epfb_par *par)
{
	if (!test_and_set_bit(UPD_APP_REQUEST, &par->stamp_mask))
		getnstimeofday(&par->update_timings[UPD_APP_REQUEST]);
}

static unsigned long omap3epfb_calc_latency(struct omap3epfb_par *par,
		int dest, int end)
{
	BUG_ON(end < UPD_PRE_BLITTER || end > UPD_DSS_STARTING);
	BUG_ON(dest < UPD_PB_LATENCY || dest > UPD_FULL_LATENCY);
	par->update_timings[dest] = timespec_sub(par->update_timings[end],
			par->update_timings[UPD_APP_REQUEST]);
	return (par->update_timings[dest].tv_sec * 1000000 +
			(par->update_timings[dest].tv_nsec / 1000));

}
static void omap3epfb_event_timestamp(struct omap3epfb_par *par, int id)
{
	BUG_ON(id <= UPD_APP_REQUEST || id > UPD_DSS_STARTING);
	if (test_bit(id - 1, &par->stamp_mask)) {
		if (!test_and_set_bit(id, &par->stamp_mask))
			getnstimeofday(&par->update_timings[id]);
	}
}
static void omap3epfb_store_latencies(struct omap3epfb_par *par)
{
	if (test_bit(UPD_DSS_STARTING, &par->stamp_mask)) {
		par->stats.pre_blit_latency = omap3epfb_calc_latency(par,
				UPD_PB_LATENCY, UPD_PRE_BLITTER);
		par->stats.full_latency = omap3epfb_calc_latency(par,
				UPD_FULL_LATENCY, UPD_DSS_STARTING);
	}
}
#endif

size_t omap3epfb_videomem_size(struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;
	size_t sz = 0;

	switch (par->mode.bpp) {
		case 0:  sz = par->mode.vxres * par->mode.vyres / 2; break;
		case 4:  sz = par->mode.vxres * par->mode.vyres; break;
		case 8:  sz = par->mode.vxres * par->mode.vyres; break;
		case 16: sz = par->mode.vxres * par->mode.vyres * 2; break;
		default: BUG();
	}

	/* we need double buffering (page flipping) */
	sz *= 2;

	return PAGE_ALIGN(sz);
}

#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
size_t omap3epfb_subfmem_size(struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;

	return (par->mode.pxres * par->mode.pyres) / 4;
}
#endif

size_t omap3epfb_bfb_size(struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;

	return PAGE_ALIGN(par->mode.pxres * par->mode.pyres);
}

/* ------------------------------------------------------------------------- */
static int omap3epfb_alloc_shmem(struct fb_info *info,
				 struct omap3epfb_sharedbuf *buf)
{
	int size = PAGE_ALIGN(OMAP3EPFB_SHARED_MEM_SIZE);
	int stat;

	BUG_ON(sizeof(dma_addr_t) != sizeof(uint32_t));

	stat = omap_vram_alloc(OMAPFB_MEMTYPE_SDRAM,
			       OMAP3EPFB_SHARED_MEM_SIZE, &buf->phys);

	if(stat) {
		dev_err(info->device, "Failed to allocate shared memory (phys): %d\n", stat);
		return -ENOMEM;
	}

	buf->p =(omap3epqe_rdwr_un *)ioremap_wc(buf->phys, size);

	if (!buf->p) {
		dev_err(info->device, "Failed to allocate shared memory (virt)\n");
		omap_vram_free(buf->phys, size);
		return -ENOMEM;
	}

	buf->size = size;

	dev_dbg(info->device, "allocated %Zd bytes @%08lx / @%08lx (phys)\n",
			size, (unsigned long)buf->p,
			(unsigned long)buf->phys);
			
	memset(buf->p, 0, OMAP3EPFB_SHARED_MEM_SIZE);

	return 0;
}

static void omap3epfb_free_shmem(struct fb_info *info,
				struct omap3epfb_sharedbuf *buf)
{
	iounmap(buf->p);
	if(omap_vram_free(buf->phys, buf->size)) {
		dev_err(info->device, "VRAM FREE failed!\n");
	}
}

static inline unsigned long omap3epfb_get_dss_offset(struct omap3epfb_par *par)
{
	return par->mode.pxres * (MAX_PREOFFSET_LINES - dss_timing[10]) / 4;
}


static int omap3epfb_alloc_buffer(struct fb_info *info,
				struct omap3epfb_buffer *buf, size_t size,
				unsigned int offset_lines)
{
	int stat;
	void __iomem *virt;
	struct omap3epfb_par *par = info->par;
	long lines = par->mode.pxres * offset_lines / 4;
	unsigned long off = PAGE_ALIGN(lines);

	stat = omap_vram_alloc(OMAPFB_MEMTYPE_SDRAM, size + off,  &buf->phys_aligned);
	if(stat) {
		dev_err(info->device, "Failed to allocate memory (phys): %d\n", stat);
		return -ENOMEM;
	}

	buf->phys = buf->phys_aligned + off;
	buf->phys_lines = buf->phys - lines;

	virt = ioremap_wc(buf->phys, size);
	if (!virt) {
		dev_err(info->device, "Failed to allocate memory (virt)\n");
		omap_vram_free(buf->phys_aligned, size);
		return -ENOMEM;
	}

	buf->virt = virt;
	buf->size = size;

	dev_dbg(info->device, "allocated %Zd bytes @%08lx / @%08lx (phys)\n",
			size, (unsigned long)buf->virt,
			(unsigned long)buf->phys);

	return 0;
}


static void omap3epfb_free_buffer(struct fb_info *info,
				struct omap3epfb_buffer *buf,
				unsigned int offset_lines)
{
	struct omap3epfb_par *par = info->par;
	long lines = par->mode.pxres * offset_lines / 4;
	unsigned long off = PAGE_ALIGN(lines);

	iounmap(buf->virt);
	if(omap_vram_free(buf->phys_aligned, buf->size + off)) {
		dev_err(info->device, "VRAM FREE failed!\n");
	}
}


int omap3epfb_alloc_buffers(struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;
	int stat = -ENOMEM;
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	int i;
#endif
	stat = omap3epfb_alloc_buffer(info, &par->vmem,
					omap3epfb_videomem_size(info),
					0);
	if (stat)
		goto out;
	/* don't leave garbage accessible from user space */
	memset(par->vmem.virt, 0xff, omap3epfb_videomem_size(info));

	/* Alocate Blitter Frame Buffer*/
	stat = omap3epfb_alloc_buffer(info, &par->bfb.mem,
				      omap3epfb_bfb_size(info), 0);
	if (stat)
		goto free_videomem;

	memset(par->bfb.mem.virt, 0x0f, omap3epfb_bfb_size(info));

	/* Alocate Blitter Queue*/
	stat = omap3epfb_alloc_buffer(info, &par->bfb.que.mem,
			sizeof(struct omap3epfb_bfb_queue_element)*OMAP3EPFB_BLITQ_DEPTH, 0);
	if (stat)
		goto free_bfbmem;

	/* Alocate shared memory for reading from deamon/dsp */
	stat = omap3epfb_alloc_shmem(info,&par->subfq.shrd);
	if (stat)
		goto free_bfbquemem;



	/* Alocate shared memory for writing from deamon/dsp */
	stat = omap3epfb_alloc_shmem(info,&par->subfq.shwr);
	if (stat)
		goto free_shrdmem;

	/* Alocate shared memory for writing from deamon/dsp */
	stat = omap3epfb_alloc_shmem(info,&par->subfq.shstats);
	if (stat)
		goto free_shwrmem;

#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	par->subfq.shwr.p->wq.last_subf_queued = true;

	stat = omap3epfb_alloc_buffer(info, &par->subfq.idlesubf.sbf,
			omap3epfb_subfmem_size(info), MAX_PREOFFSET_LINES);
	if (stat)
		goto free_shstatsmem;

	/* IDLE state is encoded with zeros */
	memset(par->subfq.idlesubf.sbf.virt, 0, omap3epfb_subfmem_size(info));
	stat = omap3epfb_alloc_buffer(info, &par->subfq.idlesubf.hdr,
						HEADER_SIZE, 0);
	if (stat)
		goto free_idlesubf;

	memset(par->subfq.idlesubf.hdr.virt, 0, HEADER_SIZE);


	for (stat = i = 0; !stat && (i < OMAP3EPFB_SUBFQ_DEPTH); i++){
		stat = omap3epfb_alloc_buffer(info, &par->subfq.bufs[i].sbf,
						omap3epfb_subfmem_size(info),
						MAX_PREOFFSET_LINES);
		memset(par->subfq.bufs[i].sbf.virt, 0,
						omap3epfb_subfmem_size(info));
		stat = omap3epfb_alloc_buffer(info, &par->subfq.bufs[i].hdr,
						HEADER_SIZE, 0);
		memset(par->subfq.bufs[i].hdr.virt, 0, HEADER_SIZE);
	}
#endif /* defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS) */
	if (!stat)
		goto out;   /* success */

	dev_dbg(info->device, "%s: stat=%d\n", __func__, stat);
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	/* rewind allocation */
	while (--i >= 0) {
		omap3epfb_free_buffer(info, &par->subfq.bufs[i].sbf, MAX_PREOFFSET_LINES);
		omap3epfb_free_buffer(info, &par->subfq.bufs[i].hdr, 0);
	}
	/* free_idlesubf: */
	omap3epfb_free_buffer(info, &par->subfq.idlesubf.hdr, 0);
free_idlesubf:
	omap3epfb_free_buffer(info, &par->subfq.idlesubf.sbf, MAX_PREOFFSET_LINES);

free_shstatsmem:
#endif
	omap3epfb_free_shmem(info,&par->subfq.shstats);

free_shwrmem:
	omap3epfb_free_shmem(info,&par->subfq.shwr);
	
free_shrdmem:
	omap3epfb_free_shmem(info,&par->subfq.shrd);
	
free_bfbquemem:
	omap3epfb_free_buffer(info, &par->bfb.que.mem, 0);

free_bfbmem:
	omap3epfb_free_buffer(info, &par->bfb.mem, 0);

free_videomem:
	omap3epfb_free_buffer(info, &par->vmem, 0);

out:
	return stat;
}

void omap3epfb_free_buffers(struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	int i;

	i = OMAP3EPFB_SUBFQ_DEPTH;
	while (--i >= 0) {
		omap3epfb_free_buffer(info, &par->subfq.bufs[i].sbf, MAX_PREOFFSET_LINES);
		omap3epfb_free_buffer(info, &par->subfq.bufs[i].hdr, 0);
	}

	omap3epfb_free_buffer(info, &par->subfq.idlesubf.hdr, 0);
	omap3epfb_free_buffer(info, &par->subfq.idlesubf.sbf, MAX_PREOFFSET_LINES);
#endif
	omap3epfb_free_shmem(info,&par->subfq.shstats);
	omap3epfb_free_shmem(info,&par->subfq.shwr);
	omap3epfb_free_shmem(info,&par->subfq.shrd);

	omap3epfb_free_buffer(info, &par->bfb.que.mem, 0);
	omap3epfb_free_buffer(info, &par->bfb.mem, 0);
	omap3epfb_free_buffer(info, &par->vmem, 0);
}

#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
/* ------------------------------------------------------------------------- */

#define SUBFQ_INCREMENT_INDEX(QI)	\
		do { (QI) = ((QI) + 1) & ((SUBFQ_INDEX_MASK<<1)|1); } while(0)

/*
 * Get (peek at) an existing item from the queue.
 *
 * WARNING: User must have ensured that queue is NOT empty before calling
 * this function.
 */
static struct omap3epfb_subfbuf *subf_consumer_get(struct omap3epfb_par *par)
{
	struct omap3epfb_subfbuf *buf;

	buf = &par->subfq.bufs[par->subfq.rpi & SUBFQ_INDEX_MASK];
//printk("%s wi=%d rpi=%d\n",__func__,par->subfq.shwr.p->wq.wi & SUBFQ_INDEX_MASK,par->subfq.rpi& SUBFQ_INDEX_MASK);
	mb();	/* ensure subfq.wi is in sync */
	if (likely(par->subfq.rpi != par->subfq.shwr.p->wq.wi))
		SUBFQ_INCREMENT_INDEX(par->subfq.rpi);
	else
		BUG();

	return buf;
}

/*
 * Remove the oldest item from the subframe queue (acknowledge the pop).
 *
 * WARNING: This function must be called at least as many times as
 * subf_consumer_get() has been called, BUT NEVER MORE!
 */
static void subf_consumer_dequeue(struct omap3epfb_par *par)
{
	mb();	/* ensure subfq.wi is in sync */
//printk("%s rai=%d rpi=%d\n",__func__,par->subfq.shrd.p->rq.rai & SUBFQ_INDEX_MASK,par->subfq.rpi& SUBFQ_INDEX_MASK);
	if (likely(par->subfq.shrd.p->rq.rai != par->subfq.rpi))
		SUBFQ_INCREMENT_INDEX(par->subfq.shrd.p->rq.rai);
	else
		BUG();
}


/*
 * Return true if the next buffer to be dequeued (NOT peeked!) has the "last"
 * flag set.
 */
static bool subf_consumer_lastbuf_ontop(struct omap3epfb_par *par)
{
//	return par->subfq.bufs[par->subfq.shrd.p->rq.rai & SUBFQ_INDEX_MASK].last;
	return par->subfq.shwr.p->wq.bufs[par->subfq.shrd.p->rq.rai & SUBFQ_INDEX_MASK].last;
}


static size_t subf_queue_size(struct omap3epfb_par *par)
{
 	size_t sz;
 
 	mb();	/* ensure subfq.rXi and subfq.wi are in sync */
 	sz = (par->subfq.shwr.p->wq.wi - par->subfq.shrd.p->rq.rai) & ((SUBFQ_INDEX_MASK<<1)|1);
 	BUG_ON(sz > OMAP3EPFB_SUBFQ_DEPTH);
 
 	return sz;
}


static bool subf_queue_full(struct omap3epfb_par *par)
{
 	return subf_queue_size(par) == OMAP3EPFB_SUBFQ_DEPTH;
}


/*
 * Return true if no subframes are in the queue, even non-acknowledged ones.
 */
static inline bool subf_queue_empty(struct omap3epfb_par *par)
{
	mb();	/* ensure subfq.rai and subfq.wi are in sync */
	return par->subfq.shwr.p->wq.wi == par->subfq.shrd.p->rq.rai;
}


/* Return true if no more subf_consumer_get() are possible. */
static inline bool subf_queue_exhausted(struct omap3epfb_par *par)
{
	mb();	/* ensure subfq.rpi and subfq.wi are in sync */
	return par->subfq.shwr.p->wq.wi == par->subfq.rpi;
}

int subf_producer_get(struct omap3epfb_par *par)
{
	int i=-1;

	mb();
	if(!subf_queue_full(par))
		i = par->subfq.shwr.p->wq.wi & SUBFQ_INDEX_MASK;
	return i;
}

void subf_producer_queue(struct omap3epfb_par *par,int last)
{
	mb();
	BUG_ON(subf_queue_full(par));

	if(par->epd_fixpar.panel_type == OMAP3EPFB_PANELTYPE_AUO)
	{
		*((int*)par->subfq.bufs[par->subfq.shwr.p->wq.wi & SUBFQ_INDEX_MASK].hdr.virt) = last >> 4;
	}
	last &= 0x0000000F;
	par->subfq.shwr.p->wq.bufs[par->subfq.shwr.p->wq.wi & SUBFQ_INDEX_MASK].last = last;
	if(last)
		par->subfq.shwr.p->wq.last_subf_queued = last;
	SUBFQ_INCREMENT_INDEX(par->subfq.shwr.p->wq.wi);
}

/* ------------------------------------------------------------------------- */

void omap3epfb_newsubframe_inthandler(void *irqpar)
{
	struct fb_info *info = irqpar;
	struct omap3epfb_par *par = info->par;
	unsigned long offset;

	mb();

	/* Force a lag of two subframes. The reason is that two
	 * subframes are occupied at a time - the one showing
	 * right now, and the one queued in the DSS shadow registers.
	 */
	if (unlikely(par->subfq.first_irq_req))
		par->subfq.first_irq_req = false;
	else
		subf_consumer_dequeue(par);

	if (unlikely(subf_consumer_lastbuf_ontop(par))) {
		/* DSS is processing the last subframe, so request
		 * LCD output to turn itself off after it's sent.
		 */
		omap3epdss_request_stop();
	} else {
		/* prepare the next subframe (the one after the current) */
		struct omap3epfb_subfbuf *buf;

		if (unlikely(subf_queue_exhausted(par))) {
			/*omap3epfb_screen_sequence_task
			 * Very, very bad, but alleviate the situation by
			 * inserting an all-idle subframe.
			 */
			buf = &par->subfq.idlesubf;

#if defined(OMAP3EPFB_GATHER_STATISTICS)
			par->num_missed_subframes++;
#endif

			/* no real subframe queued, so prevent the next
			 * dequeue operation by faking the first_irq flag
			 */
			par->subfq.first_irq_req = true;
		} else {
			buf = subf_consumer_get(par);
		}

		/* load the shadow Base Address registers with the
		 * next subframe
		 *
		 * buf->sbf.phys_lines contains the maximum lines offset
		 * as specified by MAX_PREOFFSET_LINES. Correct it once
		 * again with one from dss_timings as should be
		 */
		offset = omap3epfb_get_dss_offset(par);
		omap3epdss_request_video_bufchange(buf->sbf.phys_lines + offset);
		/*set next state VCOM for auo display*/
		if (par->epd_fixpar.panel_type == OMAP3EPFB_PANELTYPE_AUO){
			pmic_set_dvcom(par->pwr_sess, ((int*) buf->hdr.virt)[0]);
		}
	}

	/* inform the worker for the free buffer */
//	wmb();
//	wake_up(&par->subfq.waitfree);
}

void omap3epfb_framedone_inthandler(void *irqpar)
{
	struct fb_info *info = irqpar;
	struct omap3epfb_par *par = info->par;

	/* last subframe sent, so remove it from queue */
	subf_consumer_dequeue(par);

//	/* all is done */
//	BUG_ON(!subf_queue_empty(par));

	/* omap3epdss_stop(); TODO - think it over */

	/* wake up worker */
	par->subfq.sequence_finished = true;
	wmb();
	wake_up(&par->subfq.waitframe);
}

/* ------------------------------------------------------------------------- */

static void omap3epfb_max_opps_request(struct fb_info *info)
{
	int i;
	i = resource_request("vdd1_opp",info->device, MAX_VDD1_OPP);
	BUG_ON(i);
	omap_pm_set_min_bus_tput(info->device, OCP_INITIATOR_AGENT,
						166 * 1000 * 4);
}

static void omap3epfb_max_opps_release(struct fb_info *info)
{
	omap_pm_set_min_bus_tput(info->device, OCP_INITIATOR_AGENT, 0);
	resource_release("vdd1_opp", info->device);
}

static void omap3epfb_screen_sequence_task(struct work_struct *work)
{
	struct omap3epfb_par *par;
	struct fb_info *info;
	struct omap3epfb_subfbuf *buf;
	int stat;
	long offset;

	par = container_of(work, struct omap3epfb_par, subfq.ss_work);
	info = par->info;

	BUG_ON((void*)par != info->par);
	BUG_ON(&par->subfq.ss_work != work);

	if (par->hwstate != OMAP3EPFB_HW_OK)
		goto out;

	omap3epfb_max_opps_request(info);

	/* initialize the subframe queue state */
	par->subfq.first_irq_req = true;
	par->subfq.sequence_finished = false;

	/* prepare DSS for operation */
	stat = omap3epdss_prepare_start(info,
					par->mode.pxres,
					par->mode.pyres,
					par->mode.f_pixclk_khz,
					par->epd_fixpar.panel_type);
	if (stat) {
		dev_err(info->device, "DSS prepstart failed (err=%d)\n", stat);
		omap3epdss_stop(info);
		return;
	}

	if (!par->pwr_sess->powered) {
		int t;
		dev_dbg(info->device, "PMIC looks unpowered - triggering temp acquisition\n");
		stat = pmic_get_temperature(par->pwr_sess, &t);
		if (stat == 0)
			par->subfq.shrd.p->rq.temperature = t;
		else if (stat == -ENOSYS)
			/* some PMIC backends lack a temperature sensor */
			stat = 0;
		else
			dev_err(info->device,
					"error %d while reading temperature\n",
						stat);
	}

	if (pmic_req_powerup(par->pwr_sess)) {
		/*
		 * The request returns error only if the power down
		 * acknowledge timed out.
		 */
		par->hwstate = OMAP3EPFB_HW_POWERDOWN_TIMEOUT;
		dev_err(info->device, "pmic power down timeout\n");
		goto out;
	}

	/* ensure power is OK before firing up DSS */
	if (pmic_sync_powerup(par->pwr_sess)) {
		par->hwstate = OMAP3EPFB_HW_POWERUP_TIMEOUT;
		dev_err(info->device, "pmic power up timeout\n");
		goto out;
	}

#if defined(OMAP3EPFB_GATHER_STATISTICS)
	par->num_missed_subframes = 0;
#endif

	/* tell DSS about the current buffer */
	buf = subf_consumer_get(par);
	offset = omap3epfb_get_dss_offset(par);
	omap3epdss_request_video_bufchange(buf->sbf.phys_lines + offset);
	/*set next state VCOM for auo display*/
	if (par->epd_fixpar.panel_type == OMAP3EPFB_PANELTYPE_AUO)
		pmic_set_dvcom(par->pwr_sess, ((int*)buf->hdr.virt)[0]);

	/* dev_dbg(info->device, "starting DSS...\n"); */

	/* enable interrupts - IRQ handler is the consumer from now on */
	mb();
	omap3epdss_start(info);

	/* wait DSS to finish with the last subframe
	 * TODO - should this be interruptible?
	 */
	wait_event(par->subfq.waitframe, par->subfq.sequence_finished);

	/* stop DSS and disable DSS interrupts */
	omap3epdss_stop(info);

#if defined(OMAP3EPFB_GATHER_STATISTICS)
	mb();
	if (test_bit(UPD_DSS_STARTING, &par->stamp_mask))
		par->stamp_mask = 0;

	mutex_lock(&par->stats_mutex);
	if (unlikely(par->num_missed_subframes)) {
		par->stats.total_missed_subf += par->num_missed_subframes;
		par->stats.max_missed_subf = max(
						par->num_missed_subframes,
						par->stats.max_missed_subf);

		dev_err(info->device,
				"%d missed subframes (tot=%d, max=%d)\n",
				par->num_missed_subframes,
				par->stats.total_missed_subf,
				par->stats.max_missed_subf);
	}
	par->stats.num_screen_seqs++;
	mutex_unlock(&par->stats_mutex);
#endif

#if defined(OMAP3EPFB_GATHER_STATISTICS)
	/* reset here so that sleek-awake workaround doesn't get confused */
	par->num_missed_subframes = 0;
#endif

	BUG_ON(waitqueue_active(&par->subfq.waitframe));

	/* we don't need display power anymore */
	pmic_release_powerup_req(par->pwr_sess);

	/* try to send sleep to dsp */
	omap3epfb_send_dsp_pm(info, true, NULL);
	

out:
	omap3epfb_max_opps_release(info);

	/* we expect the screen mutex to be locked */
	BUG_ON(down_trylock(&par->screen_update_mutex) == 0);
	/* mark screen as updated */
	up(&par->screen_update_mutex);
}


int omap3epfb_run_screen_sequence(struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;
	int stat;
	mb();
	if (unlikely(par->hwstate != OMAP3EPFB_HW_OK))
		return -EIO;

	mb();
//	if (par->subfq.shwr.p->wq.last_subf_queued)
	{
		/*
		* Screen sequence has been or is about to be terminated,
		* so restart.
		*/
		down(&par->screen_update_mutex);
		/* ensure the work thread has finished (this avoids
		* a race condition between up() and thread finish, which
		* might cause queue_work() to erroneously not run an
		* already running work/thread)
		*/
		flush_workqueue(par->subfq.ss_workq);

		par->dsp_pm_sleep = false;

		par->subfq.shwr.p->wq.last_subf_queued = false;

		/* run the screen sequence */
		BUG_ON(down_trylock(&par->screen_update_mutex) == 0);

		wmb();

		BUG_ON(!par->subfq.ss_workq);
		BUG_ON(work_pending(&par->subfq.ss_work));

		stat = queue_work(par->subfq.ss_workq, &par->subfq.ss_work);

		/* if work has already been queued then screen mutex is not working */
		BUG_ON(!stat);
	}
	return 0;
}

/*
 * Return true if in subframes queue have begin subframe.
 */
static bool subf_queue_begin_inque(struct omap3epfb_par *par)
{
	int r/*,w*/;
	bool b = false;
mb();
	r = par->subfq.shrd.p->rq.rai;
//	w = par->subfq.shwr.p->wq.wi;
	while(r!=par->subfq.shwr.p->wq.wi) {
		if( (((int*) par->subfq.bufs[r&SUBFQ_INDEX_MASK].hdr.virt)[1]) == SUBFRAME_SECOND) {
			(((int*) par->subfq.bufs[r&SUBFQ_INDEX_MASK].hdr.virt)[1]) = SUBFRAME_UNKN;
			b = true;
			break;
		}
		SUBFQ_INCREMENT_INDEX(r);
		msleep(5);
	}
	return b;
}

static void omap3epfb_check_dsp_work_task(struct work_struct *work)
{
	struct omap3epfb_par *par;
	struct fb_info *info;
	int resp;

	par = container_of(work, struct omap3epfb_par, subfq.dspck_work);
	info = par->info;

	BUG_ON((void*)par != info->par);
	BUG_ON(&par->subfq.dspck_work != work);

	while(subf_queue_empty(par)) {
		mb();
		resp = par->subfq.shwr.p->wq.bq_dsp2arm_last_resp - par->subfq.shrd.p->rq.bq_arm2dsp_last_req;
		if(resp == ARM_DSP_WORK_CODE_NOSFSEQ) {
			return;
		}
		msleep(5);
	}

	do {
		mb();
		resp = par->subfq.shwr.p->wq.bq_dsp2arm_last_resp - par->subfq.shrd.p->rq.bq_arm2dsp_last_req;
		if(subf_queue_begin_inque(par) ) {

#if defined(OMAP3EPFB_GATHER_STATISTICS)
			omap3epfb_event_timestamp(par, UPD_DSS_STARTING);
			omap3epfb_store_latencies(par);
#endif
			omap3epfb_run_screen_sequence(info);
			break;
		}

		msleep(5);

	} while(resp < 0);
}

#elif defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_GPMC)

static void omap3epfb_check_dsp_work_task(struct work_struct *work)
{
	struct omap3epfb_par *par;
	struct fb_info *info;

	par = container_of(work, struct omap3epfb_par, subfq.dspck_work);
	info = par->info;

	BUG_ON((void*)par != info->par);
	BUG_ON(&par->subfq.dspck_work != work);

	if (par->hwstate != OMAP3EPFB_HW_OK)
		return;
	pr_debug("Wait DSP SN until working\n");
	down(&par->screen_update_mutex);
	{
		//wait DSP SN start sequence
		msleep(50);
		mb();

		//wait until DSP SN work
		while(par->subfq.shwr.p->wq.wi == 1){
			msleep(10);
		}
	}
	gpmc_clock_release(info);
	up(&par->screen_update_mutex);
	pr_debug("DSP SN end working.\n");
	pmic_release_powerup_req(par->pwr_sess);
}

int omap3epfb_pmic_powerup(struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;
	int stat;
	mb();

	if(down_trylock(&par->screen_update_mutex) == 0)
	{
		if (pmic_req_powerup(par->pwr_sess)) {
			/*
			 * The request returns error only if the power down
			 * acknowledge timed out.
			 */
			par->hwstate = OMAP3EPFB_HW_POWERDOWN_TIMEOUT;
			dev_err(info->device, "pmic power down timeout\n");
			return 1;
		}

		/* ensure power is OK before firing up DSS */
		if (pmic_sync_powerup(par->pwr_sess)) {
			par->hwstate = OMAP3EPFB_HW_POWERUP_TIMEOUT;
			dev_err(info->device, "pmic power up timeout\n");
			return 1;
		}

		up(&par->screen_update_mutex);
		gpmc_clock_request(info);
		stat = queue_work(par->subfq.dspck_workq, &par->subfq.dspck_work);

	}
	return 0;
}
#endif

/*
 * Request an area update. Inputs are not sanitized, physical coordinates
 * are used.
 */
static int omap3epfb_update_area_versatile(struct fb_info *info,
				const struct omap3epfb_update_area *src_area)
{
	struct omap3epfb_par *par = info->par;
	int stat = 0;
	unsigned int vsid;
	struct omap3epfb_update_area area;

	mb();
	if (!par->reg_daemon)
		return 0;

	area = *src_area;
	vsid = !!info->var.yoffset;

	/* use the upper 16bits of wvid for out-of-band
	 * messages to the rendering subsystem
	 */
	area.wvfid &= 0x0000ffffu;
	area.wvfid |= vsid << OMAP3EPFB_OOB_VSID;

#if 0
	/* the new request has been queued */
	if (par->subfq.shwr.p->wq.last_subf_queued) {
		/* ensure the temperature value in the
		 * shared memory is up-to-date */
		int t;
		stat = pmic_get_temperature(par->pwr_sess, &t);
		if (stat == 0)
			par->subfq.shrd.p->rq.temperature = t;
		else if (stat == -ENOSYS)
			/* some PMIC backends lack a temperature sensor */
			stat = 0;
		else
			dev_err(info->device,
					"error %d while reading temperature\n",
						stat);
	}
#endif
	if (!stat) {
		stat = omap3epfb_reqq_push_back_async(info, &area);
	}

	return stat;
}

/* ------------------------------------------------------------------------- */

/* Update the whole screen. It uses an optimized subframe generation routine.
 *
 * WARNING: The process takes up to two seconds!
 */
int omap3epfb_update_screen(struct fb_info *info, int wvfid, bool retry_req)
{
	struct omap3epfb_par *par = info->par;
	int stat;
	struct omap3epfb_update_area ua;

	/* force a single global window */
	ua.x0 = ua.y0 = 0;
	ua.x1 = par->mode.vxres-1;
	ua.y1 = par->mode.vyres-1;
	ua.wvfid = wvfid;
	ua.threshold = OMAP3EPFB_THRESHOLD_GRAY;

	/* crude way of ensuring our update slips through */
	do {
		stat = omap3epfb_update_area_versatile(info, &ua);
		if (stat) {
			dev_dbg(info->device, "retrying screen update (%d)\n", stat);
			msleep(20);
		}
	} while (retry_req && ((stat == -EAGAIN) || (stat == -ENOBUFS)));

	return stat;
}

int omap3epfb_update_area(struct fb_info *info, struct omap3epfb_update_area *area)
{
	int stat;
	struct omap3epfb_par *par = info->par;

#if defined(OMAP3EPFB_GATHER_STATISTICS)
	mb();
	omap3epfb_start_timestamp(par);
#endif
	stat = omap3epfb_update_area_versatile(info,area);
#if 0
	dev_dbg(info->device, "  winupd: x=[%d:%d], y=[%d:%d] wvf=%d, st=%d\n",
			area->x0, area->x1, area->y0, area->y1, area->wvfid,
			stat);
#endif

	return stat;
}


bool omap3epfb_screen_is_flipping(struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;
	bool stat = true;

	/* TODO: think of more sane method for screen idle polling */
	if (down_trylock(&par->screen_update_mutex) == 0) {
		up(&par->screen_update_mutex);

		/* also check for empty queues */
		mb();
		if ((par->subfq.shrd.p->rq.bqwi == par->subfq.shwr.p->wq.bqri) &&
		    (par->reqq.wi == par->reqq.ri))
			stat = false;
	}

	return stat;
}

#if defined(CONFIG_FB_OMAP3EP_PGFLIP_DEFER)
static void omap3epfb_bursty_update_execute(struct work_struct *work)
{
	struct omap3epfb_par *par;
	bool retry;

	par = container_of(work, struct omap3epfb_par, subfq.bursty_work.work);

	BUG_ON(!par->subfq.bursty_alive);

	do {
		omap3epfb_update_screen(par->info, OMAP3EPFB_WVFID_AUTO, true);

		/*
		 * TODO, FIXME - RACE CONDITION: if a "page flip" request
		 * is scheduled here then AN UPDATE WILL BE LOST!!!
		 */

		spin_lock(&par->subfq.bursty_keepalive_lock);
		if (par->subfq.bursty_keepalive) {
			retry = true;
			par->subfq.bursty_keepalive = false;
		} else {
			retry = false;
			par->subfq.bursty_alive = false;
		}
		spin_unlock(&par->subfq.bursty_keepalive_lock);
	} while (retry);
}

int omap3epfb_update_screen_bursty(struct fb_info *info)
{
	const long timeout_ms = 100;
	const long timeout_jif = timeout_ms * HZ / 1000;
	struct omap3epfb_par *par = info->par;

	if (cancel_delayed_work(&par->subfq.bursty_work) == 1) {
		/* rearm */
		queue_delayed_work(par->subfq.bursty_workq,
					&par->subfq.bursty_work, timeout_jif);
	} else {
		bool rearm = false;

		spin_lock(&par->subfq.bursty_keepalive_lock);
		if (par->subfq.bursty_alive)
			par->subfq.bursty_keepalive = true;
		else {
			par->subfq.bursty_alive = true;
			rearm = true;
		}
		spin_unlock(&par->subfq.bursty_keepalive_lock);

		if (rearm)
			queue_delayed_work(par->subfq.bursty_workq,
					&par->subfq.bursty_work, timeout_jif);
	}

	return 0;
}
#elif defined(CONFIG_FB_OMAP3EP_PGFLIP_REPLACE)
int omap3epfb_update_screen_bursty(struct fb_info *info)
{
	int stat, i;

	/*
	 * Here Android is assumed to be using page flipping, and also
	 * to be the only user requesting screen updates. But reqq is
	 * also used for commands so we *cannot* guarantee that purging will
	 * leave space for the next screen request. The best we can do is
	 * try a few times and error out.
	 */
	omap3epfb_reqq_purge(info);
	for (stat = -EAGAIN, i = 5; stat && i > 0; i--) {
		stat = omap3epfb_update_screen(info,
					OMAP3EPFB_WVFID_AUTO, false);
		if (stat)
			msleep(1);
	}

	if ((i == 0) && stat)
		dev_err(info->device, "Could not post a page flip "
					"(stat=%d)\n", stat);

	return stat;
}
#endif

/* ------------------------------------------------------------------------- */

void omap3epfb_get_epd_fixpar(struct fb_info *info,
					struct omap3epfb_epd_fixpar *par)
{
	/* TODO - guard this by a mutex */
	struct omap3epfb_par *devpar = info->par;
	*par = devpar->epd_fixpar;
}


void omap3epfb_get_epd_varpar(struct fb_info *info,
					struct omap3epfb_epd_varpar *par)
{
	/* TODO - guard this by a mutex */
	struct omap3epfb_par *devpar = info->par;
	*par = devpar->epd_varpar;

	if (info->fbdefio->delay > 0)
		par->defio_timeout = (info->fbdefio->delay*1000 + HZ/2)/HZ;
	else
		par->defio_timeout = -1;
	dev_dbg(info->device, "fbdefio->delay = %ld\n", par->defio_timeout);
}


int omap3epfb_set_epd_varpar(struct fb_info *info,
					const struct omap3epfb_epd_varpar *par)
{
	/* TODO - guard this by a mutex */
	struct omap3epfb_par *devpar = info->par;
	devpar->epd_varpar = *par;

	if (par->defio_timeout > 0)
		fb_deferred_io_set_delay(info, (par->defio_timeout*HZ + 500)/1000);
	else
		fb_deferred_io_set_delay(info, -1);
	return 0;
}

int omap3epfb_get_statistics(struct fb_info *info,
					struct omap3epfb_statistics *stats)
{
#if defined(OMAP3EPFB_GATHER_STATISTICS)
	struct omap3epfb_par *par = info->par;

	mutex_lock(&par->stats_mutex);
	*stats = par->stats;
	mutex_unlock(&par->stats_mutex);

	return 0;
#else
	memset(stats, 0, sizeof(*stats));
	return -ENOSYS;
#endif
}


int omap3epfb_reset_statistics(struct fb_info *info)
{
#if defined(OMAP3EPFB_GATHER_STATISTICS)
	struct omap3epfb_par *par = info->par;

	mutex_lock(&par->stats_mutex);
	memset(&par->stats, 0, sizeof(par->stats));
	mutex_unlock(&par->stats_mutex);

	return 0;
#else
	return -ENOSYS;
#endif
}

/* ------------------------------------------------------------------------- */

void omap3epfb_init_subfq_state(struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	par->subfq.rpi /*= par->subfq.rai = par->subfq.wi*/ = 0;
//	par->subfq.last_subf_queued = true;
//	init_waitqueue_head(&par->subfq.waitfree);
	init_waitqueue_head(&par->subfq.waitframe);

#if defined(OMAP3EPFB_GATHER_STATISTICS)
	memset(&par->stats, 0, sizeof(par->stats));
	mutex_init(&par->stats_mutex);
#endif
	INIT_WORK(&par->subfq.ss_work, omap3epfb_screen_sequence_task);
#endif

	INIT_WORK(&par->subfq.dspck_work, omap3epfb_check_dsp_work_task);

	init_MUTEX(&par->screen_update_mutex);



	par->hwstate = OMAP3EPFB_HW_OK;
#if defined(CONFIG_FB_OMAP3EP_PGFLIP_DEFER)
	par->subfq.bursty_alive = false;
	par->subfq.bursty_keepalive = false;

	spin_lock_init(&par->subfq.bursty_keepalive_lock);
#endif
	/* initialize the EPD-related parameters */
	/*par->epd_fixpar.rotation = (par->mode.vxres == par->mode.pxres) ? 0 : 90;*/

	par->epd_varpar.autowvf_gu_max_changed_pixels = par->mode.vxres * par->mode.vyres / 3;
}


int omap3epfb_create_screenupdate_workqueue(struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;
#if defined(CONFIG_FB_OMAP3EP_PGFLIP_DEFER)
	INIT_DELAYED_WORK(&par->subfq.bursty_work,
					omap3epfb_bursty_update_execute);
#endif
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	par->subfq.ss_workq = create_rt_workqueue("epd-su");

	if (!par->subfq.ss_workq)
		return -ENOMEM;
#endif

	par->subfq.dspck_workq = create_rt_workqueue("epd-dspck");

	if (!par->subfq.dspck_workq) {
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
		destroy_workqueue(par->subfq.ss_workq);
#endif
		return -ENOMEM;
	}

#if defined(CONFIG_FB_OMAP3EP_PGFLIP_DEFER)
	par->subfq.bursty_workq = create_workqueue("epd-bursty");
	if (!par->subfq.bursty_workq) {
#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
		destroy_workqueue(par->subfq.ss_workq);
#endif
		destroy_workqueue(par->subfq.dsp_workq);
		return -ENOMEM;
	}
#endif

	return 0;
}


void omap3epfb_destroy_screenupdate_workqueue(struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;

	BUG_ON(!par->subfq.ss_workq);
#if defined(CONFIG_FB_OMAP3EP_PGFLIP_DEFER)
	cancel_delayed_work_sync(&par->subfq.bursty_work);
#endif

#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	destroy_workqueue(par->subfq.ss_workq);
#endif

	destroy_workqueue(par->subfq.dspck_workq);

#if defined(CONFIG_FB_OMAP3EP_PGFLIP_DEFER)
	destroy_workqueue(par->subfq.bursty_workq);
#endif

#if defined(CONFIG_FB_OMAP3EP_DRIVE_EPAPER_PANEL_DSS)
	par->subfq.ss_workq = 0;
#endif

	par->subfq.dspck_workq = 0;

#if defined(CONFIG_FB_OMAP3EP_PGFLIP_DEFER)
	par->subfq.bursty_workq = 0;
#endif
}

/* Blitter FB Functions*/

int omap3epfb_bfb_init(struct fb_info *info)
{
	struct omap3epfb_par *par = info->par;



	par->bfb.que.head = par->bfb.que.mem.virt;
	par->subfq.shwr.p->wq.bqri = par->subfq.shrd.p->rq.bqwi = 0;
	par->subfq.shrd.p->rq.bq_arm2dsp_last_req = 0x00;
	spin_lock_init(&par->bfb.que.xi_lock);
	spin_lock_init(&par->bfb.que.pm_lock);
	par->subfq.shrd.p->rq.bfb_queue_depth = OMAP3EPFB_BLITQ_DEPTH;

	return 0;
}




#define OMAP3EPFB_BLITQ_ITEM_IMASK	(OMAP3EPFB_BLITQ_DEPTH-1u)

static int omap3epfb_bfb_push_async(struct fb_info *info,
					const struct omap3epfb_bfb_update_area *a,
					int sleep)
{
	struct omap3epfb_par *par = info->par;
	int st = 0;
	struct omap3epfb_bfb_queue_element *qe;
	static time_t prev_nobufs_time = -1;
	long temp_time;	/* ms */
	mb();
	BUG_ON(!a);

	spin_lock(&par->bfb.que.xi_lock);

	qe = par->bfb.que.head + (par->subfq.shrd.p->rq.bqwi & OMAP3EPFB_BLITQ_ITEM_IMASK);

	if (((par->subfq.shrd.p->rq.bqwi - par->subfq.shwr.p->wq.bqri)
							& ((OMAP3EPFB_BLITQ_ITEM_IMASK<<1)|1))
										== OMAP3EPFB_BLITQ_DEPTH) {
		st = -ENOBUFS;
		temp_time = CURRENT_TIME.tv_sec * 1000 + CURRENT_TIME.tv_nsec/1000000;
		if (prev_nobufs_time != -1) {
			if (temp_time - prev_nobufs_time >= FULL_QUEUE_PERIOD
					|| par->num_missed_subframes >= SLEEPAWAKE_MISSED_THRESHOLD) {
				pr_info("Not removed request from BQ from dsp for %ldms\n ",
					temp_time - prev_nobufs_time);
				omap3epfb_send_recovery_signal();
				prev_nobufs_time = temp_time;
			}
		} else
			prev_nobufs_time = temp_time;
	} else {
		prev_nobufs_time = -1;
		//increment request id lower 4 bits is status
		par->subfq.shrd.p->rq.bq_arm2dsp_last_req += ARM_DSP_WORK_CODE_INCR;

		qe->x0 = a->x0;
		qe->x1 = a->x1;
		qe->y0 = a->y0;
		qe->y1 = a->y1;
		qe->bq_arm2dsp_req = par->subfq.shrd.p->rq.bq_arm2dsp_last_req;
		qe->sleep = sleep;

		par->subfq.shrd.p->rq.bqwi =
				((par->subfq.shrd.p->rq.bqwi) + 1) &
								((OMAP3EPFB_BLITQ_ITEM_IMASK<<1)|1);

	}

	spin_unlock(&par->bfb.que.xi_lock);
	return st;
}


/**
* DSP sleep sequence:
* On @prepare send SLEEP to queue and check if DSP is still running.
* If it is (most probably at the first @prepare) then reject @prepare.
* Then at the subsequent @prepare check again if DSP is running and
* return 0 only if it's in SLEEP.
* To avoid errors from random wake-up signals to DSP we have the
* same check on @suspend.
*/
int omap3epfb_send_dsp_pm(struct fb_info *info, bool sleep,
		struct omap3epfb_bfb_update_area *area)
{
	struct omap3epfb_par *par = info->par;
	int st = 0;
	struct omap3epfb_update_area area_d;
	struct omap3epfb_bfb_update_area ba;
	int dsp_running = par->subfq.shwr.p->wq.dsp_running_flag;

	mb();
	spin_lock(&par->bfb.que.pm_lock);

	if (par->reg_daemon) {
		/* use the upper 16bits of wvid for out-of-band
		 * messages to the rendering subsystem
		 */
		if (sleep) {
			if (par->dsp_pm_sleep == false) {
				/*area.wvfid = DSP_PM_SLEEP;
				 st = omap3epfb_reqq_push_back_async(info, &area); //send sleep through daemon*/
				ba.x0 = 0;
				ba.x1 = 0;
				ba.y0 = 0;
				ba.y1 = 0;
				/* send sleep through blitter queue */
				st = omap3epfb_bfb_push_async(info, &ba, 1);
				if(!st)
					par->dsp_pm_sleep = true;
		}

		/* check DSP */
		if (dsp_running)
		st = 1;
		else
		st = 0;
	} else {
		//awake
		if (par->dsp_pm_sleep) {
			if(area == NULL) {
				ba.x0 = 0;
				ba.x1 = 0;
				ba.y0 = 0;
				ba.y1 = 0;
				/* send emergency wake-up through blitter queue */
				st = omap3epfb_bfb_push_async(info, &ba, 0);
				pr_err("Awake called with NULL area!\n");
			} else {
				/* wake-up with actual passed area through blitter queue*/
				st = omap3epfb_bfb_push_async(info,area,0);
			}
			if(!st)
				par->dsp_pm_sleep = false;

			spin_unlock(&par->bfb.que.pm_lock);

			if(st) {
				pr_err("Error inserting an item while wake-up!\n");
				return st;
			}
			area_d.wvfid = DSP_PM_AWAKE;
#if 0
			st = pmic_get_temperature(par->pwr_sess, &t);
			if (st == 0)
			par->subfq.shrd.p->rq.temperature = t;
			else if (st == -ENOSYS)
			/* some PMIC backends lack a temperature sensor */
			st = 0;
			else
			dev_err(info->device,
					"error %d while reading temperature\n", st);
#endif
			/*wake-up through daemon*/
			st = omap3epfb_reqq_push_back_async(info, &area_d);
			if (st == -ENOBUFS) {
				pr_err("Error inserting a reqq item while wake-up, trying with alternate wake-up!\n");
				omap3epfb_send_recovery_signal();
				return st;
			}
			return st;
		} else {
			st = omap3epfb_bfb_push_async(info,area,0);
		}
	}
}
spin_unlock(&par->bfb.que.pm_lock);
return st;
}


int omap3epfb_bfb_update_area(struct fb_info *info, struct omap3epfb_bfb_update_area *area)
{
	struct omap3epfb_par *par = info->par;
	int st = 0,st1;
	
#if defined(OMAP3EPFB_GATHER_STATISTICS)
	mb();
	omap3epfb_event_timestamp(par, UPD_PRE_BLITTER);
#endif
	st = omap3epfb_send_dsp_pm(info,false,area);

#if 0
	pr_info( " bfbupd: x=[%d:%d], y=[%d:%d] , st=%d\n",
			area->x0, area->x1, area->y0, area->y1,st);
#endif

	if(!(area->x0&0xF0000000)) {
		if(!st) {

			st1 = queue_work(par->subfq.dspck_workq, &par->subfq.dspck_work);
		}
	}

	return st;
}

extern int omap3epfb_set_rotate(struct fb_info *info, int rotate)
{
	static int xres = -1;
	static int yres = -1;
	int st = 0;
	struct omap3epfb_par *par = info->par;

	if (par->epd_fixpar.rotation == rotate)
		return st;

	if (rotate == 0 || rotate == 90 || rotate == 180 || rotate == 270) {
		int diff_rotate = abs(par->epd_fixpar.rotation - rotate);
		if (xres == -1) {
			xres = info->var.xres;
			yres = info->var.yres;
		}
		/*
			change resolution only if really necessary
		*/
		if (diff_rotate == 90 || diff_rotate == 270) {
			int temp_xres = xres;
			int temp_yres = yres;

			info->var.xres = yres;
			info->var.yres = temp_xres;

			info->var.xres_virtual = info->var.xres;
			info->var.yres_virtual = 2 * info->var.yres;

			xres = info->var.xres;
			yres = info->var.yres;

			int temp_width = info->var.width;
			info->var.width = info->var.height;
			info->var.height = temp_width;

			if ((info->var.yoffset != 0) && (info->var.yoffset == temp_yres))
				info->var.yoffset = yres;
			else
				info->var.yoffset = 0;

			switch (par->mode.bpp) {
			case 0:
				info->fix.line_length = info->var.xres / 2;
				break;
			case 4:
				info->fix.line_length = info->var.xres;
				break;
			case 8:
				info->fix.line_length = info->var.xres;
				break;
			case 16:
				info->fix.line_length = 2 * info->var.xres;
				break;
			default:
				BUG();
			}
		}
		info->var.rotate = rotate;
			par->epd_fixpar.rotation = rotate;

		struct omap3epfb_update_area area;
			area.wvfid = EDP_ROTATE;

		mb();
		if (!par->reg_daemon)
			return 0;

#if 0
		/* the new request has been queued */
		if (par->subfq.shwr.p->wq.last_subf_queued) {
			/* ensure the temperature value in the
			* shared memory is up-to-date */
			int t;
			st = pmic_get_temperature(par->pwr_sess, &t);
			if (st == 0)
				par->subfq.shrd.p->rq.temperature = t;
			else if (st == -ENOSYS)
				/* some PMIC backends lack a temperature sensor */
				st = 0;
			else
				dev_err(info->device,
				    "error %d while reading temperature\n",
				    st);
		}
#endif

		if (!st) {
			st = omap3epfb_reqq_push_back_async(info, &area);
		}
	}

	return st;
}

