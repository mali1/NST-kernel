#ifndef _HALDDSS_H_
#define _HALDDSS_H_

#define REGBASE_DSS                             0x48050000      /*DSS REGISTER BASE*/

#define HALREG32_DSS_SYSCONFIG_OFFSET           0x010           /*DSS_SYSCONFIG Register offset*/
#define HALREG32_DSS_SYSSTATUS_OFFSET           0x014           /*DSS_SYSSTATUS Register offset*/

#define HALREG32_DSS_IRQSTATUS_OFFSET           0x018           /*DSS_IRQSTATUS Register offset*/
#define DSI_IRQ_BITPOS                          1
#define DISPC_IRQ_BITPOS                        0

#define HALREG32_DSS_CONTROL_OFFSET             0x040           /*DSS_CONTROL Register offset*/
#define DISPC_CLK_SWITCH_BITPOS                 0
#define DSI_CLK_SWITCH_BITPOS                   1

/************************************/
/* DISPC (Display Controler) module */
/************************************/
#define REGBASE_DISPC                           0x48050400      /*DISPC REGISTER BASE*/

#define HALREG32_DISPC_SYSCONFIG_OFFSET         0x010           /*DISPC_SYSCONFIG Register offset*/
#define HALREG32_DISPC_SYSSTATUS_OFFSET         0x014           /*DISPC_SYSSTATUS Register offset*/
#define HALREG32_DISPC_IRQSTATUS_OFFSET         0x018           /*DISPC_IRQSTATUS Register offset*/

#define HALREG32_DISPC_IRQENABLE_OFFSET         0x01C           /*DISPC_IRQENABLE Register offset*/
#define WAKEUP_BITPOS                           16
#define SYNCLOSTDIGITAL_BITPOS                  15
#define SYNCLOST_BITPOS                         14
#define VID2ENDWINDOW_BITPOS                    13
#define VID2FIFOUNDERFLOW_BITPOS                12
#define VID1ENDWINDOW_BITPOS                    11
#define VID1FIFOUNDERFLOW_BITPOS                10
#define OCPERROR_BITPOS                         9
#define PALETTEGAMMALOADING_BITPOS              8
#define GFXENDWINDOW_BITPOS                     7
#define GFXFIFOUNDERFLOW_BITPOS                 6
#define PROGRAMMEDLINENUMBER_BITPOS             5
#define ACBIASCOUNTSTATUS_BITPOS                4
#define EVSYNC_ODD_BITPOS                       3
#define EVSYNC_EVEN_BITPOS                      2
#define VSYNC_BITPOS                            1
#define FRAMEDONE_BITPOS                        0

#define HALREG32_DISPC_CONTROL_OFFSET           0x040           /*DISPC_CONTROL Register offset*/
#define SPATIALTEMPORALDITHERINGFRAMES_BITPOS   30
#define SPATIALTEMPORALDITHERINGFRAMES_WIDTH    2
#define LCDENABLEPOL_BITPOS                     29
#define LCDENABLESIGNAL_BITPOS                  28
#define PCKFREEENABLE_BITPOS                    27
#define TDMUNUSEDBITS_BITPOS                    25
#define TDMUNUSEDBITS_WIDTH                     2
#define TDMCYCLEFORMAT_BITPOS                   23
#define TDMCYCLEFORMAT_WIDTH                    2
#define TDMPARALLELMODE_BITPOS                  21
#define TDMPARALLELMODE_WIDTH                   2
#define TDMENABLE_BITPOS                        20
#define HT_BITPOS                               17
#define HT_WIDTH                                3
#define GPOUT1_BITPOS                           16
#define GPOUT0_BITPOS                           15
#define GPIN1_BITPOS                            14
#define GPIN0_BITPOS                            13
#define OVERLAYOPTIMIZATION_BITPOS              12
#define STALLMODE_BITPOS                        11
#define TFTDATALINES_BITPOS                     8
#define TFTDATALINES_WIDTH                      2
#define STDITHERENABLE_BITPOS                   7
#define GODIGITAL_BITPOS                        6
#define GOLCD_BITPOS                            5
#define M8B_BITPOS                              4
#define STNTFT_BITPOS                           3
#define MONOCOLOR_BITPOS                        2
#define DIGITALENABLE_BITPOS                    1
#define LCDENABLE_BITPOS                        0

#define HALREG32_DISPC_CONFIG_OFFSET            0x044           /*DISPC_CONFIG Register offset*/
#define TVALPHABLENDERENABLE_BITPOS             19
#define LCDALPHABLENDERENABLE_BITPOS            18
#define FIFOFILLING_BITPOS                      17
#define FIFOHANDCHECK_BITPOS                    16
#define CPR_BITPOS                              15
#define FIFOMERGE_BITPOS                        14
#define TCKDIGSELECTION_BITPOS                  13
#define TCKDIGENABLE_BITPOS                     12
#define TCKLCDSELECTION_BITPOS                  11
#define TCKLCDENABLE_BITPOS                     10
#define FUNCGATED_BITPOS                        9
#define ACBIASGATED_BITPOS                      8
#define VSYNCGATED_BITPOS                       7
#define HSYNCGATED_BITPOS                       6
#define PIXELCLOCKGATED_BITPOS                  5
#define PIXELDATAGATED_BITPOS                   4
#define PALETTEGAMMATABLE_BITPOS                3
#define LOADMODE_BITPOS                         1
#define LOADMODE_WIDTH                          2
#define PIXELGATED_BITPOS                       0

#define HALREG32_DISPC_DEFAULT_COLOR_m_OFFSET(m)    0x04C + (0x4*(m & 0x1))     /*DISPC_DEFAULT_COLOR_m Register offset*/
#define DEFAULTCOLOR_BITPOS                         0
#define DEFAULTCOLOR_WIDTH                          24

#define HALREG32_DISPC_TRANS_COLOR_m_OFFSET(m)      0x054 + (0x4*(m & 0x1))     /*DISPC_TRANS_COLOR_m Register offset*/
#define TRANSCOLORKEY_BITPOS                        0
#define TRANSCOLORKEY_WIDTH                         24

#define HALREG32_DISPC_LINE_STATUS_OFFSET           0x05C                       /*DISPC_LINE_STATUS Register offset*/

#define HALREG32_DISPC_LINE_NUMBER_OFFSET           0x060                       /*DISPC_LINE_NUMBER Register offset*/
#define LINENUMBER_BITPOS                           0
#define LINENUMBER_WIDTH                            11

#define HALREG32_DISPC_TIMING_H_OFFSET              0x064                       /*DISPC_TIMING_H Register offset*/
#define HBP_BITPOS                                  20
#define HBP_WIDTH                                   12
#define HFP_BITPOS                                  8
#define HFP_WIDTH                                   12
#define HSW_BITPOS                                  0
#define HSW_WIDTH                                   8

#define HALREG32_DISPC_TIMING_V_OFFSET              0x068                       /*DISPC_TIMING_V Register offset*/
#define VBP_BITPOS                                  20
#define VBP_WIDTH                                   12
#define VFP_BITPOS                                  8
#define VFP_WIDTH                                   12
#define VSW_BITPOS                                  0
#define VSW_WIDTH                                   8

#define HALREG32_DISPC_POL_FREQ_OFFSET              0x06C                       /*DISPC_POL_FREQ Register offset*/
#define ONOFF_BITPOS                                17
#define RF_BITPOS                                   16
#define IEO_BITPOS                                  15
#define IPC_BITPOS                                  14
#define IHS_BITPOS                                  13
#define IVS_BITPOS                                  12
#define ACBI_BITPOS                                 8
#define ACBI_WIDTH                                  4
#define ACB_BITPOS                                  0
#define ACB_WIDTH                                   8

#define HALREG32_DISPC_DIVISOR_OFFSET               0x070                       /*DISPC_DIVISOR Register offset*/
#define LCD_BITPOS                                  16
#define LCD_WIDTH                                   8
#define PCD_BITPOS                                  0
#define PCD_WIDTH                                   8

#define HALREG32_DISPC_SIZE_DIG_OFFSET              0x078                       /*DISPC_SIZE_DIG Register offset*/

#define HALREG32_DISPC_SIZE_LCD_OFFSET              0x07C                       /*DISPC_SIZE_LCD Register offset*/
#define LPP_BITPOS                                  16
#define LPP_WIDTH                                   11
#define PPL_BITPOS                                  0
#define PPL_WIDTH                                   11

#define HALREG32_DISPC_GFX_BAj_OFFSET(j)            0x080 + (0x4*(j & 0x1))     /*DISPC_GFX_BAj_OFFSET Register offset*/
#define GFXBA_BITPOS                                0
#define GFXBA_WIDTH                                 32

#define HALREG32_DISPC_GFX_POSITION_OFFSET          0x088                       /*DISPC_GFX_POSITION Register offset*/
#define GFXPOSY_BITPOS                              16
#define GFXPOSY_WIDTH                               11
#define GFXPOSX_BITPOS                              0
#define GFXPOSX_WIDTH                               11

#define HALREG32_DISPC_GFX_SIZE_OFFSET              0x08C                       /*DISPC_GFX_SIZE Register offset*/
#define GFXSIZEY_BITPOS                             16
#define GFXSIZEY_WIDTH                              11
#define GFXSIZEX_BITPOS                             0
#define GFXSIZEX_WIDTH                              11

#define HALREG32_DISPC_GFX_ATTRIBUTES_OFFSET        0x0A0                       /*DISPC_GFX_ATTRIBUTES Register offset*/
#define GFXSELFREFRESH_BITPOS                       15
#define GFXARBITRATION_BITPOS                       14
#define GFXROTATION_BITPOS                          12
#define GFXROTATION_WIDTH                           2
#define GFXFIFOPRELOAD_BITPOS                       11
#define GFXENDIANNESS_BITPOS                        10
#define GFXNIBBLEMODE_BITPOS                        9
#define GFXCHANNELOUT_BITPOS                        8
#define GFXBURSTSIZE_BITPOS                         6
#define GFXBURSTSIZE_WIDTH                          2
#define GFXREPLICATIONENABLE_BITPOS                 5
#define GFXFORMAT_BITPOS                            1
#define GFXFORMAT_WIDTH                             4
#define GFXENABLE_BITPOS                            0

#define HALREG32_DISPC_GFX_TABLE_BA_OFFSET        0x0B8

#define HALREG32_DISPC_VIDn_BAj_OFFSET(n, j)        0x0BC + (0x90*(n & 0x1)) + (0x4*(j & 0x1))  /*DISPC_VIDn_BAj Register offset*/
#define VIDBA_BITPOS                                0
#define VIDBA_WIDTH                                 32

#define HALREG32_DISPC_VIDn_POSITION_OFFSET(n)      0x0C4 + (0x90*(n & 0x1))                    /*DISPC_VIDn_POSITION Register offset*/
#define VIDPOSY_BITPOS                              16
#define VIDPOSY_WIDTH                               11
#define VIDPOSX_BITPOS                              0
#define VIDPOSX_WIDTH                               11

#define HALREG32_DISPC_VIDn_SIZE_OFFSET(n)          0x0C8 + (0x90*(n & 0x1))                    /*DISPC_VIDn_SIZE Register offset*/
#define VIDSIZEY_BITPOS                             16
#define VIDSIZEY_WIDTH                              11
#define VIDSIZEX_BITPOS                             0
#define VIDSIZEX_WIDTH                              11

#define HALREG32_DISPC_VIDn_ATTRIBUTES_OFFSET(n)    0x0CC + (0x90*(n & 0x1))                    /*DISPC_VIDn_ATTRIBUTES Register offset*/
#define VIDSELFREFRESH_BITPOS                       24
#define VIDARBITRATION_BITPOS                       23
#define VIDLINEBUFFERSPLIT_BITPOS                   22
#define VIDVERTICALTAPS_BITPOS                      21
#define VIDDMAOPTIMIZATION_BITPOS                   20
#define VIDFIFOPRELOAD_BITPOS                       19
#define VIDROWREPEATENABLE_BITPOS                   18
#define VIDENDIANNESS_BITPOS                        17
#define VIDCHANNELOUT_BITPOS                        16
#define VIDBURSTSIZE_BITPOS                         14
#define VIDBURSTSIZE_WIDTH                          2
#define VIDROTATION_BITPOS                          12
#define VIDROTATION_WIDTH                           2
#define VIDFULLRANGE_BITPOS                         11
#define VIDREPLICATIONENABLE_BITPOS                 10
#define VIDCOLORCONVENABLE_BITPOS                   9
#define VIDVRESIZECONF_BITPOS                       8
#define VIDHRESIZECONF_BITPOS                       7
#define VIDRESIZEENABLE_BITPOS                      5
#define VIDRESIZEENABLE_WIDTH                       2
#define VIDFORMAT_BITPOS                            1
#define VIDFORMAT_WIDTH                             4
#define VIDENABLE_BITPOS                            0

#define HALREG32_DISPC_VIDn_ROW_INC_OFFSET(n)       0x0D8 + (0x90*(n & 0x1))                    /*DISPC_VIDn_ROW_INC Register offset*/
#define VIDROWINC_BITPOS                            0
#define VIDROWINC_WIDTH                             32

#define HALREG32_DISPC_VIDn_FIR_OFFSET(n)           0x0E0 + (0x90*(n & 0x1))                    /*DISPC_VIDn_FIR Register offset*/
#define VIDFIRVINC_BITPOS                           16
#define VIDFIRVINC_WIDTH                            13
#define VIDFIRHINC_BITPOS                           0
#define VIDFIRHINC_WIDTH                            13

#define HALREG32_DISPC_VIDn_PICTURE_SIZE_OFFSET(n)  0x0E4 + (0x90*(n & 0x1))
#define VIDORGSIZEY_BITPOS                          16
#define VIDORGSIZEY_WIDTH                           11
#define VIDORGSIZEX_BITPOS                          0
#define VIDORGSIZEX_WIDTH                           11

#define HALREG32_DISPC_VIDn_ACCUl_OFFSET(n, l)      0x0E8 + (0x90*(n & 0x1)) + (0x4*(l & 0x1))  /*DISPC_VIDn_ACCUl Register offset*/
#define VIDVERTICALACCU_BITPOS                      16
#define VIDVERTICALACCU_WIDTH                       10
#define VIDHORIZONTALACCU_BITPOS                    0
#define VIDHORIZONTALACCU_WIDTH                     10

#define HALREG32_DISPC_VIDn_CONV_COEF0_OFFSET(n)    0x130 + (0x90*(n & 0x1))                    /*DISPC_VIDn_CONV_COEF0 Register offset*/
#define RCR_BITPOS                                  16
#define RCR_WIDTH                                   11
#define RY_BITPOS                                   0
#define RY_WIDTH                                    11

#define HALREG32_DISPC_VIDn_CONV_COEF1_OFFSET(n)    0x134 + (0x90*(n & 0x1))                    /*DISPC_VIDn_CONV_COEF1 Register offset*/
#define GY_BITPOS                                   16
#define GY_WIDTH                                    11
#define RCB_BITPOS                                  0
#define RCB_WIDTH                                   11

#define HALREG32_DISPC_VIDn_CONV_COEF2_OFFSET(n)    0x138 + (0x90*(n & 0x1))                    /*DISPC_VIDn_CONV_COEF2 Register offset*/
#define GCB_BITPOS                                  16
#define GCB_WIDTH                                   11
#define GCR_BITPOS                                  0
#define GCR_WIDTH                                   11

#define HALREG32_DISPC_VIDn_CONV_COEF3_OFFSET(n)    0x13C + (0x90*(n & 0x1))                    /*DISPC_VIDn_CONV_COEF3 Register offset*/
#define BCR_BITPOS                                  16
#define BCR_WIDTH                                   11
#define BY_BITPOS                                   0
#define BY_WIDTH                                    11

#define HALREG32_DISPC_VIDn_CONV_COEF4_OFFSET(n)    0x140 + (0x90*(n & 0x1))                    /*DISPC_VIDn_CONV_COEF4 Register offset*/
#define BCB_BITPOS                                  0
#define BCB_WIDTH                                   11


#define REGBASE_DSI                                 0x4804FC00

#define HALREG32_DSI_CTRL_OFFSET                    0x40
#define IF_EN_BITPOS                                0
#define CS_RX_EN_BITPOS                             1
#define ECC_RX_EN_BITPOS                            2
#define TX_FIFO_ARBITRATION_BITPOS                  3
#define VP_CLK_RATIO_BITPOS                         4
#define TRIGGER_RESET_BITPOS                        5

#define HALREG32_DSI_CLK_CTRL_OFFSET                0x54
#define LP_CLK_DIVISOR_BITPOS                       0
#define DDR_CLK_ALWAYS_ON_BITPOS                    13
#define CIO_CLK_ICG_BITPOS                          14
#define LP_CLK_NULL_PACKET_ENABLE_BITPOS            15
#define LP_CLK_NULL_PACKET_SIZE_BITPOS              16
#define HS_AUTO_STOP_ENABLE_BITPOS                  18
#define HS_MANUAL_STOP_CTR_BITPOS                   19
#define LP_CLK_ENABLE_BITPOS                        20
#define LP_RX_SYNCHRO_ENABLE_BITPOS                 21
#define PLL_PWR_STATUS_BITPOS                       28
#define PLL_PWR_CMD_BITPOS                          30

#define HALREG32_DSI_SYSCONFIG_OFFSET               0x10

#define HALREG32_DSI_SYSSTATUS_OFFSET               0x14

#define REGBASE_DSI_PLL                             0x4804FF00

#define HALREG32_DSI_PLL_CONTROL_OFFSET             0x00
#define DSI_PLL_AUTOMODE_BITPOS                    0
#define DSI_PLL_GATEMODE_BITPOS                    1
#define DSI_PLL_HALTMODE_BITPOS                    2
#define DSI_PLL_SYSRESET_BITPOS                    3
#define DSI_HSDIV_SYSRESET_BITPOS                  4

#define HALREG32_DSI_PLL_STATUS_OFFSET              0x04
#define DSI_PLLCTRL_RESET_DONE_BITPOS              0
#define DSI_PLL_LOCK_BITPOS                        1
#define DSI_PLL_RECAL_BITPOS                       2
#define DSI_PLL_LOSSREF_BITPOS                     3
#define DSI_PLL_LIMP_BITPOS                        4
#define DSI_PLL_HIGHJITTER_BITPOS                  5
#define DSI_PLL_BYPASS_BITPOS                      6
#define DSS_CLOCK_ACK_BITPOS                       7
#define DSIPROTO_CLOCK_ACK_BITPOS                  8
#define DSI_BYPASSACKZ_BITPOS                      9

#define HALREG32_DSI_PLL_GO_OFFSET                  0x08
#define DSI_PLL_GO_BITPOS                           0

#define HALREG32_DSI_PLL_CONFIGURATION1_OFFSET      0x0C
#define DSI_PLL_STOPMODE_BITPOS                     0
#define DSI_PLL_REGN_BITPOS                         1
#define DSI_PLL_REGM_BITPOS                         8
#define DSS_CLOCK_DIV_BITPOS                        19
#define DSIPROTO_CLOCK_DIV_BITPOS                   23


#define HALREG32_DSI_PLL_CONFIGURATION2_OFFSET      0x10
#define DSI_PLL_IDLE_BITPOS                        0
#define DSI_PLL_FREQSEL_BITPOS                     1
#define DSI_PLL_PLLLPMODE_BITPOS                   5
#define DSI_PLL_LOWCURRSTB_BITPOS                  6
#define DSI_PLL_TIGHTPHASELOC_BITPOS               7
#define DSI_PLL_DRIFTGUARDEN_BITPOS                8
#define DSI_PLL_LOCKSEL_BITPOS                     9
#define DSI_PLL_CLKSEL_BITPOS                      11
#define DSI_PLL_HIGHFREQ_BITPOS                    12
#define DSI_PLL_REFEN_BITPOS                       13
#define DSI_PHY_CLKINEN_BITPOS                     14
#define DSI_BYPASSEN_BITPOS                        15
#define DSS_CLOCK_EN_BITPOS                        16
#define DSS_CLOCK_PWDN_BITPOS                      17
#define DSI_PROTO_CLOCK_EN_BITPOS                  18
#define DSI_PROTO_CLOCK_PWDN_BITPOS                19
#define DSI_HSDIVBYPASS_BITPOS                     20

/*************************/
/* Various DSS constants */
/*************************/
#define HALDSS_CONST_LCD_PASSIVE                    0
#define HALDSS_CONST_LCD_ACTIVE                     1
#define HALDSS_CONST_LCD_12_BITS                    0
#define HALDSS_CONST_LCD_16_BITS                    1
#define HALDSS_CONST_LCD_18_BITS                    2
#define HALDSS_CONST_LCD_24_BITS                    3
#define HALDSS_CONST_LITTLE_ENDIAN                  0
#define HALDSS_CONST_BIG_ENDIAN                     1
#define HALDSS_CONST_VID_RGB444_EXP16               0x4
#define HALDSS_CONST_VID_RGB565                     0x6
#define HALDSS_CONST_VID_RGB888_EXP32               0x8
#define HALDSS_CONST_VID_RGB888                     0x9
#define HALDSS_CONST_VID_YUV2_422                   0xA
#define HALDSS_CONST_VID_UYVY_422                   0xB
#define HALDSS_CONST_WHITE                          0xFFFFFF
#define HALDSS_CONST_BLACK                          0x000000
#define HALDSS_CONST_RED                            0xFF0000
#define HALDSS_CONST_GREEN                          0x00FF00
#define HALDSS_CONST_BLUE                           0x0000FF
#define HALDSS_CONST_GFX_FORMAT_BITMAP8             0x3

#define HALDSS_CONST_DEF_BKCOLOR                    HALDSS_CONST_WHITE
#define HALDSS_CONST_DEF_TRCOLOR                    HALDSS_CONST_WHITE

#define DISPC_VIDn_ROW_INC_OFFSET(n)                0x0D8 + (0x90*(n & 0x1))
    #define VIDROWINC_BITPOS                        0
    #define VIDROWINC_WIDTH                         32

#include <linux/types.h>

int halDSS_Init(void);
void halDSS_Cleanup(void);
int halDSS_Reset(void);
int halDSS_DISPC_Config(int aPixClk,
                        int aHRes, int aVRes, int aTFTDataLines,
                        int aHBP, int aHFP, int aHPW,
                        int aVBP, int aVFP, int aVPW,
                        int aIsPclkInvert,
                        int aIsHsyncInvert, int aIsVsyncInvert);

void halDSS_DISPC_SetColorMap(u32 *ColorMapAddr);

void LCD_enable(void);
void LCD_disable(void);

void GFX_Layer_Config(u32 GfxAttributes, u32 GfxRowIncrement, u32 GfxHsize, u32 GfxVsize, u32 GfxHpos, u32 GfxVpos);

void GFX_Layer_Address_Set(u32 *Addr);

void GFX_Layer_enable(void);

void halDSS_set_go(void);
int halDSS_check_go(void);

#endif
