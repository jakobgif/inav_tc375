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

// Mitigation action-type encoding. Each of the four independent per-peripheral
// mitigation functions in fiu_mitigation.c (mitigateBaro/mitigateMotor/
// mitigateBattery/mitigateGyro) computes its own action level below;
// fiuMitigationUpdate() takes the highest one active this cycle as the
// single summary value logged to Blackbox (field `fiuMitStage` -- name,
// type and 0-3 numbering unchanged from the old three-stage escalation
// model on purpose, so historical Blackbox logs stay numerically
// interpretable -- only the *meaning* changed, from "escalation stage" to
// "action type currently applied by whichever peripheral needs it". See
// fiu_mitigation.c file header for the full peripheral-based design.
#define FIU_MITIGATION_ACTION_NONE              0
#define FIU_MITIGATION_ACTION_MODE_RESTRICTION  1   // GREEN  -- ANGLE_MODE forced (baro only)
#define FIU_MITIGATION_ACTION_LANDING           2   // YELLOW -- forced emergency landing running, not yet disarmed
#define FIU_MITIGATION_ACTION_DISARMED          3   // RED    -- disarmed (landing completed, or gyro immediate disarm)

// Snapshot written to Blackbox each frame -- one independent action-level
// field per peripheral, plus the overall summary (`currentAction`, logged
// as fiuMitStage). There is no shared "stage" between peripherals any more:
// each field below is driven exclusively by its own mitigateX() function.
typedef struct {
    uint8_t currentAction;  // highest of the four fields below this cycle -- logged as fiuMitStage
    uint8_t baroAction;     // FIU_MITIGATION_ACTION_NONE or _MODE_RESTRICTION -- see mitigateBaro()
    uint8_t motorAction;    // FIU_MITIGATION_ACTION_NONE / _LANDING / _DISARMED -- see mitigateMotor()
    uint8_t batteryAction;  // FIU_MITIGATION_ACTION_NONE / _LANDING / _DISARMED -- see mitigateBattery()
    uint8_t gyroAction;     // FIU_MITIGATION_ACTION_NONE / _LANDING / _DISARMED -- see mitigateGyro()
    bool     disarmLatched;     // true while the FIU-disarm display latch is active
    uint32_t disarmSourceMask;  // fiu_detection.h fault bit(s) of the family/families that triggered it
} fiuMitigationState_t;

void fiuMitigationUpdate(void);

const fiuMitigationState_t *fiuMitigationGetState(void);
