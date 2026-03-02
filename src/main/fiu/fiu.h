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

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "drivers/pwm_mapping.h"
#include "drivers/bus_spi.h"
#include "drivers/bus_i2c.h"

/*
 * Fault Insertion Unit (FIU) for AURIX TC375 Flight Controller
 *
 * Master Thesis Project: Embedded Fault Insertion Unit for UAS
 *
 * Uses INAV Global Variables for configuration:
 *   GV0 (FIU_GV_MOTOR): Bitmask of motors to disable
 *     Bit 0 = Motor 0, Bit 1 = Motor 1, ..., Bit 5 = Motor 5
 *     Example: GV0 = 7  (0b000111) -> disable motors 0, 1, 2
 *     Example: GV0 = 56 (0b111000) -> disable motors 3, 4, 5
 *     Example: GV0 = 63 (0b111111) -> disable all 6 motors
 *
 *   GV1 (FIU_GV_I2C): Bitmask of I2C buses to block
 *     Bit 0 = I2CDEV_1, Bit 1 = I2CDEV_2, Bit 2 = I2CDEV_3
 *     Example: GV1 = 1 (0b001) -> block only I2C1
 *     Example: GV1 = 3 (0b011) -> block I2C1 + I2C2
 *
 *   GV2 (FIU_GV_SPI): Bitmask of SPI buses to block
 *     Bit 0 = SPIDEV_1, Bit 1 = SPIDEV_2, Bit 2 = SPIDEV_3, Bit 3 = SPIDEV_4
 *     Example: GV2 = 4 (0b0100) -> block only SPI3
 *     Example: GV2 = 3 (0b0011) -> block SPI1 + SPI2
 *
 * Configure via INAV Configurator Logic Conditions to set GV0/GV1/GV2.
 */

// GV indices
#define FIU_GV_MOTOR   0    // GV0: Bitmask of motors to disable
#define FIU_GV_I2C     1    // GV1: Bitmask of I2C buses to block (Bit 0=I2C1, Bit 1=I2C2, ...)
#define FIU_GV_SPI     2    // GV2: Bitmask of SPI buses to block (Bit 0=SPI1, Bit 1=SPI2, ...)

// GV1: I2C bus block bitmask (Bit N = I2CDEV_(N+1))
// GV2: SPI bus block bitmask (Bit N = SPIDEV_(N+1))

// Update FIU state from Global Variables (call from taskUpdateAux at 100Hz)
void fiuUpdateFromGlobalVars(void);

// Check if specific motor should be disabled (called by PWM driver)
bool fiuIsMotorDisabled(uint8_t motorIndex);

// Check if specific I2C bus read should be blocked (called by bus.c)
bool fiuIsI2cBusReadBlocked(I2CDevice bus);

// Check if specific SPI bus read should be blocked (called by bus.c)
bool fiuIsSpiBusReadBlocked(SPIDevice bus);
