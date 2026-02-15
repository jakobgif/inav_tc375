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
 *   GV0 (FIU_GV_MOTOR_MASK): Bitmask of motors to disable
 *     Bit 0 = Motor 0, Bit 1 = Motor 1, ..., Bit 5 = Motor 5
 *     Example: GV0 = 7  (0b000111) -> disable motors 0, 1, 2
 *     Example: GV0 = 56 (0b111000) -> disable motors 3, 4, 5
 *     Example: GV0 = 63 (0b111111) -> disable all 6 motors
 *
 * Configure via INAV Configurator Logic Conditions to set GV0.
 */

// GV index for motor disable bitmask
#define FIU_GV_MOTOR   0

// Update FIU state from Global Variables (call from taskUpdateAux at 100Hz)
void fiuUpdateFromGlobalVars(void);

// Check if specific motor should be disabled (called by PWM driver)
bool fiuIsMotorDisabled(uint8_t motorIndex);
