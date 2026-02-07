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

/*
 * Fault Insertion Unit (FIU) for AURIX TC375 Flight Controller
 *
 * Master Thesis Project: Embedded Fault Insertion Unit for UAS
 *
 * This module allows testing fault-tolerant algorithms by injecting
 * realistic faults that can be activated via RC controller.
 */

#define FIU_MAX_MOTORS 12

// FIU configuration
typedef struct {
    bool enabled;                    // Enable/disable FIU module
    uint8_t rcChannelIndex;          // RC channel for activation (e.g., 7 for AUX3)
    uint16_t rcThreshold;            // RC value threshold (typically 1500)
    uint8_t targetMotorIndex;        // Which motor to disable (0-3 for quad)
} fiuConfig_t;

// FIU runtime state
typedef struct {
    bool isActive;                   // Is fault currently active?
    uint32_t activationCount;        // Number of fault activations
} fiuState_t;

// Initialize FIU module
void fiuInit(void);

// Update FIU state (call from main loop)
void fiuUpdate(void);

// Check if specific motor should be disabled
bool fiuIsMotorDisabled(uint8_t motorIndex);

// Get current state
const fiuState_t* fiuGetState(void);

// Get current configuration
const fiuConfig_t* fiuGetConfig(void);