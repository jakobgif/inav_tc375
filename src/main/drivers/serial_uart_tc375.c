/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file serial_uart_tc375.c
 * @author Jakob Frenzel (jakob.frenzel@hotmail.com)
 * @brief UART driver for AURIX
 * @date 2025-04-10
 */

#include <stdbool.h>
#include <stdint.h>

#include "platform.h"

#include "drivers/time.h"
#include "drivers/io.h"
#include "drivers/io_impl.h"
#include "rcc.h"
#include "drivers/nvic.h"

#include "serial.h"
#include "serial_uart.h"
#include "serial_uart_impl.h"

#define UART_RX_BUFFER_SIZE UART1_RX_BUFFER_SIZE
#define UART_TX_BUFFER_SIZE UART1_RX_BUFFER_SIZE

//for interrupt vector
#define AURIX_CORE_ID 0

typedef struct uartDevice_s {
    IfxAsclin_Asc *handle; //handle statically defined somewhere and referenced here
    IfxAsclin_Asc_Config *config; //we store this statically somewhere so we can modifiy the existin config afterwards
    uartPort_t port;
    ioTag_t rx;
    ioTag_t tx;
    volatile uint8_t rxBuffer[UART_RX_BUFFER_SIZE];
    volatile uint8_t txBuffer[UART_TX_BUFFER_SIZE];
    //uint8_t irq;
} uartDevice_t;

#ifdef USE_UART1
IfxAsclin_Asc uart1handle = {
    .asclin = UART1,
};
IfxAsclin_Asc_Config uart1Config;
static uartDevice_t uart1 = {
    .handle = &uart1handle, //connect aurix driver to inav uart device
    .config = &uart1Config, //connect aurix driver config to inav uart device
    .rx = IO_TAG(UART1_PIN_RX),
    .tx = IO_TAG(UART1_PIN_TX),
    //.irq = NULL_PTR,
};
#endif

#ifdef USE_UART2
IfxAsclin_Asc uart2handle = {
    .asclin = UART2,
};
IfxAsclin_Asc_Config uart2Config;
static uartDevice_t uart2 = {
    .handle = &uart2handle,
    .config = &uart2Config,
    .rx = IO_TAG(UART2_PIN_RX),
    .tx = IO_TAG(UART2_PIN_TX),
    //.irq = NULL,
};
#endif

#ifdef USE_UART3
IfxAsclin_Asc uart3handle = {
    .asclin = UART3,
};
IfxAsclin_Asc_Config uart3Config;
static uartDevice_t uart3 = {
    .handle = &uart3handle,
    .config = &uart3Config,
    .rx = IO_TAG(UART3_PIN_RX),
    .tx = IO_TAG(UART3_PIN_TX),
    //.irq = NULL,
};
#endif

#ifdef USE_UART4
IfxAsclin_Asc uart4handle = {
    .asclin = UART4,
};
IfxAsclin_Asc_Config uart4Config;
static uartDevice_t uart4 = {
    .handle = &uart4handle,
    .config = &uart4Config,
    .rx = IO_TAG(UART4_PIN_RX),
    .tx = IO_TAG(UART4_PIN_TX),
    //.irq = NULL,
};
#endif

#ifdef USE_UART5
IfxAsclin_Asc uart5handle = {
    .asclin = UART5,
};
IfxAsclin_Asc_Config uart5Config;
static uartDevice_t uart5 = {
    .handle = &uart5handle,
    .config = &uart5Config,
    .rx = IO_TAG(UART5_PIN_RX),
    .tx = IO_TAG(UART5_PIN_TX),
    .irq = NULL,
};
#endif

#ifdef USE_UART6
IfxAsclin_Asc uart6handle = {
    .asclin = UART6,
};
IfxAsclin_Asc_Config uart6Config;
static uartDevice_t uart6 = {
    .handle = &uart6handle,
    .config = &uart6Config,
    .rx = IO_TAG(UART6_PIN_RX),
    .tx = IO_TAG(UART6_PIN_TX),
    .irq = NULL,
};
#endif

#ifdef USE_UART7
IfxAsclin_Asc uart7handle = {
    .asclin = UART7,
};
IfxAsclin_Asc_Config uart7Config;
static uartDevice_t uart7 = {
    .handle = &uart7handle,
    .config = &uart7Config,
    .rx = IO_TAG(UART7_PIN_RX),
    .tx = IO_TAG(UART7_PIN_TX),
    .irq = NULL,
};
#endif

#ifdef USE_UART8
IfxAsclin_Asc uart8handle = {
    .asclin = UART8,
};
IfxAsclin_Asc_Config uart8Config;
static uartDevice_t uart8 = {
    .handle = &uart8handle,
    .config = &uart8Config,
    .rx = IO_TAG(UART8_PIN_RX),
    .tx = IO_TAG(UART8_PIN_TX),
    .irq = NULL,
};
#endif

static uartDevice_t* uartHardwareMap[] = {
#ifdef USE_UART1
    &uart1,
#else
    NULL,
#endif
#ifdef USE_UART2
    &uart2,
#else
    NULL,
#endif
#ifdef USE_UART3
    &uart3,
#else
    NULL,
#endif
#ifdef USE_UART4
    &uart4,
#else
    NULL,
#endif
#ifdef USE_UART5
    &uart5,
#else
    NULL,
#endif
#ifdef USE_UART6
    &uart6,
#else
    NULL,
#endif
#ifdef USE_UART7
    &uart7,
#else
    NULL,
#endif
#ifdef USE_UART8
    &uart8,
#else
    NULL,
#endif
};

IfxAsclin_Asc_Pins uartPinConfigs[sizeof(uartHardwareMap)/sizeof(uartDevice_t*)];

static IfxAsclin_Rx_In * getRxInPinmapFromIoTag(ioTag_t tag, Ifx_ASCLIN * module){
    IfxAsclin_Rx_In * pinmap = NULL_PTR;
    //get port and pin from tag
    ioRec_t *ioRec = IO_Rec(IOGetByTag(tag));
    IfxPort_Pin gpio = ioRec->gpio;

    for (int i=0; i<IFXASCLIN_PINMAP_NUM_MODULES; i++){
        for (int j=0; j<IFXASCLIN_PINMAP_RX_IN_NUM_ITEMS; j++){
            //module must match
            if (IfxAsclin_Rx_In_pinTable[i][j] != NULL){
                if( IfxAsclin_Rx_In_pinTable[i][j]->pin.pinIndex == gpio.pinIndex && 
                    IfxAsclin_Rx_In_pinTable[i][j]->pin.port == gpio.port && 
                    IfxAsclin_Rx_In_pinTable[i][j]->module == module
                ){
                    pinmap = IfxAsclin_Rx_In_pinTable[i][j];
                }
            }
        }
    }

    return pinmap;
}

static IfxAsclin_Tx_Out * getTxOutPinmapFromIoTag(ioTag_t tag, Ifx_ASCLIN * module){
    IfxAsclin_Tx_Out * pinmap = NULL_PTR;
    //get port and pin from tag
    ioRec_t *ioRec = IO_Rec(IOGetByTag(tag));
    IfxPort_Pin gpio = ioRec->gpio;
    
    for (int i=0; i<IFXASCLIN_PINMAP_NUM_MODULES; i++){
        for (int j=0; j<IFXASCLIN_PINMAP_TX_OUT_NUM_ITEMS; j++){
            //module must match
            if (IfxAsclin_Tx_Out_pinTable[i][j] != NULL){
                if( IfxAsclin_Tx_Out_pinTable[i][j]->pin.pinIndex == gpio.pinIndex && 
                    IfxAsclin_Tx_Out_pinTable[i][j]->pin.port == gpio.port && 
                    IfxAsclin_Tx_Out_pinTable[i][j]->module == module
                ){
                    pinmap = IfxAsclin_Tx_Out_pinTable[i][j];
                }
            }
        }
    }

    return pinmap;
}

void uartGetPortPins(UARTDevice_e device, serialPortPins_t * pins){
    uartDevice_t *uart = uartHardwareMap[device];

    if (uart) {
        pins->txPin = uart->tx;
        pins->rxPin = uart->rx;
    }
    else {
        pins->txPin = IO_TAG(NONE);
        pins->rxPin = IO_TAG(NONE);
    }
}

void uartTxIrqHandler(uartPort_t *s) {
    //IfxAsclin_Asc_isrTransmit(s->handle);

    if (s->port.txBufferTail == s->port.txBufferHead) {
        s->handle->txInProgress = FALSE;
    }
    else {
        IfxAsclin_write8(s->handle->asclin, (uint8_t*)&(s->port.txBuffer[s->port.txBufferTail]), 1);
        s->port.txBufferTail = (s->port.txBufferTail + 1) % s->port.txBufferSize;
    }
}

void uartRxIrqHandler(uartPort_t *s) {
    //move data to FIFO
    //IfxAsclin_Asc_isrReceive(s->handle);

    uint8_t rxData;
	//Ifx_SizeT size = sizeof(uint8_t); //one byte
    //IfxAsclin_Asc_read(s->handle, &rxData, &size, TIME_NULL); //read from FIFO //TIME_INFINITE

    IfxAsclin_read8(s->handle->asclin, &rxData, 1);

	if(s->port.rxCallback) {
		s->port.rxCallback(rxData, s->port.rxCallbackData);
	}
    else {
        s->port.rxBuffer[s->port.rxBufferHead] = rxData;
        s->port.rxBufferHead = (s->port.rxBufferHead + 1) % s->port.rxBufferSize;
    }
}

uartPort_t *serialUART(UARTDevice_e device, uint32_t baudRate, portMode_t mode, portOptions_t options) {
    uartPort_t *s;

    uartDevice_t *uart = uartHardwareMap[device];
    if (!uart) return NULL;

    //init the config for specific Asclin Module
    IfxAsclin_Asc_initModuleConfig(uart->config, uart->handle->asclin);

    s = &(uart->port);
    s->port.vTable = uartVTable;

    s->port.baudRate = baudRate;

    s->port.rxBuffer = uart->rxBuffer;
    s->port.txBuffer = uart->txBuffer;
    s->port.rxBufferSize = sizeof(uart->rxBuffer);
    s->port.txBufferSize = sizeof(uart->txBuffer);

    s->handle = uart->handle;
    s->config = uart->config;

    IO_t tx = IOGetByTag(uart->tx);
    IO_t rx = IOGetByTag(uart->rx);

    if (options & SERIAL_BIDIR) {
        //SERIAL_BIDIR not supported in this firmware port
    }
    else {
        if (mode & MODE_TX) {
            IOInit(tx, OWNER_SERIAL, RESOURCE_UART_TX, RESOURCE_INDEX(device));
        }

        if (mode & MODE_RX) {
            IOInit(rx, OWNER_SERIAL, RESOURCE_UART_RX, RESOURCE_INDEX(device));
        }
    }

    //make some basic configs
    //inav does not use the aurix FIFO
    //s->config->txBuffer = uart->txBuffer;
    //s->config->txBufferSize = sizeof(uart->txBuffer);
    //s->config->rxBuffer = uart->rxBuffer;
    //s->config->rxBufferSize = sizeof(uart->rxBuffer);

    s->config->baudrate.oversampling = 15;
    s->config->bitTiming.medianFilter = 3;
    s->config->bitTiming.samplePointPosition = 9;

    IfxAsclin_Asc_Pins *pins = &uartPinConfigs[device];
    pins->cts = NULL_PTR;
    pins->ctsMode = IfxPort_InputMode_pullUp;
    pins->rx = getRxInPinmapFromIoTag(uart->rx, uart->handle->asclin);
    pins->rxMode = IfxPort_InputMode_pullUp;
    pins->rts = NULL_PTR;
    pins->rtsMode = IfxPort_OutputMode_pushPull;
    pins->tx = getTxOutPinmapFromIoTag(uart->tx, uart->handle->asclin);
    pins->txMode = IfxPort_OutputMode_pushPull;
    pins->pinDriver = DEFAULT_IfxPort_PadDriver;

    s->config->pins = pins;

    return s;
}

#ifdef USE_UART1
uartPort_t *serialUART1(uint32_t baudRate, portMode_t mode, portOptions_t options)
{
    uartPort_t *s = serialUART(UARTDEV_1, baudRate, mode, options);
    //here we set the int prios
    s->config->interrupt.txPriority = INTPRIO_ASCLIN0_TX;
    s->config->interrupt.rxPriority = INTPRIO_ASCLIN0_RX;
    s->config->interrupt.typeOfService = IfxCpu_Irq_getTos(IfxCpu_getCoreIndex());

    return s;
}

IFX_INTERRUPT(asclin0TxISR, AURIX_CORE_ID, INTPRIO_ASCLIN0_TX);
void asclin0TxISR(void) {
    uartPort_t *s = &(uartHardwareMap[UARTDEV_1]->port);
    uartTxIrqHandler(s);
}

IFX_INTERRUPT(asclin0RxISR, AURIX_CORE_ID, INTPRIO_ASCLIN0_RX);
void asclin0RxISR(void) {
    uartPort_t *s = &(uartHardwareMap[UARTDEV_1]->port);
    uartRxIrqHandler(s);
}
#endif

#ifdef USE_UART2
uartPort_t *serialUART2(uint32_t baudRate, portMode_t mode, portOptions_t options)
{
    uartPort_t *s = serialUART(UARTDEV_2, baudRate, mode, options);
    //here we set the int prios
    s->config->interrupt.txPriority = INTPRIO_ASCLIN1_TX;
    s->config->interrupt.rxPriority = INTPRIO_ASCLIN1_RX;
    s->config->interrupt.typeOfService = IfxCpu_Irq_getTos(IfxCpu_getCoreIndex());

    return s;
}

IFX_INTERRUPT(asclin1TxISR, AURIX_CORE_ID, INTPRIO_ASCLIN1_TX);
void asclin1TxISR(void) {
    uartPort_t *s = &(uartHardwareMap[UARTDEV_2]->port);
    uartTxIrqHandler(s);
}

IFX_INTERRUPT(asclin1RxISR, AURIX_CORE_ID, INTPRIO_ASCLIN1_RX);
void asclin1RxISR(void) {
    uartPort_t *s = &(uartHardwareMap[UARTDEV_2]->port);
    uartRxIrqHandler(s);
}
#endif

#ifdef USE_UART3
uartPort_t *serialUART3(uint32_t baudRate, portMode_t mode, portOptions_t options)
{
    uartPort_t *s = serialUART(UARTDEV_3, baudRate, mode, options);
    //here we set the int prios
    s->config->interrupt.txPriority = INTPRIO_ASCLIN2_TX;
    s->config->interrupt.rxPriority = INTPRIO_ASCLIN2_RX;
    s->config->interrupt.typeOfService = IfxCpu_Irq_getTos(IfxCpu_getCoreIndex());

    return s;
}

IFX_INTERRUPT(asclin2TxISR, AURIX_CORE_ID, INTPRIO_ASCLIN2_TX);
void asclin2TxISR(void) {
    uartPort_t *s = &(uartHardwareMap[UARTDEV_3]->port);
    uartTxIrqHandler(s);
}

IFX_INTERRUPT(asclin2RxISR, AURIX_CORE_ID, INTPRIO_ASCLIN2_RX);
void asclin2RxISR(void) {
    uartPort_t *s = &(uartHardwareMap[UARTDEV_3]->port);
    uartRxIrqHandler(s);
}
#endif

#ifdef USE_UART4
uartPort_t *serialUART4(uint32_t baudRate, portMode_t mode, portOptions_t options)
{
    uartPort_t *s = serialUART(UARTDEV_4, baudRate, mode, options);
    //here we set the int prios
    s->config->interrupt.txPriority = INTPRIO_ASCLIN3_TX;
    s->config->interrupt.rxPriority = INTPRIO_ASCLIN3_RX;
    s->config->interrupt.typeOfService = IfxCpu_Irq_getTos(IfxCpu_getCoreIndex());

    return s;
}

IFX_INTERRUPT(asclin3TxISR, AURIX_CORE_ID, INTPRIO_ASCLIN3_TX);
void asclin3TxISR(void) {
    uartPort_t *s = &(uartHardwareMap[UARTDEV_4]->port);
    uartTxIrqHandler(s);
}

IFX_INTERRUPT(asclin3RxISR, AURIX_CORE_ID, INTPRIO_ASCLIN3_RX);
void asclin3RxISR(void) {
    uartPort_t *s = &(uartHardwareMap[UARTDEV_4]->port);
    uartRxIrqHandler(s);
}
#endif

#ifdef USE_UART5
uartPort_t *serialUART5(uint32_t baudRate, portMode_t mode, portOptions_t options)
{
    uartPort_t *s = serialUART(UARTDEV_5, baudRate, mode, options);
    //here we set the int prios
    s->config->interrupt.txPriority = INTPRIO_ASCLIN4_TX;
    s->config->interrupt.rxPriority = INTPRIO_ASCLIN4_RX;
    s->config->interrupt.typeOfService = IfxCpu_Irq_getTos(IfxCpu_getCoreIndex());

    return s;
}

IFX_INTERRUPT(asclin4TxISR, AURIX_CORE_ID, INTPRIO_ASCLIN4_TX);
void asclin4TxISR(void) {
    uartPort_t *s = &(uartHardwareMap[UARTDEV_5]->port);
    uartTxIrqHandler(s);
}

IFX_INTERRUPT(asclin4RxISR, AURIX_CORE_ID, INTPRIO_ASCLIN4_RX);
void asclin4RxISR(void) {
    uartPort_t *s = &(uartHardwareMap[UARTDEV_5]->port);
    uartRxIrqHandler(s);
}
#endif

#ifdef USE_UART6
uartPort_t *serialUART6(uint32_t baudRate, portMode_t mode, portOptions_t options)
{
    uartPort_t *s = serialUART(UARTDEV_6, baudRate, mode, options);
    //here we set the int prios
    s->config->interrupt.txPriority = INTPRIO_ASCLIN5_TX;
    s->config->interrupt.rxPriority = INTPRIO_ASCLIN5_RX;
    s->config->interrupt.typeOfService = IfxCpu_Irq_getTos(IfxCpu_getCoreIndex());

    return s;
}

IFX_INTERRUPT(asclin5TxISR, AURIX_CORE_ID, INTPRIO_ASCLIN5_TX);
void asclin5TxISR(void) {
    uartPort_t *s = &(uartHardwareMap[UARTDEV_6]->port);
    uartTxIrqHandler(s);
}

IFX_INTERRUPT(asclin5RxISR, AURIX_CORE_ID, INTPRIO_ASCLIN5_RX);
void asclin5RxISR(void) {
    uartPort_t *s = &(uartHardwareMap[UARTDEV_6]->port);
    uartRxIrqHandler(s);
}
#endif

#ifdef USE_UART7
uartPort_t *serialUART7(uint32_t baudRate, portMode_t mode, portOptions_t options)
{
    uartPort_t *s = serialUART(UARTDEV_7, baudRate, mode, options);
    //here we set the int prios
    s->config->interrupt.txPriority = INTPRIO_ASCLIN6_TX;
    s->config->interrupt.rxPriority = INTPRIO_ASCLIN6_RX;
    s->config->interrupt.typeOfService = IfxCpu_Irq_getTos(IfxCpu_getCoreIndex());

    return s;
}

IFX_INTERRUPT(asclin6TxISR, AURIX_CORE_ID, INTPRIO_ASCLIN6_TX);
void asclin6TxISR(void) {
    uartPort_t *s = &(uartHardwareMap[UARTDEV_7]->port);
    uartTxIrqHandler(s);
}

IFX_INTERRUPT(asclin6RxISR, AURIX_CORE_ID, INTPRIO_ASCLIN6_RX);
void asclin6RxISR(void) {
    uartPort_t *s = &(uartHardwareMap[UARTDEV_7]->port);
    uartRxIrqHandler(s);
}
#endif

#ifdef USE_UART8
uartPort_t *serialUART8(uint32_t baudRate, portMode_t mode, portOptions_t options)
{
    uartPort_t *s = serialUART(UARTDEV_8, baudRate, mode, options);
    //here we set the int prios
    s->config->interrupt.txPriority = INTPRIO_ASCLIN7_TX;
    s->config->interrupt.rxPriority = INTPRIO_ASCLIN7_RX;
    s->config->interrupt.typeOfService = IfxCpu_Irq_getTos(IfxCpu_getCoreIndex());

    return s;
}

IFX_INTERRUPT(asclin7TxISR, AURIX_CORE_ID, INTPRIO_ASCLIN7_TX);
void asclin7TxISR(void) {
    uartPort_t *s = &(uartHardwareMap[UARTDEV_8]->port);
    uartTxIrqHandler(s);
}

IFX_INTERRUPT(asclin7RxISR, AURIX_CORE_ID, INTPRIO_ASCLIN7_RX);
void asclin7RxISR(void) {
    uartPort_t *s = &(uartHardwareMap[UARTDEV_8]->port);
    uartRxIrqHandler(s);
}
#endif
