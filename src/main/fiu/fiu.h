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

//GV indices for fault configuration via INAV Logic Conditions
#define FIU_GV_MOTOR     0  // GV0: Bitmask of motors to disable
#define FIU_GV_I2C       1  // GV1: Bitmask of I2C buses affected (Bit 0=I2C1, Bit 1=I2C2, ...)
#define FIU_GV_SPI       2  // GV2: Bitmask of SPI buses affected  (Bit 0=SPI1, Bit 1=SPI2, ...)
#define FIU_GV_I2C_RATE  3  // GV3: I2C error rate 0-100% (RC knob)
#define FIU_GV_SPI_RATE  4  // GV4: SPI error rate 0-100% (RC knob)
#define FIU_GV_RC_LOSS   5  // GV5: 1 = simulate RC link loss (failsafe trigger)
#define FIU_GV_BATT      6  // GV6: battery voltage fault (0=off, 1=warning-level, 2=critical-level)
#define FIU_MAX_I2C_ERROR_RATE 50// percent
#define FIU_MAX_SPI_ERROR_RATE 80// percent

// FIU state snapshot for blackbox logging
typedef struct {
    uint8_t motorMask;   // GV0: bitmask of disabled motors
    uint8_t i2cMask;     // GV1: bitmask of affected I2C buses
    uint8_t spiMask;     // GV2: bitmask of affected SPI buses
    uint8_t i2cRate;     // GV3: I2C error rate 0-100
    uint8_t spiRate;     // GV4: SPI error rate 0-100
    uint8_t rcLossFault; // GV5: 1 = RC link loss fault active
    uint8_t battFault;   // GV6: 0=off, 1=warning-level, 2=critical-level
} fiuState_t;

void fiuUpdateFromGlobalVars(void);
bool fiuIsMotorDisabled(uint8_t motorIndex);
bool fiuIsI2cBusReadBlocked(I2CDevice bus);
bool fiuIsSpiBusReadBlocked(SPIDevice bus);
bool fiuIsRcLossActive(void);
uint8_t fiuGetBatteryFaultLevel(void);
const fiuState_t *fiuGetState(void);
