/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DRV_USART_H_
#define _DRV_USART_H_

#include <rtdef.h>

#ifdef __cplusplus
extern "C" {
#endif
/// definition for usart handle.
typedef void *usart_handle_t;

/*----- USART Control Codes: Mode -----*/
typedef enum
{
    USART_MODE_ASYNCHRONOUS         = 0,   ///< USART (Asynchronous)
    USART_MODE_SYNCHRONOUS_MASTER,         ///< Synchronous Master
    USART_MODE_SYNCHRONOUS_SLAVE,          ///< Synchronous Slave (external clock signal)
    USART_MODE_SINGLE_WIRE,                 ///< USART Single-wire (half-duplex)
    USART_MODE_SINGLE_IRDA,                 ///< UART IrDA
    USART_MODE_SINGLE_SMART_CARD,           ///< UART Smart Card
} usart_mode_e;

/*----- USART Control Codes: Mode Parameters: Data Bits -----*/
typedef enum
{
    USART_DATA_BITS_5             = 0,    ///< 5 Data bits
    USART_DATA_BITS_6,                    ///< 6 Data bit
    USART_DATA_BITS_7,                    ///< 7 Data bits
    USART_DATA_BITS_8,                    ///< 8 Data bits (default)
    USART_DATA_BITS_9                     ///< 9 Data bits
} usart_data_bits_e;

/*----- USART Control Codes: Mode Parameters: Parity -----*/
typedef enum
{
    USART_PARITY_NONE            = 0,       ///< No Parity (default)
    USART_PARITY_EVEN,                      ///< Even Parity
    USART_PARITY_ODD,                       ///< Odd Parity
    USART_PARITY_1,                         ///< Parity forced to 1
    USART_PARITY_0                          ///< Parity forced to 0
} usart_parity_e;

/*----- USART Control Codes: Mode Parameters: Stop Bits -----*/
typedef enum
{
    USART_STOP_BITS_1            = 0,    ///< 1 Stop bit (default)
    USART_STOP_BITS_2,                   ///< 2 Stop bits
    USART_STOP_BITS_1_5,                 ///< 1.5 Stop bits
    USART_STOP_BITS_0_5                  ///< 0.5 Stop bits
} usart_stop_bits_e;

/*----- USART Control Codes: Mode Parameters: Clock Polarity (Synchronous mode) -----*/
typedef enum
{
    USART_CPOL0                  = 0,    ///< CPOL = 0 (default). data are captured on rising edge (low->high transition)
    USART_CPOL1                          ///< CPOL = 1. data are captured on falling edge (high->lowh transition)
} usart_cpol_e;

/*----- USART Control Codes: Mode Parameters: Clock Phase (Synchronous mode) -----*/
typedef enum
{
    USART_CPHA0                  = 0,   ///< CPHA = 0 (default). sample on first (leading) edge
    USART_CPHA1                         ///< CPHA = 1. sample on second (trailing) edge
} usart_cpha_e;

/*----- USART Control Codes: flush data type-----*/
typedef enum
{
    USART_FLUSH_WRITE,
    USART_FLUSH_READ
} usart_flush_type_e;

/*----- USART Control Codes: flow control type-----*/
typedef enum
{
    USART_FLOWCTRL_NONE,
    USART_FLOWCTRL_CTS,
    USART_FLOWCTRL_RTS,
    USART_FLOWCTRL_CTS_RTS
} usart_flowctrl_type_e;

/*----- USART Modem Control -----*/
typedef enum
{
    USART_RTS_CLEAR,                  ///< Deactivate RTS
    USART_RTS_SET,                    ///< Activate RTS
    USART_DTR_CLEAR,                  ///< Deactivate DTR
    USART_DTR_SET                     ///< Activate DTR
} usart_modem_ctrl_e;

/*----- USART Modem Status -----*/
typedef struct
{
    rt_uint32_t cts : 1;                     ///< CTS state: 1=Active, 0=Inactive
    rt_uint32_t dsr : 1;                     ///< DSR state: 1=Active, 0=Inactive
    rt_uint32_t dcd : 1;                     ///< DCD state: 1=Active, 0=Inactive
    rt_uint32_t ri  : 1;                     ///< RI  state: 1=Active, 0=Inactive
} usart_modem_stat_t;

/*----- USART Control Codes: on-off intrrupte type-----*/
typedef enum
{
    USART_INTR_WRITE,
    USART_INTR_READ
} usart_intr_type_e;

/**
\brief USART Status
*/
typedef struct  {
    rt_uint32_t tx_busy          : 1;        ///< Transmitter busy flag
    rt_uint32_t rx_busy          : 1;        ///< Receiver busy flag
    rt_uint32_t tx_underflow     : 1;        ///< Transmit data underflow detected (cleared on start of next send operation)(Synchronous Slave)
    rt_uint32_t rx_overflow      : 1;        ///< Receive data overflow detected (cleared on start of next receive operation)
    rt_uint32_t rx_break         : 1;        ///< Break detected on receive (cleared on start of next receive operation)
    rt_uint32_t rx_framing_error : 1;        ///< Framing error detected on receive (cleared on start of next receive operation)
    rt_uint32_t rx_parity_error  : 1;        ///< Parity error detected on receive (cleared on start of next receive operation)
    rt_uint32_t tx_enable        : 1;        ///< Transmitter enable flag
    rt_uint32_t rx_enable        : 1;        ///< Receiver enbale flag
} usart_status_t;

/****** USART Event *****/
typedef enum
{
    USART_EVENT_SEND_COMPLETE       = 0,  ///< Send completed; however USART may still transmit data
    USART_EVENT_RECEIVE_COMPLETE    = 1,  ///< Receive completed
    USART_EVENT_TRANSFER_COMPLETE   = 2,  ///< Transfer completed
    USART_EVENT_TX_COMPLETE         = 3,  ///< Transmit completed (optional)
    USART_EVENT_TX_UNDERFLOW        = 4,  ///< Transmit data not available (Synchronous Slave)
    USART_EVENT_RX_OVERFLOW         = 5,  ///< Receive data overflow
    USART_EVENT_RX_TIMEOUT          = 6,  ///< Receive character timeout (optional)
    USART_EVENT_RX_BREAK            = 7,  ///< Break detected on receive
    USART_EVENT_RX_FRAMING_ERROR    = 8,  ///< Framing error detected on receive
    USART_EVENT_RX_PARITY_ERROR     = 9,  ///< Parity error detected on receive
    USART_EVENT_CTS                 = 10, ///< CTS state changed (optional)
    USART_EVENT_DSR                 = 11, ///< DSR state changed (optional)
    USART_EVENT_DCD                 = 12, ///< DCD state changed (optional)
    USART_EVENT_RI                  = 13, ///< RI  state changed (optional)
    USART_EVENT_RECEIVED            = 14, ///< Data Received, only in usart fifo, call receive()/transfer() get the data
} usart_event_e;

typedef void (*usart_event_cb_t)(rt_int32_t idx, usart_event_e event);   ///< Pointer to \ref usart_event_cb_t : USART Event call back.

/**
\brief USART Driver Capabilities.
*/
typedef struct  {
    rt_uint32_t asynchronous       : 1;      ///< supports UART (Asynchronous) mode
    rt_uint32_t synchronous_master : 1;      ///< supports Synchronous Master mode
    rt_uint32_t synchronous_slave  : 1;      ///< supports Synchronous Slave mode
    rt_uint32_t single_wire        : 1;      ///< supports UART Single-wire mode
    rt_uint32_t irda               : 1;      ///< supports UART IrDA mode
    rt_uint32_t smart_card         : 1;      ///< supports UART Smart Card mode
    rt_uint32_t smart_card_clock   : 1;      ///< Smart Card Clock generator available
    rt_uint32_t flow_control_rts   : 1;      ///< RTS Flow Control available
    rt_uint32_t flow_control_cts   : 1;      ///< CTS Flow Control available
    rt_uint32_t event_tx_complete  : 1;      ///< Transmit completed event: \ref USART_EVENT_TX_COMPLETE
    rt_uint32_t event_rx_timeout   : 1;      ///< Signal receive character timeout event: \ref USART_EVENT_RX_TIMEOUT
    rt_uint32_t rts                : 1;      ///< RTS Line: 0=not available, 1=available
    rt_uint32_t cts                : 1;      ///< CTS Line: 0=not available, 1=available
    rt_uint32_t dtr                : 1;      ///< DTR Line: 0=not available, 1=available
    rt_uint32_t dsr                : 1;      ///< DSR Line: 0=not available, 1=available
    rt_uint32_t dcd                : 1;      ///< DCD Line: 0=not available, 1=available
    rt_uint32_t ri                 : 1;      ///< RI Line: 0=not available, 1=available
    rt_uint32_t event_cts          : 1;      ///< Signal CTS change event: \ref USART_EVENT_CTS
    rt_uint32_t event_dsr          : 1;      ///< Signal DSR change event: \ref USART_EVENT_DSR
    rt_uint32_t event_dcd          : 1;      ///< Signal DCD change event: \ref USART_EVENT_DCD
    rt_uint32_t event_ri           : 1;      ///< Signal RI change event: \ref USART_EVENT_RI
} usart_capabilities_t;

/**
  \brief       Initialize USART Interface. 1. Initializes the resources needed for the USART interface 2.registers event callback function
  \param[in]   idx usart index
  \param[in]   cb_event  event call back function \ref usart_event_cb_t
  \return      return usart handle if success
*/
usart_handle_t csi_usart_initialize(rt_int32_t idx, usart_event_cb_t cb_event);

/**
  \brief       De-initialize USART Interface. stops operation and releases the software resources used by the interface
  \param[in]   handle  usart handle to operate.
  \return      error code
*/
rt_int32_t csi_usart_uninitialize(usart_handle_t handle);

/**
  \brief       config usart mode.
  \param[in]   handle  usart handle to operate.
  \param[in]   baud      baud rate.
  \param[in]   mode      \ref usart_mode_e .
  \param[in]   parity    \ref usart_parity_e .
  \param[in]   stopbits  \ref usart_stop_bits_e .
  \param[in]   bits      \ref usart_data_bits_e .
  \return      error code
*/
rt_int32_t csi_usart_config(usart_handle_t handle,
                         rt_uint32_t baud,
                         usart_mode_e mode,
                         usart_parity_e parity,
                         usart_stop_bits_e stopbits,
                         usart_data_bits_e bits);

/**
  \brief       Start synchronously sends data to the USART transmitter and receives data from the USART receiver. used in synchronous mode
               This function is non-blocking,\ref usart_event_e is signaled when operation completes or error happens.
               \ref csi_usart_get_status can get operation status.
  \param[in]   handle  usart handle to operate.
  \param[in]   data_out  Pointer to buffer with data to send to USART transmitter.data_type is : uint8_t for 5..8 data bits, uint16_t for 9 data bits
  \param[out]  data_in   Pointer to buffer for data to receive from USART receiver.data_type is : uint8_t for 5..8 data bits, uint16_t for 9 data bits
  \param[in]   num       Number of data items to transfer
  \return      error code
*/
rt_int32_t csi_usart_transfer(usart_handle_t handle, const void *data_out, void *data_in, rt_uint32_t num);

/**
  \brief       abort sending/receiving data to/from USART transmitter/receiver.
  \param[in]   handle  usart handle to operate.
  \return      error code
*/
rt_int32_t csi_usart_abort_transfer(usart_handle_t handle);

/**
  \brief       set the baud rate of usart.
  \param[in]   baud  usart base to operate.
  \param[in]   baudrate baud rate
  \return      error code
*/
rt_int32_t csi_usart_config_baudrate(usart_handle_t handle, rt_uint32_t baud);

/**
  \brief       config usart mode.
  \param[in]   handle  usart handle to operate.
  \param[in]   mode    \ref usart_mode_e
  \return      error code
*/
rt_int32_t csi_usart_config_mode(usart_handle_t handle, usart_mode_e mode);

/**
  \brief       config usart parity.
  \param[in]   handle  usart handle to operate.
  \param[in]   parity    \ref usart_parity_e
  \return      error code
*/
rt_int32_t csi_usart_config_parity(usart_handle_t handle, usart_parity_e parity);

/**
  \brief       config usart stop bit number.
  \param[in]   handle  usart handle to operate.
  \param[in]   stopbits  \ref usart_stop_bits_e
  \return      error code
*/
rt_int32_t csi_usart_config_stopbits(usart_handle_t handle, usart_stop_bits_e stopbit);

/**
  \brief       config usart data length.
  \param[in]   handle  usart handle to operate.
  \param[in]   databits      \ref usart_data_bits_e
  \return      error code
*/
rt_int32_t csi_usart_config_databits(usart_handle_t handle, usart_data_bits_e databits);

/**
  \brief       transmit character in query mode.
  \param[in]   handle  usart handle to operate.
  \param[in]   ch  the input character
  \return      error code
*/
rt_int32_t csi_usart_putchar(usart_handle_t handle, rt_uint8_t ch);

/**
  \brief       config usart clock Polarity and Phase.
  \param[in]   handle  usart handle to operate.
  \param[in]   cpol    Clock Polarity.\ref usart_cpol_e.
  \param[in]   cpha    Clock Phase.\ref usart_cpha_e.
  \return      error code
*/
rt_int32_t csi_usart_config_clock(usart_handle_t handle, usart_cpol_e cpol, usart_cpha_e cpha);

/**
  \brief       control the break.
  \param[in]   handle  usart handle to operate.
  \param[in]   enable  1- Enable continuous Break transmission,0 - disable continuous Break transmission
  \return      error code
*/
rt_int32_t csi_usart_control_break(usart_handle_t handle, rt_uint32_t enable);

int csi_uart_getchar(usart_handle_t handle);

rt_int32_t ck_usart_clr_int_flag(usart_handle_t handle, rt_uint32_t flag);

rt_int32_t ck_usart_set_int_flag(usart_handle_t handle, rt_uint32_t flag);

#ifdef __cplusplus
}
#endif

#endif /* _DRV_USART_H_ */
