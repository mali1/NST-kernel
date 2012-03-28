/*=======================================================*/
/* OMAP3430 Register, Bit, Constant and Type definitions */
/*=======================================================*/

#ifndef OMAP3430_H
    #define OMAP3430_H

    /*******************************/
    /* SCM (System Control) module */
    /*******************************/
    #define INTERFACE_SCM_BASE                           0x48002000

    #define PADCONF_SCM_BASE                             0x48002030

    #define CONTROL_PADCONF_GPMC_NCS3_OFFSET             0x084
    #define CONTROL_PADCONF_GPMC_NCS5_OFFSET             0x088
    #define CONTROL_PADCONF_GPMC_NCS7_OFFSET             0x08C
    #define CONTROL_PADCONF_CAM_HS_OFFSET                0x0DC
    #define CONTROL_PADCONF_CAM_XCLKA_OFFSET             0x0E0
    #define CONTROL_PADCONF_CAM_FLD_OFFSET               0x0E4
    #define CONTROL_PADCONF_CAM_D1_OFFSET                0x0E8
    #define CONTROL_PADCONF_CAM_D3_OFFSET                0x0EC
    #define CONTROL_PADCONF_CAM_D5_OFFSET                0x0F0
    #define CONTROL_PADCONF_CAM_D7_OFFSET                0x0F4
    #define CONTROL_PADCONF_CAM_D9_OFFSET                0x0F8
    #define CONTROL_PADCONF_CAM_D11_OFFSET               0x0FC
    #define CONTROL_PADCONF_CAM_WEN_OFFSET               0x100
    #define CONTROL_PADCONF_CSI2_DX0_OFFSET              0x104
    #define CONTROL_PADCONF_CSI2_DX1_OFFSET              0x108
    #define CONTROL_PADCONF_MCBSP4_DX_OFFSET             0x158
    #define CONTROL_PADCONF_MCBSP_CLKS_OFFSET            0x164
    #define CONTROL_PADCONF_I2C1_SDA_OFFSET              0x18C
    #define CONTROL_PADCONF_I2C2_SDA_OFFSET              0x190
    #define CONTROL_PADCONF_MCSPI1_CLK_OFFSET            0x198
    #define CONTROL_PADCONF_MCSPI1_SOMI_OFFSET           0x19C
    #define CONTROL_PADCONF_MCSPI1_CS1_OFFSET            0x1A0

    #define CONTROL_PADCONF_DSS_PCLK_OFFSET              0x0D4
    #define CONTROL_PADCONF_DSS_VSYNC_OFFSET             0x0D8
    #define CONTROL_PADCONF_DSS_DATA0_OFFSET             0x0DC
    #define CONTROL_PADCONF_DSS_DATA2_OFFSET             0x0E0
    #define CONTROL_PADCONF_DSS_DATA4_OFFSET             0x0E4
    #define CONTROL_PADCONF_DSS_DATA6_OFFSET             0x0E8
    #define CONTROL_PADCONF_DSS_DATA8_OFFSET             0x0EC
    #define CONTROL_PADCONF_DSS_DATA10_OFFSET            0x0F0
    #define CONTROL_PADCONF_DSS_DATA12_OFFSET            0x0F4
    #define CONTROL_PADCONF_DSS_DATA14_OFFSET            0x0F8
    #define CONTROL_PADCONF_DSS_DATA16_OFFSET            0x0FC
    #define CONTROL_PADCONF_DSS_DATA18_OFFSET            0x100
    #define CONTROL_PADCONF_DSS_DATA20_OFFSET            0x104
    #define CONTROL_PADCONF_DSS_DATA22_OFFSET            0x108

    #define GENERAL_SCM_BASE                             0x48002270

    #define CONTROL_PADCONF_OFF_OFFSET                   0x000
    #define CONTROL_DEVCONF1_OFFSET                      0x068
        #define I2C3HSMASTER_BITPOS                      14
        #define I2C2HSMASTER_BITPOS                      13
        #define I2C1HSMASTER_BITPOS                      12
        #define MPUFORCEWRNP_BITPOS                      9
    #define CONTROL_CSIRXFE_OFFSET                       0x06C
        #define CSIB_RESET_BITPOS                        13
        #define CSIB_PWRDNZ_BITPOS                       12
        #define CSIB_SELFORM_BITPOS                      10
        #define CSIB_RESENABLE_BITPOS                    8
        #define CSIB_INV_BITPOS                          7

    #define PADCONF_WKUP_SCM_BASE                        0x48002A00

    #define CONTROL_PADCONF_SYS_BOOT3_OFFSET             0x10
    #define CONTROL_PADCONF_SYS_BOOT5_OFFSET             0x14

    #define GENERAL_WKUP_SCM_BASE                        0x48002A60

    /******************************/
    /* Common SCM bit definitions */
    /******************************/
    #define WAKEUPEVENT1_BITPOS                          31
    #define WAKEUPENABLE1_BITPOS                         30
    #define OFFPULLTYPESELECT1_BITPOS                    29
    #define OFFPULLUDENABLE1_BITPOS                      28
    #define OFFOUTVALUE1_BITPOS                          27
    #define OFFOUTENABLE1_BITPOS                         26
    #define OFFENABLE1_BITPOS                            25
    #define INPUTENABLE1_BITPOS                          24
    #define PULLTYPESELECT1_BITPOS                       20
    #define PULLUDENABLE1_BITPOS                         19
    #define MUXMODE1_BITPOS                              16
    #define WAKEUPEVENT0_BITPOS                          15
    #define WAKEUPENABLE0_BITPOS                         14
    #define OFFPULLTYPESELECT0_BITPOS                    13
    #define OFFPULLUDENABLE0_BITPOS                      12
    #define OFFOUTVALUE0_BITPOS                          11
    #define OFFOUTENABLE0_BITPOS                         10
    #define OFFENABLE0_BITPOS                            9
    #define INPUTENABLE0_BITPOS                          8
    #define PULLTYPESELECT0_BITPOS                       4
    #define PULLUDENABLE0_BITPOS                         3
    #define MUXMODE0_BITPOS                              0
    #define MUXMODE_WIDTH                                3

    /*****************************/
    /* CM (Clock Manager) module */
    /*****************************/
    #define CORE_CM_BASE                                 0x48004A00
    #define WKUP_CM_BASE                                 0x48004C00
    #define CLOCK_CM_BASE                                0x48004D00
    #define DSS_CM_BASE                                  0x48004E00
    #define CAM_CM_BASE                                  0x48004F00
    #define PER_CM_BASE                                  0x48005000

    #define FCLKEN_OFFSET                                0x00
    #define ICLKEN_OFFSET                                0x10
        #define EN_MCSPI4_BITPOS                         21
        #define EN_MCSPI3_BITPOS                         20
        #define EN_MCSPI2_BITPOS                         19
        #define EN_MCSPI1_BITPOS                         18
        #define EN_I2C3_BITPOS                           17
        #define EN_I2C2_BITPOS                           16
        #define EN_I2C1_BITPOS                           15
        #define EN_GPIO6_BITPOS                          17
        #define EN_GPIO5_BITPOS                          16
        #define EN_GPIO4_BITPOS                          15
        #define EN_GPIO3_BITPOS                          14
        #define EN_GPIO2_BITPOS                          13
        #define EN_GPIO1_BITPOS                          3
        #define EN_GPT11_BITPOS                          12
        #define EN_GPT10_BITPOS                          11
        #define EN_GPT9_BITPOS                           10
        #define EN_GPT8_BITPOS                           9
        #define EN_GPT7_BITPOS                           8
        #define EN_GPT6_BITPOS                           7
        #define EN_GPT5_BITPOS                           6
        #define EN_GPT4_BITPOS                           5
        #define EN_GPT3_BITPOS                           4
        #define EN_GPT2_BITPOS                           3
        #define EN_GPT1_BITPOS                           0
        #define EN_CSI2_BITPOS                           1
        #define EN_DSS_BITPOS                            0
        #define EN_TV_BITPOS                             2
        #define EN_DSS2_BITPOS                           1
        #define EN_DSS1_BITPOS                           0
        #define EN_CAM_BITPOS                            0
    #define IDLEST_OFFSET                                0x20
        #define ST_DSS_IDLE_BITPOS                       1
        #define ST_DSS_STDBY_BITPOS                      0
    #define AUTOIDLE_OFFSET                              0x30
    #define CLKSEL_OFFSET                                0x40
        #define CLKSEL_GPT11_BITPOS                      7
        #define CLKSEL_GPT10_BITPOS                      6
        #define CLKSEL_GPT9_BITPOS                       7
        #define CLKSEL_GPT8_BITPOS                       6
        #define CLKSEL_GPT7_BITPOS                       5
        #define CLKSEL_GPT6_BITPOS                       4
        #define CLKSEL_GPT5_BITPOS                       3
        #define CLKSEL_GPT4_BITPOS                       2
        #define CLKSEL_GPT3_BITPOS                       1
        #define CLKSEL_GPT2_BITPOS                       0
        #define CLKSEL_GPT1_BITPOS                       0
        #define CLKSEL_CAM_BITPOS                        0
    #define CLKSTCTRL_OFFSET                             0x48
    #define CLKSTST_OFFSET                               0x4C

    /****************************************/
    /* PRM (Power and Reset Manager) module */
    /****************************************/
    #define CORE_PRM_BASE                                0x48306A00
    #define WKUP_PRM_BASE                                0x48306C00
    #define DSS_PRM_BASE                                 0x48306E00
    #define CAM_PRM_BASE                                 0x48306F00
    #define PER_PRM_BASE                                 0x48307000

    #define RSTST_OFFSET                                 0x58
        #define GLOBALWARM_RST_BITPOS                    1
        #define GLOBALCOLD_RST_BITPOS                    0
    #define PWSTCTRL_OFFSET                              0xE0
        #define POWERSTATE_BITPOS                        0
        #define POWERSTATE_WIDTH                         2
    #define PWSTST_OFFSET                                0xE4
        #define INTRANSITION_BITPOS                      20
        #define POWERSTATEST_BITPOS                      0
        #define POWERSTATEST_WIDTH                       2

    #define CLOCK_PRM_BASE                               0x48306D00

    #define PRM_CLKSEL_OFFSET                            0x40
        #define SYS_CLKIN_SEL_BITPOS                     0
        #define SYS_CLKIN_SEL_WIDTH                      3

    #define GLOBAL_PRM_BASE                              0x48307200

    #define PRM_RSTCTRL_OFFSET                           0x50
    #define PRM_RSTST_OFFSET                             0x58
    #define PRM_VOLTCTRL_OFFSET                          0x60
    #define PRM_CLKSRC_CTRL_OFFSET                       0x70
    #define PRM_POLCTRL_OFFSET                           0x9C
        #define OFFMODE_POL_BITPOS                       3
        #define CLKOUT_POL_BITPOS                        2
        #define CLKREQ_POL_BITPOS                        1
        #define EXTVOL_POL_BITPOS                        0

    /*************************/
    /* Various PRM constants */
    /*************************/
    #define DOMAIN_OFF                                   0
    #define DOMAIN_RETENTION                             1
    #define DOMAIN_INACTIVE                              2
    #define DOMAIN_ON                                    3

    /**********************************/
    /* DSS (Display SubSystem) module */
    /**********************************/
    #define DSS_BASE                                     0x48050000

    #define DSS_SYSCONFIG_OFFSET                         0x010
    #define DSS_SYSSTATUS_OFFSET                         0x014
    #define DSS_IRQSTATUS_OFFSET                         0x018
        #define DSI_IRQ_BITPOS                           1
        #define DISPC_IRQ_BITPOS                         0
    #define DSS_CONTROL_OFFSET                           0x040

    /************************************/
    /* DISPC (Display Controler) module */
    /************************************/
    #define DISPC_BASE                                   0x48050400

    #define DISPC_SYSCONFIG_OFFSET                       0x010
    #define DISPC_SYSSTATUS_OFFSET                       0x014
    #define DISPC_IRQSTATUS_OFFSET                       0x018
    #define DISPC_IRQENABLE_OFFSET                       0x01C
        #define WAKEUP_BITPOS                            16
        #define SYNCLOSTDIGITAL_BITPOS                   15
        #define SYNCLOST_BITPOS                          14
        #define VID2ENDWINDOW_BITPOS                     13
        #define VID2FIFOUNDERFLOW_BITPOS                 12
        #define VID1ENDWINDOW_BITPOS                     11
        #define VID1FIFOUNDERFLOW_BITPOS                 10
        #define OCPERROR_BITPOS                          9
        #define PALETTEGAMMALOADING_BITPOS               8
        #define GFXENDWINDOW_BITPOS                      7
        #define GFXFIFOUNDERFLOW_BITPOS                  6
        #define PROGRAMMEDLINENUMBER_BITPOS              5
        #define ACBIASCOUNTSTATUS_BITPOS                 4
        #define EVSYNC_ODD_BITPOS                        3
        #define EVSYNC_EVEN_BITPOS                       2
        #define VSYNC_BITPOS                             1
        #define FRAMEDONE_BITPOS                         0
    #define DISPC_CONTROL_OFFSET                         0x040
        #define SPATIALTEMPORALDITHERINGFRAMES_BITPOS    30
        #define SPATIALTEMPORALDITHERINGFRAMES_WIDTH     2
        #define LCDENABLEPOL_BITPOS                      29
        #define LCDENABLESIGNAL_BITPOS                   28
        #define PCKFREEENABLE_BITPOS                     27
        #define TDMUNUSEDBITS_BITPOS                     25
        #define TDMUNUSEDBITS_WIDTH                      2
        #define TDMCYCLEFORMAT_BITPOS                    23
        #define TDMCYCLEFORMAT_WIDTH                     2
        #define TDMPARALLELMODE_BITPOS                   21
        #define TDMPARALLELMODE_WIDTH                    2
        #define TDMENABLE_BITPOS                         20
        #define HT_BITPOS                                17
        #define HT_WIDTH                                 3
        #define GPOUT1_BITPOS                            16
        #define GPOUT0_BITPOS                            15
        #define GPIN1_BITPOS                             14
        #define GPIN0_BITPOS                             13
        #define OVERLAYOPTIMIZATION_BITPOS               12
        #define STALLMODE_BITPOS                         11
        #define TFTDATALINES_BITPOS                      8
        #define TFTDATALINES_WIDTH                       2
        #define STDITHERENABLE_BITPOS                    7
        #define GODIGITAL_BITPOS                         6
        #define GOLCD_BITPOS                             5
        #define M8B_BITPOS                               4
        #define STNTFT_BITPOS                            3
        #define MONOCOLOR_BITPOS                         2
        #define DIGITALENABLE_BITPOS                     1
        #define LCDENABLE_BITPOS                         0
    #define DISPC_CONFIG_OFFSET                          0x044
        #define TVALPHABLENDERENABLE_BITPOS              19
        #define LCDALPHABLENDERENABLE_BITPOS             18
        #define FIFOFILLING_BITPOS                       17
        #define FIFOHANDCHECK_BITPOS                     16
        #define CPR_BITPOS                               15
        #define FIFOMERGE_BITPOS                         14
        #define TCKDIGSELECTION_BITPOS                   13
        #define TCKDIGENABLE_BITPOS                      12
        #define TCKLCDSELECTION_BITPOS                   11
        #define TCKLCDENABLE_BITPOS                      10
        #define FUNCGATED_BITPOS                         9
        #define ACBIASGATED_BITPOS                       8
        #define VSYNCGATED_BITPOS                        7
        #define HSYNCGATED_BITPOS                        6
        #define PIXELCLOCKGATED_BITPOS                   5
        #define PIXELDATAGATED_BITPOS                    4
        #define PALETTEGAMMATABLE_BITPOS                 3
        #define LOADMODE_BITPOS                          1
        #define LOADMODE_WIDTH                           2
        #define PIXELGATED_BITPOS                        0
    #define DISPC_DEFAULT_COLOR_m_OFFSET(m)              0x04C + (0x4*(m & 0x1))
        #define DEFAULTCOLOR_BITPOS                      0
        #define DEFAULTCOLOR_WIDTH                       24
    #define DISPC_TRANS_COLOR_m_OFFSET(m)                0x054 + (0x4*(m & 0x1))
        #define TRANSCOLORKEY_BITPOS                     0
        #define TRANSCOLORKEY_WIDTH                      24
    #define DISPC_LINE_STATUS_OFFSET                     0x05C
    #define DISPC_LINE_NUMBER_OFFSET                     0x060
        #define LINENUMBER_BITPOS                        0
        #define LINENUMBER_WIDTH                         11
    #define DISPC_TIMING_H_OFFSET                        0x064
        #define HBP_BITPOS                               20
        #define HBP_WIDTH                                12
        #define HFP_BITPOS                               8
        #define HFP_WIDTH                                12
        #define HSW_BITPOS                               0
        #define HSW_WIDTH                                8
    #define DISPC_TIMING_V_OFFSET                        0x068
        #define VBP_BITPOS                               20
        #define VBP_WIDTH                                12
        #define VFP_BITPOS                               8
        #define VFP_WIDTH                                12
        #define VSW_BITPOS                               0
        #define VSW_WIDTH                                8
    #define DISPC_POL_FREQ_OFFSET                        0x06C
        #define ONOFF_BITPOS                             17
        #define RF_BITPOS                                16
        #define IEO_BITPOS                               15
        #define IPC_BITPOS                               14
        #define IHS_BITPOS                               13
        #define IVS_BITPOS                               12
        #define ACBI_BITPOS                              8
        #define ACBI_WIDTH                               4
        #define ACB_BITPOS                               0
        #define ACB_WIDTH                                8
    #define DISPC_DIVISOR_OFFSET                         0x070
        #define LCD_BITPOS                               16
        #define LCD_WIDTH                                8
        #define PCD_BITPOS                               0
        #define PCD_WIDTH                                8
    #define DISPC_SIZE_DIG_OFFSET                        0x078
    #define DISPC_SIZE_LCD_OFFSET                        0x07C
        #define LPP_BITPOS                               16
        #define LPP_WIDTH                                11
        #define PPL_BITPOS                               0
        #define PPL_WIDTH                                11
    #define DISPC_GFX_BAj_OFFSET(j)                      0x080 + (0x4*(j & 0x1))
        #define GFXBA_BITPOS                             0
        #define GFXBA_WIDTH                              32
    #define DISPC_GFX_POSITION_OFFSET                    0x088
        #define GFXPOSY_BITPOS                           16
        #define GFXPOSY_WIDTH                            11
        #define GFXPOSX_BITPOS                           0
        #define GFXPOSX_WIDTH                            11
    #define DISPC_GFX_SIZE_OFFSET                        0x08C
        #define GFXSIZEY_BITPOS                          16
        #define GFXSIZEY_WIDTH                           11
        #define GFXSIZEX_BITPOS                          0
        #define GFXSIZEX_WIDTH                           11
    #define DISPC_GFX_ATTRIBUTES_OFFSET                  0x0A0
        #define GFXSELFREFRESH_BITPOS                    15
        #define GFXARBITRATION_BITPOS                    14
        #define GFXROTATION_BITPOS                       12
        #define GFXROTATION_WIDTH                        2
        #define GFXFIFOPRELOAD_BITPOS                    11
        #define GFXENDIANNESS_BITPOS                     10
        #define GFXNIBBLEMODE_BITPOS                     9
        #define GFXCHANNELOUT_BITPOS                     8
        #define GFXBURSTSIZE_BITPOS                      6
        #define GFXBURSTSIZE_WIDTH                       2
        #define GFXREPLICATIONENABLE_BITPOS              5
        #define GFXFORMAT_BITPOS                         1
        #define GFXFORMAT_WIDTH                          4
        #define GFXENABLE_BITPOS                         0
    #define DISPC_VIDn_BAj_OFFSET(n, j)                  0x0BC + (0x90*(n & 0x1)) + (0x4*(j & 0x1))
        #define VIDBA_BITPOS                             0
        #define VIDBA_WIDTH                              32
    #define DISPC_VIDn_POSITION_OFFSET(n)                0x0C4 + (0x90*(n & 0x1))
        #define VIDPOSY_BITPOS                           16
        #define VIDPOSY_WIDTH                            11
        #define VIDPOSX_BITPOS                           0
        #define VIDPOSX_WIDTH                            11
    #define DISPC_VIDn_SIZE_OFFSET(n)                    0x0C8 + (0x90*(n & 0x1))
        #define VIDSIZEY_BITPOS                          16
        #define VIDSIZEY_WIDTH                           11
        #define VIDSIZEX_BITPOS                          0
        #define VIDSIZEX_WIDTH                           11
    #define DISPC_VIDn_ATTRIBUTES_OFFSET(n)              0x0CC + (0x90*(n & 0x1))
        #define VIDSELFREFRESH_BITPOS                    24
        #define VIDARBITRATION_BITPOS                    23
        #define VIDLINEBUFFERSPLIT_BITPOS                22
        #define VIDVERTICALTAPS_BITPOS                   21
        #define VIDDMAOPTIMIZATION_BITPOS                20
        #define VIDFIFOPRELOAD_BITPOS                    19
        #define VIDROWREPEATENABLE_BITPOS                18
        #define VIDENDIANNESS_BITPOS                     17
        #define VIDCHANNELOUT_BITPOS                     16
        #define VIDBURSTSIZE_BITPOS                      14
        #define VIDBURSTSIZE_WIDTH                       2
        #define VIDROTATION_BITPOS                       12
        #define VIDROTATION_WIDTH                        2
        #define VIDFULLRANGE_BITPOS                      11
        #define VIDREPLICATIONENABLE_BITPOS              10
        #define VIDCOLORCONVENABLE_BITPOS                9
        #define VIDVRESIZECONF_BITPOS                    8
        #define VIDHRESIZECONF_BITPOS                    7
        #define VIDRESIZEENABLE_BITPOS                   5
        #define VIDRESIZEENABLE_WIDTH                    2
        #define VIDFORMAT_BITPOS                         1
        #define VIDFORMAT_WIDTH                          4
        #define VIDENABLE_BITPOS                         0
    #define DISPC_VIDn_ROW_INC_OFFSET(n)                 0x0D8 + (0x90*(n & 0x1))
        #define VIDROWINC_BITPOS                         0
        #define VIDROWINC_WIDTH                          32
    #define DISPC_VIDn_FIR_OFFSET(n)                     0x0E0 + (0x90*(n & 0x1))
        #define VIDFIRVINC_BITPOS                        16
        #define VIDFIRVINC_WIDTH                         13
        #define VIDFIRHINC_BITPOS                        0
        #define VIDFIRHINC_WIDTH                         13
    #define DISPC_VIDn_PICTURE_SIZE_OFFSET(n)            0x0E4 + (0x90*(n & 0x1))
        #define VIDORGSIZEY_BITPOS                       16
        #define VIDORGSIZEY_WIDTH                        11
        #define VIDORGSIZEX_BITPOS                       0
        #define VIDORGSIZEX_WIDTH                        11
    #define DISPC_VIDn_ACCUl_OFFSET(n, l)                0x0E8 + (0x90*(n & 0x1)) + (0x4*(l & 0x1))
        #define VIDVERTICALACCU_BITPOS                   16
        #define VIDVERTICALACCU_WIDTH                    10
        #define VIDHORIZONTALACCU_BITPOS                 0
        #define VIDHORIZONTALACCU_WIDTH                  10
    #define DISPC_VIDn_CONV_COEF0_OFFSET(n)              0x130 + (0x90*(n & 0x1))
        #define RCR_BITPOS                               16
        #define RCR_WIDTH                                11
        #define RY_BITPOS                                0
        #define RY_WIDTH                                 11
    #define DISPC_VIDn_CONV_COEF1_OFFSET(n)              0x134 + (0x90*(n & 0x1))
        #define GY_BITPOS                                16
        #define GY_WIDTH                                 11
        #define RCB_BITPOS                               0
        #define RCB_WIDTH                                11
    #define DISPC_VIDn_CONV_COEF2_OFFSET(n)              0x138 + (0x90*(n & 0x1))
        #define GCB_BITPOS                               16
        #define GCB_WIDTH                                11
        #define GCR_BITPOS                               0
        #define GCR_WIDTH                                11
    #define DISPC_VIDn_CONV_COEF3_OFFSET(n)              0x13C + (0x90*(n & 0x1))
        #define BCR_BITPOS                               16
        #define BCR_WIDTH                                11
        #define BY_BITPOS                                0
        #define BY_WIDTH                                 11
    #define DISPC_VIDn_CONV_COEF4_OFFSET(n)              0x140 + (0x90*(n & 0x1))
        #define BCB_BITPOS                               0
        #define BCB_WIDTH                                11

    /*************************/
    /* Various DSS constants */
    /*************************/
    #define LCD_PASSIVE                                  0
    #define LCD_ACTIVE                                   1
    #define LCD_12_BITS                                  0
    #define LCD_16_BITS                                  1
    #define LCD_18_BITS                                  2
    #define LCD_24_BITS                                  3
    #define LITTLE_ENDIAN                                0
    #define BIG_ENDIAN                                   1
    #define VID_RGB444_EXP16                             0x4
    #define VID_RGB565                                   0x6
    #define VID_RGB888_EXP32                             0x8
    #define VID_RGB888                                   0x9
    #define VID_YUV2_422                                 0xA
    #define VID_UYVY_422                                 0xB
    #define WHITE                                        0xFFFFFF
    #define BLACK                                        0x000000
    #define RED                                          0xFF0000
    #define GREEN                                        0x00FF00
    #define BLUE                                         0x0000FF

    /**************/
    /* ISP module */
    /**************/
    #define ISP_BASE                                     0x480BC000

    #define ISP_SYSCONFIG_OFFSET                         0x04
    #define ISP_SYSSTATUS_OFFSET                         0x08
    #define ISP_IRQ0ENABLE_OFFSET                        0x0C
    #define ISP_IRQ0STATUS_OFFSET                        0x10
        #define HS_VS_IRQ_BITPOS                         31
        #define RSZ_DONE_IRQ_BITPOS                      24
        #define CBUFF_IRQ_BITPOS                         21
        #define PRV_DONE_IRQ_BITPOS                      20
        #define CCDC_VD2_IRQ_BITPOS                      10
        #define CSIA_IRQ_BITPOS                          0
    #define TCTRL_GRESET_LENGTH_OFFSET                   0x30
        #define LENGTH_BITPOS                            0
    #define ISP_CTRL_OFFSET                              0x40
        #define FLUSH_BITPOS                             31
        #define JPEG_FLUSH_BITPOS                        30
        #define CCDC_WEN_POL_BITPOS                      29
        #define SBL_SHARED_RPORTB_BITPOS                 28
        #define SBL_SHARED_RPORTA_BITPOS                 27
        #define CBUFF1_BCF_CTRL_BITPOS                   24
        #define CBUFF0_BCF_CTRL_BITPOS                   22
        #define SBL_AUTOIDLE_BITPOS                      21
        #define SBL_WR0_RAM_EN_BITPOS                    20
        #define SBL_WR1_RAM_EN_BITPOS                    19
        #define SBL_RD_RAM_EN_BITPOS                     18
        #define PREV_RAM_EN_BITPOS                       17
        #define CCDC_RAM_EN_BITPOS                       16
        #define SYNC_DETECT_BITPOS                       14
        #define RSZ_CLK_EN_BITPOS                        13
        #define PRV_CLK_EN_BITPOS                        12
        #define HIST_CLK_EN_BITPOS                       11
        #define H3A_CLK_EN_BITPOS                        10
        #define CBUFF_AUTOGATING_BITPOS                  9
        #define CCDC_CLK_EN_BITPOS                       8
        #define SHIFT_BITPOS                             6
        #define PAR_CLK_POL_BITPOS                       4
        #define PAR_BRIDGE_BITPOS                        2
        #define PAR_SER_CLK_SEL_BITPOS                   0
    #define TCTRL_CTRL_OFFSET                            0x50
        #define GRESETDIR_BITPOS                         31
        #define GRESETPOL_BITPOS                         30
        #define GRESETEN_BITPOS                          29
        #define INSEL_BITPOS                             27
        #define STRBPSTRBPOL_BITPOS                      26
        #define SHUTPOL_BITPOS                           24
        #define STRBEN_BITPOS                            23
        #define PSTRBEN_BITPOS                           22
        #define SHUTEN_BITPOS                            21
        #define DIVC_BITPOS                              10
        #define DIVB_BITPOS                              5
        #define DIVA_BITPOS                              0
    #define TCTRL_FRAME_OFFSET                           0x54
        #define STRB_BITPOS                              12
        #define PSTRB_BITPOS                             6
        #define SHUT_BITPOS                              0
    #define TCTRL_PSTRB_DELAY_OFFSET                     0x58
    #define TCTRL_STRB_DELAY_OFFSET                      0x5C
    #define TCTRL_SHUT_DELAY_OFFSET                      0x60
        #define DELAY_BITPOS                             0
    #define TCTRL_PSTRB_LENGTH_OFFSET                    0x64
    #define TCTRL_STRB_LENGTH_OFFSET                     0x68
    #define TCTRL_SHUT_LENGTH_OFFSET                     0x6C

    /*************************/
    /* Various ISP constants */
    /*************************/
    #define NO_SHIFT                                     0
    #define SHIFT_BY_2                                   1
    #define SHIFT_BY_4                                   2
    #define SHIFT_BY_6                                   3
    #define NO_BRIDGE                                    0
    #define LSB_BRIDGE                                   2
    #define MSB_BRIDGE                                   3
    #define CCDC_PAR_IN                                  0
    #define CCDC_CSIA_IN                                 1
    #define CCDC_CSIB_IN                                 2
    #define HS_FALL                                      0
    #define HS_RISE                                      1
    #define VS_FALL                                      2
    #define VS_RISE                                      3

    /********************/
    /* ISP CBUFF module */
    /********************/
    #define ISP_CBUFF_BASE                               0x480BC100

    #define CBUFF_IRQSTATUS_OFFSET                       0x18
    #define CBUFFx_CTRL_OFFSET(x)                        0x20 + (0x4*(x & 0x1))
        #define DONE_BITPOS                              2
        #define RWMODE_BITPOS                            1
    #define CBUFFx_STATUS_OFFSET(x)                      0x30 + (0x4*(x & 0x1))
        #define NW_BITPOS                                16
        #define CW_BITPOS                                8
        #define CPUW_BITPOS                              0
    #define CBUFFx_START_OFFSET(x)                       0x40 + (0x4*(x & 0x1))
    #define CBUFFx_END_OFFSET(x)                         0x50 + (0x4*(x & 0x1))
        #define ADDR_CBUFF_BITPOS                        3
    #define CBUFFx_WINDOWSIZE_OFFSET(x)                  0x60 + (0x4*(x & 0x1))
        #define SIZE_BITPOS                              3
    #define CBUFFx_THRESHOLD_OFFSET(x)                   0x70 + (0x4*(x & 0x1))
        #define THRESHOLD_BITPOS                         0

    /*******************/
    /* ISP CCDC module */
    /*******************/
    #define ISP_CCDC_BASE                                0x480BC600

    #define CCDC_PCR_OFFSET                              0x04
        #define BUSY_BITPOS                              1
        #define CCDC_EN_BITPOS                           0
    #define CCDC_SYN_MODE_OFFSET                         0x08
        #define SDR2RSZ_BITPOS                           19
        #define VP2SDR_BITPOS                            18
        #define WEN_BITPOS                               17
        #define VDHDEN_BITPOS                            16
        #define FLDSTAT_BITPOS                           15
        #define LPF_BITPOS                               14
        #define INPMOD_BITPOS                            12
        #define PACK8_BITPOS                             11
        #define DATSIZ_BITPOS                            8
        #define FLDMODE_BITPOS                           7
        #define DATAPOL_BITPOS                           6
        #define EXWEN_BITPOS                             5
        #define FLDPOL_BITPOS                            4
        #define HDPOL_BITPOS                             3
        #define VDPOL_BITPOS                             2
        #define FLDOUT_BITPOS                            1
        #define VDHDOUT_BITPOS                           0
    #define CCDC_HD_VD_WID_OFFSET                        0x0C
        #define HDW_BITPOS                               16
        #define VDW_BITPOS                               0
    #define CCDC_PIX_LINES_OFFSET                        0x10
        #define PPLN_BITPOS                              16
        #define HLPRF_BITPOS                             0
    #define CCDC_HORZ_INFO_OFFSET                        0x14
        #define SPH_BITPOS                               16
        #define NPH_BITPOS                               0
    #define CCDC_VERT_START_OFFSET                       0x18
        #define SLV0_BITPOS                              16
        #define SLV1_BITPOS                              0
    #define CCDC_VERT_LINES_OFFSET                       0x1C
        #define NLV_BITPOS                               0
    #define CCDC_HSIZE_OFF_OFFSET                        0x24
        #define LNOFST_BITPOS                            0
        #define LNOFST_WIDTH                             16
    #define CCDC_SDOFST_OFFSET                           0x28
        #define FIINV_BITPOS                             14
        #define FOFST_BITPOS                             12
        #define LOFST0_BITPOS                            9
        #define LOFST1_BITPOS                            6
        #define LOFST2_BITPOS                            3
        #define LOFST3_BITPOS                            0
    #define CCDC_SDR_ADDR_OFFSET                         0x2C
        #define ADDR_SDR_BITPOS                          0
        #define ADDR_SDR_WIDTH                           32
    #define CCDC_CLAMP_OFFSET                            0x30
        #define CLAMPEN_BITPOS                           31
        #define OBSLEN_BITPOS                            28
        #define OBSLEN_WIDTH                             3
        #define OBST_BITPOS                              10
        #define OBST_WIDTH                               15
        #define OBGAIN_BITPOS                            0
        #define OBGAIN_WIDTH                             5
    #define CCDC_DCSUB_OFFSET                            0x34
        #define DCSUB_BITPOS                             0
        #define DCSUB_WIDTH                              14
    #define CCDC_COLPTN_OFFSET                           0x38
    #define CCDC_BLKCMP_OFFSET                           0x3C
    #define CCDC_FPC_OFFSET                              0x40
        #define FPERR_BITPOS                             16
        #define FPCEN_BITPOS                             15
        #define FPNUM_BITPOS                             0
        #define FPNUM_WIDTH                              15
    #define CCDC_FPC_ADDR_OFFSET                         0x44
        #define ADDR_FPC_BITPOS                          0
    #define CCDC_ALAW_OFFSET                             0x4C
        #define CCDTBL_BITPOS                            3
        #define GWDI_BITPOS                              0
        #define GWDI_WIDTH                               3
    #define CCDC_REC656IF_OFFSET                         0x50
        #define ECCFVH_BITPOS                            1
        #define R656ON_BITPOS                            0
    #define CCDC_CFG_OFFSET                              0x54
        #define VDLC_BITPOS                              15
        #define MSBINVI_BITPOS                           13
        #define BSWD_BITPOS                              12
        #define Y8POS_BITPOS                             11
        #define WENLOG_BITPOS                            8
        #define FIDMD_BITPOS                             6
        #define BW656_BITPOS                             5
    #define CCDC_FMTCFG_OFFSET                           0x58
        #define VPIF_FRQ_BITPOS                          16
        #define VPIF_FRQ_WIDTH                           3
        #define VPEN_BITPOS                              15
        #define VPIN_BITPOS                              12
        #define VPIN_WIDTH                               3
        #define PLEN_EVEN_BITPOS                         8
        #define PLEN_EVEN_WIDTH                          4
        #define PLEN_ODD_BITPOS                          4
        #define PLEN_ODD_WIDTH                           4
        #define LNUM_BITPOS                              2
        #define LNUM_WIDTH                               2
        #define LNALT_BITPOS                             1
        #define FMTEN_BITPOS                             0
    #define CCDC_FMT_HORZ_OFFSET                         0x5C
        #define FMTSPH_BITPOS                            16
        #define FMTLNH_BITPOS                            0
    #define CCDC_FMT_VERT_OFFSET                         0x60
        #define FMTSLV_BITPOS                            16
        #define FMTLNV_BITPOS                            0
    #define CCDC_VP_OUT_OFFSET                           0x94
        #define VERT_NUM_BITPOS                          17
        #define HORZ_NUM_BITPOS                          4
        #define HORZ_ST_BITPOS                           0

    /**************************/
    /* Various CCDC constants */
    /**************************/
    #define CCDC_RAW                                     0
    #define CCDC_YUV_16bit                               1
    #define CCDC_YUV_8bit                                2
    #define CCDC_RAW_8to16                               0
    #define CCDC_RAW_12                                  4
    #define CCDC_RAW_11                                  5
    #define CCDC_RAW_10                                  6
    #define CCDC_RAW_8                                   7
    #define EVEN_PIXEL                                   0
    #define ODD_PIXEL                                    1

    /**********************/
    /* ISP Preview module */
    /**********************/
    #define ISP_PREVIEW_BASE                             0x480BCE00

    /**********************/
    /* ISP Resizer module */
    /**********************/
    #define ISP_RESIZER_BASE                             0x480BD000

    #define RSZ_PCR_OFFSET                               0x04
        #define ONESHOT_BITPOS                           2
    #define RSZ_CNT_OFFSET                               0x08
        #define CBILIN_BITPOS                            29
        #define INPSRC_BITPOS                            28
        #define INPTYP_BITPOS                            27
        #define YCPOS_BITPOS                             26
        #define VSTPH_BITPOS                             23
        #define HSTPH_BITPOS                             20
        #define VRSZ_BITPOS                              10
        #define HRSZ_BITPOS                              0
    #define RSZ_OUT_SIZE_OFFSET                          0x0C
        #define VERT_BITPOS                              16
        #define HORZ_BITPOS                              0
    #define RSZ_IN_START_OFFSET                          0x10
        #define VERT_ST_BITPOS                           16
        #define HORZ_ST_BITPOS                           0
    #define RSZ_IN_SIZE_OFFSET                           0x14
    #define RSZ_SDR_INADD_OFFSET                         0x18
    #define RSZ_SDR_INOFF_OFFSET                         0x1C
    #define RSZ_SDR_OUTADD_OFFSET                        0x20
    #define RSZ_SDR_OUTOFF_OFFSET                        0x24
    #define RSZ_HFILT10_OFFSET                           0x28
    #define RSZ_VFILT10_OFFSET                           0x68
        #define COEF1_BITPOS                             16
        #define COEF0_BITPOS                             0
    #define RSZ_HFILT32_OFFSET                           0x2C
    #define RSZ_VFILT32_OFFSET                           0x6C
        #define COEF3_BITPOS                             16
        #define COEF2_BITPOS                             0
    #define RSZ_HFILT54_OFFSET                           0x30
    #define RSZ_VFILT54_OFFSET                           0x70
        #define COEF5_BITPOS                             16
        #define COEF4_BITPOS                             0
    #define RSZ_HFILT76_OFFSET                           0x34
    #define RSZ_VFILT76_OFFSET                           0x74
        #define COEF7_BITPOS                             16
        #define COEF6_BITPOS                             0
    #define RSZ_HFILT98_OFFSET                           0x38
    #define RSZ_VFILT98_OFFSET                           0x78
        #define COEF9_BITPOS                             16
        #define COEF8_BITPOS                             0
    #define RSZ_HFILT1110_OFFSET                         0x3C
    #define RSZ_VFILT1110_OFFSET                         0x7C
        #define COEF11_BITPOS                            16
        #define COEF10_BITPOS                            0
    #define RSZ_HFILT1312_OFFSET                         0x40
    #define RSZ_VFILT1312_OFFSET                         0x80
        #define COEF13_BITPOS                            16
        #define COEF12_BITPOS                            0
    #define RSZ_HFILT1514_OFFSET                         0x44
    #define RSZ_VFILT1514_OFFSET                         0x84
        #define COEF15_BITPOS                            16
        #define COEF14_BITPOS                            0
    #define RSZ_HFILT1716_OFFSET                         0x48
    #define RSZ_VFILT1716_OFFSET                         0x88
        #define COEF17_BITPOS                            16
        #define COEF16_BITPOS                            0
    #define RSZ_HFILT1918_OFFSET                         0x4C
    #define RSZ_VFILT1918_OFFSET                         0x8C
        #define COEF19_BITPOS                            16
        #define COEF18_BITPOS                            0
    #define RSZ_HFILT2120_OFFSET                         0x50
    #define RSZ_VFILT2120_OFFSET                         0x90
        #define COEF21_BITPOS                            16
        #define COEF20_BITPOS                            0
    #define RSZ_HFILT2322_OFFSET                         0x54
    #define RSZ_VFILT2322_OFFSET                         0x94
        #define COEF23_BITPOS                            16
        #define COEF22_BITPOS                            0
    #define RSZ_HFILT2524_OFFSET                         0x58
    #define RSZ_VFILT2524_OFFSET                         0x98
        #define COEF25_BITPOS                            16
        #define COEF24_BITPOS                            0
    #define RSZ_HFILT2726_OFFSET                         0x5C
    #define RSZ_VFILT2726_OFFSET                         0x9C
        #define COEF27_BITPOS                            16
        #define COEF26_BITPOS                            0
    #define RSZ_HFILT2928_OFFSET                         0x60
    #define RSZ_VFILT2928_OFFSET                         0xA0
        #define COEF29_BITPOS                            16
        #define COEF28_BITPOS                            0
    #define RSZ_HFILT3130_OFFSET                         0x64
    #define RSZ_VFILT3130_OFFSET                         0xA4
        #define COEF31_BITPOS                            16
        #define COEF30_BITPOS                            0
    #define RSZ_YENH_OFFSET                              0xA8
        #define ALGO_BITPOS                              16
        #define GAIN_BITPOS                              12
        #define SLOP_BITPOS                              8
        #define CORE_BITPOS                              0

    /******************/
    /* ISP SBL module */
    /******************/
    #define ISP_SBL_BASE                                 0x480BD200

    #define SBL_PCR_OFFSET                               0x04
        #define CSIB_WBL_OVF_BITPOS                      26
        #define CSIA_WBL_OVF_BITPOS                      25
        #define CCDCPRV_2_RSZ_OVF_BITPOS                 24
        #define CCDC_WBL_OVF_BITPOS                      23
        #define PRV_WBL_OVF_BITPOS                       22
        #define RSZ1_WBL_OVF_BITPOS                      21
        #define RSZ2_WBL_OVF_BITPOS                      20
        #define RSZ3_WBL_OVF_BITPOS                      19
        #define RSZ4_WBL_OVF_BITPOS                      18
        #define H3A_AF_WBL_OVF_BITPOS                    17
        #define H3A_AEAWB_WBL_OVF_BITPOS                 16
    #define SBL_SDR_REQ_EXP_OFFSET                       0xF8
        #define PRV_EXP_BITPOS                           20
        #define PRV_EXP_WIDTH                            10
        #define RSZ_EXP_BITPOS                           10
        #define RSZ_EXP_WIDTH                            10
        #define HIST_EXP_BITPOS                          0
        #define HIST_EXP_WIDTH                           10

    /*******************/
    /* ISP CSI2 module */
    /*******************/
    #define ISP_CSI2A_BASE                               0x480BD800

    #define CSI2_SYSCONFIG_OFFSET                        0x10
    #define CSI2_SYSSTATUS_OFFSET                        0x14
    #define CSI2_IRQSTATUS_OFFSET                        0x18
    #define CSI2_IRQENABLE_OFFSET                        0x1C
        #define OCP_ERR_IRQ_BITPOS                       14
        #define SHORT_PACKET_IRQ_BITPOS                  13
        #define ECC_CORRECTION_IRQ_BITPOS                12
        #define ECC_NO_CORRECTION_IRQ_BITPOS             11
        #define COMPLEXIO2_ERR_IRQ_BITPOS                10
        #define COMPLEXIO1_ERR_IRQ_BITPOS                9
        #define FIFO_OVF_IRQ_BITPOS                      8
        #define CONTEXT7_BITPOS                          7
        #define CONTEXT6_BITPOS                          6
        #define CONTEXT5_BITPOS                          5
        #define CONTEXT4_BITPOS                          4
        #define CONTEXT3_BITPOS                          3
        #define CONTEXT2_BITPOS                          2
        #define CONTEXT1_BITPOS                          1
        #define CONTEXT0_BITPOS                          0
    #define CSI2_CTRL_OFFSET                             0x40
        #define VP_CLK_EN_BITPOS                         15
        #define VP_ONLY_EN_BITPOS                        11
        #define VP_OUT_CTRL_BITPOS                       8
        #define DBG_EN_BITPOS                            7
        #define FRAME_BITPOS                             3
        #define ECC_EN_BITPOS                            2
        #define IF_EN_BITPOS                             0
    #define CSI2_DBG_H_OFFSET                            0x44
    #define CSI2_DBG_P_OFFSET                            0x68
    #define CSI2_GNQ_OFFSET                              0x48
        #define FIFODEPTH_BITPOS                         2
        #define NBCONTEXTS_BITPOS                        0
    #define CSI2_COMPLEXIO_CFG1_OFFSET                   0x50
        #define RESET_DONE_BITPOS                        29
        #define PWR_CMD_BITPOS                           27
        #define PWR_CMD_WIDTH                            2
        #define PWR_STATUS_BITPOS                        25
        #define PWR_STATUS_WIDTH                         2
        #define PWR_AUTO_BITPOS                          24
        #define DATA2_POL_BITPOS                         11
        #define DATA2_POSITION_BITPOS                    8
        #define DATA1_POL_BITPOS                         7
        #define DATA1_POSITION_BITPOS                    4
        #define CLOCK_POL_BITPOS                         3
        #define CLOCK_POSITION_BITPOS                    0
    #define CSI2_COMPLEXIO1_IRQSTATUS_OFFSET             0x54
    #define CSI2_COMPLEXIO1_IRQENABLE_OFFSET             0x60
        #define STATEALLULPMEXIT_BITPOS                  26
        #define STATEALLULPMENTER_BITPOS                 25
        #define STATEULPM5_BITPOS                        24
        #define STATEULPM4_BITPOS                        23
        #define STATEULPM3_BITPOS                        22
        #define STATEULPM2_BITPOS                        21
        #define STATEULPM1_BITPOS                        20
        #define ERRCONTROL5_BITPOS                       19
        #define ERRCONTROL4_BITPOS                       18
        #define ERRCONTROL3_BITPOS                       17
        #define ERRCONTROL2_BITPOS                       16
        #define ERRCONTROL1_BITPOS                       15
        #define ERRESC5_BITPOS                           14
        #define ERRESC4_BITPOS                           13
        #define ERRESC3_BITPOS                           12
        #define ERRESC2_BITPOS                           11
        #define ERRESC1_BITPOS                           10
        #define ERRSOTSYNCHS5_BITPOS                     9
        #define ERRSOTSYNCHS4_BITPOS                     8
        #define ERRSOTSYNCHS3_BITPOS                     7
        #define ERRSOTSYNCHS2_BITPOS                     6
        #define ERRSOTSYNCHS1_BITPOS                     5
        #define ERRSOTHS5_BITPOS                         4
        #define ERRSOTHS4_BITPOS                         3
        #define ERRSOTHS3_BITPOS                         2
        #define ERRSOTHS2_BITPOS                         1
        #define ERRSOTHS1_BITPOS                         0
    #define CSI2_SHORT_PACKET_OFFSET                     0x5C
        #define SHORT_PACKET_BITPOS                      0
    #define CSI2_TIMING_OFFSET                           0x6C
        #define FORCE_RX_MODE_IO2_BITPOS                 31
        #define STOP_STATE_X16_IO2_BITPOS                30
        #define STOP_STATE_X4_IO2_BITPOS                 29
        #define STOP_STATE_COUNTER_IO2_BITPOS            16
        #define STOP_STATE_COUNTER_IO2_WIDTH             13
        #define FORCE_RX_MODE_IO1_BITPOS                 15
        #define STOP_STATE_X16_IO1_BITPOS                14
        #define STOP_STATE_X4_IO1_BITPOS                 13
        #define STOP_STATE_COUNTER_IO1_BITPOS            0
        #define STOP_STATE_COUNTER_IO1_WIDTH             13
    #define CSI2_CTx_CTRL1_OFFSET(x)                     0x70 + (0x20*(x & 0x7))
        #define FEC_NUMBER_BITPOS                        16
        #define FEC_NUMBER_WIDTH                         8
        #define COUNT_BITPOS                             8
        #define COUNT_WIDTH                              8
        #define EOF_EN_BITPOS                            7
        #define EOL_EN_BITPOS                            6
        #define CS_EN_BITPOS                             5
        #define COUNT_UNLOCK_BITPOS                      4
        #define PING_PONG_BITPOS                         3
        #define LINE_MODULO_BITPOS                       1
        #define CTX_EN_BITPOS                            0
    #define CSI2_CTx_CTRL2_OFFSET(x)                     0x74 + (0x20*(x & 0x7))
        #define FRAME_NUMBER_BITPOS                      16
        #define FRAME_NUMBER_WIDTH                       16
        #define USER_DEF_MAPPING_BITPOS                  13
        #define USER_DEF_MAPPING_WIDTH                   2
        #define VIRTUAL_ID_BITPOS                        11
        #define VIRTUAL_ID_WIDTH                         2
        #define DPCM_PRED_BITPOS                         10
        #define FORMAT_BITPOS                            0
        #define FORMAT_WIDTH                             10
    #define CSI2_CTx_DAT_OFST_OFFSET(x)                  0x78 + (0x20*(x & 0x7))
        #define OFST_BITPOS                              5
        #define OFST_WIDTH                               11
    #define CSI2_CTx_DAT_PING_ADDR_OFFSET(x)             0x7C + (0x20*(x & 0x7))
    #define CSI2_CTx_DAT_PONG_ADDR_OFFSET(x)             0x80 + (0x20*(x & 0x7))
        #define ADDR_CSI2_BITPOS                         5
        #define ADDR_CSI2_WIDTH                          27
    #define CSI2_CTx_IRQENABLE_OFFSET(x)                 0x84 + (0x20*(x & 0x7))
    #define CSI2_CTx_IRQSTATUS_OFFSET(x)                 0x88 + (0x20*(x & 0x7))
        #define ECC_CORRECTION_IRQ_CTx                   8
        #define LINE_NUMBER_IRQ_BITPOS                   7
        #define FRAME_NUMBER_IRQ_BITPOS                  6
        #define CS_IRQ_BITPOS                            5
        #define LE_IRQ_BITPOS                            3
        #define LS_IRQ_BITPOS                            2
        #define FE_IRQ_BITPOS                            1
        #define FS_IRQ_BITPOS                            0
    #define CSI2_CTx_CTRL3_OFFSET(x)                     0x8C + (0x20*(x & 0x7))
        #define ALPHA_BITPOS                             16
        #define ALPHA_WIDTH                              14
        #define LINE_NUMBER_BITPOS                       0
        #define LINE_NUMBER_WIDTH                        16

    /**********************/
    /* ISP CSI2PHY module */
    /**********************/
    #define CSI2PHY_SCP_BASE                             0x480BD970

    #define CSI2PHY_CFG0_OFFSET                          0x00
        #define TCLK_MISS_DETECT                         24
        #define THS_TERM_BITPOS                          8
        #define THS_TERM_WIDTH                           8
        #define THS_SETTLE_BITPOS                        0
        #define THS_SETTLE_WIDTH                         8
    #define CSI2PHY_CFG1_OFFSET                          0x04
        #define RESETDONECTRLCLK_BITPOS                  29
        #define RESETDONERXBYTECLK_BITPOS                28
        #define TCLK_TERM_BITPOS                         18
        #define TCLK_TERM_WIDTH                          7
        #define TCLK_MISS_BITPOS                         8
        #define TCLK_MISS_WIDTH                          2
        #define TCLK_SETTLE_BITPOS                       0
        #define TCLK_SETTLE_WIDTH                        8

    /**************************/
    /* Various CSI2 constants */
    /**************************/
    #define CSI2_FSC                                     0x0
    #define CSI2_FEC                                     0x1
    #define CSI2_LSC                                     0x2
    #define CSI2_LEC                                     0x3
    #define IMMEDIATE                                    0
    #define AFTER_FEC                                    1
    #define DPCM_ADVANCED                                0
    #define DPCM_SIMPLE                                  1
    #define CSI2_YUV420_8bit                             0x18
    #define CSI2_YUV420_10bit                            0x19
    #define CSI2_YUV422_8bit                             0x1E
    #define CSI2_YUV422_10bit                            0x1F
    #define CSI2_RGB565                                  0x22
    #define CSI2_RGB888                                  0x24
    #define CSI2_RAW6                                    0x28
    #define CSI2_RAW7                                    0x29
    #define CSI2_RAW8                                    0x2A
    #define CSI2_RAW10                                   0x2B
    #define CSI2_RAW12                                   0x2C
    #define CSI2_RAW14                                   0x2D
    #define CSI2_RGB666_EXP32_24                         0x33
    #define CSI2_RAW6_EXP8                               0x68
    #define CSI2_RAW7_EXP8                               0x69
    #define CSI2_YUV422_8bit_VP                          0x9E
    #define CSI2_RGB444_EXP16                            0xA0
    #define CSI2_RGB555_EXP16                            0xA1
    #define CSI2_RAW10_EXP16                             0xAB
    #define CSI2_RAW12_EXP16                             0xAC
    #define CSI2_RAW14_EXP16                             0xAD
    #define CSI2_RGB666_EXP32                            0xE3
    #define CSI2_RGB888_EXP32                            0xE4
    #define CSI2_RAW6_DPCM10_VP                          0xE8
    #define CSI2_RAW8_VP                                 0x12A
    #define CSI2_RAW12_VP                                0x12C
    #define CSI2_RAW14_VP                                0x12D
    #define CSI2_RAW10_VP                                0x12F
    #define CSI2_RAW7_DPCM10_EXP16                       0x229
    #define CSI2_RAW6_DPCM10_EXP16                       0x2A8
    #define CSI2_RAW8_DPCM10_EXP16                       0x2AA
    #define CSI2_RAW7_DPCM10_VP                          0x329
    #define CSI2_RAW8_DPCM10_VP                          0x32A

    /****************/
    /* GPIO modules */
    /****************/
    #define GPIO1_BASE                                   0x48310000
    #define GPIO2_BASE                                   0x49050000
    #define GPIO3_BASE                                   0x49052000
    #define GPIO4_BASE                                   0x49054000
    #define GPIO5_BASE                                   0x49056000
    #define GPIO6_BASE                                   0x49058000

    #define GPIO_SYSCONFIG_OFFSET                        0x10
    #define GPIO_SYSSTATUS_OFFSET                        0x14
    #define GPIO_CTRL_OFFSET                             0x30
    #define GPIO_OE_OFFSET                               0x34
    #define GPIO_DATAIN_OFFSET                           0x38
    #define GPIO_DATAOUT_OFFSET                          0x3C
    #define GPIO_CLEARDATAOUT_OFFSET                     0x90
    #define GPIO_SETDATAOUT_OFFSET                       0x94
        #define GPIO_BITPOS(num)                         (num & 0x1F)

    /***************/
    /* GPT modules */
    /***************/
    #define GPT1_BASE                                    0x48318000
    #define GPT2_BASE                                    0x49032000
    #define GPT3_BASE                                    0x49034000
    #define GPT4_BASE                                    0x49036000
    #define GPT5_BASE                                    0x49038000
    #define GPT6_BASE                                    0x4903A000
    #define GPT7_BASE                                    0x4903C000
    #define GPT8_BASE                                    0x4903E000
    #define GPT9_BASE                                    0x49040000
    #define GPT10_BASE                                   0x48086000
    #define GPT11_BASE                                   0x48088000

    #define TIOCP_CFG_OFFSET                             0x10
    #define TISTAT_OFFSET                                0x14
    #define TISR_OFFSET                                  0x18
    #define TCLR_OFFSET                                  0x24
        #define ST_BITPOS                                0
    #define TCRR_OFFSET                                  0x28

    /***************/
    /* I2C modules */
    /***************/
    #define I2C1_BASE                                    0x48070000
    #define I2C2_BASE                                    0x48072000
    #define I2C3_BASE                                    0x48060000

    #define I2C_IE_OFFSET                                0x4
        #define XDR_IE_BITPOS                            14
        #define RDR_IE_BITPOS                            13
        #define AAS_IE_BITPOS                            9
        #define BF_IE_BITPOS                             8
        #define AERR_IE_BITPOS                           7
        #define STC_IE_BITPOS                            6
        #define GC_IE_BITPOS                             5
        #define XRDY_IE_BITPOS                           4
        #define RRDY_IE_BITPOS                           3
        #define ARDY_IE_BITPOS                           2
        #define NACK_IE_BITPOS                           1
        #define AL_IE_BITPOS                             0
    #define I2C_STAT_OFFSET                              0x8
        #define XDR_BITPOS                               14
        #define RDR_BITPOS                               13
        #define BB_BITPOS                                12
        #define ROVR_BITPOS                              11
        #define XUDF_BITPOS                              10
        #define AAS_BITPOS                               9
        #define BF_BITPOS                                8
        #define AERR_BITPOS                              7
        #define STC_BITPOS                               6
        #define GC_BITPOS                                5
        #define XRDY_BITPOS                              4
        #define RRDY_BITPOS                              3
        #define ARDY_BITPOS                              2
        #define NACK_BITPOS                              1
        #define AL_BITPOS                                0
    #define I2C_SYSS_OFFSET                              0x10
    #define I2C_BUF_OFFSET                               0x14
        #define RDMA_EN_BITPOS                           15
        #define RXFIFO_CLR_BITPOS                        14
        #define RTRSH_BITPOS                             8
        #define XDMA_EN_BITPOS                           7
        #define TXFIFO_CLR_BITPOS                        6
        #define XTRSH_BITPOS                             0
    #define I2C_CNT_OFFSET                               0x18
        #define DCOUNT_BITPOS                            0
    #define I2C_DATA_OFFSET                              0x1C
        #define DATA_BITPOS                              0
    #define I2C_SYSC_OFFSET                              0x20
    #define I2C_CON_OFFSET                               0x24
        #define I2C_EN_BITPOS                            15
        #define OPMODE_BITPOS                            12
        #define STB_BITPOS                               11
        #define MST_BITPOS                               10
        #define TRX_BITPOS                               9
        #define XSA_BITPOS                               8
        #define XOA0_BITPOS                              7
        #define XOA1_BITPOS                              6
        #define XOA2_BITPOS                              5
        #define XOA3_BITPOS                              4
        #define STP_BITPOS                               1
        #define STT_BITPOS                               0
    #define I2C_OA0_OFFSET                               0x28
        #define MCODE_BITPOS                             13
        #define OA_BITPOS                                0
    #define I2C_SA_OFFSET                                0x2C
        #define SA_BITPOS                                0
    #define I2C_PSC_OFFSET                               0x30
        #define PSC_BITPOS                               0
    #define I2C_SCLL_OFFSET                              0x34
        #define HSSCLL_BITPOS                            8
        #define SCLL_BITPOS                              0
    #define I2C_SCLH_OFFSET                              0x38
        #define HSSCLH_BITPOS                            8
        #define SCLH_BITPOS                              0
    #define I2C_SYSTEST_OFFSET                           0x3C
        #define ST_EN_BITPOS                             15
        #define FREE_BITPOS                              14
        #define TMODE_BITPOS                             12
        #define SSB_BITPOS                               11
        #define SCCBE_O_BITPOS                           4
        #define SCL_I_BITPOS                             3
        #define SCL_O_BITPOS                             2
        #define SDA_I_BITPOS                             1
        #define SDA_O_BITPOS                             0

    /*****************/
    /* MCSPI modules */
    /*****************/
    #define MCSPI1_BASE                                  0x48098000
    #define MCSPI2_BASE                                  0x4809A000
    #define MCSPI3_BASE                                  0x480B8000
    #define MCSPI4_BASE                                  0x480BA000

    #define MCSPI_SYSCONFIG_OFFSET                       0x10
    #define MCSPI_SYSSTATUS_OFFSET                       0x14
    #define MCSPI_IRQSTATUS_OFFSET                       0x18
    #define MCSPI_IRQENABLE_OFFSET                       0x1C
        #define EOW_BITPOS                               17
        #define WKS_BITPOS                               16
        #define RX3_FULL_BITPOS                          14
        #define TX3_UNDERFLOW_BITPOS                     13
        #define TX3_EMPTY_BITPOS                         12
        #define RX2_FULL_BITPOS                          10
        #define TX2_UNDERFLOW_BITPOS                     9
        #define TX2_EMPTY_BITPOS                         8
        #define RX1_FULL_BITPOS                          6
        #define TX1_UNDERFLOW_BITPOS                     5
        #define TX1_EMPTY_BITPOS                         4
        #define RX0_OVERFLOW_BITPOS                      3
        #define RX0_FULL_BITPOS                          2
        #define TX0_UNDERFLOW_BITPOS                     1
        #define TX0_EMPTY_BITPOS                         0
    #define MCSPI_MODULCTRL_OFFSET                       0x28
        #define SYSTEM_TEST_BITPOS                       3
        #define MS_BITPOS                                2
        #define SINGLE_BITPOS                            0
    #define MCSPI_CHxCONF_OFFSET(x)                      0x2C + (0x14*(x & 0x3))
        #define CLKG_BITPOS                              29
        #define FFER_BITPOS                              28
        #define FFEW_BITPOS                              27
        #define TCS_BITPOS                               25
        #define TCS_WIDTH                                2
        #define SBPOL_BITPOS                             24
        #define SBE_BITPOS                               23
        #define SPIENSLV_BITPOS                          21
        #define SPIENSLV_WIDTH                           2
        #define FORCE_BITPOS                             20
        #define TURBO_BITPOS                             19
        #define IS_BITPOS                                18
        #define DPE1_BITPOS                              17
        #define DPE0_BITPOS                              16
        #define DMAR_BITPOS                              15
        #define DMAW_BITPOS                              14
        #define TRM_BITPOS                               12
        #define TRM_WIDTH                                2
        #define WL_BITPOS                                7
        #define WL_WIDTH                                 5
        #define EPOL_BITPOS                              6
        #define CLKD_BITPOS                              2
        #define CLKD_WIDTH                               4
        #define POL_BITPOS                               1
        #define PHA_BITPOS                               0
    #define MCSPI_CHxSTAT_OFFSET(x)                      0x30 + (0x14*(x & 0x3))
        #define RXFFF_BITPOS                             6
        #define RXFFE_BITPOS                             5
        #define TXFFF_BITPOS                             4
        #define TXFFE_BITPOS                             3
        #define EOT_BITPOS                               2
        #define TXS_BITPOS                               1
        #define RXS_BITPOS                               0
    #define MCSPI_CHxCTRL_OFFSET(x)                      0x34 + (0x14*(x & 0x3))
        #define EXTCLK_BITPOS                            8
        #define EXTCLK_WIDTH                             8
        #define EN_BITPOS                                0
    #define MCSPI_TXx_OFFSET(x)                          0x38 + (0x14*(x & 0x3))
        #define TDATA_BITPOS                             0
        #define TDATA_WIDTH                              32
    #define MCSPI_RXx_OFFSET(x)                          0x3C + (0x14*(x & 0x3))
        #define RDATA_BITPOS                             0
        #define RDATA_WIDTH                              32
    #define MCSPI_XFERLEVEL_OFFSET                       0x7C
        #define WCNT_BITPOS                              16
        #define WCNT_WIDTH                               16
        #define AFL_BITPOS                               8
        #define AFL_WIDTH                                6
        #define AEL_BITPOS                               0
        #define AEL_WIDTH                                6

    /***************************/
    /* Various MCSPI constants */
    /***************************/
    #define MCSPI_TXRX                                   0
    #define MCSPI_RX                                     1
    #define MCSPI_TX                                     2

    /**************************/
    /* Common bit definitions */
    /**************************/
    #define IDLEMODE_BITPOS                              3
    #define ENAWAKEUP_BITPOS                             2
    #define MIDLEMODE_BITPOS                             12
    #define MIDLEMODE_WIDTH                              2
    #define CLOCKACTIVITY_BITPOS                         8
    #define CLOCKACTIVITY_WIDTH                          2
    #define SIDLEMODE_BITPOS                             3
    #define SIDLEMODE_WIDTH                              2
    #define ENWAKEUP_BITPOS                              2
    #define SOFTRESET_BITPOS                             1
    #define AUTOIDLE_BITPOS                              0
    #define RESETDONE_BITPOS                             0
    #define STATE_BITPOS                                 0
    #define CLKACTIVITY_BITPOS                           0
    #define BUSY_BITPOS                                  1
    #define ENABLE_BITPOS                                0

    /*********************/
    /* Various constants */
    /*********************/
    #define LOW                                          0
    #define HIGH                                         1
    #define POSITIVE                                     0
    #define NEGATIVE                                     1
    #define INPUT                                        0
    #define OUTPUT                                       1

    /********************/
    /* Type definitions */
    /********************/
    typedef enum {
        GPIO1 = 1,
        GPIO2 = 2,
        GPIO3 = 3,
        GPIO4 = 4,
        GPIO5 = 5,
        GPIO6 = 6
    } GPIO_Module_t;

    typedef enum {
        MCSPI1 = 1,
        MCSPI2 = 2,
        MCSPI3 = 3,
        MCSPI4 = 4
    } MCSPI_Module_t;
#endif
