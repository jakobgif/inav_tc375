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
static bool rcLossFaultActive = false;
static uint8_t battFaultLevel = 0;

//per-bus rate and mask: set by fiuUpdateFromGlobalVars(), consumed by read-path functions
static uint8_t i2cErrorRate = 0;
static uint8_t spiErrorRate = 0;
static uint8_t i2cVariableRate = 0;
static uint8_t spiVariableRate = 0;
static uint8_t i2cActiveMask = 0;
static uint8_t spiActiveMask = 0;

//per-bus call counters incremented on actual sensor reads, not on FIU tick
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
    i2cActiveMask  = (uint8_t)i2cMask;
    i2cVariableRate   = (uint8_t)((i2cClamped - 1000) / 10);
    i2cErrorRate   = (uint8_t)(i2cVariableRate * FIU_MAX_I2C_ERROR_RATE / 100);

    //GV2: SPI bus select, GV4: error rate (RC knob 1000-2000 -> 0-100%)
    int32_t spiMask = gvGet(FIU_GV_SPI);
    int32_t spiRaw  = gvGet(FIU_GV_SPI_RATE);
    int32_t spiClamped = spiRaw < 1000 ? 1000 : spiRaw > 2000 ? 2000 : spiRaw;
    spiActiveMask  = (uint8_t)spiMask;
    spiVariableRate    = (uint8_t)((spiClamped - 1000) / 10);
    spiErrorRate   = (uint8_t)(spiVariableRate * FIU_MAX_SPI_ERROR_RATE / 100);

    //GV5: RC link loss fault — state only, rx.c reads fiuIsRcLossActive()
    rcLossFaultActive = (gvGet(FIU_GV_RC_LOSS) != 0);

    //GV6: battery voltage fault — passthrough knob 1000-2000 → level 0/1/2
    int32_t battRaw     = gvGet(FIU_GV_BATT);
    int32_t battClamped = battRaw < 1000 ? 1000 : battRaw > 2000 ? 2000 : battRaw;
    if (battClamped < 1250)
        battFaultLevel = 0;
    else if (battClamped <= 1750)
        battFaultLevel = 1;
    else
        battFaultLevel = 2;

    //update blackbox state snapshot
    fiuState.motorMask   = (uint8_t)motorMask;
    fiuState.i2cMask     = (uint8_t)i2cMask;
    fiuState.spiMask     = (uint8_t)spiMask;
    fiuState.i2cRate     = i2cErrorRate;
    fiuState.spiRate     = spiErrorRate;
    fiuState.rcLossFault = rcLossFaultActive ? 1 : 0;
    fiuState.battFault   = battFaultLevel;
}

bool fiuIsRcLossActive(void)
{
    return rcLossFaultActive;
}

uint8_t fiuGetBatteryFaultLevel(void)
{
    return battFaultLevel;
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
    if (!(i2cActiveMask & BIT(bus)) || i2cErrorRate == 0) return false;
    if (i2cErrorRate >= 100) return true;
    bool blocked = (i2cCallCount[bus] % 100) < i2cErrorRate;
    i2cCallCount[bus] = (i2cCallCount[bus] + 1) % 100;
    return blocked;
}

bool fiuIsSpiBusReadBlocked(SPIDevice bus)
{
    if (bus < 0 || bus >= SPIDEV_COUNT) return false;
    if (!(spiActiveMask & BIT(bus)) || spiErrorRate == 0) return false;
    if (spiErrorRate >= 100) return true;
    bool blocked = (spiCallCount[bus] % 100) < spiErrorRate;
    spiCallCount[bus] = (spiCallCount[bus] + 1) % 100;
    return blocked;
}