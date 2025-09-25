/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DRV_UART_H_
#define _DRV_UART_H_

#include <rtdef.h>

#ifdef __cplusplus
extern "C" {
#endif
/// definition for uart handle.
typedef void *uart_handle_t;

/*----- UART Control Codes: Mode -----*/
typedef volatile enum
{
    UART_MODE_ASYNCHRONOUS = 0,   
    UART_MODE_SYNCHRONOUS_MASTER,         
    UART_MODE_SYNCHRONOUS_SLAVE,          
    UART_MODE_SINGLE_WIRE,                
    UART_MODE_SINGLE_IRDA,                
    UART_MODE_SINGLE_SMART_CARD,          
} uart_mode_e;

/*----- UART Control Codes: Mode Parameters: Data Bits -----*/
typedef volatile enum
{
    UART_DATA_BITS_5 = 0,   
    UART_DATA_BITS_6,                    
    UART_DATA_BITS_7,                    
    UART_DATA_BITS_8,                   
    UART_DATA_BITS_9                    
} uart_data_bits_e;

/*----- UART Control Codes: Mode Parameters: Parity -----*/
typedef volatile enum
{
    UART_PARITY_NONE = 0,      
    UART_PARITY_EVEN,                      
    UART_PARITY_ODD,                      
    UART_PARITY_1,                        
    UART_PARITY_0                         
} uart_parity_e;

/*----- UART Control Codes: Mode Parameters: Stop Bits -----*/
typedef volatile enum
{
    UART_STOP_BITS_1 = 0,    
    UART_STOP_BITS_2,                
    UART_STOP_BITS_1_5,                
    UART_STOP_BITS_0_5                 
} uart_stop_bits_e;

/*----- UART Control Codes: Mode Parameters: Clock Polarity (Synchronous mode) -----*/
typedef volatile enum
{
    UART_CPOL0 = 0,    
    UART_CPOL1                          
} uart_cpol_e;

/*----- UART Control Codes: Mode Parameters: Clock Phase (Synchronous mode) -----*/
typedef volatile enum
{
    UART_CPHA0 = 0,  
    UART_CPHA1                    
} uart_cpha_e;

/*----- UART Control Codes: flush data type-----*/
typedef volatile enum
{
    UART_FLUSH_WRITE,
    UART_FLUSH_READ
} uart_flush_type_e;

/*----- UART Control Codes: flow control type-----*/
typedef volatile enum
{
    UART_FLOWCTRL_NONE,
    UART_FLOWCTRL_CTS,
    UART_FLOWCTRL_RTS,
    UART_FLOWCTRL_CTS_RTS
} uart_flowctrl_type_e;

/*----- UART Modem Control -----*/
typedef volatile enum
{
    UART_RTS_CLEAR,                  ///< Deactivate RTS
    UART_RTS_SET,                    ///< Activate RTS
    UART_DTR_CLEAR,                  ///< Deactivate DTR
    UART_DTR_SET                     ///< Activate DTR
} uart_modem_ctrl_e;

/*----- UART Modem Status -----*/
typedef volatile struct
{
    rt_uint32_t cts : 1;                     ///< CTS state: 1=Active, 0=Inactive
    rt_uint32_t dsr : 1;                     ///< DSR state: 1=Active, 0=Inactive
    rt_uint32_t dcd : 1;                     ///< DCD state: 1=Active, 0=Inactive
    rt_uint32_t ri  : 1;                     ///< RI  state: 1=Active, 0=Inactive
} uart_modem_stat_t;

/*----- UART Control Codes: on-off intrrupte type-----*/
typedef volatile enum
{
    UART_INTR_WRITE,
    UART_INTR_READ
} uart_intr_type_e;

/**
\brief UART Status
*/
typedef volatile struct  {
    rt_uint32_t tx_busy          : 1;        ///< Transmitter busy flag
    rt_uint32_t rx_busy          : 1;        ///< Receiver busy flag
    rt_uint32_t tx_underflow     : 1;        ///< Transmit data underflow detected (cleared on start of next send operation)(Synchronous Slave)
    rt_uint32_t rx_overflow      : 1;        ///< Receive data overflow detected (cleared on start of next receive operation)
    rt_uint32_t rx_break         : 1;        ///< Break detected on receive (cleared on start of next receive operation)
    rt_uint32_t rx_framing_error : 1;        ///< Framing error detected on receive (cleared on start of next receive operation)
    rt_uint32_t rx_parity_error  : 1;        ///< Parity error detected on receive (cleared on start of next receive operation)
    rt_uint32_t tx_enable        : 1;        ///< Transmitter enable flag
    rt_uint32_t rx_enable        : 1;        ///< Receiver enbale flag
} uart_status_t;

/****** UART Event *****/
typedef volatile enum
{
    UART_EVENT_SEND_COMPLETE       = 0,  ///< Send completed; however UART may still transmit data
    UART_EVENT_RECEIVE_COMPLETE    = 1,  ///< Receive completed
    UART_EVENT_TRANSFER_COMPLETE   = 2,  ///< Transfer completed
    UART_EVENT_TX_COMPLETE         = 3,  ///< Transmit completed (optional)
    UART_EVENT_TX_UNDERFLOW        = 4,  ///< Transmit data not available (Synchronous Slave)
    UART_EVENT_RX_OVERFLOW         = 5,  ///< Receive data overflow
    UART_EVENT_RX_TIMEOUT          = 6,  ///< Receive character timeout (optional)
    UART_EVENT_RX_BREAK            = 7,  ///< Break detected on receive
    UART_EVENT_RX_FRAMING_ERROR    = 8,  ///< Framing error detected on receive
    UART_EVENT_RX_PARITY_ERROR     = 9,  ///< Parity error detected on receive
    UART_EVENT_CTS                 = 10, ///< CTS state changed (optional)
    UART_EVENT_DSR                 = 11, ///< DSR state changed (optional)
    UART_EVENT_DCD                 = 12, ///< DCD state changed (optional)
    UART_EVENT_RI                  = 13, ///< RI  state changed (optional)
    UART_EVENT_RECEIVED            = 14, ///< Data Received, only in uart fifo, call receive()/transfer() get the data
} uart_event_e;

typedef void (*uart_event_cb_t)(rt_int32_t idx, uart_event_e event);   ///< Pointer to \ref uart_event_cb_t : UART Event call back.

/**
\brief UART Driver Capabilities.
*/
typedef volatile struct  {
    rt_uint32_t asynchronous       : 1;      ///< supports UART (Asynchronous) mode
    rt_uint32_t synchronous_master : 1;      ///< supports Synchronous Master mode
    rt_uint32_t synchronous_slave  : 1;      ///< supports Synchronous Slave mode
    rt_uint32_t single_wire        : 1;      ///< supports UART Single-wire mode
    rt_uint32_t irda               : 1;      ///< supports UART IrDA mode
    rt_uint32_t smart_card         : 1;      ///< supports UART Smart Card mode
    rt_uint32_t smart_card_clock   : 1;      ///< Smart Card Clock generator available
    rt_uint32_t flow_control_rts   : 1;      ///< RTS Flow Control available
    rt_uint32_t flow_control_cts   : 1;      ///< CTS Flow Control available
    rt_uint32_t event_tx_complete  : 1;      ///< Transmit completed event: \ref UART_EVENT_TX_COMPLETE
    rt_uint32_t event_rx_timeout   : 1;      ///< Signal receive character timeout event: \ref UART_EVENT_RX_TIMEOUT
    rt_uint32_t rts                : 1;      ///< RTS Line: 0=not available, 1=available
    rt_uint32_t cts                : 1;      ///< CTS Line: 0=not available, 1=available
    rt_uint32_t dtr                : 1;      ///< DTR Line: 0=not available, 1=available
    rt_uint32_t dsr                : 1;      ///< DSR Line: 0=not available, 1=available
    rt_uint32_t dcd                : 1;      ///< DCD Line: 0=not available, 1=available
    rt_uint32_t ri                 : 1;      ///< RI Line: 0=not available, 1=available
    rt_uint32_t event_cts          : 1;      ///< Signal CTS change event: \ref UART_EVENT_CTS
    rt_uint32_t event_dsr          : 1;      ///< Signal DSR change event: \ref UART_EVENT_DSR
    rt_uint32_t event_dcd          : 1;      ///< Signal DCD change event: \ref UART_EVENT_DCD
    rt_uint32_t event_ri           : 1;      ///< Signal RI change event: \ref UART_EVENT_RI
} uart_capabilities_t;

/**
  \brief       Initialize UART Interface. 1. Initializes the resources needed for the UART interface 2.registers event callback function
  \param[in]   idx uart index
  \param[in]   cb_event  event call back function \ref uart_event_cb_t
  \return      return uart handle if success
*/
uart_handle_t pxa_uart_initialize(rt_int32_t idx, uart_event_cb_t cb_event);

/**
  \brief       De-initialize UART Interface. stops operation and releases the software resources used by the interface
  \param[in]   handle  uart handle to operate.
  \return      error code
*/
rt_int32_t pxa_uart_uninitialize(uart_handle_t handle);

/**
  \brief       config uart mode.
  \param[in]   handle  uart handle to operate.
  \param[in]   baud      baud rate.
  \param[in]   mode      \ref uart_mode_e .
  \param[in]   parity    \ref uart_parity_e .
  \param[in]   stopbits  \ref uart_stop_bits_e .
  \param[in]   bits      \ref uart_data_bits_e .
  \return      error code
*/
rt_int32_t pxa_uart_config(uart_handle_t handle,
                         rt_uint32_t baud,
                         uart_mode_e mode,
                         uart_parity_e parity,
                         uart_stop_bits_e stopbits,
                         uart_data_bits_e bits);

/**
  \brief       Start synchronously sends data to the UART transmitter and receives data from the UART receiver. used in synchronous mode
               This function is non-blocking,\ref uart_event_e is signaled when operation completes or error happens.
               \ref pxa_uart_get_status can get operation status.
  \param[in]   handle  uart handle to operate.
  \param[in]   data_out  Pointer to buffer with data to send to UART transmitter.data_type is : uint8_t for 5..8 data bits, uint16_t for 9 data bits
  \param[out]  data_in   Pointer to buffer for data to receive from UART receiver.data_type is : uint8_t for 5..8 data bits, uint16_t for 9 data bits
  \param[in]   num       Number of data items to transfer
  \return      error code
*/
rt_int32_t pxa_uart_transfer(uart_handle_t handle, const void *data_out, void *data_in, rt_uint32_t num);

/**
  \brief       abort sending/receiving data to/from UART transmitter/receiver.
  \param[in]   handle  uart handle to operate.
  \return      error code
*/
rt_int32_t pxa_uart_abort_transfer(uart_handle_t handle);

/**
  \brief       set the baud rate of uart.
  \param[in]   baud  uart base to operate.
  \param[in]   baudrate baud rate
  \return      error code
*/
rt_int32_t pxa_uart_config_baudrate(uart_handle_t handle, rt_uint32_t baud);

/**
  \brief       config uart mode.
  \param[in]   handle  uart handle to operate.
  \param[in]   mode    \ref uart_mode_e
  \return      error code
*/
rt_int32_t pxa_uart_config_mode(uart_handle_t handle, uart_mode_e mode);

/**
  \brief       config uart parity.
  \param[in]   handle  uart handle to operate.
  \param[in]   parity    \ref uart_parity_e
  \return      error code
*/
rt_int32_t pxa_uart_config_parity(uart_handle_t handle, uart_parity_e parity);

/**
  \brief       config uart stop bit number.
  \param[in]   handle  uart handle to operate.
  \param[in]   stopbits  \ref uart_stop_bits_e
  \return      error code
*/
rt_int32_t pxa_uart_config_stopbits(uart_handle_t handle, uart_stop_bits_e stopbit);

/**
  \brief       config uart data length.
  \param[in]   handle  uart handle to operate.
  \param[in]   databits      \ref uart_data_bits_e
  \return      error code
*/
rt_int32_t pxa_uart_config_databits(uart_handle_t handle, uart_data_bits_e databits);

/**
  \brief       transmit character in query mode.
  \param[in]   handle  uart handle to operate.
  \param[in]   ch  the input character
  \return      error code
*/
rt_int32_t pxa_uart_putchar(uart_handle_t handle, rt_uint8_t ch);

/**
  \brief       config uart clock Polarity and Phase.
  \param[in]   handle  uart handle to operate.
  \param[in]   cpol    Clock Polarity.\ref uart_cpol_e.
  \param[in]   cpha    Clock Phase.\ref uart_cpha_e.
  \return      error code
*/
rt_int32_t pxa_uart_config_clock(uart_handle_t handle, uart_cpol_e cpol, uart_cpha_e cpha);

/**
  \brief       control the break.
  \param[in]   handle  uart handle to operate.
  \param[in]   enable  1- Enable continuous Break transmission,0 - disable continuous Break transmission
  \return      error code
*/
rt_int32_t pxa_uart_control_break(uart_handle_t handle, rt_uint32_t enable);

int pxa_uart_getchar(uart_handle_t handle);

rt_int32_t pxa_uart_clr_int_flag(uart_handle_t handle, rt_uint32_t flag);

rt_int32_t pxa_uart_set_int_flag(uart_handle_t handle, rt_uint32_t flag);

int alloc_uart_memory(rt_uint32_t num);

#ifdef __cplusplus
}
#endif

#endif /* _DRV_UART_H_ */
