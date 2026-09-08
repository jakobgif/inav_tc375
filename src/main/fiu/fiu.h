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
#include "drivers/bus_spi.h"
#include "drivers/bus_i2c.h"

// GV indices for the 6 physical controller slots (3 RC layers x 2 switches,
// see guidelines_lc.md) + 2 shared knobs. Each slot GV is a plain 0/1
// activation flag; WHICH fault type a slot triggers is a runtime-configurable
// CLI setting (fiu_slot_a..fiu_slot_f, fiu/fiu_config.h), not tied to the GV
// index anymore -- fiuUpdateFromGlobalVars() dispatches on fiuSlotConfig().
#define FIU_GV_SLOT_L1_SW1  0  // GV0: RC layer 1 (CH7 low), switch 1 (CH5)
#define FIU_GV_SLOT_L1_SW2  1  // GV1: RC layer 1 (CH7 low), switch 2 (CH6)
#define FIU_GV_SLOT_L2_SW1  2  // GV2: RC layer 2 (CH7 mid), switch 1 (CH5)
#define FIU_GV_SLOT_L2_SW2  3  // GV3: RC layer 2 (CH7 mid), switch 2 (CH6)
#define FIU_GV_SLOT_L3_SW1  4  // GV4: RC layer 3 (CH7 high), switch 1 (CH5)
#define FIU_GV_SLOT_L3_SW2  5  // GV5: RC layer 3 (CH7 high), switch 2 (CH6)
#define FIU_GV_KNOB_A       6  // GV6: raw CH9 passthrough (1000-2000), meaning depends on active slot's type
#define FIU_GV_KNOB_B       7  // GV7: raw CH10 passthrough (1000-2000), meaning depends on active slot's type

// I2C/SPI error-rate ceilings (formerly FIU_MAX_I2C_ERROR_RATE/FIU_MAX_SPI_ERROR_RATE
// compile-time #defines) are now runtime CLI settings, used only by the
// *_ANOMALY types -- see fiuSlotConfig()->i2cMaxRate/spiMaxRate in
// fiu/fiu_config.h. *_STUCK types are always a fixed 100%, no setting.

// Overrange fill-byte bounds (see fiuGetSpiOverrangeFillByte() in fiu.c).
// 16-bit axis value becomes (fill<<8)|fill; dps = value / 16.4 (ICM-42688 +-2000dps range).
// FIU_SPI_OVERRANGE_FILL_MAX reduced 2026-08-17 (was 0x7F/~1990dps): HW bench test at max
// intensity caused a violently aggressive motor reaction in the ~30ms before Stage 2 could
// engage -- 0x50/~1254dps stays well clear of the 900dps detection threshold while cutting
// the injected rate error roughly in half.
#define FIU_SPI_OVERRANGE_FILL_MIN 0x40  // ~1003 dps
#define FIU_SPI_OVERRANGE_FILL_MAX 0x50  // ~1254 dps

// Per-layer mutual exclusion: within any one of the 3 RC layers, only one of
// its two switches may be active at a time -- both ON simultaneously is a
// safety reset (nothing active in that layer, knobs ignored), enforced
// generically in fiuUpdateFromGlobalVars() for all 3 layers, not just I2C/SPI.

// SPI gyro axis selection (KNOB_A bands, FIU_SLOT_GYRO_OVERRANGE only):
// Knob 1000-1250 (0-24%): X only
// Knob 1250-1500 (25-49%): Y only
// Knob 1500-1750 (50-74%): Z only
// Knob 1750-2000 (75-100%): XYZ (all axes)
#define FIU_SPI_AXIS_X    0x01
#define FIU_SPI_AXIS_Y    0x02
#define FIU_SPI_AXIS_Z    0x04
#define FIU_SPI_AXIS_XYZ  0x07

// FIU state snapshot for blackbox logging — grouped by fault type. Field
// names/shape are unchanged from before the slot redesign so existing
// Blackbox field names (fiuInjMotor/I2c/Spi/...) and analysis scripts
// (messungen/analyze_fiu_log.py, summarize_fiu_tests.py) keep working
// unmodified; whichever slot type is currently active fills in exactly the
// fields it always did, all others stay at their zero/off default.
typedef struct {
    // Motor fault (active when a slot is configured as FIU_SLOT_MOTOR)
    uint8_t motorMask;    // bitmask of disabled motors, from fiuMotorMaskFromConfig()

    // I2C / Baro fault (active when a slot is FIU_SLOT_BARO_STUCK/_ANOMALY)
    uint8_t i2cMask;      // bitmask of affected I2C buses
    uint8_t i2cRate;      // I2C error rate 0-100

    // SPI / Gyro fault (active when a slot is FIU_SLOT_GYRO_STUCK/_ANOMALY/_OVERRANGE)
    uint8_t spiMask;      // bitmask of affected SPI buses
    uint8_t spiRate;      // knob-driven rate/intensity 0-100 (meaning depends on active slot type)
    uint8_t spiOverrange; // 1 = active slot type is FIU_SLOT_GYRO_OVERRANGE
    uint8_t spiAxisMask;  // affected axes in overrange mode (FIU_SPI_AXIS_*)

    // Battery fault (active when a slot is FIU_SLOT_BATT_WARNING/_CRITICAL)
    uint8_t battFault;    // 0=off, 1=warning-level, 2=critical-level

    // RC Loss fault (active when a slot is configured as FIU_SLOT_RC_LOSS)
    uint8_t rcLossFault;  // 1 = RC link loss fault active
} fiuState_t;

void fiuUpdateFromGlobalVars(void);

// Motor fault
bool fiuIsMotorDisabled(uint8_t motorIndex);

// I2C / Baro fault
bool fiuIsI2cBusReadBlocked(I2CDevice bus);

// SPI / Gyro fault
bool fiuIsSpiBusReadBlocked(SPIDevice bus);
bool fiuIsSpiOverrangeActive(SPIDevice bus);
uint8_t fiuGetSpiOverrangeFillByte(void);
uint8_t fiuGetSpiAxisMask(void);

// Battery fault
uint8_t fiuGetBatteryFaultLevel(void);

// RC Loss fault
bool fiuIsRcLossActive(void);

const fiuState_t *fiuGetState(void);