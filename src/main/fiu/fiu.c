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

#include "common/utils.h"
#include "fiu/fiu.h"
#include "programming/global_variables.h"

// Motor disable flags, updated at 100Hz from taskUpdateAux
static bool motorDisabled[MAX_MOTORS] = {false};

// Bus fault bitmasks, updated at 100Hz from taskUpdateAux
// Bit N = I2CDEV_(N+1) or SPIDEV_(N+1)
static uint32_t i2cBusMask = 0;
static uint32_t spiBusMask = 0;

void fiuUpdateFromGlobalVars(void)
{
    // GV0: Motor disable bitmask
    int32_t motorMask = gvGet(FIU_GV_MOTOR);

    for (int i = 0; i < MAX_MOTORS; i++) {
        motorDisabled[i] = (motorMask & BIT(i)) != 0;
    }

    // GV1: I2C bus block bitmask (Bit N = I2CDEV_(N+1))
    i2cBusMask = (uint32_t)gvGet(FIU_GV_I2C);

    // GV2: SPI bus block bitmask (Bit N = SPIDEV_(N+1))
    spiBusMask = (uint32_t)gvGet(FIU_GV_SPI);
}

bool fiuIsMotorDisabled(uint8_t motorIndex)
{
    if (motorIndex >= MAX_MOTORS) {
        return false;
    }
    return motorDisabled[motorIndex];
}

bool fiuIsBusReadBlocked(I2CDevice bus)
{
    if (bus < 0) return false;
    return (i2cBusMask & BIT(bus)) != 0;
}

bool fiuIsSpiReadBlocked(SPIDevice bus)
{
    if (bus < 0) return false;
    return (spiBusMask & BIT(bus)) != 0;
}