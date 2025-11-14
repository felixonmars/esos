/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-06-28     misonyo     the first version.
 */
 
#ifndef DRV_CAN_H__
#define DRV_CAN_H__
#include <register_defination.h>

/* ----------------------------------------------------------------------------
   -- CAN Peripheral Access Layer
   ---------------------------------------------------------------------------- */

/* FLEXCAN module features */

/** CAN - Register Layout Typedef */
typedef struct {
  uint32_t MCR;                               /**< Module Configuration Register, offset: 0x0 */
  uint32_t CTRL1;                             /**< Control 1 Register..Control 1 register, offset: 0x4 */
  uint32_t TIMER;                             /**< Free Running Timer Register..Free Running Timer, offset: 0x8 */
  uint8_t RESERVED_0[4];
  uint32_t RXMGMASK;                          /**< Rx Mailboxes Global Mask Register, offset: 0x10 */
  uint32_t RX14MASK;                          /**< Rx Buffer 14 Mask Register..Rx 14 Mask register, offset: 0x14 */
  uint32_t RX15MASK;                          /**< Rx Buffer 15 Mask Register..Rx 15 Mask register, offset: 0x18 */
  uint32_t ECR;                               /**< Error Counter Register..Error Counter, offset: 0x1C */
  uint32_t ESR1;                              /**< Error and Status 1 Register..Error and Status 1 register, offset: 0x20 */
  uint32_t IMASK2;                            /**< Interrupt Masks 2 Register..Interrupt Masks 2 register, offset: 0x24 */
  uint32_t IMASK1;                            /**< Interrupt Masks 1 Register..Interrupt Masks 1 register, offset: 0x28 */
  uint32_t IFLAG2;                            /**< Interrupt Flags 2 Register..Interrupt Flags 2 register, offset: 0x2C */
  uint32_t IFLAG1;                            /**< Interrupt Flags 1 Register..Interrupt Flags 1 register, offset: 0x30 */
  uint32_t CTRL2;                             /**< Control 2 Register..Control 2 register, offset: 0x34 */
  uint32_t ESR2;                              /**< Error and Status 2 Register..Error and Status 2 register, offset: 0x38 */
  uint8_t RESERVED_1[8];
  uint32_t CRCR;                              /**< CRC Register, offset: 0x44 */
  uint32_t RXFGMASK;                          /**< Rx FIFO Global Mask Register..Legacy Rx FIFO Global Mask register, offset: 0x48 */
  uint32_t RXFIR;                             /**< Rx FIFO Information Register..Legacy Rx FIFO Information Register, offset: 0x4C */
  uint32_t CBT;                               /**< CAN Bit Timing Register, offset: 0x50 */
  uint8_t RESERVED_2[44];
  struct {                                         /* offset: 0x80, array step: 0x10 */
      uint32_t CS;                                /**< Message Buffer 0 CS Register..Message Buffer 63 CS Register, array offset: 0x80, array step: 0x10 */
      uint32_t ID;                                /**< Message Buffer 0 ID Register..Message Buffer 63 ID Register, array offset: 0x84, array step: 0x10 */
      uint32_t WORD0;                             /**< Message Buffer 0 WORD0 Register..Message Buffer 63 WORD0 Register, array offset: 0x88, array step: 0x10 */
      uint32_t WORD1;                             /**< Message Buffer 0 WORD1 Register..Message Buffer 63 WORD1 Register, array offset: 0x8C, array step: 0x10 */
  } MB[64];
  uint8_t RESERVED_3[1024];
  uint32_t RXIMR[64];                         /**< Rx Individual Mask Registers, array offset: 0x880, array step: 0x4 */
  uint8_t RESERVED_4[96];
  uint32_t GFWR;                              /**< Glitch Filter Width Registers, offset: 0x9E0 */
  uint8_t RESERVED_5[524];
  uint32_t EPRS;                              /**< Enhanced CAN Bit Timing Prescalers, offset: 0xBF0 */
  uint32_t ENCBT;                             /**< Enhanced Nominal CAN Bit Timing, offset: 0xBF4 */
  uint32_t EDCBT;                             /**< Enhanced Data Phase CAN bit Timing, offset: 0xBF8 */
  uint32_t ETDC;                              /**< Enhanced Transceiver Delay Compensation, offset: 0xBFC */
  uint32_t FDCTRL;                            /**< CAN FD Control Register, offset: 0xC00 */
  uint32_t FDCBT;                             /**< CAN FD Bit Timing Register, offset: 0xC04 */
  uint32_t FDCRC;                             /**< CAN FD CRC Register, offset: 0xC08 */
  uint32_t ERFCR;                             /**< Enhanced Rx FIFO Control Register, offset: 0xC0C */
  uint32_t ERFIER;                            /**< Enhanced Rx FIFO Interrupt Enable register, offset: 0xC10 */
  uint32_t ERFSR;                             /**< Enhanced Rx FIFO Status Register, offset: 0xC14 */
  uint8_t RESERVED_6[24];
  uint32_t HR_TIME_STAMP[64];                 /**< High Resolution Time Stamp, array offset: 0xC30, array step: 0x4 */
  uint8_t RESERVED_7[8912];
  uint32_t ERFFEL[128];                       /**< Enhanced Rx FIFO Filter Element, array offset: 0x3000, array step: 0x4 */
} CAN_Type;

/* ----------------------------------------------------------------------------
   -- CAN Register Masks
   ---------------------------------------------------------------------------- */

/*!
 * @addtogroup CAN_Register_Masks CAN Register Masks
 * @{
 */

/*! @name MCR - Module Configuration Register */
/*! @{ */
#define CAN_MCR_MAXMB_MASK                       (0x7FU)
#define CAN_MCR_MAXMB_SHIFT                      (0U)
#define CAN_MCR_MAXMB(x)                         (((uint32_t)(((uint32_t)(x)) << CAN_MCR_MAXMB_SHIFT)) & CAN_MCR_MAXMB_MASK)
#define CAN_MCR_IDAM_MASK                        (0x300U)
#define CAN_MCR_IDAM_SHIFT                       (8U)
/*! IDAM
 *  0b00..Format A One full ID (standard or extended) per ID filter Table element.
 *  0b01..Format B Two full standard IDs or two partial 14-bit extended IDs per ID filter Table element.
 *  0b10..Format C Four partial 8-bit IDs (standard or extended) per ID filter Table element.
 *  0b11..Format D All frames rejected.
 */
#define CAN_MCR_IDAM(x)                          (((uint32_t)(((uint32_t)(x)) << CAN_MCR_IDAM_SHIFT)) & CAN_MCR_IDAM_MASK)
#define CAN_MCR_FDEN_MASK                        (0x800U)
#define CAN_MCR_FDEN_SHIFT                       (11U)
/*! FDEN - CAN FD operation enable
 *  0b1..CAN FD is enabled. FlexCAN is able to receive and transmit messages in both CAN FD and CAN 2.0 formats.
 *  0b0..CAN FD is disabled. FlexCAN is able to receive and transmit messages in CAN 2.0 format.
 */
#define CAN_MCR_FDEN(x)                          (((uint32_t)(((uint32_t)(x)) << CAN_MCR_FDEN_SHIFT)) & CAN_MCR_FDEN_MASK)
#define CAN_MCR_AEN_MASK                         (0x1000U)
#define CAN_MCR_AEN_SHIFT                        (12U)
/*! AEN
 *  0b1..Abort enabled
 *  0b0..Abort disabled
 */
#define CAN_MCR_AEN(x)                           (((uint32_t)(((uint32_t)(x)) << CAN_MCR_AEN_SHIFT)) & CAN_MCR_AEN_MASK)
#define CAN_MCR_LPRIOEN_MASK                     (0x2000U)
#define CAN_MCR_LPRIOEN_SHIFT                    (13U)
/*! LPRIOEN
 *  0b1..Local Priority enabled
 *  0b0..Local Priority disabled
 */
#define CAN_MCR_LPRIOEN(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_MCR_LPRIOEN_SHIFT)) & CAN_MCR_LPRIOEN_MASK)
#define CAN_MCR_DMA_MASK                         (0x8000U)
#define CAN_MCR_DMA_SHIFT                        (15U)
/*! DMA - DMA Enable
 *  0b0..DMA feature for Legacy RX FIFO or Enhanced Rx FIFO are disabled.
 *  0b1..DMA feature for Legacy RX FIFO or Enhanced Rx FIFO are enabled.
 */
#define CAN_MCR_DMA(x)                           (((uint32_t)(((uint32_t)(x)) << CAN_MCR_DMA_SHIFT)) & CAN_MCR_DMA_MASK)
#define CAN_MCR_IRMQ_MASK                        (0x10000U)
#define CAN_MCR_IRMQ_SHIFT                       (16U)
/*! IRMQ
 *  0b1..Individual Rx masking and queue feature are enabled.
 *  0b0..Individual Rx masking and queue feature are disabled.For backward compatibility, the reading of C/S word locks the MB even if it is EMPTY.
 */
#define CAN_MCR_IRMQ(x)                          (((uint32_t)(((uint32_t)(x)) << CAN_MCR_IRMQ_SHIFT)) & CAN_MCR_IRMQ_MASK)
#define CAN_MCR_SRXDIS_MASK                      (0x20000U)
#define CAN_MCR_SRXDIS_SHIFT                     (17U)
/*! SRXDIS
 *  0b1..Self reception disabled
 *  0b0..Self reception enabled
 */
#define CAN_MCR_SRXDIS(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_MCR_SRXDIS_SHIFT)) & CAN_MCR_SRXDIS_MASK)
#define CAN_MCR_DOZE_MASK                        (0x40000U)
#define CAN_MCR_DOZE_SHIFT                       (18U)
/*! DOZE - Doze Mode Enable
 *  0b0..FlexCAN is not enabled to enter low-power mode when Doze mode is requested.
 *  0b1..FlexCAN is enabled to enter low-power mode when Doze mode is requested.
 */
#define CAN_MCR_DOZE(x)                          (((uint32_t)(((uint32_t)(x)) << CAN_MCR_DOZE_SHIFT)) & CAN_MCR_DOZE_MASK)
#define CAN_MCR_WAKSRC_MASK                      (0x80000U)
#define CAN_MCR_WAKSRC_SHIFT                     (19U)
/*! WAKSRC
 *  0b1..FLEXCAN uses the filtered FLEXCAN_RX input to detect recessive to dominant edges on the CAN bus
 *  0b0..FLEXCAN uses the unfiltered FLEXCAN_RX input to detect recessive to dominant edges on the CAN bus.
 */
#define CAN_MCR_WAKSRC(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_MCR_WAKSRC_SHIFT)) & CAN_MCR_WAKSRC_MASK)
#define CAN_MCR_LPMACK_MASK                      (0x100000U)
#define CAN_MCR_LPMACK_SHIFT                     (20U)
/*! LPMACK
 *  0b1..FLEXCAN is either in Disable Mode, or Stop mode
 *  0b0..FLEXCAN not in any of the low power modes
 */
#define CAN_MCR_LPMACK(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_MCR_LPMACK_SHIFT)) & CAN_MCR_LPMACK_MASK)
#define CAN_MCR_WRNEN_MASK                       (0x200000U)
#define CAN_MCR_WRNEN_SHIFT                      (21U)
/*! WRNEN
 *  0b1..TWRN_INT and RWRN_INT bits are set when the respective error counter transition from <96 to >= 96.
 *  0b0..TWRN_INT and RWRN_INT bits are zero, independent of the values in the error counters.
 */
#define CAN_MCR_WRNEN(x)                         (((uint32_t)(((uint32_t)(x)) << CAN_MCR_WRNEN_SHIFT)) & CAN_MCR_WRNEN_MASK)
#define CAN_MCR_SLFWAK_MASK                      (0x400000U)
#define CAN_MCR_SLFWAK_SHIFT                     (22U)
/*! SLFWAK
 *  0b1..FLEXCAN Self Wake Up feature is enabled
 *  0b0..FLEXCAN Self Wake Up feature is disabled
 */
#define CAN_MCR_SLFWAK(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_MCR_SLFWAK_SHIFT)) & CAN_MCR_SLFWAK_MASK)
#define CAN_MCR_SUPV_MASK                        (0x800000U)
#define CAN_MCR_SUPV_SHIFT                       (23U)
/*! SUPV
 *  0b1..FlexCAN is in Supervisor Mode. Affected registers allow only Supervisor access. Unrestricted access behaves as though the access was done to an unimplemented register location
 *  0b0..FlexCAN is in User Mode. Affected registers allow both Supervisor and Unrestricted accesses
 */
#define CAN_MCR_SUPV(x)                          (((uint32_t)(((uint32_t)(x)) << CAN_MCR_SUPV_SHIFT)) & CAN_MCR_SUPV_MASK)
#define CAN_MCR_FRZACK_MASK                      (0x1000000U)
#define CAN_MCR_FRZACK_SHIFT                     (24U)
/*! FRZACK
 *  0b1..FLEXCAN in Freeze Mode, prescaler stopped
 *  0b0..FLEXCAN not in Freeze Mode, prescaler running
 */
#define CAN_MCR_FRZACK(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_MCR_FRZACK_SHIFT)) & CAN_MCR_FRZACK_MASK)
#define CAN_MCR_SOFTRST_MASK                     (0x2000000U)
#define CAN_MCR_SOFTRST_SHIFT                    (25U)
/*! SOFTRST
 *  0b1..Reset the registers
 *  0b0..No reset request
 */
#define CAN_MCR_SOFTRST(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_MCR_SOFTRST_SHIFT)) & CAN_MCR_SOFTRST_MASK)
#define CAN_MCR_WAKMSK_MASK                      (0x4000000U)
#define CAN_MCR_WAKMSK_SHIFT                     (26U)
/*! WAKMSK
 *  0b1..Wake Up Interrupt is enabled
 *  0b0..Wake Up Interrupt is disabled
 */
#define CAN_MCR_WAKMSK(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_MCR_WAKMSK_SHIFT)) & CAN_MCR_WAKMSK_MASK)
#define CAN_MCR_NOTRDY_MASK                      (0x8000000U)
#define CAN_MCR_NOTRDY_SHIFT                     (27U)
/*! NOTRDY
 *  0b1..FLEXCAN module is either in Disable Mode, Stop Mode or Freeze Mode
 *  0b0..FLEXCAN module is either in Normal Mode, Listen-Only Mode or Loop-Back Mode
 */
#define CAN_MCR_NOTRDY(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_MCR_NOTRDY_SHIFT)) & CAN_MCR_NOTRDY_MASK)
#define CAN_MCR_HALT_MASK                        (0x10000000U)
#define CAN_MCR_HALT_SHIFT                       (28U)
/*! HALT
 *  0b1..Enters Freeze Mode if the FRZ bit is asserted.
 *  0b0..No Freeze Mode request.
 */
#define CAN_MCR_HALT(x)                          (((uint32_t)(((uint32_t)(x)) << CAN_MCR_HALT_SHIFT)) & CAN_MCR_HALT_MASK)
#define CAN_MCR_RFEN_MASK                        (0x20000000U)
#define CAN_MCR_RFEN_SHIFT                       (29U)
/*! RFEN
 *  0b1..FIFO enabled
 *  0b0..FIFO not enabled
 */
#define CAN_MCR_RFEN(x)                          (((uint32_t)(((uint32_t)(x)) << CAN_MCR_RFEN_SHIFT)) & CAN_MCR_RFEN_MASK)
#define CAN_MCR_FRZ_MASK                         (0x40000000U)
#define CAN_MCR_FRZ_SHIFT                        (30U)
/*! FRZ
 *  0b1..Enabled to enter Freeze Mode
 *  0b0..Not enabled to enter Freeze Mode
 */
#define CAN_MCR_FRZ(x)                           (((uint32_t)(((uint32_t)(x)) << CAN_MCR_FRZ_SHIFT)) & CAN_MCR_FRZ_MASK)
#define CAN_MCR_MDIS_MASK                        (0x80000000U)
#define CAN_MCR_MDIS_SHIFT                       (31U)
/*! MDIS
 *  0b1..Disable the FLEXCAN module
 *  0b0..Enable the FLEXCAN module
 */
#define CAN_MCR_MDIS(x)                          (((uint32_t)(((uint32_t)(x)) << CAN_MCR_MDIS_SHIFT)) & CAN_MCR_MDIS_MASK)
/*! @} */

/*! @name CTRL1 - Control 1 Register..Control 1 register */
/*! @{ */
#define CAN_CTRL1_PROPSEG_MASK                   (0x7U)
#define CAN_CTRL1_PROPSEG_SHIFT                  (0U)
#define CAN_CTRL1_PROPSEG(x)                     (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_PROPSEG_SHIFT)) & CAN_CTRL1_PROPSEG_MASK)
#define CAN_CTRL1_LOM_MASK                       (0x8U)
#define CAN_CTRL1_LOM_SHIFT                      (3U)
/*! LOM
 *  0b1..FLEXCAN module operates in Listen Only Mode
 *  0b0..Listen Only Mode is deactivated
 */
#define CAN_CTRL1_LOM(x)                         (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_LOM_SHIFT)) & CAN_CTRL1_LOM_MASK)
#define CAN_CTRL1_LBUF_MASK                      (0x10U)
#define CAN_CTRL1_LBUF_SHIFT                     (4U)
/*! LBUF
 *  0b1..Lowest number buffer is transmitted first
 *  0b0..Buffer with highest priority is transmitted first
 */
#define CAN_CTRL1_LBUF(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_LBUF_SHIFT)) & CAN_CTRL1_LBUF_MASK)
#define CAN_CTRL1_TSYN_MASK                      (0x20U)
#define CAN_CTRL1_TSYN_SHIFT                     (5U)
/*! TSYN
 *  0b1..Timer Sync feature enabled
 *  0b0..Timer Sync feature disabled
 */
#define CAN_CTRL1_TSYN(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_TSYN_SHIFT)) & CAN_CTRL1_TSYN_MASK)
#define CAN_CTRL1_BOFFREC_MASK                   (0x40U)
#define CAN_CTRL1_BOFFREC_SHIFT                  (6U)
/*! BOFFREC
 *  0b1..Automatic recovering from Bus Off state disabled
 *  0b0..Automatic recovering from Bus Off state enabled, according to CAN Spec 2.0 part B
 */
#define CAN_CTRL1_BOFFREC(x)                     (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_BOFFREC_SHIFT)) & CAN_CTRL1_BOFFREC_MASK)
#define CAN_CTRL1_SMP_MASK                       (0x80U)
#define CAN_CTRL1_SMP_SHIFT                      (7U)
/*! SMP
 *  0b1..Three samples are used to determine the value of the received bit: the regular one (sample point) and 2 preceding samples, a majority rule is used
 *  0b0..Just one sample is used to determine the bit value
 */
#define CAN_CTRL1_SMP(x)                         (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_SMP_SHIFT)) & CAN_CTRL1_SMP_MASK)
#define CAN_CTRL1_RWRNMSK_MASK                   (0x400U)
#define CAN_CTRL1_RWRNMSK_SHIFT                  (10U)
/*! RWRNMSK
 *  0b1..Rx Warning Interrupt enabled
 *  0b0..Rx Warning Interrupt disabled
 */
#define CAN_CTRL1_RWRNMSK(x)                     (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_RWRNMSK_SHIFT)) & CAN_CTRL1_RWRNMSK_MASK)
#define CAN_CTRL1_TWRNMSK_MASK                   (0x800U)
#define CAN_CTRL1_TWRNMSK_SHIFT                  (11U)
/*! TWRNMSK
 *  0b1..Tx Warning Interrupt enabled
 *  0b0..Tx Warning Interrupt disabled
 */
#define CAN_CTRL1_TWRNMSK(x)                     (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_TWRNMSK_SHIFT)) & CAN_CTRL1_TWRNMSK_MASK)
#define CAN_CTRL1_LPB_MASK                       (0x1000U)
#define CAN_CTRL1_LPB_SHIFT                      (12U)
/*! LPB
 *  0b1..Loop Back enabled
 *  0b0..Loop Back disabled
 */
#define CAN_CTRL1_LPB(x)                         (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_LPB_SHIFT)) & CAN_CTRL1_LPB_MASK)
#define CAN_CTRL1_CLKSRC_MASK                    (0x2000U)
#define CAN_CTRL1_CLKSRC_SHIFT                   (13U)
/*! CLKSRC - CAN Engine Clock Source
 *  0b0..The CAN engine clock source is the oscillator clock. Under this condition, the oscillator clock frequency must be lower than the bus clock.
 *  0b1..The CAN engine clock source is the peripheral clock.
 */
#define CAN_CTRL1_CLKSRC(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_CLKSRC_SHIFT)) & CAN_CTRL1_CLKSRC_MASK)
#define CAN_CTRL1_ERRMSK_MASK                    (0x4000U)
#define CAN_CTRL1_ERRMSK_SHIFT                   (14U)
/*! ERRMSK
 *  0b1..Error interrupt enabled
 *  0b0..Error interrupt disabled
 */
#define CAN_CTRL1_ERRMSK(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_ERRMSK_SHIFT)) & CAN_CTRL1_ERRMSK_MASK)
#define CAN_CTRL1_BOFFMSK_MASK                   (0x8000U)
#define CAN_CTRL1_BOFFMSK_SHIFT                  (15U)
/*! BOFFMSK
 *  0b1..Bus Off interrupt enabled
 *  0b0..Bus Off interrupt disabled
 */
#define CAN_CTRL1_BOFFMSK(x)                     (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_BOFFMSK_SHIFT)) & CAN_CTRL1_BOFFMSK_MASK)
#define CAN_CTRL1_PSEG2_MASK                     (0x70000U)
#define CAN_CTRL1_PSEG2_SHIFT                    (16U)
#define CAN_CTRL1_PSEG2(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_PSEG2_SHIFT)) & CAN_CTRL1_PSEG2_MASK)
#define CAN_CTRL1_PSEG1_MASK                     (0x380000U)
#define CAN_CTRL1_PSEG1_SHIFT                    (19U)
#define CAN_CTRL1_PSEG1(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_PSEG1_SHIFT)) & CAN_CTRL1_PSEG1_MASK)
#define CAN_CTRL1_RJW_MASK                       (0xC00000U)
#define CAN_CTRL1_RJW_SHIFT                      (22U)
#define CAN_CTRL1_RJW(x)                         (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_RJW_SHIFT)) & CAN_CTRL1_RJW_MASK)
#define CAN_CTRL1_PRESDIV_MASK                   (0xFF000000U)
#define CAN_CTRL1_PRESDIV_SHIFT                  (24U)
#define CAN_CTRL1_PRESDIV(x)                     (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_PRESDIV_SHIFT)) & CAN_CTRL1_PRESDIV_MASK)
/*! @} */

/*! @name TIMER - Free Running Timer Register..Free Running Timer */
/*! @{ */
#define CAN_TIMER_TIMER_MASK                     (0xFFFFU)
#define CAN_TIMER_TIMER_SHIFT                    (0U)
#define CAN_TIMER_TIMER(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_TIMER_TIMER_SHIFT)) & CAN_TIMER_TIMER_MASK)
/*! @} */

/*! @name RXMGMASK - Rx Mailboxes Global Mask Register */
/*! @{ */
#define CAN_RXMGMASK_MG_MASK                     (0xFFFFFFFFU)
#define CAN_RXMGMASK_MG_SHIFT                    (0U)
/*! MG
 *  0b00000000000000000000000000000001..The corresponding bit in the filter is checked against the one received
 *  0b00000000000000000000000000000000..the corresponding bit in the filter is "don't care"
 */
#define CAN_RXMGMASK_MG(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_RXMGMASK_MG_SHIFT)) & CAN_RXMGMASK_MG_MASK)
/*! @} */

/*! @name RX14MASK - Rx Buffer 14 Mask Register..Rx 14 Mask register */
/*! @{ */
#define CAN_RX14MASK_RX14M_MASK                  (0xFFFFFFFFU)
#define CAN_RX14MASK_RX14M_SHIFT                 (0U)
/*! RX14M
 *  0b00000000000000000000000000000001..The corresponding bit in the filter is checked
 *  0b00000000000000000000000000000000..the corresponding bit in the filter is "don't care"
 */
#define CAN_RX14MASK_RX14M(x)                    (((uint32_t)(((uint32_t)(x)) << CAN_RX14MASK_RX14M_SHIFT)) & CAN_RX14MASK_RX14M_MASK)
/*! @} */

/*! @name RX15MASK - Rx Buffer 15 Mask Register..Rx 15 Mask register */
/*! @{ */
#define CAN_RX15MASK_RX15M_MASK                  (0xFFFFFFFFU)
#define CAN_RX15MASK_RX15M_SHIFT                 (0U)
/*! RX15M
 *  0b00000000000000000000000000000001..The corresponding bit in the filter is checked
 *  0b00000000000000000000000000000000..the corresponding bit in the filter is "don't care"
 */
#define CAN_RX15MASK_RX15M(x)                    (((uint32_t)(((uint32_t)(x)) << CAN_RX15MASK_RX15M_SHIFT)) & CAN_RX15MASK_RX15M_MASK)
/*! @} */

/*! @name ECR - Error Counter Register..Error Counter */
/*! @{ */
#define CAN_ECR_TXERRCNT_MASK                    (0xFFU)
#define CAN_ECR_TXERRCNT_SHIFT                   (0U)
#define CAN_ECR_TXERRCNT(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_ECR_TXERRCNT_SHIFT)) & CAN_ECR_TXERRCNT_MASK)
#define CAN_ECR_TX_ERR_COUNTER_MASK              (0xFFU)
#define CAN_ECR_TX_ERR_COUNTER_SHIFT             (0U)
#define CAN_ECR_TX_ERR_COUNTER(x)                (((uint32_t)(((uint32_t)(x)) << CAN_ECR_TX_ERR_COUNTER_SHIFT)) & CAN_ECR_TX_ERR_COUNTER_MASK)
#define CAN_ECR_RXERRCNT_MASK                    (0xFF00U)
#define CAN_ECR_RXERRCNT_SHIFT                   (8U)
#define CAN_ECR_RXERRCNT(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_ECR_RXERRCNT_SHIFT)) & CAN_ECR_RXERRCNT_MASK)
#define CAN_ECR_RX_ERR_COUNTER_MASK              (0xFF00U)
#define CAN_ECR_RX_ERR_COUNTER_SHIFT             (8U)
#define CAN_ECR_RX_ERR_COUNTER(x)                (((uint32_t)(((uint32_t)(x)) << CAN_ECR_RX_ERR_COUNTER_SHIFT)) & CAN_ECR_RX_ERR_COUNTER_MASK)
#define CAN_ECR_TXERRCNT_FAST_MASK               (0xFF0000U)
#define CAN_ECR_TXERRCNT_FAST_SHIFT              (16U)
#define CAN_ECR_TXERRCNT_FAST(x)                 (((uint32_t)(((uint32_t)(x)) << CAN_ECR_TXERRCNT_FAST_SHIFT)) & CAN_ECR_TXERRCNT_FAST_MASK)
#define CAN_ECR_RXERRCNT_FAST_MASK               (0xFF000000U)
#define CAN_ECR_RXERRCNT_FAST_SHIFT              (24U)
#define CAN_ECR_RXERRCNT_FAST(x)                 (((uint32_t)(((uint32_t)(x)) << CAN_ECR_RXERRCNT_FAST_SHIFT)) & CAN_ECR_RXERRCNT_FAST_MASK)
/*! @} */

/*! @name ESR1 - Error and Status 1 Register..Error and Status 1 register */
/*! @{ */
#define CAN_ESR1_WAKINT_MASK                     (0x1U)
#define CAN_ESR1_WAKINT_SHIFT                    (0U)
/*! WAKINT
 *  0b1..Indicates a recessive to dominant transition received on the CAN bus when the FLEXCAN module is in Stop Mode
 *  0b0..No such occurrence
 */
#define CAN_ESR1_WAKINT(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_WAKINT_SHIFT)) & CAN_ESR1_WAKINT_MASK)
#define CAN_ESR1_ERRINT_MASK                     (0x2U)
#define CAN_ESR1_ERRINT_SHIFT                    (1U)
/*! ERRINT
 *  0b1..Indicates setting of any Error Bit in the Error and Status Register
 *  0b0..No such occurrence
 */
#define CAN_ESR1_ERRINT(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_ERRINT_SHIFT)) & CAN_ESR1_ERRINT_MASK)
#define CAN_ESR1_BOFFINT_MASK                    (0x4U)
#define CAN_ESR1_BOFFINT_SHIFT                   (2U)
/*! BOFFINT
 *  0b1..FLEXCAN module entered 'Bus Off' state
 *  0b0..No such occurrence
 */
#define CAN_ESR1_BOFFINT(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_BOFFINT_SHIFT)) & CAN_ESR1_BOFFINT_MASK)
#define CAN_ESR1_RX_MASK                         (0x8U)
#define CAN_ESR1_RX_SHIFT                        (3U)
/*! RX
 *  0b1..FLEXCAN is transmitting a message
 *  0b0..FLEXCAN is receiving a message
 */
#define CAN_ESR1_RX(x)                           (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_RX_SHIFT)) & CAN_ESR1_RX_MASK)
#define CAN_ESR1_FLTCONF_MASK                    (0x30U)
#define CAN_ESR1_FLTCONF_SHIFT                   (4U)
/*! FLTCONF
 *  0b00..Error Active
 *  0b01..Error Passive
 *  0b1x..Bus off
 */
#define CAN_ESR1_FLTCONF(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_FLTCONF_SHIFT)) & CAN_ESR1_FLTCONF_MASK)
#define CAN_ESR1_TX_MASK                         (0x40U)
#define CAN_ESR1_TX_SHIFT                        (6U)
/*! TX
 *  0b1..FLEXCAN is transmitting a message
 *  0b0..FLEXCAN is receiving a message
 */
#define CAN_ESR1_TX(x)                           (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_TX_SHIFT)) & CAN_ESR1_TX_MASK)
#define CAN_ESR1_IDLE_MASK                       (0x80U)
#define CAN_ESR1_IDLE_SHIFT                      (7U)
/*! IDLE
 *  0b1..CAN bus is now IDLE
 *  0b0..No such occurrence
 */
#define CAN_ESR1_IDLE(x)                         (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_IDLE_SHIFT)) & CAN_ESR1_IDLE_MASK)
#define CAN_ESR1_RXWRN_MASK                      (0x100U)
#define CAN_ESR1_RXWRN_SHIFT                     (8U)
/*! RXWRN
 *  0b1..Rx_Err_Counter >= 96
 *  0b0..No such occurrence
 */
#define CAN_ESR1_RXWRN(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_RXWRN_SHIFT)) & CAN_ESR1_RXWRN_MASK)
#define CAN_ESR1_TXWRN_MASK                      (0x200U)
#define CAN_ESR1_TXWRN_SHIFT                     (9U)
/*! TXWRN
 *  0b1..TX_Err_Counter >= 96
 *  0b0..No such occurrence
 */
#define CAN_ESR1_TXWRN(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_TXWRN_SHIFT)) & CAN_ESR1_TXWRN_MASK)
#define CAN_ESR1_STFERR_MASK                     (0x400U)
#define CAN_ESR1_STFERR_SHIFT                    (10U)
/*! STFERR
 *  0b1..A Stuffing Error occurred since last read of this register.
 *  0b0..No such occurrence.
 */
#define CAN_ESR1_STFERR(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_STFERR_SHIFT)) & CAN_ESR1_STFERR_MASK)
#define CAN_ESR1_FRMERR_MASK                     (0x800U)
#define CAN_ESR1_FRMERR_SHIFT                    (11U)
/*! FRMERR
 *  0b1..A Form Error occurred since last read of this register
 *  0b0..No such occurrence
 */
#define CAN_ESR1_FRMERR(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_FRMERR_SHIFT)) & CAN_ESR1_FRMERR_MASK)
#define CAN_ESR1_CRCERR_MASK                     (0x1000U)
#define CAN_ESR1_CRCERR_SHIFT                    (12U)
/*! CRCERR
 *  0b1..A CRC error occurred since last read of this register.
 *  0b0..No such occurrence
 */
#define CAN_ESR1_CRCERR(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_CRCERR_SHIFT)) & CAN_ESR1_CRCERR_MASK)
#define CAN_ESR1_ACKERR_MASK                     (0x2000U)
#define CAN_ESR1_ACKERR_SHIFT                    (13U)
/*! ACKERR
 *  0b1..An ACK error occurred since last read of this register
 *  0b0..No such occurrence
 */
#define CAN_ESR1_ACKERR(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_ACKERR_SHIFT)) & CAN_ESR1_ACKERR_MASK)
#define CAN_ESR1_BIT0ERR_MASK                    (0x4000U)
#define CAN_ESR1_BIT0ERR_SHIFT                   (14U)
/*! BIT0ERR
 *  0b1..At least one bit sent as dominant is received as recessive
 *  0b0..No such occurrence
 */
#define CAN_ESR1_BIT0ERR(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_BIT0ERR_SHIFT)) & CAN_ESR1_BIT0ERR_MASK)
#define CAN_ESR1_BIT1ERR_MASK                    (0x8000U)
#define CAN_ESR1_BIT1ERR_SHIFT                   (15U)
/*! BIT1ERR
 *  0b1..At least one bit sent as recessive is received as dominant
 *  0b0..No such occurrence
 */
#define CAN_ESR1_BIT1ERR(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_BIT1ERR_SHIFT)) & CAN_ESR1_BIT1ERR_MASK)
#define CAN_ESR1_RWRNINT_MASK                    (0x10000U)
#define CAN_ESR1_RWRNINT_SHIFT                   (16U)
/*! RWRNINT
 *  0b1..The Rx error counter transition from < 96 to >= 96
 *  0b0..No such occurrence
 */
#define CAN_ESR1_RWRNINT(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_RWRNINT_SHIFT)) & CAN_ESR1_RWRNINT_MASK)
#define CAN_ESR1_TWRNINT_MASK                    (0x20000U)
#define CAN_ESR1_TWRNINT_SHIFT                   (17U)
/*! TWRNINT
 *  0b1..The Tx error counter transition from < 96 to >= 96
 *  0b0..No such occurrence
 */
#define CAN_ESR1_TWRNINT(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_TWRNINT_SHIFT)) & CAN_ESR1_TWRNINT_MASK)
#define CAN_ESR1_SYNCH_MASK                      (0x40000U)
#define CAN_ESR1_SYNCH_SHIFT                     (18U)
/*! SYNCH
 *  0b1..FlexCAN is synchronized to the CAN bus
 *  0b0..FlexCAN is not synchronized to the CAN bus
 */
#define CAN_ESR1_SYNCH(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_SYNCH_SHIFT)) & CAN_ESR1_SYNCH_MASK)
#define CAN_ESR1_BOFFDONEINT_MASK                (0x80000U)
#define CAN_ESR1_BOFFDONEINT_SHIFT               (19U)
/*! BOFFDONEINT - Bus Off Done Interrupt
 *  0b0..No such occurrence.
 *  0b1..FlexCAN module has completed Bus Off process.
 */
#define CAN_ESR1_BOFFDONEINT(x)                  (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_BOFFDONEINT_SHIFT)) & CAN_ESR1_BOFFDONEINT_MASK)
#define CAN_ESR1_ERRINT_FAST_MASK                (0x100000U)
#define CAN_ESR1_ERRINT_FAST_SHIFT               (20U)
/*! ERRINT_FAST - Error Interrupt for errors detected in the Data Phase of CAN FD
              frames with the BRS bit set
 *  0b0..No such occurrence.
 *  0b1..Indicates setting of any Error Bit detected in the Data Phase of CAN FD frames with the BRS bit set.
 */
#define CAN_ESR1_ERRINT_FAST(x)                  (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_ERRINT_FAST_SHIFT)) & CAN_ESR1_ERRINT_FAST_MASK)
#define CAN_ESR1_ERROVR_MASK                     (0x200000U)
#define CAN_ESR1_ERROVR_SHIFT                    (21U)
/*! ERROVR - Error Overrun bit
 *  0b0..Overrun has not occurred.
 *  0b1..Overrun has occurred.
 */
#define CAN_ESR1_ERROVR(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_ERROVR_SHIFT)) & CAN_ESR1_ERROVR_MASK)
#define CAN_ESR1_STFERR_FAST_MASK                (0x4000000U)
#define CAN_ESR1_STFERR_FAST_SHIFT               (26U)
/*! STFERR_FAST - Stuffing Error in the Data Phase of CAN FD frames with the BRS bit
              set
 *  0b0..No such occurrence.
 *  0b1..A Stuffing Error occurred since last read of this register.
 */
#define CAN_ESR1_STFERR_FAST(x)                  (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_STFERR_FAST_SHIFT)) & CAN_ESR1_STFERR_FAST_MASK)
#define CAN_ESR1_FRMERR_FAST_MASK                (0x8000000U)
#define CAN_ESR1_FRMERR_FAST_SHIFT               (27U)
/*! FRMERR_FAST - Form Error in the Data Phase of CAN FD frames with the BRS bit
              set
 *  0b0..No such occurrence.
 *  0b1..A Form Error occurred since last read of this register.
 */
#define CAN_ESR1_FRMERR_FAST(x)                  (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_FRMERR_FAST_SHIFT)) & CAN_ESR1_FRMERR_FAST_MASK)
#define CAN_ESR1_CRCERR_FAST_MASK                (0x10000000U)
#define CAN_ESR1_CRCERR_FAST_SHIFT               (28U)
/*! CRCERR_FAST - Cyclic Redundancy Check Error in the CRC field of CAN FD frames with
              the BRS bit set
 *  0b0..No such occurrence.
 *  0b1..A CRC error occurred since last read of this register.
 */
#define CAN_ESR1_CRCERR_FAST(x)                  (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_CRCERR_FAST_SHIFT)) & CAN_ESR1_CRCERR_FAST_MASK)
#define CAN_ESR1_BIT0ERR_FAST_MASK               (0x40000000U)
#define CAN_ESR1_BIT0ERR_FAST_SHIFT              (30U)
/*! BIT0ERR_FAST - Bit0 Error in the Data Phase of CAN FD frames with the BRS bit
              set
 *  0b0..No such occurrence.
 *  0b1..At least one bit sent as dominant is received as recessive.
 */
#define CAN_ESR1_BIT0ERR_FAST(x)                 (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_BIT0ERR_FAST_SHIFT)) & CAN_ESR1_BIT0ERR_FAST_MASK)
#define CAN_ESR1_BIT1ERR_FAST_MASK               (0x80000000U)
#define CAN_ESR1_BIT1ERR_FAST_SHIFT              (31U)
/*! BIT1ERR_FAST - Bit1 Error in the Data Phase of CAN FD frames with the BRS bit
              set
 *  0b0..No such occurrence.
 *  0b1..At least one bit sent as recessive is received as dominant.
 */
#define CAN_ESR1_BIT1ERR_FAST(x)                 (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_BIT1ERR_FAST_SHIFT)) & CAN_ESR1_BIT1ERR_FAST_MASK)
/*! @} */

/*! @name IMASK2 - Interrupt Masks 2 Register..Interrupt Masks 2 register */
/*! @{ */
#define CAN_IMASK2_BUF63TO32M_MASK               (0xFFFFFFFFU)
#define CAN_IMASK2_BUF63TO32M_SHIFT              (0U)
#define CAN_IMASK2_BUF63TO32M(x)                 (((uint32_t)(((uint32_t)(x)) << CAN_IMASK2_BUF63TO32M_SHIFT)) & CAN_IMASK2_BUF63TO32M_MASK)
#define CAN_IMASK2_BUFHM_MASK                    (0xFFFFFFFFU)
#define CAN_IMASK2_BUFHM_SHIFT                   (0U)
/*! BUFHM
 *  0b00000000000000000000000000000001..The corresponding buffer Interrupt is enabled
 *  0b00000000000000000000000000000000..The corresponding buffer Interrupt is disabled
 */
#define CAN_IMASK2_BUFHM(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_IMASK2_BUFHM_SHIFT)) & CAN_IMASK2_BUFHM_MASK)
/*! @} */

/*! @name IMASK1 - Interrupt Masks 1 Register..Interrupt Masks 1 register */
/*! @{ */
#define CAN_IMASK1_BUF31TO0M_MASK                (0xFFFFFFFFU)
#define CAN_IMASK1_BUF31TO0M_SHIFT               (0U)
#define CAN_IMASK1_BUF31TO0M(x)                  (((uint32_t)(((uint32_t)(x)) << CAN_IMASK1_BUF31TO0M_SHIFT)) & CAN_IMASK1_BUF31TO0M_MASK)
#define CAN_IMASK1_BUFLM_MASK                    (0xFFFFFFFFU)
#define CAN_IMASK1_BUFLM_SHIFT                   (0U)
/*! BUFLM
 *  0b00000000000000000000000000000001..The corresponding buffer Interrupt is enabled
 *  0b00000000000000000000000000000000..The corresponding buffer Interrupt is disabled
 */
#define CAN_IMASK1_BUFLM(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_IMASK1_BUFLM_SHIFT)) & CAN_IMASK1_BUFLM_MASK)
/*! @} */

/*! @name IFLAG2 - Interrupt Flags 2 Register..Interrupt Flags 2 register */
/*! @{ */
#define CAN_IFLAG2_BUF63TO32I_MASK               (0xFFFFFFFFU)
#define CAN_IFLAG2_BUF63TO32I_SHIFT              (0U)
#define CAN_IFLAG2_BUF63TO32I(x)                 (((uint32_t)(((uint32_t)(x)) << CAN_IFLAG2_BUF63TO32I_SHIFT)) & CAN_IFLAG2_BUF63TO32I_MASK)
#define CAN_IFLAG2_BUFHI_MASK                    (0xFFFFFFFFU)
#define CAN_IFLAG2_BUFHI_SHIFT                   (0U)
/*! BUFHI
 *  0b00000000000000000000000000000001..The corresponding buffer has successfully completed transmission or reception
 *  0b00000000000000000000000000000000..No such occurrence
 */
#define CAN_IFLAG2_BUFHI(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_IFLAG2_BUFHI_SHIFT)) & CAN_IFLAG2_BUFHI_MASK)
/*! @} */

/*! @name IFLAG1 - Interrupt Flags 1 Register..Interrupt Flags 1 register */
/*! @{ */
#define CAN_IFLAG1_BUF0I_MASK                    (0x1U)
#define CAN_IFLAG1_BUF0I_SHIFT                   (0U)
/*! BUF0I -  Buffer MB0 Interrupt Or Clear Legacy
              FIFO bit
 *  0b0..The corresponding buffer has no occurrence of successfully completed transmission or reception when CAN_MCR[RFEN]=0.
 *  0b1..The corresponding buffer has successfully completed transmission or reception when CAN_MCR[RFEN]=0.
 */
#define CAN_IFLAG1_BUF0I(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_IFLAG1_BUF0I_SHIFT)) & CAN_IFLAG1_BUF0I_MASK)
#define CAN_IFLAG1_BUF4TO0I_MASK                 (0x1FU)
#define CAN_IFLAG1_BUF4TO0I_SHIFT                (0U)
/*! BUF4TO0I
 *  0b00001..Corresponding MB completed transmission/reception
 *  0b00000..No such occurrence
 */
#define CAN_IFLAG1_BUF4TO0I(x)                   (((uint32_t)(((uint32_t)(x)) << CAN_IFLAG1_BUF4TO0I_SHIFT)) & CAN_IFLAG1_BUF4TO0I_MASK)
#define CAN_IFLAG1_BUF4TO1I_MASK                 (0x1EU)
#define CAN_IFLAG1_BUF4TO1I_SHIFT                (1U)
#define CAN_IFLAG1_BUF4TO1I(x)                   (((uint32_t)(((uint32_t)(x)) << CAN_IFLAG1_BUF4TO1I_SHIFT)) & CAN_IFLAG1_BUF4TO1I_MASK)
#define CAN_IFLAG1_BUF5I_MASK                    (0x20U)
#define CAN_IFLAG1_BUF5I_SHIFT                   (5U)
/*! BUF5I
 *  0b1..MB5 completed transmission/reception or frames available in the FIFO
 *  0b0..No such occurrence
 */
#define CAN_IFLAG1_BUF5I(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_IFLAG1_BUF5I_SHIFT)) & CAN_IFLAG1_BUF5I_MASK)
#define CAN_IFLAG1_BUF6I_MASK                    (0x40U)
#define CAN_IFLAG1_BUF6I_SHIFT                   (6U)
/*! BUF6I
 *  0b1..MB6 completed transmission/reception or FIFO almost full
 *  0b0..No such occurrence
 */
#define CAN_IFLAG1_BUF6I(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_IFLAG1_BUF6I_SHIFT)) & CAN_IFLAG1_BUF6I_MASK)
#define CAN_IFLAG1_BUF7I_MASK                    (0x80U)
#define CAN_IFLAG1_BUF7I_SHIFT                   (7U)
/*! BUF7I
 *  0b1..MB7 completed transmission/reception or FIFO overflow
 *  0b0..No such occurrence
 */
#define CAN_IFLAG1_BUF7I(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_IFLAG1_BUF7I_SHIFT)) & CAN_IFLAG1_BUF7I_MASK)
#define CAN_IFLAG1_BUF31TO8I_MASK                (0xFFFFFF00U)
#define CAN_IFLAG1_BUF31TO8I_SHIFT               (8U)
/*! BUF31TO8I
 *  0b000000000000000000000001..The corresponding MB has successfully completed transmission or reception
 *  0b000000000000000000000000..No such occurrence
 */
#define CAN_IFLAG1_BUF31TO8I(x)                  (((uint32_t)(((uint32_t)(x)) << CAN_IFLAG1_BUF31TO8I_SHIFT)) & CAN_IFLAG1_BUF31TO8I_MASK)
/*! @} */

/*! @name CTRL2 - Control 2 Register..Control 2 register */
/*! @{ */
#define CAN_CTRL2_TSTAMPCAP_MASK                 (0xC0U)
#define CAN_CTRL2_TSTAMPCAP_SHIFT                (6U)
/*! TSTAMPCAP -  Time Stamp Capture Point
 *  0b00..The high resolution time stamp capture is disabled
 *  0b01..The high resolution time stamp is captured in the end of the CAN frame
 *  0b10..The high resolution time stamp is captured in the start of the CAN frame
 *  0b11..The high resolution time stamp is captured in the start of frame for classical CAN frames and in res bit for CAN FD frames
 */
#define CAN_CTRL2_TSTAMPCAP(x)                   (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_TSTAMPCAP_SHIFT)) & CAN_CTRL2_TSTAMPCAP_MASK)
#define CAN_CTRL2_MBTSBASE_MASK                  (0x300U)
#define CAN_CTRL2_MBTSBASE_SHIFT                 (8U)
/*! MBTSBASE -  Message Buffer Time Stamp Base
 *  0b00..Message Buffer Time Stamp base is CAN_TIMER
 *  0b01..Message Buffer Time Stamp base is lower 16-bits of high resolution timer
 *  0b10..Message Buffer Time Stamp base is upper 16-bits of high resolution timerT
 *  0b11..Reserved.
 */
#define CAN_CTRL2_MBTSBASE(x)                    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_MBTSBASE_SHIFT)) & CAN_CTRL2_MBTSBASE_MASK)
#define CAN_CTRL2_EDFLTDIS_MASK                  (0x800U)
#define CAN_CTRL2_EDFLTDIS_SHIFT                 (11U)
/*! EDFLTDIS - Edge Filter Disable
 *  0b0..Edge Filter is enabled
 *  0b1..Edge Filter is disabled
 */
#define CAN_CTRL2_EDFLTDIS(x)                    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_EDFLTDIS_SHIFT)) & CAN_CTRL2_EDFLTDIS_MASK)
#define CAN_CTRL2_ISOCANFDEN_MASK                (0x1000U)
#define CAN_CTRL2_ISOCANFDEN_SHIFT               (12U)
/*! ISOCANFDEN - ISO CAN FD Enable
 *  0b0..FlexCAN operates using the non-ISO CAN FD protocol.
 *  0b1..FlexCAN operates using the ISO CAN FD protocol (ISO 11898-1).
 */
#define CAN_CTRL2_ISOCANFDEN(x)                  (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_ISOCANFDEN_SHIFT)) & CAN_CTRL2_ISOCANFDEN_MASK)
#define CAN_CTRL2_BTE_MASK                       (0x2000U)
#define CAN_CTRL2_BTE_SHIFT                      (13U)
/*! BTE -  Bit Timing Expansion enable
 *  0b0..CAN Bit timing expansion is disabled.
 *  0b1..CAN bit timing expansion is enabled.
 */
#define CAN_CTRL2_BTE(x)                         (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_BTE_SHIFT)) & CAN_CTRL2_BTE_MASK)
#define CAN_CTRL2_PREXCEN_MASK                   (0x4000U)
#define CAN_CTRL2_PREXCEN_SHIFT                  (14U)
/*! PREXCEN - Protocol Exception Enable
 *  0b0..Protocol Exception is disabled.
 *  0b1..Protocol Exception is enabled.
 */
#define CAN_CTRL2_PREXCEN(x)                     (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_PREXCEN_SHIFT)) & CAN_CTRL2_PREXCEN_MASK)
#define CAN_CTRL2_TIMER_SRC_MASK                 (0x8000U)
#define CAN_CTRL2_TIMER_SRC_SHIFT                (15U)
/*! TIMER_SRC - Timer Source
 *  0b0..The Free Running Timer is clocked by the CAN bit clock, which defines the baud rate on the CAN bus.
 *  0b1..The Free Running Timer is clocked by an external time tick. The period can be either adjusted to be equal to the baud rate on the CAN bus, or a different value as required. See the device specific section for details about the external time tick.
 */
#define CAN_CTRL2_TIMER_SRC(x)                   (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_TIMER_SRC_SHIFT)) & CAN_CTRL2_TIMER_SRC_MASK)
#define CAN_CTRL2_EACEN_MASK                     (0x10000U)
#define CAN_CTRL2_EACEN_SHIFT                    (16U)
/*! EACEN
 *  0b1..Enables the comparison of both Rx Mailbox filter's IDE and RTR bit with their corresponding bits within the incoming frame. Mask bits do apply.
 *  0b0..Rx Mailbox filter's IDE bit is always compared and RTR is never compared despite mask bits.
 */
#define CAN_CTRL2_EACEN(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_EACEN_SHIFT)) & CAN_CTRL2_EACEN_MASK)
#define CAN_CTRL2_RRS_MASK                       (0x20000U)
#define CAN_CTRL2_RRS_SHIFT                      (17U)
/*! RRS
 *  0b1..Remote Request Frame is stored
 *  0b0..Remote Response Frame is generated
 */
#define CAN_CTRL2_RRS(x)                         (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_RRS_SHIFT)) & CAN_CTRL2_RRS_MASK)
#define CAN_CTRL2_MRP_MASK                       (0x40000U)
#define CAN_CTRL2_MRP_SHIFT                      (18U)
/*! MRP
 *  0b1..Matching starts from Mailboxes and continues on Rx FIFO
 *  0b0..Matching starts from Rx FIFO and continues on Mailboxes
 */
#define CAN_CTRL2_MRP(x)                         (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_MRP_SHIFT)) & CAN_CTRL2_MRP_MASK)
#define CAN_CTRL2_TASD_MASK                      (0xF80000U)
#define CAN_CTRL2_TASD_SHIFT                     (19U)
#define CAN_CTRL2_TASD(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_TASD_SHIFT)) & CAN_CTRL2_TASD_MASK)
#define CAN_CTRL2_RFFN_MASK                      (0xF000000U)
#define CAN_CTRL2_RFFN_SHIFT                     (24U)
#define CAN_CTRL2_RFFN(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_RFFN_SHIFT)) & CAN_CTRL2_RFFN_MASK)
#define CAN_CTRL2_WRMFRZ_MASK                    (0x10000000U)
#define CAN_CTRL2_WRMFRZ_SHIFT                   (28U)
/*! WRMFRZ
 *  0b1..Enable unrestricted write access to FlexCAN memory
 *  0b0..Keep the write access restricted in some regions of FlexCAN memory
 */
#define CAN_CTRL2_WRMFRZ(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_WRMFRZ_SHIFT)) & CAN_CTRL2_WRMFRZ_MASK)
#define CAN_CTRL2_BOFFDONEMSK_MASK               (0x40000000U)
#define CAN_CTRL2_BOFFDONEMSK_SHIFT              (30U)
/*! BOFFDONEMSK - Bus Off Done Interrupt Mask
 *  0b0..Bus Off Done interrupt disabled.
 *  0b1..Bus Off Done interrupt enabled.
 */
#define CAN_CTRL2_BOFFDONEMSK(x)                 (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_BOFFDONEMSK_SHIFT)) & CAN_CTRL2_BOFFDONEMSK_MASK)
#define CAN_CTRL2_ERRMSK_FAST_MASK               (0x80000000U)
#define CAN_CTRL2_ERRMSK_FAST_SHIFT              (31U)
/*! ERRMSK_FAST - Error Interrupt Mask for errors detected in the Data Phase of fast
              CAN FD frames
 *  0b0..ERRINT_FAST Error interrupt disabled.
 *  0b1..ERRINT_FAST Error interrupt enabled.
 */
#define CAN_CTRL2_ERRMSK_FAST(x)                 (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_ERRMSK_FAST_SHIFT)) & CAN_CTRL2_ERRMSK_FAST_MASK)
/*! @} */

/*! @name ESR2 - Error and Status 2 Register..Error and Status 2 register */
/*! @{ */
#define CAN_ESR2_IMB_MASK                        (0x2000U)
#define CAN_ESR2_IMB_SHIFT                       (13U)
/*! IMB
 *  0b1..If ESR2[VPS] is asserted, there is at least one inactive Mailbox. LPTM content is the number of the first one.
 *  0b0..If ESR2[VPS] is asserted, the ESR2[LPTM] is not an inactive Mailbox.
 */
#define CAN_ESR2_IMB(x)                          (((uint32_t)(((uint32_t)(x)) << CAN_ESR2_IMB_SHIFT)) & CAN_ESR2_IMB_MASK)
#define CAN_ESR2_VPS_MASK                        (0x4000U)
#define CAN_ESR2_VPS_SHIFT                       (14U)
/*! VPS
 *  0b1..Contents of IMB and LPTM are valid
 *  0b0..Contents of IMB and LPTM are invalid
 */
#define CAN_ESR2_VPS(x)                          (((uint32_t)(((uint32_t)(x)) << CAN_ESR2_VPS_SHIFT)) & CAN_ESR2_VPS_MASK)
#define CAN_ESR2_LPTM_MASK                       (0x7F0000U)
#define CAN_ESR2_LPTM_SHIFT                      (16U)
#define CAN_ESR2_LPTM(x)                         (((uint32_t)(((uint32_t)(x)) << CAN_ESR2_LPTM_SHIFT)) & CAN_ESR2_LPTM_MASK)
/*! @} */

/*! @name CRCR - CRC Register */
/*! @{ */
#define CAN_CRCR_TXCRC_MASK                      (0x7FFFU)
#define CAN_CRCR_TXCRC_SHIFT                     (0U)
#define CAN_CRCR_TXCRC(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_CRCR_TXCRC_SHIFT)) & CAN_CRCR_TXCRC_MASK)
#define CAN_CRCR_MBCRC_MASK                      (0x7F0000U)
#define CAN_CRCR_MBCRC_SHIFT                     (16U)
#define CAN_CRCR_MBCRC(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_CRCR_MBCRC_SHIFT)) & CAN_CRCR_MBCRC_MASK)
/*! @} */

/*! @name RXFGMASK - Rx FIFO Global Mask Register..Legacy Rx FIFO Global Mask register */
/*! @{ */
#define CAN_RXFGMASK_FGM_MASK                    (0xFFFFFFFFU)
#define CAN_RXFGMASK_FGM_SHIFT                   (0U)
/*! FGM
 *  0b00000000000000000000000000000001..The corresponding bit in the filter is checked
 *  0b00000000000000000000000000000000..The corresponding bit in the filter is "don't care"
 */
#define CAN_RXFGMASK_FGM(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_RXFGMASK_FGM_SHIFT)) & CAN_RXFGMASK_FGM_MASK)
/*! @} */

/*! @name RXFIR - Rx FIFO Information Register..Legacy Rx FIFO Information Register */
/*! @{ */
#define CAN_RXFIR_IDHIT_MASK                     (0x1FFU)
#define CAN_RXFIR_IDHIT_SHIFT                    (0U)
#define CAN_RXFIR_IDHIT(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_RXFIR_IDHIT_SHIFT)) & CAN_RXFIR_IDHIT_MASK)
/*! @} */

/*! @name CBT - CAN Bit Timing Register */
/*! @{ */
#define CAN_CBT_EPSEG2_MASK                      (0x1FU)
#define CAN_CBT_EPSEG2_SHIFT                     (0U)
#define CAN_CBT_EPSEG2(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_CBT_EPSEG2_SHIFT)) & CAN_CBT_EPSEG2_MASK)
#define CAN_CBT_EPSEG1_MASK                      (0x3E0U)
#define CAN_CBT_EPSEG1_SHIFT                     (5U)
#define CAN_CBT_EPSEG1(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_CBT_EPSEG1_SHIFT)) & CAN_CBT_EPSEG1_MASK)
#define CAN_CBT_EPROPSEG_MASK                    (0xFC00U)
#define CAN_CBT_EPROPSEG_SHIFT                   (10U)
#define CAN_CBT_EPROPSEG(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_CBT_EPROPSEG_SHIFT)) & CAN_CBT_EPROPSEG_MASK)
#define CAN_CBT_ERJW_MASK                        (0x1F0000U)
#define CAN_CBT_ERJW_SHIFT                       (16U)
#define CAN_CBT_ERJW(x)                          (((uint32_t)(((uint32_t)(x)) << CAN_CBT_ERJW_SHIFT)) & CAN_CBT_ERJW_MASK)
#define CAN_CBT_EPRESDIV_MASK                    (0x7FE00000U)
#define CAN_CBT_EPRESDIV_SHIFT                   (21U)
#define CAN_CBT_EPRESDIV(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_CBT_EPRESDIV_SHIFT)) & CAN_CBT_EPRESDIV_MASK)
#define CAN_CBT_BTF_MASK                         (0x80000000U)
#define CAN_CBT_BTF_SHIFT                        (31U)
/*! BTF - Bit Timing Format Enable
 *  0b0..Extended bit time definitions disabled.
 *  0b1..Extended bit time definitions enabled.
 */
#define CAN_CBT_BTF(x)                           (((uint32_t)(((uint32_t)(x)) << CAN_CBT_BTF_SHIFT)) & CAN_CBT_BTF_MASK)
/*! @} */

/*! @name CS - Message Buffer 0 CS Register..Message Buffer 13 CS Register */
/*! @{ */
#define CAN_CS_TIME_STAMP_MASK                   (0xFFFFU)
#define CAN_CS_TIME_STAMP_SHIFT                  (0U)
#define CAN_CS_TIME_STAMP(x)                     (((uint32_t)(((uint32_t)(x)) << CAN_CS_TIME_STAMP_SHIFT)) & CAN_CS_TIME_STAMP_MASK)
#define CAN_CS_DLC_MASK                          (0xF0000U)
#define CAN_CS_DLC_SHIFT                         (16U)
#define CAN_CS_DLC(x)                            (((uint32_t)(((uint32_t)(x)) << CAN_CS_DLC_SHIFT)) & CAN_CS_DLC_MASK)
#define CAN_CS_RTR_MASK                          (0x100000U)
#define CAN_CS_RTR_SHIFT                         (20U)
#define CAN_CS_RTR(x)                            (((uint32_t)(((uint32_t)(x)) << CAN_CS_RTR_SHIFT)) & CAN_CS_RTR_MASK)
#define CAN_CS_IDE_MASK                          (0x200000U)
#define CAN_CS_IDE_SHIFT                         (21U)
#define CAN_CS_IDE(x)                            (((uint32_t)(((uint32_t)(x)) << CAN_CS_IDE_SHIFT)) & CAN_CS_IDE_MASK)
#define CAN_CS_SRR_MASK                          (0x400000U)
#define CAN_CS_SRR_SHIFT                         (22U)
#define CAN_CS_SRR(x)                            (((uint32_t)(((uint32_t)(x)) << CAN_CS_SRR_SHIFT)) & CAN_CS_SRR_MASK)
#define CAN_CS_CODE_MASK                         (0xF000000U)
#define CAN_CS_CODE_SHIFT                        (24U)
#define CAN_CS_CODE(x)                           (((uint32_t)(((uint32_t)(x)) << CAN_CS_CODE_SHIFT)) & CAN_CS_CODE_MASK)
#define CAN_CS_ESI_MASK                          (0x20000000U)
#define CAN_CS_ESI_SHIFT                         (29U)
#define CAN_CS_ESI(x)                            (((uint32_t)(((uint32_t)(x)) << CAN_CS_ESI_SHIFT)) & CAN_CS_ESI_MASK)
#define CAN_CS_BRS_MASK                          (0x40000000U)
#define CAN_CS_BRS_SHIFT                         (30U)
#define CAN_CS_BRS(x)                            (((uint32_t)(((uint32_t)(x)) << CAN_CS_BRS_SHIFT)) & CAN_CS_BRS_MASK)
#define CAN_CS_EDL_MASK                          (0x80000000U)
#define CAN_CS_EDL_SHIFT                         (31U)
#define CAN_CS_EDL(x)                            (((uint32_t)(((uint32_t)(x)) << CAN_CS_EDL_SHIFT)) & CAN_CS_EDL_MASK)
/*! @} */

/*! @name ID - Message Buffer 0 ID Register..Message Buffer 13 ID Register */
/*! @{ */
#define CAN_ID_EXT_MASK                          (0x3FFFFU)
#define CAN_ID_EXT_SHIFT                         (0U)
#define CAN_ID_EXT(x)                            (((uint32_t)(((uint32_t)(x)) << CAN_ID_EXT_SHIFT)) & CAN_ID_EXT_MASK)
#define CAN_ID_STD_MASK                          (0x1FFC0000U)
#define CAN_ID_STD_SHIFT                         (18U)
#define CAN_ID_STD(x)                            (((uint32_t)(((uint32_t)(x)) << CAN_ID_STD_SHIFT)) & CAN_ID_STD_MASK)
#define CAN_ID_PRIO_MASK                         (0xE0000000U)
#define CAN_ID_PRIO_SHIFT                        (29U)
#define CAN_ID_PRIO(x)                           (((uint32_t)(((uint32_t)(x)) << CAN_ID_PRIO_SHIFT)) & CAN_ID_PRIO_MASK)
/*! @} */

/*! @name RXIMR - Rx Individual Mask Registers */
/*! @{ */
#define CAN_RXIMR_MI_MASK                        (0xFFFFFFFFU)
#define CAN_RXIMR_MI_SHIFT                       (0U)
/*! MI
 *  0b00000000000000000000000000000001..The corresponding bit in the filter is checked
 *  0b00000000000000000000000000000000..the corresponding bit in the filter is "don't care"
 */
#define CAN_RXIMR_MI(x)                          (((uint32_t)(((uint32_t)(x)) << CAN_RXIMR_MI_SHIFT)) & CAN_RXIMR_MI_MASK)
/*! @} */

/* The count of CAN_RXIMR */
#define CAN_RXIMR_COUNT                          (64U)

/*! @name GFWR - Glitch Filter Width Registers */
/*! @{ */
#define CAN_GFWR_GFWR_MASK                       (0xFFU)
#define CAN_GFWR_GFWR_SHIFT                      (0U)
#define CAN_GFWR_GFWR(x)                         (((uint32_t)(((uint32_t)(x)) << CAN_GFWR_GFWR_SHIFT)) & CAN_GFWR_GFWR_MASK)
/*! @} */

/*! @name EPRS - Enhanced CAN Bit Timing Prescalers */
/*! @{ */
#define CAN_EPRS_ENPRESDIV_MASK                  (0x3FFU)
#define CAN_EPRS_ENPRESDIV_SHIFT                 (0U)
#define CAN_EPRS_ENPRESDIV(x)                    (((uint32_t)(((uint32_t)(x)) << CAN_EPRS_ENPRESDIV_SHIFT)) & CAN_EPRS_ENPRESDIV_MASK)
#define CAN_EPRS_EDPRESDIV_MASK                  (0x3FF0000U)
#define CAN_EPRS_EDPRESDIV_SHIFT                 (16U)
#define CAN_EPRS_EDPRESDIV(x)                    (((uint32_t)(((uint32_t)(x)) << CAN_EPRS_EDPRESDIV_SHIFT)) & CAN_EPRS_EDPRESDIV_MASK)
/*! @} */

/*! @name ENCBT - Enhanced Nominal CAN Bit Timing */
/*! @{ */
#define CAN_ENCBT_NTSEG1_MASK                    (0xFFU)
#define CAN_ENCBT_NTSEG1_SHIFT                   (0U)
#define CAN_ENCBT_NTSEG1(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_ENCBT_NTSEG1_SHIFT)) & CAN_ENCBT_NTSEG1_MASK)
#define CAN_ENCBT_NTSEG2_MASK                    (0x7F000U)
#define CAN_ENCBT_NTSEG2_SHIFT                   (12U)
#define CAN_ENCBT_NTSEG2(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_ENCBT_NTSEG2_SHIFT)) & CAN_ENCBT_NTSEG2_MASK)
#define CAN_ENCBT_NRJW_MASK                      (0x1FC00000U)
#define CAN_ENCBT_NRJW_SHIFT                     (22U)
#define CAN_ENCBT_NRJW(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_ENCBT_NRJW_SHIFT)) & CAN_ENCBT_NRJW_MASK)
/*! @} */

/*! @name EDCBT - Enhanced Data Phase CAN bit Timing */
/*! @{ */
#define CAN_EDCBT_DTSEG1_MASK                    (0x1FU)
#define CAN_EDCBT_DTSEG1_SHIFT                   (0U)
#define CAN_EDCBT_DTSEG1(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_EDCBT_DTSEG1_SHIFT)) & CAN_EDCBT_DTSEG1_MASK)
#define CAN_EDCBT_DTSEG2_MASK                    (0xF000U)
#define CAN_EDCBT_DTSEG2_SHIFT                   (12U)
#define CAN_EDCBT_DTSEG2(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_EDCBT_DTSEG2_SHIFT)) & CAN_EDCBT_DTSEG2_MASK)
#define CAN_EDCBT_DRJW_MASK                      (0x3C00000U)
#define CAN_EDCBT_DRJW_SHIFT                     (22U)
#define CAN_EDCBT_DRJW(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_EDCBT_DRJW_SHIFT)) & CAN_EDCBT_DRJW_MASK)
/*! @} */

/*! @name ETDC - Enhanced Transceiver Delay Compensation */
/*! @{ */
#define CAN_ETDC_ETDCVAL_MASK                    (0xFFU)
#define CAN_ETDC_ETDCVAL_SHIFT                   (0U)
#define CAN_ETDC_ETDCVAL(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_ETDC_ETDCVAL_SHIFT)) & CAN_ETDC_ETDCVAL_MASK)
#define CAN_ETDC_ETDCOFF_MASK                    (0x7F0000U)
#define CAN_ETDC_ETDCOFF_SHIFT                   (16U)
#define CAN_ETDC_ETDCOFF(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_ETDC_ETDCOFF_SHIFT)) & CAN_ETDC_ETDCOFF_MASK)
#define CAN_ETDC_TDMDIS_MASK                     (0x80000000U)
#define CAN_ETDC_TDMDIS_SHIFT                    (31U)
/*! TDMDIS -  Transceiver Delay Measurement Disable
 *  0b0..TDC measurement is enabled
 *  0b1..TDC measurement is disabled
 */
#define CAN_ETDC_TDMDIS(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_ETDC_TDMDIS_SHIFT)) & CAN_ETDC_TDMDIS_MASK)
/*! @} */

/*! @name FDCTRL - CAN FD Control Register */
/*! @{ */
#define CAN_FDCTRL_TDCVAL_MASK                   (0x3FU)
#define CAN_FDCTRL_TDCVAL_SHIFT                  (0U)
#define CAN_FDCTRL_TDCVAL(x)                     (((uint32_t)(((uint32_t)(x)) << CAN_FDCTRL_TDCVAL_SHIFT)) & CAN_FDCTRL_TDCVAL_MASK)
#define CAN_FDCTRL_TDCOFF_MASK                   (0x1F00U)
#define CAN_FDCTRL_TDCOFF_SHIFT                  (8U)
#define CAN_FDCTRL_TDCOFF(x)                     (((uint32_t)(((uint32_t)(x)) << CAN_FDCTRL_TDCOFF_SHIFT)) & CAN_FDCTRL_TDCOFF_MASK)
#define CAN_FDCTRL_TDCFAIL_MASK                  (0x4000U)
#define CAN_FDCTRL_TDCFAIL_SHIFT                 (14U)
/*! TDCFAIL - Transceiver Delay Compensation Fail
 *  0b0..Measured loop delay is in range.
 *  0b1..Measured loop delay is out of range.
 */
#define CAN_FDCTRL_TDCFAIL(x)                    (((uint32_t)(((uint32_t)(x)) << CAN_FDCTRL_TDCFAIL_SHIFT)) & CAN_FDCTRL_TDCFAIL_MASK)
#define CAN_FDCTRL_TDCEN_MASK                    (0x8000U)
#define CAN_FDCTRL_TDCEN_SHIFT                   (15U)
/*! TDCEN - Transceiver Delay Compensation Enable
 *  0b0..TDC is disabled
 *  0b1..TDC is enabled
 */
#define CAN_FDCTRL_TDCEN(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_FDCTRL_TDCEN_SHIFT)) & CAN_FDCTRL_TDCEN_MASK)
#define CAN_FDCTRL_MBDSR0_MASK                   (0x30000U)
#define CAN_FDCTRL_MBDSR0_SHIFT                  (16U)
/*! MBDSR0 - Message Buffer Data Size for Region 0
 *  0b00..Selects 8 bytes per Message Buffer.
 *  0b01..Selects 16 bytes per Message Buffer.
 *  0b10..Selects 32 bytes per Message Buffer.
 *  0b11..Selects 64 bytes per Message Buffer.
 */
#define CAN_FDCTRL_MBDSR0(x)                     (((uint32_t)(((uint32_t)(x)) << CAN_FDCTRL_MBDSR0_SHIFT)) & CAN_FDCTRL_MBDSR0_MASK)
#define CAN_FDCTRL_MBDSR1_MASK                   (0x180000U)
#define CAN_FDCTRL_MBDSR1_SHIFT                  (19U)
/*! MBDSR1 - Message Buffer Data Size for Region 1
 *  0b00..Selects 8 bytes per Message Buffer.
 *  0b01..Selects 16 bytes per Message Buffer.
 *  0b10..Selects 32 bytes per Message Buffer.
 *  0b11..Selects 64 bytes per Message Buffer.
 */
#define CAN_FDCTRL_MBDSR1(x)                     (((uint32_t)(((uint32_t)(x)) << CAN_FDCTRL_MBDSR1_SHIFT)) & CAN_FDCTRL_MBDSR1_MASK)
#define CAN_FDCTRL_FDRATE_MASK                   (0x80000000U)
#define CAN_FDCTRL_FDRATE_SHIFT                  (31U)
/*! FDRATE - Bit Rate Switch Enable
 *  0b0..Transmit a frame in nominal rate. The BRS bit in the Tx MB has no effect.
 *  0b1..Transmit a frame with bit rate switching if the BRS bit in the Tx MB is recessive.
 */
#define CAN_FDCTRL_FDRATE(x)                     (((uint32_t)(((uint32_t)(x)) << CAN_FDCTRL_FDRATE_SHIFT)) & CAN_FDCTRL_FDRATE_MASK)
/*! @} */

/*! @name FDCBT - CAN FD Bit Timing Register */
/*! @{ */
#define CAN_FDCBT_FPSEG2_MASK                    (0x7U)
#define CAN_FDCBT_FPSEG2_SHIFT                   (0U)
#define CAN_FDCBT_FPSEG2(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_FDCBT_FPSEG2_SHIFT)) & CAN_FDCBT_FPSEG2_MASK)
#define CAN_FDCBT_FPSEG1_MASK                    (0xE0U)
#define CAN_FDCBT_FPSEG1_SHIFT                   (5U)
#define CAN_FDCBT_FPSEG1(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_FDCBT_FPSEG1_SHIFT)) & CAN_FDCBT_FPSEG1_MASK)
#define CAN_FDCBT_FPROPSEG_MASK                  (0x7C00U)
#define CAN_FDCBT_FPROPSEG_SHIFT                 (10U)
#define CAN_FDCBT_FPROPSEG(x)                    (((uint32_t)(((uint32_t)(x)) << CAN_FDCBT_FPROPSEG_SHIFT)) & CAN_FDCBT_FPROPSEG_MASK)
#define CAN_FDCBT_FRJW_MASK                      (0x70000U)
#define CAN_FDCBT_FRJW_SHIFT                     (16U)
#define CAN_FDCBT_FRJW(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_FDCBT_FRJW_SHIFT)) & CAN_FDCBT_FRJW_MASK)
#define CAN_FDCBT_FPRESDIV_MASK                  (0x3FF00000U)
#define CAN_FDCBT_FPRESDIV_SHIFT                 (20U)
#define CAN_FDCBT_FPRESDIV(x)                    (((uint32_t)(((uint32_t)(x)) << CAN_FDCBT_FPRESDIV_SHIFT)) & CAN_FDCBT_FPRESDIV_MASK)
/*! @} */

/*! @name FDCRC - CAN FD CRC Register */
/*! @{ */
#define CAN_FDCRC_FD_TXCRC_MASK                  (0x1FFFFFU)
#define CAN_FDCRC_FD_TXCRC_SHIFT                 (0U)
#define CAN_FDCRC_FD_TXCRC(x)                    (((uint32_t)(((uint32_t)(x)) << CAN_FDCRC_FD_TXCRC_SHIFT)) & CAN_FDCRC_FD_TXCRC_MASK)
#define CAN_FDCRC_FD_MBCRC_MASK                  (0x7F000000U)
#define CAN_FDCRC_FD_MBCRC_SHIFT                 (24U)
#define CAN_FDCRC_FD_MBCRC(x)                    (((uint32_t)(((uint32_t)(x)) << CAN_FDCRC_FD_MBCRC_SHIFT)) & CAN_FDCRC_FD_MBCRC_MASK)
/*! @} */

/*! @name ERFCR - Enhanced Rx FIFO Control Register */
/*! @{ */
#define CAN_ERFCR_ERFWM_MASK                     (0x1FU)
#define CAN_ERFCR_ERFWM_SHIFT                    (0U)
#define CAN_ERFCR_ERFWM(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_ERFCR_ERFWM_SHIFT)) & CAN_ERFCR_ERFWM_MASK)
#define CAN_ERFCR_NFE_MASK                       (0x3F00U)
#define CAN_ERFCR_NFE_SHIFT                      (8U)
#define CAN_ERFCR_NFE(x)                         (((uint32_t)(((uint32_t)(x)) << CAN_ERFCR_NFE_SHIFT)) & CAN_ERFCR_NFE_MASK)
#define CAN_ERFCR_NEXIF_MASK                     (0x7F0000U)
#define CAN_ERFCR_NEXIF_SHIFT                    (16U)
#define CAN_ERFCR_NEXIF(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_ERFCR_NEXIF_SHIFT)) & CAN_ERFCR_NEXIF_MASK)
#define CAN_ERFCR_DMALW_MASK                     (0x7C000000U)
#define CAN_ERFCR_DMALW_SHIFT                    (26U)
#define CAN_ERFCR_DMALW(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_ERFCR_DMALW_SHIFT)) & CAN_ERFCR_DMALW_MASK)
#define CAN_ERFCR_ERFEN_MASK                     (0x80000000U)
#define CAN_ERFCR_ERFEN_SHIFT                    (31U)
/*! ERFEN -  Enhanced Rx FIFO enable
 *  0b0..Enhanced Rx FIFO is disabled
 *  0b1..Enhanced Rx FIFO is enabled
 */
#define CAN_ERFCR_ERFEN(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_ERFCR_ERFEN_SHIFT)) & CAN_ERFCR_ERFEN_MASK)
/*! @} */

/*! @name ERFIER - Enhanced Rx FIFO Interrupt Enable register */
/*! @{ */
#define CAN_ERFIER_ERFDAIE_MASK                  (0x10000000U)
#define CAN_ERFIER_ERFDAIE_SHIFT                 (28U)
/*! ERFDAIE -  Enhanced Rx FIFO Data Available Interrupt Enable
 *  0b0..Enhanced Rx FIFO Data Available Interrupt is disabled
 *  0b1..Enhanced Rx FIFO Data Available Interrupt is enabled
 */
#define CAN_ERFIER_ERFDAIE(x)                    (((uint32_t)(((uint32_t)(x)) << CAN_ERFIER_ERFDAIE_SHIFT)) & CAN_ERFIER_ERFDAIE_MASK)
#define CAN_ERFIER_ERFWMIIE_MASK                 (0x20000000U)
#define CAN_ERFIER_ERFWMIIE_SHIFT                (29U)
/*! ERFWMIIE -  Enhanced Rx FIFO Watermark Indication Interrupt Enable
 *  0b0..Enhanced Rx FIFO Watermark Interrupt is disabled
 *  0b1..Enhanced Rx FIFO Watermark Interrupt is enabled
 */
#define CAN_ERFIER_ERFWMIIE(x)                   (((uint32_t)(((uint32_t)(x)) << CAN_ERFIER_ERFWMIIE_SHIFT)) & CAN_ERFIER_ERFWMIIE_MASK)
#define CAN_ERFIER_ERFOVFIE_MASK                 (0x40000000U)
#define CAN_ERFIER_ERFOVFIE_SHIFT                (30U)
/*! ERFOVFIE -  Enhanced Rx FIFO Overflow Interrupt Enable
 *  0b0..Enhanced Rx FIFO Overflow is disabled
 *  0b1..Enhanced Rx FIFO Overflow is enabled
 */
#define CAN_ERFIER_ERFOVFIE(x)                   (((uint32_t)(((uint32_t)(x)) << CAN_ERFIER_ERFOVFIE_SHIFT)) & CAN_ERFIER_ERFOVFIE_MASK)
#define CAN_ERFIER_ERFUFWIE_MASK                 (0x80000000U)
#define CAN_ERFIER_ERFUFWIE_SHIFT                (31U)
/*! ERFUFWIE -  Enhanced Rx FIFO Underflow Interrupt Enable
 *  0b0..Enhanced Rx FIFO Underflow interrupt is disabled
 *  0b1..Enhanced Rx FIFO Underflow interrupt is enabled
 */
#define CAN_ERFIER_ERFUFWIE(x)                   (((uint32_t)(((uint32_t)(x)) << CAN_ERFIER_ERFUFWIE_SHIFT)) & CAN_ERFIER_ERFUFWIE_MASK)
/*! @} */

/*! @name ERFSR - Enhanced Rx FIFO Status Register */
/*! @{ */
#define CAN_ERFSR_ERFEL_MASK                     (0x3FU)
#define CAN_ERFSR_ERFEL_SHIFT                    (0U)
#define CAN_ERFSR_ERFEL(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_ERFSR_ERFEL_SHIFT)) & CAN_ERFSR_ERFEL_MASK)
#define CAN_ERFSR_ERFF_MASK                      (0x10000U)
#define CAN_ERFSR_ERFF_SHIFT                     (16U)
/*! ERFF -  Enhanced Rx FIFO full
 *  0b0..Enhanced Rx FIFO is not full
 *  0b1..Enhanced Rx FIFO is full
 */
#define CAN_ERFSR_ERFF(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_ERFSR_ERFF_SHIFT)) & CAN_ERFSR_ERFF_MASK)
#define CAN_ERFSR_ERFE_MASK                      (0x20000U)
#define CAN_ERFSR_ERFE_SHIFT                     (17U)
/*! ERFE -  Enhanced Rx FIFO empty
 *  0b0..Enhanced Rx FIFO is not empty
 *  0b1..Enhanced Rx FIFO is empty
 */
#define CAN_ERFSR_ERFE(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_ERFSR_ERFE_SHIFT)) & CAN_ERFSR_ERFE_MASK)
#define CAN_ERFSR_ERFCLR_MASK                    (0x8000000U)
#define CAN_ERFSR_ERFCLR_SHIFT                   (27U)
/*! ERFCLR -  Enhanced Rx FIFO Clear
 *  0b0..No effect
 *  0b1..Clear Enhanced Rx FIFO content
 */
#define CAN_ERFSR_ERFCLR(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_ERFSR_ERFCLR_SHIFT)) & CAN_ERFSR_ERFCLR_MASK)
#define CAN_ERFSR_ERFDA_MASK                     (0x10000000U)
#define CAN_ERFSR_ERFDA_SHIFT                    (28U)
/*! ERFDA -  Enhanced Rx FIFO Data Available
 *  0b0..No such occurrence
 *  0b1..There is at least one message stored in Enhanced Rx FIFO
 */
#define CAN_ERFSR_ERFDA(x)                       (((uint32_t)(((uint32_t)(x)) << CAN_ERFSR_ERFDA_SHIFT)) & CAN_ERFSR_ERFDA_MASK)
#define CAN_ERFSR_ERFWMI_MASK                    (0x20000000U)
#define CAN_ERFSR_ERFWMI_SHIFT                   (29U)
/*! ERFWMI -  Enhanced Rx FIFO Watermark Indication
 *  0b0..No such occurrence
 *  0b1..The number of messages in FIFO is greater than the watermark
 */
#define CAN_ERFSR_ERFWMI(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_ERFSR_ERFWMI_SHIFT)) & CAN_ERFSR_ERFWMI_MASK)
#define CAN_ERFSR_ERFOVF_MASK                    (0x40000000U)
#define CAN_ERFSR_ERFOVF_SHIFT                   (30U)
/*! ERFOVF -  Enhanced Rx FIFO Overflow
 *  0b0..No such occurrence
 *  0b1..Enhanced Rx FIFO overflow
 */
#define CAN_ERFSR_ERFOVF(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_ERFSR_ERFOVF_SHIFT)) & CAN_ERFSR_ERFOVF_MASK)
#define CAN_ERFSR_ERFUFW_MASK                    (0x80000000U)
#define CAN_ERFSR_ERFUFW_SHIFT                   (31U)
/*! ERFUFW -  Enhanced Rx FIFO Underflow
 *  0b0..No such occurrence
 *  0b1..Enhanced Rx FIFO underflow
 */
#define CAN_ERFSR_ERFUFW(x)                      (((uint32_t)(((uint32_t)(x)) << CAN_ERFSR_ERFUFW_SHIFT)) & CAN_ERFSR_ERFUFW_MASK)
/*! @} */

/*! @name HR_TIME_STAMP - High Resolution Time Stamp */
/*! @{ */
#define CAN_HR_TIME_STAMP_TS_MASK                (0xFFFFFFFFU)
#define CAN_HR_TIME_STAMP_TS_SHIFT               (0U)
#define CAN_HR_TIME_STAMP_TS(x)                  (((uint32_t)(((uint32_t)(x)) << CAN_HR_TIME_STAMP_TS_SHIFT)) & CAN_HR_TIME_STAMP_TS_MASK)
/*! @} */

/* The count of CAN_HR_TIME_STAMP */
#define CAN_HR_TIME_STAMP_COUNT                  (64U)

/*! @name ERFFEL - Enhanced Rx FIFO Filter Element */
/*! @{ */
#define CAN_ERFFEL_FEL_MASK                      (0xFFFFFFFFU)
#define CAN_ERFFEL_FEL_SHIFT                     (0U)
#define CAN_ERFFEL_FEL(x)                        (((uint32_t)(((uint32_t)(x)) << CAN_ERFFEL_FEL_SHIFT)) & CAN_ERFFEL_FEL_MASK)
/*! @} */

/* The count of CAN_ERFFEL */
#define CAN_ERFFEL_COUNT                         (128U)

/*!
 * @}
 */ /* end of group CAN_Peripheral_Access_Layer */

int rt_hw_can_init(void);

#endif /* DRV_CAN_H__ */
