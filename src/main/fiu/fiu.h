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

// GV indices for fault configuration via INAV Logic Conditions
#define FIU_GV_MOTOR     0  // GV0: bitmask of motors to disable
#define FIU_GV_I2C       1  // GV1: bitmask of I2C buses affected (Bit 0=I2C1, Bit 1=I2C2, ...)
#define FIU_GV_I2C_RATE  3  // GV3: I2C error rate 0-100% (RC knob)
#define FIU_GV_SPI       2  // GV2: bitmask of SPI buses affected  (Bit 0=SPI1, Bit 1=SPI2, ...)
#define FIU_GV_SPI_RATE  4  // GV4: SPI knob 0-100% (error rate or overrange intensity)
#define FIU_GV_BATT      6  // GV6: battery voltage fault level (knob: 0=off, 1=warning, 2=critical)
#define FIU_GV_RC_LOSS   5  // GV5: 1 = simulate RC link loss (failsafe trigger)

#define FIU_MAX_I2C_ERROR_RATE 50  // percent
#define FIU_MAX_SPI_ERROR_RATE 80  // percent

// FIU state snapshot for blackbox logging — grouped by fault type
typedef struct {
    // Motor fault (GV0)
    uint8_t motorMask;    // bitmask of disabled motors

    // I2C / Baro fault (GV1, GV3)
    uint8_t i2cMask;      // bitmask of affected I2C buses
    uint8_t i2cRate;      // I2C error rate 0-100

    // SPI / Gyro fault (GV2, GV4)
    uint8_t spiMask;      // bitmask of affected SPI buses
    uint8_t spiRate;      // knob value 0-100 (rate in error-rate mode, intensity in overrange mode)
    uint8_t spiOverrange; // 1 = overrange mode active (toggled by knob=0 + switch ON->OFF)

    // Battery fault (GV6)
    uint8_t battFault;    // 0=off, 1=warning-level, 2=critical-level

    // RC Loss fault (GV5)
    uint8_t rcLossFault;  // 1 = RC link loss fault active
} fiuState_t;

void fiuUpdateFromGlobalVars(void);

// Motor fault
bool fiuIsMotorDisabled(uint8_t motorIndex);

// I2C / Baro fault
bool fiuIsI2cBusReadBlocked(I2CDevice bus);

// SPI / Gyro fault
bool fiuIsSpiBusReadBlocked(SPIDevice bus);
bool fiuIsSpiOverrangeActive(SPIDevice bus);
uint8_t fiuGetSpiOverrangeFillByte(void);

// Battery fault
uint8_t fiuGetBatteryFaultLevel(void);

// RC Loss fault
bool fiuIsRcLossActive(void);

const fiuState_t *fiuGetState(void);
