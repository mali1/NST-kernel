/* ============================================================================
*                       MultiMedia Solutions
*   (c) Copyright 2010, MultiMedia Solutions  All Rights Reserved.
*
*   Use of this software is controlled by the terms and conditions found
*   in the license agreement under which this software has been supplied.
* =========================================================================== */
/**
*  @file  dsi_pll_drv.h
*
*  Definitions, types and API function prototypes for OMAP3630 DSI PLL driver
*
*  @author  Pavel Nedev
*
*  @date  17/03/2010
*
*  @version  1.00
*/
/* ========================================================================== */

#ifndef _DSI_PLL_DRV_H
#define _DSI_PLL_DRV_H

#include <linux/types.h>

typedef volatile uint32_t vu32;

/******************************************************************************
 * DEFINE SECTION
 ******************************************************************************/
#define DSI_PLL_POLLING_INTERVAL    100
#define DSI_PLL_TIMEOUT             100 /* x Polling Interval [us] */
#define DSI_PLL_REINIT_CNT 			3

/* DSS base address */
#define DSS_BASE    0x48050000

#define DSS_CONTROL    (0x40 >> 2)

#define DSI_CLK_SWITCH_BITPOS      1
#define DSI_CLK_SWITCH_MASK        (1 << 1)
#define DISPC_CLK_SWITCH_BITPOS    0
#define DISPC_CLK_SWITCH_MASK      (1 << 0)



/* DSI base address */
#define DSI_BASE    0x4804FC00

#define DSI_CLK_CTRL    (0x54 >> 2)

#define PLL_PWR_CMD_BITPOS       30
#define PLL_PWR_CMD_MASK         (3 << 30)
#define PLL_PWR_STATUS_BITPOS    28
#define PLL_PWR_STATUS_MASK      (3 << 28)
#define CIO_CLK_ICG_BITPOS       14
#define CIO_CLK_ICG_MASK         (1 << 14)



/* DSI PLL base address */
#define DSI_PLL_BASE    0x4804FF00

#define DSI_PLL_CONTROL    (0x00 >> 2)

#define DSI_HSDIV_SYSRESET_BITPOS    4
#define DSI_HSDIV_SYSRESET_MASK      (1 << 4)
#define DSI_PLL_SYSRESET_BITPOS      3
#define DSI_PLL_SYSRESET_MASK        (1 << 3)
#define DSI_PLL_HALTMODE_BITPOS      2
#define DSI_PLL_HALTMODE_MASK        (1 << 2)
#define DSI_PLL_GATEMODE_BITPOS      1
#define DSI_PLL_GATEMODE_MASK        (1 << 1)
#define DSI_PLL_AUTOMODE_BITPOS      0
#define DSI_PLL_AUTOMODE_MASK        (1 << 0)



#define DSI_PLL_STATUS    (0x04 >> 2)

#define DSI_BYPASSACKZ_BITPOS            9
#define DSI_BYPASSACKZ_MASK              (1 << 9)
#define DSIPROTO_CLOCK_ACK_BITPOS        8
#define DSIPROTO_CLOCK_ACK_MASK          (1 << 8)
#define DSS_CLOCK_ACK_BITPOS             7
#define DSS_CLOCK_ACK_MASK               (1 << 7)
#define DSI_PLL_BYPASS_BITPOS            6
#define DSI_PLL_BYPASS_MASK              (1 << 6)
#define DSI_PLL_HIGHJITTER_BITPOS        5
#define DSI_PLL_HIGHJITTER_MASK          (1 << 5)
#define DSI_PLL_LIMP_BITPOS              4
#define DSI_PLL_LIMP_MASK                (1 << 4)
#define DSI_PLL_LOSSREF_BITPOS           3
#define DSI_PLL_LOSSREF_MASK             (1 << 3)
#define DSI_PLL_RECAL_BITPOS             2
#define DSI_PLL_RECAL_MASK               (1 << 2)
#define DSI_PLL_LOCK_BITPOS              1
#define DSI_PLL_LOCK_MASK                (1 << 1)
#define DSI_PLLCTRL_RESET_DONE_BITPOS    0
#define DSI_PLLCTRL_RESET_DONE_MASK      (1 << 0)



#define DSI_PLL_GO    (0x08 >> 2)

#define DSI_PLL_GO_BITPOS    0
#define DSI_PLL_GO_MASK      (1 << 0)



#define DSI_PLL_CONFIGURATION1    (0x0C >> 2)

#define DSIPROTO_CLOCK_DIV_BITPOS    23
#define DSIPROTO_CLOCK_DIV_MASK      (0xF << 23)
#define DSS_CLOCK_DIV_BITPOS         19
#define DSS_CLOCK_DIV_MASK           (0xF << 19)
#define DSI_PLL_REGM_BITPOS          8
#define DSI_PLL_REGM_MASK            (0x7FF << 8)
#define DSI_PLL_REGN_BITPOS          1
#define DSI_PLL_REGN_MASK            (0x7F << 1)
#define DSI_PLL_STOPMODE_BITPOS      0
#define DSI_PLL_STOPMODE_MASK        (1 << 0)



#define DSI_PLL_CONFIGURATION2    (0x10 >> 2)

#define DSI_HSDIVBYPASS_BITPOS           20
#define DSI_HSDIVBYPASS_MASK             (1 << 20)
#define DSI_PROTO_CLOCK_PWDN_BITPOS      19
#define DSI_PROTO_CLOCK_PWDN_MASK        (1 << 19)
#define DSI_PROTO_CLOCK_EN_BITPOS        18
#define DSI_PROTO_CLOCK_EN_MASK          (1 << 18)
#define DSS_CLOCK_PWDN_BITPOS            17
#define DSS_CLOCK_PWDN_MASK              (1 << 17)
#define DSS_CLOCK_EN_BITPOS              16
#define DSS_CLOCK_EN_MASK                (1 << 16)
#define DSI_BYPASSEN_BITPOS              15
#define DSI_BYPASSEN_MASK                (1 << 15)
#define DSI_PHY_CLKINEN_BITPOS           14
#define DSI_PHY_CLKINEN_MASK             (1 << 14)
#define DSI_PLL_REFEN_BITPOS             13
#define DSI_PLL_REFEN_MASK               (1 << 13)
#define DSI_PLL_HIGHFREQ_BITPOS          12
#define DSI_PLL_HIGHFREQ_MASK            (1 << 12)
#define DSI_PLL_CLKSEL_BITPOS            11
#define DSI_PLL_CLKSEL_MASK              (1 << 11)
#define DSI_PLL_LOCKSEL_BITPOS           9
#define DSI_PLL_LOCKSEL_MASK             (3 << 9)
#define DSI_PLL_DRIFTGUARDEN_BITPOS      8
#define DSI_PLL_DRIFTGUARDEN_MASK        (1 << 8)
#define DSI_PLL_TIGHTPHASELOCK_BITPOS    7
#define DSI_PLL_TIGHTPHASELOCK_MASK      (1 << 7)
#define DSI_PLL_LOWCURRSTBY_BITPOS       6
#define DSI_PLL_LOWCURRSTBY_MASK         (1 << 6)
#define DSI_PLL_PLLLPMODE_BITPOS         5
#define DSI_PLL_PLLLPMODE_MASK           (1 << 5)
#define DSI_PLL_IDLE_BITPOS              0
#define DSI_PLL_IDLE_MASK                (1 << 0)



/******************************************************************************
 * VARIABLE OPERATION MACROS
 ******************************************************************************/


/******************************************************************************
 * TYPES
 ******************************************************************************/
/* ========================================================================== */
/**
*  dsi_pll_pwr_cmd_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    DSI_PLL_PWR_OFF   = 0,
    DSI_PLL_PWR_PLL   = 1,
    DSI_PLL_PWR_ALL   = 2,
    DSI_PLL_PWR_HSDIV = 3
} dsi_pll_pwr_t;



/* ========================================================================== */
/**
*  dsi_pll_lock_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    DSI_PLL_PHASE_LOCK = 0,
    DSI_PLL_FREQ_LOCK  = 1
} dsi_pll_lock_t;



/* ========================================================================== */
/**
*  dsi_fclk_src_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    DSI_FCLK_PRCM    = 0,
    DSI_FCLK_DSI_PLL = 1
} dsi_fclk_src_t;



/* ========================================================================== */
/**
*  dispc_fclk_src_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    DISPC_FCLK_PRCM    = 0,
    DISPC_FCLK_DSI_PLL = 1
} dispc_fclk_src_t;



/******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************/
/* ========================================================================== */
/**
*  dsi_pll_power()    function_description
*
*  @param   param_name    param_description
*
*  @return  return_value    return_value_description
*
*  @pre     pre_conditions
*
*  @post    post_conditions
*
*  @see     related_functions
*/
/* ========================================================================== */
int dsi_pll_power(dsi_pll_pwr_t cmd);



/* ========================================================================== */
/**
*  dsi_pll_init()    function_description
*
*  @param   param_name    param_description
*
*  @return  return_value    return_value_description
*
*  @pre     pre_conditions
*
*  @post    post_conditions
*
*  @see     related_functions
*/
/* ========================================================================== */
int dsi_pll_init(void);

void dsi_pll_cleanup(void);

/* ========================================================================== */
/**
*  dsi_pll_set_clk()    function_description
*
*  @param   param_name    param_description
*
*  @return  return_value    return_value_description
*
*  @pre     pre_conditions
*
*  @post    post_conditions
*
*  @see     related_functions
*/
/* ========================================================================== */
int dsi_pll_set_clk(u32 pll_clk_khz);



/* ========================================================================== */
/**
*  dsi_pll_enable_dispc_clk()    function_description
*
*  @param   param_name    param_description
*
*  @return  return_value    return_value_description
*
*  @pre     pre_conditions
*
*  @post    post_conditions
*
*  @see     related_functions
*/
/* ========================================================================== */
int dsi_pll_enable_dispc_clk(u8 clk_div);

#endif /* _DSI_PLL_DRV_H */
