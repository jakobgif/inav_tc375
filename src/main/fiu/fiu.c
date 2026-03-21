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

static bool motorDisabled[MAX_MOTORS] = {false};
static fiuState_t fiuState = {0};

//non-static: readable by bus.c at bus abstraction layer
bool i2cBusBlocked[I2CDEV_COUNT] = {false};
bool spiBusBlocked[SPIDEV_COUNT] = {false};

//per-bus call counters for deterministic rate-based blocking
static uint8_t i2cCallCount[I2CDEV_COUNT] = {0};
static uint8_t spiCallCount[SPIDEV_COUNT] = {0};

void fiuUpdateFromGlobalVars(void)
{
    //GV0: motor disable bitmask
    int32_t motorMask = gvGet(FIU_GV_MOTOR);
    for (int i = 0; i < MAX_MOTORS; i++) {
        motorDisabled[i] = (motorMask & BIT(i)) != 0;
    }

    //GV1: I2C bus select, GV3: error rate (RC knob 1000-2000 -> 0-100%)
    int32_t i2cMask = gvGet(FIU_GV_I2C);
    int32_t i2cRaw  = gvGet(FIU_GV_I2C_RATE);
    int32_t i2cClamped = i2cRaw < 1000 ? 1000 : i2cRaw > 2000 ? 2000 : i2cRaw;
    uint8_t i2cErrorRate = (uint8_t)((i2cClamped - 1000) / 10);
    for (int i = 0; i < I2CDEV_COUNT; i++) {
        if ((i2cMask & BIT(i)) == 0 || i2cErrorRate == 0) {
            i2cBusBlocked[i] = false;
        } else if (i2cErrorRate >= 100) {
            i2cBusBlocked[i] = true;
        } else {
            i2cBusBlocked[i] = (i2cCallCount[i] % 100) < i2cErrorRate;
        }
        i2cCallCount[i]++;
    }

    //GV2: SPI bus select, GV4: error rate (RC knob 1000-2000 -> 0-100%)
    int32_t spiMask = gvGet(FIU_GV_SPI);
    int32_t spiRaw  = gvGet(FIU_GV_SPI_RATE);
    int32_t spiClamped = spiRaw < 1000 ? 1000 : spiRaw > 2000 ? 2000 : spiRaw;
    uint8_t spiErrorRate = (uint8_t)((spiClamped - 1000) / 10);
    for (int i = 0; i < SPIDEV_COUNT; i++) {
        if (!(spiMask & BIT(i)) || spiErrorRate == 0) {
            spiBusBlocked[i] = false;
        } else if (spiErrorRate >= 100) {
            spiBusBlocked[i] = true;
        } else {
            spiBusBlocked[i] = (spiCallCount[i] % 100) < spiErrorRate;
        }
        spiCallCount[i]++;
    }

    //update blackbox state snapshot
    fiuState.motorMask = (uint8_t)motorMask;
    fiuState.i2cMask   = (uint8_t)i2cMask;
    fiuState.spiMask   = (uint8_t)spiMask;
    fiuState.i2cRate   = i2cErrorRate;
    fiuState.spiRate   = spiErrorRate;
}

const fiuState_t *fiuGetState(void)
{
    return &fiuState;
}

bool fiuIsMotorDisabled(uint8_t motorIndex)
{
    if (motorIndex >= MAX_MOTORS) {
        return false;
    }
    return motorDisabled[motorIndex];
}

bool fiuIsI2cBusReadBlocked(I2CDevice bus)
{
    if (bus < 0 || bus >= I2CDEV_COUNT) return false;
    return i2cBusBlocked[bus];
}

bool fiuIsSpiBusReadBlocked(SPIDevice bus)
{
    if (bus < 0 || bus >= SPIDEV_COUNT) return false;
    return spiBusBlocked[bus];
}