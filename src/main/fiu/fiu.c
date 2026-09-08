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
#include "fiu/fiu_config.h"
#include "programming/global_variables.h"

// Only I2CDEV_1 (barometer) and SPIDEV_3 (gyro) have ever been exercised by
// this FIU -- there is no per-slot "which bus" selector anymore (there used
// to be a GV for it), since every test session used exactly these two.
#define FIU_I2C_BARO_BUS I2CDEV_1
#define FIU_GYRO_SPI_BUS SPIDEV_3

static fiuState_t fiuState = {0};

// --- Motor fault ---
static bool motorDisabled[MAX_MOTORS] = {false};

// --- I2C / Baro fault ---
static uint8_t i2cActiveMask   = 0;
static uint8_t i2cErrorRate    = 0;  // 100 for Stuck (fixed), knobA*fiuSlotConfig()->i2cMaxRate/100 for Anomaly
static uint8_t i2cCallCount[I2CDEV_COUNT] = {0};

// --- SPI / Gyro fault ---
static uint8_t spiActiveMask     = 0;
static uint8_t spiErrorRate      = 0;  // 100 for Stuck (fixed), knobA*fiuSlotConfig()->spiMaxRate/100 for Anomaly
static uint8_t spiRawRate        = 0;  // raw knob value 0-100 (or forced rate for *_STUCK), only for the Blackbox spiRate field
static uint8_t spiOverrangeRate  = 0;  // raw KNOB_B value 0-100, feeds fiuGetSpiOverrangeFillByte()
static uint8_t spiAxisMask       = FIU_SPI_AXIS_XYZ;
static uint8_t spiCallCount[SPIDEV_COUNT] = {0};
static bool    spiOverrangeMode  = false;

// SPI gyro fault — three mutually exclusive activation types (see fiu_config.h):
//
//  [1] FIU_SLOT_GYRO_STUCK: knob ignored, error rate fixed at 100% every
//      cycle -> permanent full block, on/off purely via the slot switch.
//  [2] FIU_SLOT_GYRO_ANOMALY: knob-driven, scaled into the configurable
//      fiuSlotConfig()->spiMaxRate ceiling -> dial anywhere from off up to
//      that ceiling; structurally distinct from Stuck's fixed 100%.
//      Both [1] and [2] use the identical underlying block mechanism
//      (fiuIsSpiBusReadBlocked()) -- only where the rate value comes from
//      differs. Blocked reads -> all gyro axes = 0 dps ("silent fault"),
//      INAV thinks the drone is stationary -> PID reacts incorrectly.
//
//  [3] FIU_SLOT_GYRO_OVERRANGE
//      Blocked reads -> selected gyro axes = ~1003-1254 dps (KNOB_B controls intensity,
//      see FIU_SPI_OVERRANGE_FILL_MIN/MAX in fiu.h)
//      Physically impossible for a multirotor (real max ~500-800 dps) -> clearly detectable
//      memset writes the fill byte to every byte in the buffer — each axis occupies
//      2 bytes (high + low), so both bytes get the same fill value:
//        min: fill=FIU_SPI_OVERRANGE_FILL_MIN -> 16-bit=(fill<<8)|fill -> ~1003 dps
//        max: fill=FIU_SPI_OVERRANGE_FILL_MAX -> 16-bit=(fill<<8)|fill -> ~1254 dps
//      (ICM-42688 +-2000 dps range, sensitivity 16.4 LSB/dps)
//      Axis selection via knobA (0-24%=X, 25-49%=Y, 50-74%=Z, 75-100%=XYZ)
//
//  Note: unselected axes receive real sensor values (real SPI read happens first)

// --- Battery fault ---
static uint8_t battFaultLevel = 0;

// --- RC Loss fault ---
static bool rcLossFaultActive = false;

// ---------------------------------------------------------------------------
// ORs together this slot's 6 independent per-motor toggles (fiu_slotN_m0..
// fiu_slotN_m5 CLI settings) into the actual disable bitmask -- any
// combination is settable directly via CLI, no curated preset list to
// maintain in firmware.
// ---------------------------------------------------------------------------
static uint8_t fiuMotorMaskFromConfig(uint8_t slotIdx)
{
    const fiuSlotConfig_t *cfg = fiuSlotConfig();
    uint8_t mask = 0;
    if (cfg->motorM0[slotIdx]) mask |= BIT(0);
    if (cfg->motorM1[slotIdx]) mask |= BIT(1);
    if (cfg->motorM2[slotIdx]) mask |= BIT(2);
    if (cfg->motorM3[slotIdx]) mask |= BIT(3);
    if (cfg->motorM4[slotIdx]) mask |= BIT(4);
    if (cfg->motorM5[slotIdx]) mask |= BIT(5);
    return mask;
}

// ---------------------------------------------------------------------------
// Per-slot-type activation helpers. Each fills in exactly the fiuState_t
// fields that fault category has always used (see fiu.h) so Blackbox field
// names/shape stay unchanged regardless of which slot the type is loaded
// into. knobA/knobB are this cycle's raw 0-100 knob values, exclusively
// available to the one active slot (only one slot per layer can be active,
// see fiuUpdateFromGlobalVars()). Convention: knobA is a type's primary
// parameter, knobB is only used by types that need a second one.
// ---------------------------------------------------------------------------

static void activateMotor(uint8_t slotIdx)
{
    uint8_t mask = fiuMotorMaskFromConfig(slotIdx);
    for (int i = 0; i < MAX_MOTORS; i++) {
        motorDisabled[i] = (mask & BIT(i)) != 0;
    }
    fiuState.motorMask = mask;
}

// Knob ignored -- switch-only on/off, fixed 100% (permanent full block,
// fiuIsI2cBusReadBlocked()'s `errorRate >= 100` shortcut, no duty-cycle).
static void activateBaroStuck(void)
{
    i2cActiveMask = BIT(FIU_I2C_BARO_BUS);
    i2cErrorRate  = 100;

    fiuState.i2cMask = i2cActiveMask;
    fiuState.i2cRate = i2cErrorRate;
}

// Knob-driven, scaled into the configurable ceiling -- structurally
// distinct from FIU_SLOT_BARO_STUCK, which is a fixed 100% with no ceiling.
static void activateBaroAnomaly(uint8_t knobA)
{
    i2cActiveMask = BIT(FIU_I2C_BARO_BUS);
    i2cErrorRate  = (uint8_t)(knobA * fiuSlotConfig()->i2cMaxRate / 100);

    fiuState.i2cMask = i2cActiveMask;
    fiuState.i2cRate = i2cErrorRate;
}

// Knob ignored -- switch-only on/off, fixed 100% (permanent full block,
// fiuIsSpiBusReadBlocked()'s `errorRate >= 100` shortcut, no duty-cycle).
// fiuState.spiRate reports the fixed rate since there is no live knob value
// to log for this type.
static void activateGyroStuck(void)
{
    spiActiveMask    = BIT(FIU_GYRO_SPI_BUS);
    spiErrorRate     = 100;
    spiRawRate       = spiErrorRate;
    spiOverrangeMode = false;

    fiuState.spiMask      = spiActiveMask;
    fiuState.spiRate      = spiRawRate;
    fiuState.spiOverrange = 0;
}

// Knob-driven, scaled into the configurable ceiling -- structurally
// distinct from FIU_SLOT_GYRO_STUCK, which is a fixed 100% with no ceiling.
static void activateGyroAnomaly(uint8_t knobA)
{
    spiActiveMask    = BIT(FIU_GYRO_SPI_BUS);
    spiErrorRate     = (uint8_t)(knobA * fiuSlotConfig()->spiMaxRate / 100);
    spiRawRate       = knobA;
    spiOverrangeMode = false;

    fiuState.spiMask      = spiActiveMask;
    fiuState.spiRate      = spiRawRate;
    fiuState.spiOverrange = 0;
}

static void activateGyroOverrange(uint8_t knobA, uint8_t knobB)
{
    if (knobA < 25)      spiAxisMask = FIU_SPI_AXIS_X;
    else if (knobA < 50) spiAxisMask = FIU_SPI_AXIS_Y;
    else if (knobA < 75) spiAxisMask = FIU_SPI_AXIS_Z;
    else                 spiAxisMask = FIU_SPI_AXIS_XYZ;

    spiActiveMask    = BIT(FIU_GYRO_SPI_BUS);
    spiOverrangeRate = knobB;
    spiOverrangeMode = true;

    fiuState.spiMask      = spiActiveMask;
    fiuState.spiRate      = spiOverrangeRate;
    fiuState.spiOverrange = 1;
}

static void activateRcLoss(void)
{
    rcLossFaultActive    = true;
    fiuState.rcLossFault = 1;
}

static void activateBattery(uint8_t level)
{
    battFaultLevel     = level;
    fiuState.battFault = level;
}

static void resetAllFaultState(void)
{
    for (int i = 0; i < MAX_MOTORS; i++) {
        motorDisabled[i] = false;
    }
    i2cActiveMask     = 0;
    i2cErrorRate      = 0;
    spiActiveMask     = 0;
    spiErrorRate      = 0;
    spiOverrangeMode  = false;
    rcLossFaultActive = false;
    battFaultLevel    = 0;

    fiuState.motorMask    = 0;
    fiuState.i2cMask      = 0;
    fiuState.i2cRate      = 0;
    fiuState.spiMask      = 0;
    fiuState.spiRate      = 0;
    fiuState.spiOverrange = 0;
    fiuState.battFault    = 0;
    fiuState.rcLossFault  = 0;
    // spiAxisMask / fiuState.spiAxisMask deliberately left untouched here --
    // matches the pre-refactor behaviour of never resetting it to a
    // "neutral" value, only ever overwriting it while an overrange fault is
    // active (see the unconditional copy at the end of
    // fiuUpdateFromGlobalVars()).
}

static void activateSlotType(uint8_t slotIdx, fiuSlotType_e type, uint8_t knobA, uint8_t knobB)
{
    switch (type) {
        case FIU_SLOT_MOTOR:          activateMotor(slotIdx); break;
        case FIU_SLOT_BARO_STUCK:     activateBaroStuck(); break;
        case FIU_SLOT_BARO_ANOMALY:   activateBaroAnomaly(knobA); break;
        case FIU_SLOT_GYRO_STUCK:     activateGyroStuck(); break;
        case FIU_SLOT_GYRO_ANOMALY:   activateGyroAnomaly(knobA); break;
        case FIU_SLOT_GYRO_OVERRANGE: activateGyroOverrange(knobA, knobB); break;
        case FIU_SLOT_RC_LOSS:        activateRcLoss(); break;
        case FIU_SLOT_BATT_WARNING:   activateBattery(1); break;
        case FIU_SLOT_BATT_CRITICAL:  activateBattery(2); break;
        case FIU_SLOT_NONE:
        default:                      break;
    }
}

// ---------------------------------------------------------------------------

void fiuUpdateFromGlobalVars(void)
{
    // 6 physical controller slots: 3 RC layers (CH7-selected) x 2 switches
    // (CH5/CH6) each. Each slot GV is a plain 0/1 activation flag -- WHICH
    // fault type a slot triggers is looked up from fiuSlotConfig() (CLI
    // setting fiu_slot_a..fiu_slot_f), not tied to the GV index.
    bool sw[3][2] = {
        { gvGet(FIU_GV_SLOT_L1_SW1) != 0, gvGet(FIU_GV_SLOT_L1_SW2) != 0 },
        { gvGet(FIU_GV_SLOT_L2_SW1) != 0, gvGet(FIU_GV_SLOT_L2_SW2) != 0 },
        { gvGet(FIU_GV_SLOT_L3_SW1) != 0, gvGet(FIU_GV_SLOT_L3_SW2) != 0 },
    };

    int32_t knobARaw     = gvGet(FIU_GV_KNOB_A);
    int32_t knobAClamped = knobARaw < 1000 ? 1000 : knobARaw > 2000 ? 2000 : knobARaw;
    uint8_t knobA        = (uint8_t)((knobAClamped - 1000) / 10);  // 0-100

    int32_t knobBRaw     = gvGet(FIU_GV_KNOB_B);
    int32_t knobBClamped = knobBRaw < 1000 ? 1000 : knobBRaw > 2000 ? 2000 : knobBRaw;
    uint8_t knobB        = (uint8_t)((knobBClamped - 1000) / 10);  // 0-100

    resetAllFaultState();

    // Per-layer mutual exclusion: both switches ON (or both OFF) in the same
    // layer -> nothing active in that layer (safety reset). Generalizes the
    // old I2C+SPI-specific exclusion check to all 3 layers.
    for (int layer = 0; layer < 3; layer++) {
        if (sw[layer][0] == sw[layer][1]) continue;
        int slotIdx = layer * 2 + (sw[layer][0] ? 0 : 1);
        activateSlotType(slotIdx, (fiuSlotType_e)fiuSlotConfig()->slot[slotIdx], knobA, knobB);
    }

    // Update blackbox state snapshot -- spiAxisMask is intentionally always
    // mirrored here regardless of which (if any) slot is active, matching
    // pre-refactor behaviour.
    fiuState.spiAxisMask = spiAxisMask;
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

// Maps the overrange intensity knob (spiOverrangeRate, 0-100) to a fill byte in range
// FIU_SPI_OVERRANGE_FILL_MIN..FIU_SPI_OVERRANGE_FILL_MAX.
// The fill byte is written to every byte of the SPI read buffer via memset, so each
// 16-bit axis value becomes (fill<<8)|fill. Lower bound ensures the injected rate
// (~1003 dps) is always well above the physical multirotor maximum (~800 dps); upper
// bound capped well below the sensor's real range so even full intensity stays
// controllable on the bench (see FIU_SPI_OVERRANGE_FILL_MAX comment in fiu.h).
uint8_t fiuGetSpiOverrangeFillByte(void)
{
    return (uint8_t)(FIU_SPI_OVERRANGE_FILL_MIN +
        (spiOverrangeRate * (FIU_SPI_OVERRANGE_FILL_MAX - FIU_SPI_OVERRANGE_FILL_MIN) / 100));
}

uint8_t fiuGetSpiAxisMask(void)
{
    return spiAxisMask;
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
