#include <linux/types.h>
#include <asm/io.h>
#include <mach/io.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <mach/irqs.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/version.h>

#include "halDSS.h"
#include "OMAP3430.h"

#include "dsi_pll_drv.h"

#define REG32_WR(addr, val) omap_writel(val, addr)
#define REG32_RD(addr) omap_readl(addr)

#define REG32_TEST_BIT(addr, bit)                    (REG32_RD(addr) & (u32)(1 << (bit)))
#define REG32_SET_BIT(addr, bit)                     (REG32_WR(addr, REG32_RD(addr) | (u32)(1 << (bit)) ))
#define REG32_CLR_BIT(addr, bit)                     (REG32_WR(addr, REG32_RD(addr) & (u32)~(1 << (bit)) ))
#define REG32_TGL_BIT(addr, bit)                     (REG32_WR(addr, REG32_RD(addr) ^ (u32)(1 << (bit)) ))
#define REG32_FIELD_MASK(field, width)               (u32)(~((~(0xFFFFFFFF << (width))) << (field)))
#define REG32_RD_FIELD(addr, field, width)           ((REG32_RD(addr) & ~REG32_FIELD_MASK(field, width)) >> (field))
#define REG32_WR_FIELD(addr, field, width, value)    (REG32_WR(addr, (u32)((REG32_FIELD_MASK(field, width) & REG32_RD(addr)) | ((value) << (field)))))

#define TRUE  (1)
#define FALSE (0)

#define HALPRCM_CONST_SYSCLK_FREQ       26000000
#define HALPRCM_CONST_SYSCLK_FREQ_MHZ   26

struct clk *dss_ick;// "dss_ick" },	/* L3 & L4 ick */
struct clk *dss1_fck;// "dss1_fck", "dss1_alwon_fck" },
struct clk *dss2_fck;// "dss2_fck", "dss2_alwon_fck" },
struct clk *dss_54m_fck;// "dss_54m_fck", "dss_tv_fck" },
struct clk *dss_96m_fck;// NULL, "dss_96m_fck" },

void line1_isr(void);
void vsync_isr(void);
void framedone_isr(void);

#define DISPC_IRQ_MASK (1 << DISPC_IRQ_BITPOS)
#define PROGRAMMEDLINENUMBER_MASK (1 << PROGRAMMEDLINENUMBER_BITPOS)
#define VSYNC_MASK (1 << VSYNC_BITPOS)
#define FRAMEDONE_MASK (1 << FRAMEDONE_BITPOS)

#define DISPCIRQ_WAKEUP_MASK		(1u << 16)
#define DISPCIRQ_SYNCLOSTDIGITAL	(1u << 15)
#define DISPCIRQ_SYNCLOST		(1u << 14)
#define DISPCIRQ_VID2ENDWINDOW		(1u << 13)
#define DISPCIRQ_VID2FIFOUNDERFLOW	(1u << 12)
#define DISPCIRQ_VID1ENDWINDOW		(1u << 11)
#define DISPCIRQ_VID1FIFOUNDERFLOW	(1u << 10)
#define DISPCIRQ_OCPERROR		(1u << 9)
#define DISPCIRQ_PALETTEGAMMALOADING	(1u << 8)
#define DISPCIRQ_GFXENDWINDOW		(1u << 7)
#define DISPCIRQ_GFXFIFOUNDERFLOW	(1u << 6)
#define DISPCIRQ_PROGRAMMEDLINENUMBER	(1u << 5)
#define DISPCIRQ_ACBIASCOUNTSTATUS	(1u << 4)
#define DISPCIRQ_EVSYNC_ODD		(1u << 3)
#define DISPCIRQ_EVSYNC_EVEN		(1u << 2)
#define DISPCIRQ_VSYNC			(1u << 1)
#define DISPCIRQ_FRAMEDONE		(1u << 0)

#define DISPCIRQ_ENABLED_MASK		( \
			DISPCIRQ_SYNCLOST | \
			DISPCIRQ_VID2FIFOUNDERFLOW | \
			DISPCIRQ_VID1FIFOUNDERFLOW | \
			DISPCIRQ_OCPERROR | \
			DISPCIRQ_GFXFIFOUNDERFLOW | \
			DISPCIRQ_VSYNC | \
			DISPCIRQ_FRAMEDONE \
					)

#if defined(CONFIG_MACH_OMAP3430_EDP1)
  #error "Please review the DSI PLL initialization for OMAP3430!"
#endif

irqreturn_t dss_isr(int irq, void *dontcare)
{
    static int frame_done_fired;

    u32 irqstatus = REG32_RD(HALREG32_DSS_IRQSTATUS_OFFSET + REGBASE_DSS);

    //printk(KERN_DEBUG "dss irq %08x\n", irqstatus);

    if (irqstatus & DISPC_IRQ_MASK)
    {
        u32 dispc_irqstatus = REG32_RD(HALREG32_DISPC_IRQSTATUS_OFFSET + REGBASE_DISPC);
    
	REG32_WR(HALREG32_DISPC_IRQSTATUS_OFFSET + REGBASE_DISPC, dispc_irqstatus);
	REG32_RD(HALREG32_DISPC_IRQSTATUS_OFFSET + REGBASE_DISPC);

        //printk(KERN_DEBUG "dispc irq %08x\n", dispc_irqstatus);        

	if (dispc_irqstatus & DISPCIRQ_ENABLED_MASK & ~(DISPCIRQ_VSYNC|DISPCIRQ_FRAMEDONE)) {
		printk(KERN_ERR "%s: IRQSTATUS=%08x\n", __func__, dispc_irqstatus);
	}

/*        if (dispc_irqstatus & VSYNC_MASK)
        {
            dispc_irqstatus &= ~VSYNC_MASK;

            vsync_isr();
        }        
*/
        if (dispc_irqstatus & FRAMEDONE_MASK)
        {
            dispc_irqstatus &= ~FRAMEDONE_MASK;
            frame_done_fired = 1;
	}

        if (dispc_irqstatus & VSYNC_MASK)
        {
            dispc_irqstatus &= ~VSYNC_MASK;

	    if (REG32_TEST_BIT(DISPC_CONTROL_OFFSET + DISPC_BASE, LCDENABLE_BITPOS)) {
		    if (!halDSS_check_go())
			line1_isr();
		    else
			printk(KERN_ERR "DSS: too long IRQs off!\n");
	    } else if (frame_done_fired) {
		framedone_isr();
                frame_done_fired = 0;
	    }
        }

        if (dispc_irqstatus != 0)
        {
            //printk(KERN_DEBUG "unhandled dispc irqstatus 0x%08x\n", dispc_irqstatus);
        }

        irqstatus &= ~DISPC_IRQ_MASK;
    }

    return IRQ_HANDLED;
}


/**
 *	\fn
 *	\brief
 *	\author Angel Gavrailov
 *	\param
 *	\return
 */
u8 halDDS_DSI_PLL_FreqSet (u16 aFreq)
{
    u32 tRegN, tRegM, tRegM3; /*Needed for DSI PLL setup*/
    u32 tHighFDiv;
    u32 tReg;
    u8 i, tRes;

    tRes = TRUE;

    REG32_CLR_BIT(HALREG32_DISPC_CONTROL_OFFSET + REGBASE_DISPC, LCDENABLE_BITPOS);         /*Disable DISPC*/

    tReg = REG32_RD(HALREG32_DSI_CTRL_OFFSET + REGBASE_DSI);
    if (tReg & (1<<IF_EN_BITPOS))                                                           /*Check if DSI interface is enabled*/
    {
        REG32_CLR_BIT(HALREG32_DSI_CTRL_OFFSET + REGBASE_DSI, IF_EN_BITPOS);                /*Disable the DSI I/F in order to modify the PLL power state*/
    }

    REG32_WR(HALREG32_DSI_CLK_CTRL_OFFSET + REGBASE_DSI, (0x03 << PLL_PWR_CMD_BITPOS));     /*Enable power of PLL and HSDIVIDER, no DSI PHY out*/
    mdelay(10);                                                                         /*Provide a short delay*/
    if ((REG32_RD(HALREG32_DSI_CLK_CTRL_OFFSET + REGBASE_DSI) & (0x03 << PLL_PWR_STATUS_BITPOS)) !=     /*Check power state*/
                                                                (0x03 << PLL_PWR_STATUS_BITPOS))
    {
        printk(KERN_ERR "halDDS_DSI_PLL_FreqSet: DSI PLL and HSDIVIDER power setting error.");
        tRes = FALSE;
    }

    if (tReg & (1<<IF_EN_BITPOS))                                               /*Check if DSI interface was enabled*/
    {
        REG32_SET_BIT(HALREG32_DSI_CTRL_OFFSET + REGBASE_DSI, IF_EN_BITPOS);    /*Restore the DSI I/F state*/
    }

    tRegN = (HALPRCM_CONST_SYSCLK_FREQ_MHZ - 1);    /*Independent of the resulting clock.*/
    tRegM = aFreq / 2;
    tHighFDiv = 0;
    tRegM3 = 0;

    if (tRes == TRUE)                                       /*Try to re-lock ONLY if the power state is GOOD*/
    {
        REG32_WR(HALREG32_DSI_PLL_CONTROL_OFFSET + REGBASE_DSI_PLL,
                 (1 << DSI_PLL_AUTOMODE_BITPOS)    |       /*Automatic update mode*/
                 (1 << DSI_PLL_GATEMODE_BITPOS)    |       /*Enable gating*/
                 (1 << DSI_PLL_HALTMODE_BITPOS));          /*Enable halt*/

        mdelay(1);

        REG32_WR(HALREG32_DSI_PLL_CONFIGURATION2_OFFSET + REGBASE_DSI_PLL,
                 (0x03 << DSI_PLL_FREQSEL_BITPOS)          |   /*Select 0.75-1.0MHz reference clock*/
                 (tHighFDiv << DSI_PLL_HIGHFREQ_BITPOS)    |   /*Select if HighFreq to be used*/
                 (0 << DSI_PLL_PLLLPMODE_BITPOS)           |   /*Use full-performance PLL mode*/
                 (0 << DSI_PLL_TIGHTPHASELOC_BITPOS)       |   /*Tight phase lock creiteria*/
                 (0 << DSI_PLL_DRIFTGUARDEN_BITPOS)        |   /*Drift guard enabled*/
                 (0 << DSI_PLL_LOCKSEL_BITPOS)             |   /*Phase lock criteria depends on DSI_PLL_TIGHTPHASELOCK setting*/
                 (0 << DSI_PLL_CLKSEL_BITPOS)              |   /*Select DSS2_ALWON_CLK as a Reference*/
                 (0 << DSI_PHY_CLKINEN_BITPOS)             |   /*Disable DSI PHY clk*/
                 (1 << DSI_PLL_REFEN_BITPOS)               |   /*Enable PLL ref clk*/
                 (1 << DSS_CLOCK_EN_BITPOS)                |   /*Enable DSS clock divider*/
                 (0 << DSI_PROTO_CLOCK_EN_BITPOS));            /*Disable DSI clock divider*/

        mdelay(1);

        REG32_WR(HALREG32_DSI_PLL_CONFIGURATION1_OFFSET + REGBASE_DSI_PLL,
                 (1      << DSI_PLL_STOPMODE_BITPOS)        | /*Stopmode enabled*/
                 (tRegN  << DSI_PLL_REGN_BITPOS)            | /*REGN Value*/
                 (tRegM  << DSI_PLL_REGM_BITPOS)            | /*REGM Value*/
                 (tRegM3 << DSS_CLOCK_DIV_BITPOS));           /*M3 Value*/

        mdelay(1);

        REG32_SET_BIT(HALREG32_DSI_PLL_GO_OFFSET + REGBASE_DSI_PLL, DSI_PLL_GO_BITPOS);  /*Start re-locking of the PLL*/
    }
    tRes = FALSE;
    for (i = 0; i < 50; i++)
    {
        mdelay(1);
        if (REG32_TEST_BIT(HALREG32_DSI_PLL_STATUS_OFFSET+REGBASE_DSI_PLL, DSI_PLL_LOCK_BITPOS) != 0)
        {
            tRes = TRUE;
            break;
        }
    }

    if (tRes == FALSE) {
        printk(KERN_ERR "ERROR: halDDS_DSI_PLL_FreqSet: DSI PLL lock timeout.\n");
    }

    REG32_WR(HALREG32_DSS_CONTROL_OFFSET + REGBASE_DSS,             /*Configure clock sources for DSS*/
             (1 << DISPC_CLK_SWITCH_BITPOS) |                       /*Select DSI PLL as a DISPC clock*/
             (0 << DSI_CLK_SWITCH_BITPOS));

    REG32_SET_BIT(HALREG32_DISPC_CONTROL_OFFSET + DISPC_BASE, GOLCD_BITPOS);                /* Set GOLCD bit to update configuration values */

    return 0;
}

int halDSS_Reset(void)
{
	int i;

	REG32_SET_BIT(HALREG32_DSS_SYSCONFIG_OFFSET + REGBASE_DSS, SOFTRESET_BITPOS);   /* Reset DSS module */

	for(i = 0; i < 100;  i++)                                                        /* Wait on DSS reset release */ {
		if (REG32_TEST_BIT(HALREG32_DSS_SYSSTATUS_OFFSET + REGBASE_DSS, RESETDONE_BITPOS) != 0) /*Reset completed*/
			return TRUE;
	}

	return FALSE;
}

/**
 *	\fn
 *	\brief
 *	\author Angel Gavrailov
 *	\param
 *	\return
 */
int halDSS_Init(void)
{
    u32 i;
    u8 tRes;
    int err;

    tRes = FALSE;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31)
    dss_ick = clk_get(NULL, "dss_ick");
    if (IS_ERR(dss_ick))
        printk(KERN_ERR "Can't get dss_ick.\n");
    
    dss1_fck = clk_get(NULL, "dss1_alwon_fck");
    if (IS_ERR(dss1_fck))
        printk(KERN_ERR "Can't get dss1_alwon_fck.\n");
    
    dss2_fck = clk_get(NULL, "dss2_alwon_fck");
    if (IS_ERR(dss2_fck))
        printk(KERN_ERR "Can't get dss2_alwon_fck.\n");
    
    dss_54m_fck = clk_get(NULL, "dss_tv_fck");
    if (IS_ERR(dss_54m_fck))
        printk(KERN_ERR "Can't get dss_tv_fck.\n");

    dss_96m_fck = clk_get(NULL, "dss_96m_fck");
    if (IS_ERR(dss_96m_fck))
        printk(KERN_ERR "Can't get dss_96m_fck.\n");
#else
    dss_ick = clk_get_sys(NULL, "ick");
    if (IS_ERR(dss_ick))
        printk(KERN_ERR "Can't get dss_ick.\n");
    
    dss1_fck = clk_get_sys(NULL, "dss1_fck");
    if (IS_ERR(dss1_fck))
        printk(KERN_ERR "Can't get dss1_fck.\n");
    
    dss2_fck = clk_get_sys(NULL, "dss2_fck");
    if (IS_ERR(dss2_fck))
        printk(KERN_ERR "Can't get dss2_fck.\n");
    
    dss_54m_fck = clk_get_sys(NULL, "tv_fck");
    if (IS_ERR(dss_54m_fck))
        printk(KERN_ERR "Can't get tv_fck.\n");

    dss_96m_fck = clk_get_sys(NULL, "video_fck");
    if (IS_ERR(dss_96m_fck))
        printk(KERN_ERR "Can't get video_fck.\n");
#endif
 
    clk_enable(dss_ick);
    clk_enable(dss1_fck);
    clk_enable(dss2_fck); 
    clk_enable(dss_54m_fck);
    clk_enable(dss_96m_fck);    

    pr_debug("dss_ick = %ld\n", clk_get_rate(dss_ick));
    pr_debug("dss1_fck = %ld\n", clk_get_rate(dss1_fck));
    pr_debug("dss2_fck = %ld\n", clk_get_rate(dss2_fck));
    pr_debug("dss_54m_fck = %ld\n", clk_get_rate(dss_54m_fck));
    pr_debug("dss_96m_fck = %ld\n", clk_get_rate(dss_96m_fck));

    tRes = halDSS_Reset();

    if (tRes == FALSE)                                                                      /*Reset did not release*/
    {
        printk(KERN_ERR "halDSS_Init: DSS reset not released.\n");
    }
    else
    {
        tRes = FALSE;
        REG32_SET_BIT(HALREG32_DISPC_SYSCONFIG_OFFSET + REGBASE_DISPC, SOFTRESET_BITPOS);   /* Reset DISPC module */
        for (i = 0; i < 10; i++)                                                            /* Wait on DISPC reset release */
        {
            if (REG32_TEST_BIT(HALREG32_DISPC_SYSSTATUS_OFFSET + REGBASE_DISPC, RESETDONE_BITPOS) !=0 ) /*Reset completed*/
            {
                tRes = TRUE;
                break;
            }
        }
        if (tRes == FALSE)                                                                  /*DISPC Reset did not release*/
        {
            printk(KERN_ERR "halDSS_Init: DISPC reset not released.\n");
        }
    }

    REG32_WR(HALREG32_DSS_CONTROL_OFFSET + REGBASE_DSS,                                     /*Configure clock sources for DSS*/
             (0 << DISPC_CLK_SWITCH_BITPOS) | (0 << DSI_CLK_SWITCH_BITPOS));


    /*Reset DSI protocol engine*/
    tRes = FALSE;
    REG32_SET_BIT(HALREG32_DSI_SYSCONFIG_OFFSET + REGBASE_DSI, SOFTRESET_BITPOS);
    for (i = 0; i < 10; i++)                                                                    /* Wait on DSI Prot engine reset release */
    {
        if (REG32_TEST_BIT(HALREG32_DSI_SYSSTATUS_OFFSET + REGBASE_DSI, RESETDONE_BITPOS) !=0 ) /*Reset completed*/
        {
            tRes = TRUE;
            break;
        }
    }
    if (tRes == FALSE)                                                                          /*DSI Reset did not release*/
    {
        printk(KERN_ERR "halDSS_Init: DSI reset not released.\n");
    }

    REG32_WR(HALREG32_DSI_SYSCONFIG_OFFSET + REGBASE_DSI, 0x310);

    err = dsi_pll_init();
    if (err) {
    	printk(KERN_ERR "halDSS_Init: DSI PLL initialization failed.\n");
    }
        

    REG32_WR(HALREG32_DISPC_LINE_NUMBER_OFFSET + REGBASE_DISPC, 100);
    REG32_WR(HALREG32_DISPC_IRQSTATUS_OFFSET + REGBASE_DISPC, DISPCIRQ_ENABLED_MASK); // clear all interrupts
    REG32_WR(HALREG32_DISPC_IRQENABLE_OFFSET + REGBASE_DISPC, DISPCIRQ_ENABLED_MASK);

    err = request_irq(INT_24XX_DSS_IRQ, dss_isr, IRQF_DISABLED, "dss-irq", NULL);
    if (err)
    {
        printk(KERN_ERR "cannot register irq handler\n");
    }    


//    REG32_CLR_BIT(HALREG32_FCLKEN_OFFSET + REGBASE_DSS_CM, EN_TV_BITPOS);           /* TV FCLK disable */

    return 0;
}

/**
 *	\fn
 *	\brief
 *	\author Angel Gavrailov
 *	\param
 *	\return
 */
int halDSS_DISPC_Config(int aPixClk,
                        int aHRes, int aVRes, int aTFTDataLines,
                        int aHBP, int aHFP, int aHPW,
                        int aVBP, int aVFP, int aVPW,
                        int aIsPclkInvert,
                        int aIsHsyncInvert, int aIsVsyncInvert)
{
    const u32 PixClkDiv = 2;
    u8 tTFTLinesVal;
    int st;

    switch (aTFTDataLines)
    {
        case 12:
            tTFTLinesVal = 0x00;
        break;
        case 16:
            tTFTLinesVal = 0x01;
        break;
        case 18:
            tTFTLinesVal = 0x02;
        break;
        case 24:
            tTFTLinesVal = 0x03;
        break;
        default:
            tTFTLinesVal = 0x03;
        break;
    }

    REG32_WR(HALREG32_DISPC_CONFIG_OFFSET + REGBASE_DISPC,
                        3            << LOADMODE_BITPOS);            /* Load color map on first frame, then load only frame data */

    REG32_WR(HALREG32_DISPC_CONTROL_OFFSET + REGBASE_DISPC,
                        1            << LCDENABLEPOL_BITPOS        | /* LCD enable signal polarity */
                        1            << LCDENABLESIGNAL_BITPOS     |
                        1            << PCKFREEENABLE_BITPOS       | /* Free running PCLK */
                        1            << GPOUT1_BITPOS              |
                        1            << GPOUT0_BITPOS              |
                        0            << OVERLAYOPTIMIZATION_BITPOS |
                        tTFTLinesVal << TFTDATALINES_BITPOS        |
                        1            << STNTFT_BITPOS);              /* LCD matrix type */

    REG32_WR(HALREG32_DISPC_DIVISOR_OFFSET + REGBASE_DISPC,
                        1            << LCD_BITPOS                 | /* Display logic clock divider (LCDCLK = FCLK/x) */
                        PixClkDiv    << PCD_BITPOS);                 /* PCLK divider (PCLK = LCDCLK/x). Values 0 and 1 are invalid */

    REG32_WR(HALREG32_DISPC_POL_FREQ_OFFSET + REGBASE_DISPC,
                        1              << ONOFF_BITPOS             | /* SYNC signals are driven: (1)according RF bit/(0)on opposite PCLK edge than data */
                        0              << RF_BITPOS                | /* If ONOFF is 1, SYNC signals are driven on (1)positive/(0)negative PCLK edge */
                        aIsPclkInvert  << IPC_BITPOS               | /* Invert PCLK */
                        aIsHsyncInvert << IHS_BITPOS               | /* Invert HSYNC */
                        aIsHsyncInvert << IVS_BITPOS);               /* Invert VSYNC */

    REG32_WR(HALREG32_DISPC_SIZE_LCD_OFFSET + REGBASE_DISPC,
                        (aVRes-1) << LPP_BITPOS                    |
                        (aHRes-1) << PPL_BITPOS);                    /* Must be a multiple of 8 */

    REG32_WR(HALREG32_DISPC_TIMING_H_OFFSET + REGBASE_DISPC,
                        (aHBP-1) << HBP_BITPOS                     | /* LCD Horizontal Back Porch */
                        (aHFP-1) << HFP_BITPOS                     | /* LCD Horizontal Front Porch */
                        (aHPW-1) << HSW_BITPOS);                     /* LCD HSYNC Width */

    REG32_WR(HALREG32_DISPC_TIMING_V_OFFSET + REGBASE_DISPC,
                        (aVBP)   << VBP_BITPOS                     | /* LCD Vertical Back Porch */
                        (aVFP)   << VFP_BITPOS                     | /* LCD Vertical Front Porch */
                        (aVPW-1) << VSW_BITPOS);                     /* LCD VSYNC Width */

    REG32_WR(HALREG32_DISPC_DEFAULT_COLOR_m_OFFSET(0) + REGBASE_DISPC, HALDSS_CONST_DEF_BKCOLOR << DEFAULTCOLOR_BITPOS);  /* LCD Background color */
    REG32_WR(HALREG32_DISPC_TRANS_COLOR_m_OFFSET(0)   + REGBASE_DISPC, HALDSS_CONST_DEF_TRCOLOR << TRANSCOLORKEY_BITPOS); /* Transparent color */

    REG32_SET_BIT(DISPC_CONTROL_OFFSET + DISPC_BASE, GOLCD_BITPOS);     /* Set GOLCD bit to update configuration values */

    st = dsi_pll_set_clk(aPixClk * PixClkDiv);
    if (st) {
    	printk(KERN_ERR "dsi_pll_set_clk: DSI PLL lock error!\n");
    }

    st = dsi_pll_enable_dispc_clk(1);
    if (st) {
    	printk(KERN_ERR "dsi_pll_enable_dispc_clk: DISPC clock divider set error!\n");
    }

    return TRUE;
}

void halDSS_Cleanup(void)
{
    clk_disable(dss_ick);
    clk_disable(dss1_fck);
    clk_disable(dss2_fck); 
    clk_disable(dss_54m_fck);
    clk_disable(dss_96m_fck);

    clk_put(dss_ick);
    clk_put(dss1_fck);
    clk_put(dss2_fck);
    clk_put(dss_54m_fck);
    clk_put(dss_96m_fck);

    dsi_pll_cleanup();

    free_irq(INT_24XX_DSS_IRQ, NULL);
}

void halDSS_DISPC_SetColorMap(u32 *ColorMapAddr)
{
    REG32_WR(HALREG32_DISPC_GFX_TABLE_BA_OFFSET + DISPC_BASE, (u32)ColorMapAddr);
}

void LCD_enable(void)
{
    REG32_SET_BIT(DISPC_CONTROL_OFFSET + DISPC_BASE, LCDENABLE_BITPOS);
}

void LCD_disable(void)
{
    REG32_CLR_BIT(DISPC_CONTROL_OFFSET + DISPC_BASE, LCDENABLE_BITPOS);
}


/*============================= Function Separator =============================*/
void GFX_Layer_Config(u32 GfxAttributes, u32 GfxRowIncrement, u32 GfxHsize, u32 GfxVsize, u32 GfxHpos, u32 GfxVpos)
{
    REG32_WR(DISPC_GFX_ATTRIBUTES_OFFSET + DISPC_BASE, GfxAttributes);
    REG32_WR(DISPC_GFX_SIZE_OFFSET + DISPC_BASE, (GfxVsize-1) << GFXSIZEY_BITPOS | (GfxHsize-1) << GFXSIZEX_BITPOS);
    REG32_WR(DISPC_GFX_POSITION_OFFSET + DISPC_BASE, GfxVpos << GFXPOSY_BITPOS | GfxHpos << GFXPOSX_BITPOS);
}


/*============================= Function Separator =============================*/
void GFX_Layer_Address_Set(u32 *Addr)
{
    REG32_WR(DISPC_GFX_BAj_OFFSET(0) + DISPC_BASE, (u32)Addr);
}


/*============================= Function Separator =============================*/
void GFX_Layer_enable(void)
{
    REG32_SET_BIT(DISPC_GFX_ATTRIBUTES_OFFSET + DISPC_BASE, GFXENABLE_BITPOS);
}

void halDSS_set_go(void)
{
    REG32_SET_BIT(HALREG32_DISPC_CONTROL_OFFSET + DISPC_BASE, GOLCD_BITPOS);                /* Set GOLCD bit to update configuration values */
}

int halDSS_check_go(void)
{
    return REG32_TEST_BIT(HALREG32_DISPC_CONTROL_OFFSET + DISPC_BASE, GOLCD_BITPOS);
}


