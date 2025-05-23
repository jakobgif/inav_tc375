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
 * @file target.h
 * @author Jakob Frenzel (jakob.frenzel@hotmail.com)
 * @brief This file defines the target config for the TC375 litekit
 * @version 0.1
 * @date 2025-03-17
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#define TARGET_BOARD_IDENTIFIER "FHTW_TC375_LK"

// *************** LED **********************

#define LED0        MODULE_P00_5
#define LED0_PINMAP IfxGtm_TOM0_0N_TOUT106_P10_4_OUT
#define LED1        MODULE_P00_6

//TODO: from this 
//#define LED0_PINMAP         IfxAsclin0_RXA_P14_1_IN
//IfxAsclin_Rx_In IfxAsclin9_RXB_P01_7_IN = {&MODULE_ASCLIN9, {&MODULE_P01, 7}, Ifx_RxSel_b};
//shall be automatically created
//#define LED0    MODULE_P00_5

// *************** SPI: Gyro & ACC & OSD **********************
// #define USE_SPI
// #define USE_SPI_DEVICE_1
// #define USE_SPI_DEVICE_2

// #define SPI1_SCK_PIN        PA5
// #define SPI1_MISO_PIN   	PA6
// #define SPI1_MOSI_PIN   	PA7

// #define SPI2_SCK_PIN        PD3
// #define SPI2_MISO_PIN   	PC2
// #define SPI2_MOSI_PIN   	PC3

// #define USE_IMU_BMI088
// #define IMU_BMI088_ALIGN        CW270_DEG
// #define BMI088_SPI_BUS          BUS_SPI2
// #define BMI088_GYRO_CS_PIN      PD5
// #define BMI088_ACC_CS_PIN       PD4

// #define USE_MAX7456
// #define MAX7456_SPI_BUS         BUS_SPI1
// #define MAX7456_CS_PIN          PB12

// *************** UART *****************************
#define USE_UART1 //to uart - usb brigde
//on aurix we need pinmap to route peripherals correctly. But inav also needs a pin number
#define UART1_PINMAP_RX   IfxAsclin0_RXA_P14_1_IN
#define UART1_PIN_RX      MODULE_P14_1
#define UART1_PINMAP_TX   IfxAsclin0_TX_P14_0_OUT
#define UART1_PIN_TX      MODULE_P14_0

#define USE_UART2 //mikroBus connector
#define UART2_PINMAP_RX   IfxAsclin1_RXA_P15_1_IN
#define UART2_PIN_RX      MODULE_P15_1
#define UART2_PINMAP_TX   IfxAsclin1_TX_P15_0_OUT
#define UART2_PIN_TX      MODULE_P15_0

#define USE_UART3 //Shield2Go connector 1
#define UART3_PINMAP_RX   IfxAsclin2_RXE_P33_8_IN
#define UART3_PIN_RX      MODULE_P33_8
#define UART3_PINMAP_TX   IfxAsclin2_TX_P33_9_OUT
#define UART3_PIN_TX      MODULE_P33_9

#define USE_UART4 //Shield2Go connector 2
#define UART4_PINMAP_RX   IfxAsclin3_RXC_P20_3_IN
#define UART4_PIN_RX      MODULE_P20_3
#define UART4_PINMAP_TX   IfxAsclin3_TX_P20_0_OUT
#define UART4_PIN_TX      MODULE_P20_0

#define SERIAL_PORT_COUNT   4

// #define DEFAULT_RX_TYPE         RX_TYPE_SERIAL
// #define SERIALRX_PROVIDER       SERIALRX_SBUS
// #define SERIALRX_UART           SERIAL_PORT_USART6

// *************** I2C: BARO & MAG ****************************
// #define USE_I2C
// #define USE_I2C_DEVICE_1
// #define USE_I2C_DEVICE_2
// #define I2C1_SCL                PB6
// #define I2C1_SDA                PB7
// #define I2C2_SCL                PB10
// #define I2C2_SDA                PB11

// #define USE_BARO
// #define USE_BARO_DPS310
// #define BARO_I2C_BUS            BUS_I2C2

// #define USE_MAG

// #ifdef MICOAIR743_EXTMAG
// // External compass
// #define MAG_I2C_BUS             BUS_I2C1
// #else
// // Onboard compass
// #define MAG_I2C_BUS             BUS_I2C2
// #endif
// #define USE_MAG_ALL

// *************** ENABLE OPTICAL FLOW & RANGEFINDER *****************************
// #define USE_RANGEFINDER
// #define USE_RANGEFINDER_MSP
// #define USE_OPFLOW
// #define USE_OPFLOW_MSP

// *************** SDIO SD BLACKBOX*******************
// #define USE_SDCARD
// #define USE_SDCARD_SDIO
// #define SDCARD_SDIO_DEVICE      SDIODEV_1
// #define SDCARD_SDIO_4BIT
// #define ENABLE_BLACKBOX_LOGGING_ON_SDCARD_BY_DEFAULT

// *************** ADC *****************************
// #define USE_ADC
// #define ADC_INSTANCE                ADC1
// #define ADC_CHANNEL_1_PIN           PC0
// #define ADC_CHANNEL_2_PIN           PC1
// #define VBAT_ADC_CHANNEL            ADC_CHN_1
// #define CURRENT_METER_ADC_CHANNEL   ADC_CHN_2
// #define VBAT_SCALE_DEFAULT          2121
// #define CURRENT_METER_SCALE         402

// #define DEFAULT_FEATURES        (FEATURE_VBAT | FEATURE_CURRENT_METER | FEATURE_OSD | FEATURE_TELEMETRY)

// #define USE_DSHOT
// #define USE_ESC_SENSOR
// #define USE_SERIAL_4WAY_BLHELI_INTERFACE

#define TARGET_IO_PORTMODULE_P00 0x60
#define TARGET_IO_PORTMODULE_P14 0b11
#define TARGET_IO_PORTMODULE_P15 0b11
#define TARGET_IO_PORTMODULE_P20 0b1001
#define TARGET_IO_PORTMODULE_P33 0b1100000000

/*#define DEFIO_GPIOID__MODULE_P01 12
#define DEFIO_GPIOID__MODULE_P02 12
#define DEFIO_GPIOID__MODULE_P10 12
#define DEFIO_GPIOID__MODULE_P11 12
#define DEFIO_GPIOID__MODULE_P12 12
#define DEFIO_GPIOID__MODULE_P13 12
#define DEFIO_GPIOID__MODULE_P14 12
#define DEFIO_GPIOID__MODULE_P15 12
#define DEFIO_GPIOID__MODULE_P20 12
#define DEFIO_GPIOID__MODULE_P21 12
#define DEFIO_GPIOID__MODULE_P22 12
#define DEFIO_GPIOID__MODULE_P23 12
#define DEFIO_GPIOID__MODULE_P32 12
#define DEFIO_GPIOID__MODULE_P33 12
#define DEFIO_GPIOID__MODULE_P34 12
#define DEFIO_GPIOID__MODULE_P40 12*/

#define MAX_PWM_OUTPUT_PORTS       10
