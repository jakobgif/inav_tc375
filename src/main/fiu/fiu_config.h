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

#include "config/parameter_group.h"

// Which of the 6 physical controller slots (3 RC layers x 2 switches) drives
// which fault. Runtime-configurable via CLI (fiu_slot_a..fiu_slot_f) instead of
// hardcoded per-GV in fiu.h/fiu.c -- lets the same 6 switches be reassigned to
// any of the fault types below without a firmware rebuild.
// BARO_STUCK/BARO_ANOMALY (and GYRO_STUCK/GYRO_ANOMALY) share the same
// underlying I2C/SPI error-rate mechanism (fiu.c) -- what actually
// distinguishes them is how the rate is chosen: *_STUCK ignores the knob and
// always injects at a fixed 100% (permanent full block -- see
// fiuIsI2cBusReadBlocked()/fiuIsSpiBusReadBlocked() in fiu.c, `errorRate >=
// 100` skips the duty-cycle entirely), on/off purely via the slot switch;
// *_ANOMALY is knob-driven, scaled into the configurable ceiling below
// (i2cMaxRate/spiMaxRate), so it can be dialled from "off" up to that
// ceiling. There is deliberately no equivalent ceiling for *_STUCK -- it is
// not a tunable rate, just an on/off fault via the switch.
typedef enum {
    FIU_SLOT_NONE = 0,
    FIU_SLOT_MOTOR,
    FIU_SLOT_BARO_STUCK,
    FIU_SLOT_BARO_ANOMALY,
    FIU_SLOT_GYRO_STUCK,
    FIU_SLOT_GYRO_ANOMALY,
    FIU_SLOT_GYRO_OVERRANGE,
    FIU_SLOT_RC_LOSS,
    FIU_SLOT_BATT_WARNING,
    FIU_SLOT_BATT_CRITICAL,
} fiuSlotType_e;

// index = layer * 2 + switchIndex (layer 0-2, switchIndex 0-1) -- matches the
// physical CH7-selected-layer / CH5+CH6-switch layout described in
// guidelines_lc.md.
//
// motorM0..motorM5 cover motors 0-5 (the 6 motors this Y6 mixer actually has,
// target/UASTW_TC375_LK_COMET/config.c primaryMotorMixerMutable(0..5)), not
// the generic driver-level MAX_MOTORS ceiling.
typedef struct {
    uint8_t slot[6];  // fiuSlotType_e per slot

    // Per-slot, per-motor ON/OFF toggles for slots configured as
    // FIU_SLOT_MOTOR -- ORed together at runtime (fiuMotorMaskFromConfig() in
    // fiu.c) into the actual disable bitmask. Independent booleans instead of
    // a curated preset table so any motor combination is settable directly
    // via CLI (`fiu_slot_X_ma`..`fiu_slot_X_mf`) without needing a new named
    // preset added to the firmware first. Ignored when slot[i] != FIU_SLOT_MOTOR.
    // uint8_t (not bool) to match the settings.yaml `type: bool` convention
    // used elsewhere in this codebase (e.g. stats_enabled, fc/stats.h).
    uint8_t motorM0[6];
    uint8_t motorM1[6];
    uint8_t motorM2[6];
    uint8_t motorM3[6];
    uint8_t motorM4[6];
    uint8_t motorM5[6];

    // Rate ceiling (0-100 %) that a *_ANOMALY slot's Knob A is linearly
    // scaled into (0-100% knob -> 0-this% error rate). Only read by
    // FIU_SLOT_BARO_ANOMALY/FIU_SLOT_GYRO_ANOMALY -- FIU_SLOT_BARO_STUCK/
    // FIU_SLOT_GYRO_STUCK are always a fixed 100%, no ceiling to configure.
    uint8_t i2cMaxRate;
    uint8_t spiMaxRate;
} fiuSlotConfig_t;

PG_DECLARE(fiuSlotConfig_t, fiuSlotConfig);
