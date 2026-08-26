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

// Mitigation stage encoding — increasing severity. A given cycle reports the
// HIGHEST stage whose trigger condition is active; the stages are not
// mutually exclusive internally (e.g. a baro fault and a gyro fault at the
// same time keep both Stage 1's and Stage 2's actions running), this value
// is only the single most-severe-stage summary logged to Blackbox.
#define FIU_MITIGATION_STAGE_NONE   0
#define FIU_MITIGATION_STAGE_1      1   // Mode restriction (forced ANGLE_MODE)
#define FIU_MITIGATION_STAGE_2      2   // Emergency landing (forced nav emergency landing)
#define FIU_MITIGATION_STAGE_3      3   // Immediate disarm (Stage-2 fault set active while STATE(LANDING_DETECTED))

// Snapshot written to Blackbox each frame — mirrors fiuDetectionState_t style.
typedef struct {
    uint8_t activeStage;   // highest stage active this cycle: FIU_MITIGATION_STAGE_*
    bool    stage1Active;  // true while BARO_STUCK or BARO_ANOMALY forces ANGLE_MODE
    bool    stage2Active;  // true while GYRO_STUCK/ANOMALY/OVERRANGE, motor loss, or BATT_CRITICAL forces emergency landing
    bool    stage3Active;  // true while stage2Active AND STATE(LANDING_DETECTED) forces immediate disarm
} fiuMitigationState_t;

void fiuMitigationUpdate(void);

const fiuMitigationState_t *fiuMitigationGetState(void);
