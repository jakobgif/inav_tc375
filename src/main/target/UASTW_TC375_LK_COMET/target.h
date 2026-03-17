/*
 * This file is part of INAV.
 *
 * INAV is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * INAV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with INAV.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file target.h
 * @author Jakob Frenzel (jakob.frenzel@hotmail.com)
 * @brief target configuration for FHTW COMET drone
 * @date 2025-09-22
 */

#pragma once

#define TARGET_BOARD_IDENTIFIER "CMET"

// *************** LED **********************
//lite kit onboard LEDs
//#define LED0    MODULE_P00_5
//#define LED1    MODULE_P00_6

// *************** Motor outputs **********************
//pins to ESC
#define MOTOR_1_PIN     MODULE_P00_1
#define MOTOR_2_PIN     MODULE_P00_6
#define MOTOR_3_PIN     MODULE_P00_3
#define MOTOR_4_PIN     MODULE_P00_9
#define MOTOR_5_PIN     MODULE_P00_5
#define MOTOR_6_PIN     MODULE_P00_7

// *************** SPI: Gyro & ACC & OSD **********************
#define USE_SPI

//IMU plugged into mikroBus
#define USE_SPI_DEVICE_3
#define SPI3_PIN_SCLK       MODULE_P15_8
#define SPI3_PIN_MRST       MODULE_P15_7
#define SPI3_PIN_MTSR       MODULE_P15_6

#define USE_IMU_BMI088
#define IMU_BMI088_ALIGN    CW90_DEG
#define BMI088_SPI_BUS      BUS_SPI3
#define BMI088_GYRO_CS_PIN  MODULE_P02_8
#define BMI088_ACC_CS_PIN   MODULE_P14_7

#ifdef USE_AURIX_MULTICORE
//move SPI1 bus to CPU 1 if using multicore build
#define CPU1_SPI_BUS        BMI088_SPI_BUS
#endif

// *************** UART *****************************
//to uart - usb brigde
#define USE_UART1
#define UART1_PIN_RX    MODULE_P14_1
#define UART1_PIN_TX    MODULE_P14_0

//ibus connection to receiver
#define USE_UART2
#define UART2_PIN_RX    MODULE_P15_5
#define UART2_PIN_TX    MODULE_P15_4

//connection to openlog blackbox
//Shield2Go connector 1
#define USE_UART3
#define UART3_PIN_RX    MODULE_P33_8
#define UART3_PIN_TX    MODULE_P33_9

//logging output
//Shield2Go connector 2
#define USE_UART4
#define UART4_PIN_RX    MODULE_P20_3
#define UART4_PIN_TX    MODULE_P20_0

#define SERIAL_PORT_COUNT   4

// *************** I2C: BARO & MAG ****************************
//baro plugged into Shield2Go connector 2
#define USE_I2C
#define USE_I2C_DEVICE_1

#define I2C1_SCL            MODULE_P13_1
#define I2C1_SDA            MODULE_P13_2

#define USE_BARO
#define USE_BARO_DPS310
#define BARO_I2C_BUS        BUS_I2C1

// *************** ADC *****************************
#define USE_ADC
#define ADC_CHANNEL_1_PIN   0 //aurix analog input 0
#define VBAT_ADC_CHANNEL    ADC_CHN_1
#define VBAT_SCALE_DEFAULT  127

#define DEFAULT_FEATURES    (FEATURE_VBAT | FEATURE_PWM_OUTPUT_ENABLE | FEATURE_BLACKBOX)

#define USE_DSHOT
#define USE_SERIAL_4WAY_BLHELI_INTERFACE

#define TARGET_IO_PORTMODULE_P00    0b1011101010
#define TARGET_IO_PORTMODULE_P02    0b100000000
#define TARGET_IO_PORTMODULE_P13    0b110
#define TARGET_IO_PORTMODULE_P14    0b10000011
#define TARGET_IO_PORTMODULE_P15    0b111110000
#define TARGET_IO_PORTMODULE_P20    0b1001
#define TARGET_IO_PORTMODULE_P33    0b1100000000

#define MAX_PWM_OUTPUT_PORTS 6
