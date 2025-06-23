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
 * @date 2025-03-17
 */

#pragma once

#define TARGET_BOARD_IDENTIFIER "FHTW_TC375_LK"

// *************** LED **********************
#define LED0        MODULE_P00_5
#define LED1        MODULE_P00_6

// *************** SPI: Gyro & ACC & OSD **********************
#define USE_SPI

//IMU is connected to mikroBus
//CS Pins must have valid IfxQspi_Slso_Out variables. otherwise init cannot be performed. 
//Note: for this implemenation it does not matter if IfxQspi_Slso_Out is linked to a different QSPI module.
//MODULE_QSPI2
#define USE_SPI_DEVICE_3 
#define SPI3_PIN_SCLK       MODULE_P15_8
#define SPI3_PIN_MRST       MODULE_P15_7
#define SPI3_PIN_MTSR       MODULE_P15_6

#define USE_IMU_BMI088
#define IMU_BMI088_ALIGN    CW90_DEG
#define BMI088_SPI_BUS      BUS_SPI3
#define BMI088_GYRO_CS_PIN  MODULE_P02_8
#define BMI088_ACC_CS_PIN   MODULE_P14_7

// *************** UART *****************************
#define USE_UART1 //to uart - usb brigde

#define UART1_PIN_RX      MODULE_P14_1
#define UART1_PIN_TX      MODULE_P14_0

// #define USE_UART2 //mikroBus connector
// #define UART2_PIN_RX      MODULE_P15_1
// #define UART2_PIN_TX      MODULE_P15_0

#define USE_UART3 //Shield2Go connector 1
#define UART3_PIN_RX      MODULE_P33_8
#define UART3_PIN_TX      MODULE_P33_9

#define USE_UART4 //Shield2Go connector 2
#define UART4_PIN_RX      MODULE_P20_3
#define UART4_PIN_TX      MODULE_P20_0

#define SERIAL_PORT_COUNT   3

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

#define TARGET_IO_PORTMODULE_P00    0b1100000
#define TARGET_IO_PORTMODULE_P02    0b100000000
#define TARGET_IO_PORTMODULE_P14    0b10000011
#define TARGET_IO_PORTMODULE_P15    0b111000000
#define TARGET_IO_PORTMODULE_P20    0b1001
#define TARGET_IO_PORTMODULE_P33    0b1100000000

#define MAX_PWM_OUTPUT_PORTS    0
