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

void fiuUpdateFromGlobalVars(void)
{
    int32_t motorMask = gvGet(FIU_GV_MOTOR_MASK);

    for (int i = 0; i < MAX_MOTORS; i++) {
        motorDisabled[i] = (motorMask & BIT(i)) != 0;
    }
}

bool fiuIsMotorDisabled(uint8_t motorIndex)
{
    if (motorIndex >= MAX_MOTORS) {
        return false;
    }
    return motorDisabled[motorIndex];
}