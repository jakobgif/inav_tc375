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
#include "rcc.h"
#include "drivers/nvic.h"

#include "serial.h"
#include "serial_uart.h"
#include "serial_uart_impl.h"

#define UART_RX_BUFFER_SIZE UART1_RX_BUFFER_SIZE
#define UART_TX_BUFFER_SIZE UART1_RX_BUFFER_SIZE

typedef struct uartDevice_s {
    IfxAsclin_Asc *handle; //handle statically defined somewhere and referenced here
    IfxAsclin_Asc_Config *config; //we store this statically somewhere so we can modifiy the existin config afterwards
    uartPort_t port;
    ioTag_t rx;
    ioTag_t tx;
    volatile uint8_t rxBuffer[UART_RX_BUFFER_SIZE];
    volatile uint8_t txBuffer[UART_TX_BUFFER_SIZE];
    uint8_t irq;
} uartDevice_t;

#ifdef USE_UART1
IfxAsclin_Asc uart1handle = {
    .asclin = &MODULE_ASCLIN0,
};
IfxAsclin_Asc_Pins uart1PinConfig = {
    .rx = &UART1_PINMAP_RX,
    .tx = &UART1_PINMAP_TX,
};
IfxAsclin_Asc_Config uart1Config = {
    .pins = &uart1PinConfig,
};
static uartDevice_t uart1 = {
    .handle = &uart1handle, //connect aurix driver to inav uart device
    .config = &uart1Config, //connect aurix driver config to inav uart device
    .rx = IO_TAG(UART1_PIN_RX),
    .tx = IO_TAG(UART1_PIN_TX),
    .irq = NULL,
};
#endif

#ifdef USE_UART2

#endif

#ifdef USE_UART3

#endif

#ifdef USE_UART4

#endif

#ifdef USE_UART5

#endif

#ifdef USE_UART6

#endif

#ifdef USE_UART7

#endif

#ifdef USE_UART8

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

void uartTxIrqHandler(uartPort_t *s) {
    IfxAsclin_Asc_isrTransmit(s->handle);
}

void uartRxIrqHandler(uartPort_t *s) {
    IfxAsclin_Asc_isrReceive(s->handle);

	if(s->port.rxCallback) {
		uint8_t rxData;
		Ifx_SizeT size = sizeof(uint8_t); //one byte
		IfxAsclin_Asc_read(s->handle, &rxData, &size, TIME_INFINITE); //read from FIFO
		s->port.rxCallback(rxData, s->port.rxCallbackData);
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
    s->config->txBuffer = NULL_PTR; //buffer will be dynimcally assigned by aurix driver, but size is 0 so we will not create a buffer TODO: check if this is true by checking this address after init is done
    s->config->txBufferSize = 0; //buffer done by inav
    s->config->rxBuffer = NULL_PTR;
    s->config->rxBufferSize = 0;

    return s;
}

#ifdef USE_UART1
uartPort_t *serialUART1(uint32_t baudRate, portMode_t mode, portOptions_t options)
{
    uartPort_t *s = serialUART(UARTDEV_1, baudRate, mode, options);
    //here we set the int prios
    s->config->interrupt.txPriority = (inav_tc375_prio_levels)INTPRIO_ASCLIN0_TX;
    s->config->interrupt.rxPriority = (inav_tc375_prio_levels)INTPRIO_ASCLIN0_RX;
    s->config->interrupt.typeOfService = IfxCpu_Irq_getTos(IfxCpu_getCoreIndex());

    return s;
}

IFX_INTERRUPT(asclin0TxISR, 0, (inav_tc375_prio_levels)INTPRIO_ASCLIN0_TX);
void asclin0TxISR(void){
    uartPort_t *s = &(uartHardwareMap[UARTDEV_1]->port);
    uartTxIrqHandler(s);
}

IFX_INTERRUPT(asclin0RxISR, 0, (inav_tc375_prio_levels)INTPRIO_ASCLIN0_RX);
void asclin0RxISR(void){
    uartPort_t *s = &(uartHardwareMap[UARTDEV_1]->port);
    uartRxIrqHandler(s);
}
#endif

#ifdef USE_UART2

#endif

#ifdef USE_UART3

#endif

#ifdef USE_UART4

#endif

#ifdef USE_UART5

#endif

#ifdef USE_UART6

#endif

#ifdef USE_UART7

#endif

#ifdef USE_UART8

#endif
