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

#include <stdbool.h>
#include <stdint.h>

#include "platform.h"

#include "fiu/fiu.h"
#include "rx/rx.h"

// FIU configuration - can be modified via configurator later
static fiuConfig_t fiuConfig = {
    .enabled = true,              // FIU enabled by default
    .rcChannelIndex = 2,          // AUX index (0=AUX1, 1=AUX2, 2=AUX3, 3=AUX4)
    .rcThreshold = 1500,          // Switch threshold (> 1500 = active)
    .targetMotorIndex = 0         // Motor 0 (front-right on typical quad)
};

// FIU runtime state
static fiuState_t fiuState = {
    .isActive = false,
    .activationCount = 0
};

void fiuInit(void)
{
    // Initialize FIU state
    fiuState.isActive = false;
    fiuState.activationCount = 0;
}

void fiuUpdate(void)
{
    if (!fiuConfig.enabled) {
        fiuState.isActive = false;
        return;
    }

    // Read RC channel value
    // AUX channels start after the 4 main channels (Roll, Pitch, Yaw, Throttle)
    // So AUX1=4, AUX2=5, AUX3=6, AUX4=7
    int16_t channelValue = rxGetChannelValue(fiuConfig.rcChannelIndex + NON_AUX_CHANNEL_COUNT);

    // Determine if fault should be active based on RC switch position
    bool shouldBeActive = (channelValue > fiuConfig.rcThreshold);

    // Detect rising edge (fault activation)
    if (shouldBeActive && !fiuState.isActive) {
        fiuState.activationCount++;
    }

    fiuState.isActive = shouldBeActive;
}

bool fiuIsMotorDisabled(uint8_t motorIndex)
{
    if (!fiuConfig.enabled) {
        return false;
    }

    if (!fiuState.isActive) {
        return false;
    }

    // DEBUG TEST: Disable ALL motors to verify FIU is working
    // TODO: Change back to: return (motorIndex == fiuConfig.targetMotorIndex);
    return true;  // Temporarily disable ALL motors for testing
}

const fiuState_t* fiuGetState(void)
{
    return &fiuState;
}

const fiuConfig_t* fiuGetConfig(void)
{
    return &fiuConfig;
}