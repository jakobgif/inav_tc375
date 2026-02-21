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
 *   GV1 (FIU_GV_SENSOR): Bitmask of sensor bus faults to activate
 *     Bit 2 = DPS310 barometer bus read block
 *     When active: busRead/busReadBuf returns zero-filled data + true
 *     Simulates sensor chip failure at bus level
 *
 * Configure via INAV Configurator Logic Conditions to set GV0/GV1.
 */

// GV indices
#define FIU_GV_MOTOR   0    // GV0: Bitmask of motors to disable
#define FIU_GV_SENSOR  1    // GV1: Bitmask of sensor bus faults to activate

// GV1 sensor bus fault bitmask bits
#define FIU_SENSOR_BARO_BUS_BLOCK   BIT(2)  // Bit 2: Block DPS310 barometer bus reads

// Update FIU state from Global Variables (call from taskUpdateAux at 100Hz)
void fiuUpdateFromGlobalVars(void);

// Check if specific motor should be disabled (called by PWM driver)
bool fiuIsMotorDisabled(uint8_t motorIndex);

// Check if bus read should be blocked (called by bus.c)
// Returns true if bus reads should be intercepted (zero-fill + return true)
bool fiuIsBusReadBlocked(void);
