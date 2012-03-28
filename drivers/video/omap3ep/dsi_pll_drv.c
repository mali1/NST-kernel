/* ============================================================================
*                       MultiMedia Solutions
*   (c) Copyright 2010, MultiMedia Solutions  All Rights Reserved.
*
*   Use of this software is controlled by the terms and conditions found
*   in the license agreement under which this software has been supplied.
* =========================================================================== */
/**
*  @file  dsi_pll_drv.c
*
*  OMAP3630 DSI PLL driver functions
*
*  @author  Pavel Nedev
*
*  @date  17/03/2010
*
*  @version  1.00
*/
/* ========================================================================== */

/******************************************************************************
 * INCLUDE SECTION
 ******************************************************************************/
//#include <common.h>
#include "dsi_pll_drv.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/io.h>
#include <mach/io.h>
#include <linux/delay.h>


/******************************************************************************
 * DEFINES
 ******************************************************************************/
#define SYS_CLK_KHZ    26000


#define REG32_WR(addr, val) omap_writel(val, (u32)(addr))
#define REG32_RD(addr) omap_readl((u32)(addr))

#define IO_RD_FIELD(var, pos, mask)    ((REG32_RD(var) & (mask)) >> (pos))
#define IO_WR_FIELD(var, pos, mask, val) \
                REG32_WR(var, (REG32_RD(var) & ~(mask)) | (((val) << (pos)) & (mask)))
#define VAR_RD_FIELD(var, pos, mask)    (((var) & (mask)) >> (pos))
#define VAR_WR_FIELD(var, pos, mask, val) \
                var = ((var) & ~(mask)) | (((val) << (pos)) & (mask))

/******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/
static vu32 * DssBase = (vu32*)DSS_BASE;
static vu32 * DsiBase = (vu32*)DSI_BASE;
static vu32 * PllBase = (vu32*)DSI_PLL_BASE;



/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
/* ========================================================================== */
/**
*  @fn  dsi_pll_power()    function_description
*
*  @see  dsi_pll_drv.h
*/
/* ========================================================================== */
int dsi_pll_power(dsi_pll_pwr_t cmd)
{
    int  retVal = 0;
    u32  timeoutCnt;

    IO_WR_FIELD(DsiBase+DSI_CLK_CTRL, PLL_PWR_CMD_BITPOS,
                                    PLL_PWR_CMD_MASK, cmd);

    for(timeoutCnt = DSI_PLL_TIMEOUT; timeoutCnt; timeoutCnt--)
    {
        if(IO_RD_FIELD(DsiBase+DSI_CLK_CTRL, PLL_PWR_STATUS_BITPOS,
                                           PLL_PWR_STATUS_MASK) == cmd)
        {
            break;
        }
        udelay(DSI_PLL_POLLING_INTERVAL);
    }
    if(!timeoutCnt)
    {
	pr_err("DSI PWR ALL timeout\n");
        retVal = -1;
    }

    return retVal;
}



/* ========================================================================== */
/**
*  @fn  dsi_pll_init()    function_description
*
*  @see  dsi_pll_drv.h
*/
/* ========================================================================== */
int dsi_pll_init(void)
{
    int  retVal = 0;
    u32  regBuf;

    if (dsi_pll_power(DSI_PLL_PWR_ALL)) {
        retVal = -1;
	pr_err("DSI PWR ALL error\n");
        goto EXIT;
    }

    regBuf = REG32_RD(PllBase+DSI_PLL_CONTROL);
    /* SET */
    regBuf |= (DSI_PLL_HALTMODE_MASK | \
               DSI_PLL_GATEMODE_MASK);
    /* RESET */
    regBuf &= ~(DSI_PLL_AUTOMODE_MASK);
    REG32_WR(PllBase+DSI_PLL_CONTROL, regBuf);

    regBuf = REG32_RD(PllBase+DSI_PLL_CONFIGURATION2);
    /* SET */
    regBuf |= (DSI_HSDIVBYPASS_MASK | \
               DSI_PLL_LOWCURRSTBY_MASK);
    /* RESET */
    regBuf &= ~(DSI_PROTO_CLOCK_PWDN_MASK | \
	        DSI_PROTO_CLOCK_EN_MASK | \
                DSS_CLOCK_PWDN_MASK | \
                DSS_CLOCK_EN_MASK | \
                DSI_BYPASSEN_MASK | \
                DSI_PHY_CLKINEN_MASK | \
                DSI_PLL_REFEN_MASK | \
               /* PLL Reference is lower than 32 MHz */
                DSI_PLL_HIGHFREQ_MASK | \
                /* PLL Reference is the System Clk */
                DSI_PLL_CLKSEL_MASK | \
                DSI_PLL_DRIFTGUARDEN_MASK | \
                DSI_PLL_TIGHTPHASELOCK_MASK | \
                DSI_PLL_PLLLPMODE_MASK | \
                DSI_PLL_IDLE_MASK);
    VAR_WR_FIELD(regBuf, DSI_PLL_LOCKSEL_BITPOS, DSI_PLL_LOCKSEL_MASK,
             DSI_PLL_PHASE_LOCK);
    REG32_WR(PllBase+DSI_PLL_CONFIGURATION2, regBuf);

    //PllBase[DSI_PLL_CONFIGURATION1] |= DSI_PLL_STOPMODE_MASK;
    REG32_WR(PllBase+DSI_PLL_CONFIGURATION1,
		   REG32_RD(PllBase+DSI_PLL_CONFIGURATION1) | DSI_PLL_STOPMODE_MASK);

EXIT:
    return retVal;
}


void dsi_pll_cleanup(void)
{
}

/* ========================================================================== */
/**
*  @fn  dsi_pll_set_clk()    function_description
*
*  @see  dsi_pll_drv.h
*/
/* ========================================================================== */
int dsi_pll_set_clk(u32 pll_clk_khz)
{
    int  retVal = 0;
    u32  timeoutCnt,reinitCnt;
    u32  fIntKHz = 1000; /* Desired PLL internal freq */
    u32  regN, regM;
    u32  storedCfg;
    u32  regBuf;


    for(reinitCnt=DSI_PLL_REINIT_CNT;reinitCnt;reinitCnt--)
    {
		/* Enable Reference */
		//PllBase[DSI_PLL_CONFIGURATION2] |= DSI_PLL_REFEN_MASK;
		REG32_WR(PllBase+DSI_PLL_CONFIGURATION2,
			   REG32_RD(PllBase+DSI_PLL_CONFIGURATION2) | DSI_PLL_REFEN_MASK);

		/* Store current config */
		storedCfg = REG32_RD(PllBase+DSI_PLL_CONFIGURATION2);

		/* Bypass clocks while locking */
		regBuf = storedCfg;
		regBuf |= DSI_HSDIVBYPASS_MASK;
		regBuf &= ~DSI_PHY_CLKINEN_MASK;
		REG32_WR(PllBase+DSI_PLL_CONFIGURATION2, regBuf);

		regN = SYS_CLK_KHZ / fIntKHz;
		if (regN * fIntKHz >= SYS_CLK_KHZ)
		{
			regN--;
		}

		/* Calculate actual PLL internal freq */
		fIntKHz = SYS_CLK_KHZ / (regN + 1);

		regM = pll_clk_khz / (fIntKHz << 1);

		pr_debug("dsi_pll: setting up %dkHz pixel clock frequency\n",
								(2 * regM * fIntKHz / 2));

		/* Check clocks are bypassed and reference is active */
		for (timeoutCnt = DSI_PLL_TIMEOUT; timeoutCnt; timeoutCnt--)
		{
			if (!(REG32_RD(PllBase+DSI_PLL_STATUS) & (DSI_BYPASSACKZ_MASK |
											 DSI_PLL_LOSSREF_MASK)))
			{
				break;
			}
			udelay(DSI_PLL_POLLING_INTERVAL);
		}

		if(!timeoutCnt)
		{
			retVal = -1;
			goto EXIT;
		}

		/* Set dividers */
		regBuf = REG32_RD(PllBase+DSI_PLL_CONFIGURATION1);
		VAR_WR_FIELD(regBuf, DSI_PLL_REGM_BITPOS, DSI_PLL_REGM_MASK, regM);
		VAR_WR_FIELD(regBuf, DSI_PLL_REGN_BITPOS, DSI_PLL_REGN_MASK, regN);
		REG32_WR(PllBase+DSI_PLL_CONFIGURATION1, regBuf);

		/* Start PLL locking */
		//PllBase[DSI_PLL_GO] |= DSI_PLL_GO_MASK;
		REG32_WR(PllBase+DSI_PLL_GO, REG32_RD(PllBase+DSI_PLL_GO) | DSI_PLL_GO_MASK);

		for (timeoutCnt = DSI_PLL_TIMEOUT; timeoutCnt; timeoutCnt--)
		{
			if (REG32_RD(PllBase+DSI_PLL_STATUS) & DSI_PLL_LOCK_MASK)
			{
				break;
			}
			udelay(DSI_PLL_POLLING_INTERVAL);
		}


		if(timeoutCnt)
		{
			break;
		}
		pr_debug("dsi_pll: reinit \n");
		udelay(100);
    }

    if(!reinitCnt)
    {
        retVal = -1;
        goto EXIT;
    }

    /* Restore config */
    REG32_WR(PllBase+DSI_PLL_CONFIGURATION2, storedCfg);

EXIT:
    return retVal;
}



/* ========================================================================== */
/**
*  @fn  dsi_pll_enable_dispc_clk()    function_description
*
*  @see  dsi_pll_drv.h
*/
/* ========================================================================== */
int dsi_pll_enable_dispc_clk(u8 clk_div)
{
    int  retVal = 0;
    u32  timeoutCnt;
    u32  regBuf;

    /* Check PLL power state (HS Divider powered?) */
    if(!(IO_RD_FIELD(DsiBase+DSI_CLK_CTRL, PLL_PWR_STATUS_BITPOS,
                                         PLL_PWR_STATUS_MASK) &
         DSI_PLL_PWR_ALL))
    {
        if (dsi_pll_power(DSI_PLL_PWR_ALL))
        {
            retVal = -1;
            goto EXIT;
        }
    }

    /* Set PLL DISPC clock divider (DISPC clk = PLL clk / clk_div) */
    IO_WR_FIELD(PllBase+DSI_PLL_CONFIGURATION1, DSS_CLOCK_DIV_BITPOS,
                                              DSS_CLOCK_DIV_MASK, clk_div - 1);

    /* Enable DISPC clock */
    regBuf = REG32_RD(PllBase+DSI_PLL_CONFIGURATION2);
    regBuf &= ~(DSI_HSDIVBYPASS_MASK);
    regBuf |= DSS_CLOCK_EN_MASK;
    REG32_WR(PllBase+DSI_PLL_CONFIGURATION2, regBuf);

    /* Check DISPC clock is active */
    for (timeoutCnt = DSI_PLL_TIMEOUT; timeoutCnt; timeoutCnt--)
    {
        if (REG32_RD(PllBase+DSI_PLL_STATUS) & DSS_CLOCK_ACK_MASK)
        {
            break;
        }
        udelay(DSI_PLL_POLLING_INTERVAL);
    }
    if(!timeoutCnt)
    {
        retVal = -1;
        goto EXIT;
    }

    /* Switch DISPC clock source to DSI PLL */
    IO_WR_FIELD(DssBase+DSS_CONTROL, DISPC_CLK_SWITCH_BITPOS,
                                   DISPC_CLK_SWITCH_MASK,
             DISPC_FCLK_DSI_PLL);

EXIT:
    return retVal;
}
