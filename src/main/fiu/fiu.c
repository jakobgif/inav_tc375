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

static fiuState_t fiuState = {0};

// --- Motor fault (GV0) ---
static bool motorDisabled[MAX_MOTORS] = {false};

// --- I2C / Baro fault (GV1, GV3) ---
static uint8_t i2cActiveMask   = 0;
static uint8_t i2cVariableRate = 0;  // raw knob 0-100
static uint8_t i2cErrorRate    = 0;  // scaled to FIU_MAX_I2C_ERROR_RATE
static uint8_t i2cCallCount[I2CDEV_COUNT] = {0};

// --- SPI / Gyro fault (GV2, GV4) ---
static uint8_t spiActiveMask   = 0;
static uint8_t spiVariableRate = 0;  // raw knob 0-100
static uint8_t spiErrorRate    = 0;  // scaled to FIU_MAX_SPI_ERROR_RATE
static uint8_t spiCallCount[SPIDEV_COUNT] = {0};

// SPI gyro fault — two exclusive modes, toggled by: knob=0 + switch ON->OFF edge
//
//  [1] Error-rate mode (default)
//      Blocked reads -> all gyro axes = 0 dps ("silent fault")
//      INAV thinks the drone is stationary -> PID reacts incorrectly
//
//  [2] Overrange mode
//      Blocked reads -> all gyro axes = ~1000-1990 dps (knob controls intensity)
//      Physically impossible for a multirotor (real max ~500-800 dps) -> clearly detectable
//      memset writes the fill byte to every byte in the buffer — each axis occupies
//      2 bytes (high + low), so both bytes get the same fill value:
//        min: fill=0x40 -> high=0x40, low=0x40 -> 16-bit=0x4040=16448 LSB -> ~1003 dps
//        max: fill=0x7F -> high=0x7F, low=0x7F -> 16-bit=0x7F7F=32639 LSB -> ~1990 dps
//      (ICM-42688 +-2000 dps range, sensitivity 16.4 LSB/dps)
//
//  Note: all three axes always receive the same injected value (memset limitation)
static bool spiOverrangeMode = false;
static bool spiSwitchPrev    = false;

// --- Battery fault (GV6) ---
static uint8_t battFaultLevel = 0;

// --- RC Loss fault (GV5) ---
static bool rcLossFaultActive = false;

// ---------------------------------------------------------------------------

void fiuUpdateFromGlobalVars(void)
{
    // GV0: motor disable bitmask
    int32_t motorMask = gvGet(FIU_GV_MOTOR);
    for (int i = 0; i < MAX_MOTORS; i++) {
        motorDisabled[i] = (motorMask & BIT(i)) != 0;
    }

    // GV1 + GV3: I2C bus select + error rate (RC knob 1000-2000 -> 0-100%)
    int32_t i2cMask    = gvGet(FIU_GV_I2C);
    int32_t i2cRaw     = gvGet(FIU_GV_I2C_RATE);
    int32_t i2cClamped = i2cRaw < 1000 ? 1000 : i2cRaw > 2000 ? 2000 : i2cRaw;
    i2cActiveMask   = (uint8_t)i2cMask;
    i2cVariableRate = (uint8_t)((i2cClamped - 1000) / 10);
    i2cErrorRate    = (uint8_t)(i2cVariableRate * FIU_MAX_I2C_ERROR_RATE / 100);

    // GV2 + GV4: SPI bus select + knob (error rate or overrange intensity)
    int32_t spiMask    = gvGet(FIU_GV_SPI);
    int32_t spiRaw     = gvGet(FIU_GV_SPI_RATE);
    int32_t spiClamped = spiRaw < 1000 ? 1000 : spiRaw > 2000 ? 2000 : spiRaw;
    spiActiveMask   = (uint8_t)spiMask;
    spiVariableRate = (uint8_t)((spiClamped - 1000) / 10);
    spiErrorRate    = (uint8_t)(spiVariableRate * FIU_MAX_SPI_ERROR_RATE / 100);

    // SPI overrange mode toggle: knob=0 + switch ON->OFF edge switches between
    // error-rate mode (zeros) and overrange mode (0x40-0x7F fill)
    bool spiSwitchNow = (spiActiveMask != 0);
    if (spiSwitchPrev && !spiSwitchNow && (spiVariableRate == 0)) {
        spiOverrangeMode = !spiOverrangeMode;
    }
    spiSwitchPrev = spiSwitchNow;

    // GV6: battery voltage fault — knob 1000-2000 -> level 0/1/2
    int32_t battRaw     = gvGet(FIU_GV_BATT);
    int32_t battClamped = battRaw < 1000 ? 1000 : battRaw > 2000 ? 2000 : battRaw;
    if (battClamped < 1250)
        battFaultLevel = 0;
    else if (battClamped <= 1750)
        battFaultLevel = 1;
    else
        battFaultLevel = 2;

    // GV5: RC link loss fault — state only, rx.c reads fiuIsRcLossActive()
    rcLossFaultActive = (gvGet(FIU_GV_RC_LOSS) != 0);

    // Update blackbox state snapshot
    fiuState.motorMask    = (uint8_t)motorMask;
    fiuState.i2cMask      = (uint8_t)i2cMask;
    fiuState.i2cRate      = i2cErrorRate;
    fiuState.spiMask      = (uint8_t)spiMask;
    fiuState.spiRate      = spiVariableRate;
    fiuState.spiOverrange = spiOverrangeMode ? 1 : 0;
    fiuState.battFault    = battFaultLevel;
    fiuState.rcLossFault  = rcLossFaultActive ? 1 : 0;
}

// --- Motor fault ---

bool fiuIsMotorDisabled(uint8_t motorIndex)
{
    if (motorIndex >= MAX_MOTORS) {
        return false;
    }
    return motorDisabled[motorIndex];
}

// --- I2C / Baro fault ---

bool fiuIsI2cBusReadBlocked(I2CDevice bus)
{
    if (bus < 0 || bus >= I2CDEV_COUNT) return false;
    if (!(i2cActiveMask & BIT(bus)) || i2cErrorRate == 0) return false;
    if (i2cErrorRate >= 100) return true;
    bool blocked = (i2cCallCount[bus] % 100) < i2cErrorRate;
    i2cCallCount[bus] = (i2cCallCount[bus] + 1) % 100;
    return blocked;
}

// --- SPI / Gyro fault ---

bool fiuIsSpiBusReadBlocked(SPIDevice bus)
{
    if (spiOverrangeMode) return false;  // overrange mode handled separately
    if (bus < 0 || bus >= SPIDEV_COUNT) return false;
    if (!(spiActiveMask & BIT(bus)) || spiErrorRate == 0) return false;
    if (spiErrorRate >= 100) return true;
    bool blocked = (spiCallCount[bus] % 100) < spiErrorRate;
    spiCallCount[bus] = (spiCallCount[bus] + 1) % 100;
    return blocked;
}

bool fiuIsSpiOverrangeActive(SPIDevice bus)
{
    if (!spiOverrangeMode) return false;
    if (bus < 0 || bus >= SPIDEV_COUNT) return false;
    return (spiActiveMask & BIT(bus)) != 0;
}

// Maps knob position (spiVariableRate 0-100) to a fill byte in range 0x40..0x7F.
// The fill byte is written to every byte of the SPI read buffer via memset, so each
// 16-bit axis value becomes (fill<<8)|fill. Lower bound 0x40 ensures the injected
// rate (~1003 dps) is always well above the physical multirotor maximum (~800 dps).
uint8_t fiuGetSpiOverrangeFillByte(void)
{
    return (uint8_t)(0x40 + (spiVariableRate * (0x7F - 0x40) / 100));
}

// --- Battery fault ---

uint8_t fiuGetBatteryFaultLevel(void)
{
    return battFaultLevel;
}

// --- RC Loss fault ---

bool fiuIsRcLossActive(void)
{
    return rcLossFaultActive;
}

// --- State ---

const fiuState_t *fiuGetState(void)
{
    return &fiuState;
}
