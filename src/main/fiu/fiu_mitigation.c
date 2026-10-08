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

/*
 * FIU Mitigation Module
 *
 * Reads fault-detection state from fiu_detection.c (read-only) and reacts
 * with one independent, self-contained mitigation function per peripheral
 * (sensor/actuator) -- NOT a cross-family escalation-stage model. Each
 * peripheral function below decides its own action and owns its own
 * edge-tracking state; there is no shared "current stage" concept spanning
 * multiple fault families any more.
 *
 * Called at 100 Hz from taskUpdateAux() -- same task as fiuDetectionUpdate(),
 * immediately after it, so mitigation always acts on the current cycle's
 * detection result.
 *
 * REWORK NOTE (2026-09-09): This replaces the previous three-stage
 * escalation model (Stage 1/2/3 across all fault families, see
 * guidelines_fiu.md "Fault Mitigation Plan" for the retained historical
 * design/rationale of the underlying API choices). The peripheral-based
 * split was an architecture idea documented but not yet implemented
 * (guidelines_fiu.md Punkt 29) -- this is its code-side realization.
 * NOT HW-VALIDATED: the test drone was damaged before this rework could be
 * flown, so it is verified by code review / desk-check only, analogous to
 * the gyro per-axis detection fix (guidelines_fiu.md Punkt 31).
 *
 *   mitigateBaro() -- Mode Restriction
 *     Trigger: FIU_FAULT_BARO_STUCK or FIU_FAULT_BARO_ANOMALY.
 *     Action:  ENABLE_FLIGHT_MODE(ANGLE_MODE) every cycle while active --
 *              unchanged from the old Stage 1. KNOWN NO-OP ON THIS BOARD
 *              (found 2026-08-03): UASTW_TC375_LK_COMET's only BOXANGLE
 *              range (900-2100) covers the entire possible RC channel
 *              range, so ANGLE_MODE is already permanently hard-on
 *              independent of this call -- see guidelines_fiu.md "Fault
 *              Mitigation Plan" for the full analysis. Kept for
 *              architectural completeness/thesis discussion, not because it
 *              has a measurable effect on this board. Also does NOT disable
 *              nav altitude-hold/RTH/poshold -- ANGLE_MODE and the
 *              NAV_*_MODE bits are independent flightModeFlags_e bits;
 *              deliberately out of scope, see the same guidelines section.
 *
 *   mitigateMotor() -- Forced Landing then Disarm
 *     Trigger: FIU_FAULT_MOTOR_LOSS_ANY (any motor lost), via
 *              fiuDetectionIsFaultActive().
 *     Action:  mitigateWithForcedLanding() (shared helper, see below) --
 *              unchanged behavior from the old Stage 2+3 combination for
 *              this fault set.
 *
 *   mitigateBattery() -- Forced Landing then Disarm
 *     Trigger: FIU_FAULT_BATT_CRITICAL.
 *     Action:  identical pattern to mitigateMotor(), through the same
 *              shared helper (separate, private edge-tracking state).
 *
 *   mitigateGyro() -- split internally by fault character (deliberate,
 *   pragmatic owner decision -- NOT a third cross-family escalation stage):
 *     - Structural (the sensor stays broken): FIU_FAULT_GYRO_STUCK (all 3
 *       axes), FIU_FAULT_GYRO_STUCK_AXIS_ANY (single axis), or
 *       FIU_FAULT_GYRO_OVERRANGE -- immediate disarm(DISARM_FIU_FAULT),
 *       edge-triggered, WITHOUT attempting a forced landing and WITHOUT any
 *       dependency on STATE(LANDING_DETECTED). This is exactly the reaction
 *       path missing during the real LOG00292_2 crash (gyroADC[0] frozen at
 *       0 for the full 4.67 s flight with no detection-driven response at
 *       all) -- see guidelines_fiu.md "HW-Test Erkenntnisse" Punkt 23/25.
 *       Positive side effect of this split: this path has NO race condition
 *       with INAV's native nav_disarm_on_landing, because it never reads
 *       STATE(LANDING_DETECTED) at all.
 *     - Anomaly ALONE (no structural bit active at the same time): transient
 *       and self-clearing (never observed longer than ~8 ms in measurements
 *       so far) -- routed through the same mitigateWithForcedLanding()
 *       helper as Motor/Battery, NOT the immediate-disarm path above.
 *     - If a structural bit AND anomaly are active at the same time, the
 *       structural path wins: immediate disarm fires and the landing helper
 *       is not engaged this cycle -- the sensor is confirmed broken, so a
 *       landing attempt flown on frozen/overrange gyro data is not useful.
 *
 *   mitigateWithForcedLanding() -- shared helper (replaces the old Stage 3):
 *     Calls activateForcedEmergLanding() / abortForcedEmergLanding()
 *     (navigation/navigation.h) edge-triggered on the fault's own
 *     active/inactive transition, then disarms once
 *     (disarm(DISARM_FIU_FAULT), fc/fc_core.h) the first time
 *     STATE(LANDING_DETECTED) becomes true while the fault is still active.
 *     Exactly the previous mitigateStage2() + mitigateStage3() logic,
 *     factored into a reusable building block instead of a global stage
 *     concept -- used by mitigateMotor(), mitigateBattery(), and
 *     mitigateGyro()'s anomaly-only branch, each passing its own private
 *     pair of edge-tracking statics (no state shared between callers).
 *     FIXED (2026-09-09, guidelines_fiu.md Punkt 24): while any of the three
 *     callers has an active forced-landing episode, navConfigMutable()->
 *     general.flags.disarm_on_landing is programmatically forced to 0 (and
 *     the original user-configured value restored once the last episode
 *     ends), so INAV's native nav_disarm_on_landing (navigation.c:3558,
 *     ~1kHz PID loop) can no longer race this helper's own
 *     STATE(LANDING_DETECTED) check (100Hz TASK_AUX) for the same disarm
 *     decision -- see forcedLandingActiveCount/savedDisarmOnLanding below.
 *     Not HW-validated (same reason as the rest of this rework). Does NOT
 *     apply to mitigateGyro()'s structural path above -- that path never
 *     reads STATE(LANDING_DETECTED) at all, so it never had this race to
 *     begin with (a positive side effect of the peripheral split, not this
 *     fix).
 *
 * The verified API choices behind the building blocks used here
 * (activateForcedEmergLanding() over failsafeOnValidDataFailed(),
 * disarm(DISARM_FIU_FAULT) over raw DISABLE_ARMING_FLAG(ARMED), the
 * disarmReasonStr[] osd.c fix, and the reverted opt-out-setting/re-arm-lock
 * exploration) are unchanged by this rework and stay documented in
 * guidelines_fiu.md "Fault Mitigation Plan" -- not repeated here.
 *
 * DISPLAY-LATCH NOTE (2026-09-13): FIU_MITIGATION_ACTION_DISARMED was found
 * to be practically invisible on LED6/7 during hand-testing for any
 * mitigation path whose triggering fault flag can self-clear immediately
 * after disarm() runs (confirmed for Motor: detectMotorFault() zeroes
 * FIU_FAULT_MOTOR_LOSS_ANY the instant !ARMING_FLAG(ARMED), which is already
 * true on the very next 100Hz tick since Detection runs before Mitigation).
 * The affected action level would revert from DISARMED to NONE within a
 * single ~10ms tick, often before it was ever rendered. This is a general
 * property of deriving the action level from live detection state, not a
 * Motor-specific issue -- it applies identically to every disarm() call site
 * in this file (mitigateMotor/mitigateBattery/mitigateGyro-anomaly via
 * mitigateWithForcedLanding(), and mitigateGyro()'s structural branch).
 * Fixed by decoupling the DISARMED *display* from the volatile fault flags:
 * a separate latch (fiuFaultDisarmLatched/fiuFaultDisarmSourceMask) is set
 * at every disarm(DISARM_FIU_FAULT) call site and cleared only once
 * failsafeIsReceivingRxData() is true again AND the RC arm switch itself
 * (IS_RC_MODE_ACTIVE(BOXARM)) is off -- i.e. the display tracks "did an FIU
 * fault disarm the vehicle, and has the pilot acknowledged it," not the
 * instantaneous fault-detection state. The RC-link condition is deliberate:
 * during a genuine RC-loss event INAV disarms/lands on its own regardless of
 * the FIU, so it is correct for the display to stay red until the link is
 * back and the switch is off, with no separate code path for that case.
 */

#include <stdint.h>
#include <stdbool.h>

#include "platform.h"

#include "fiu/fiu_mitigation.h"
#include "fiu/fiu_detection.h"
#include "fc/fc_core.h"
#include "fc/runtime_config.h"
#include "fc/rc_modes.h"
#include "flight/failsafe.h"
#include "navigation/navigation.h"

static fiuMitigationState_t mitigationState;

// Reference count of forced-landing episodes currently in flight across all
// three callers of mitigateWithForcedLanding() (motor/battery/gyro-anomaly).
// Guards navConfig()->general.flags.disarm_on_landing: see savedDisarmOnLanding
// below. Centralized here (not per-caller) because all three callers funnel
// through this one shared helper -- see file header "INHERITED, NOT FIXED"
// note, now fixed (guidelines_fiu.md Punkt 24).
static uint8_t forcedLandingActiveCount = 0;
static uint8_t savedDisarmOnLanding = 0;   // valid only while forcedLandingActiveCount > 0

// Display latch for LED6/7 -- separate from the disarm/landing logic above.
// Keeps the DISARMED action level visible until the pilot puts the RC arm
// switch back to disarm (and, for a genuine RC-loss disarm, until the link
// is restored too). Deliberately NOT cleared when the triggering fault flag
// clears -- that flag flickering off quickly is exactly the bug this fixes.
static bool     fiuFaultDisarmLatched    = false;
static uint32_t fiuFaultDisarmSourceMask = 0;

// Shared building block: "forced landing, then disarm once landed". Used by
// mitigateMotor(), mitigateBattery(), and mitigateGyro()'s anomaly-only
// branch -- see file header for the full behavior description. wasActive/
// wasLanded are private, per-call-site edge-tracking state; each caller
// passes its own pair of static locals, so callers never share state.
//
// Race fix (guidelines_fiu.md Punkt 24, 2026-09-09): while any forced-landing
// episode is active, INAV's own nav_disarm_on_landing (navigation.c:3558,
// read every ~1kHz PID-loop cycle) is programmatically disabled so it cannot
// race mitigateWithForcedLanding()'s own STATE(LANDING_DETECTED) check below
// (this function runs at 100Hz in TASK_AUX). Both paths disarm on the same
// condition, so no safety behavior is lost -- only which of the two
// redundant paths is authoritative during a fault becomes deterministic
// instead of a scheduler-order race. The original user-configured value is
// restored once the last of the three callers' episodes ends (reference
// counted, not a plain bool, because motor/battery/gyro-anomaly can be
// active simultaneously).
static bool mitigateWithForcedLanding(bool faultActive, bool *wasActive, bool *wasLanded, uint32_t familyMask)
{
    if (faultActive && !*wasActive) {
        // Rising edge: fault just appeared -- force emergency landing once.
        if (forcedLandingActiveCount == 0) {
            savedDisarmOnLanding = navConfig()->general.flags.disarm_on_landing;
            navConfigMutable()->general.flags.disarm_on_landing = 0;
        }
        forcedLandingActiveCount++;
        activateForcedEmergLanding();
    } else if (!faultActive && *wasActive) {
        // Falling edge: fault cleared -- release the forced override once,
        // and re-arm the disarm latch for the next fault episode.
        abortForcedEmergLanding();
        *wasLanded = false;
        forcedLandingActiveCount--;
        if (forcedLandingActiveCount == 0) {
            navConfigMutable()->general.flags.disarm_on_landing = savedDisarmOnLanding;
        }
    }
    *wasActive = faultActive;

    if (faultActive && !*wasLanded && STATE(LANDING_DETECTED)) {
        // Must be set before disarm(): disarm() synchronously calls
        // blackboxFinish() (fc_core.c), which closes the log before this
        // function returns to its caller -- currentAction would otherwise
        // still read its pre-disarm value at that point (guidelines_fiu.md
        // Punkt 35).
        const uint8_t previousAction = mitigationState.currentAction;
        mitigationState.currentAction = FIU_MITIGATION_ACTION_DISARMED;
        disarm(DISARM_FIU_FAULT);

        // disarm() has no return value; ARMING_FLAG(ARMED) is the only way to
        // confirm it actually took effect. Defensive only -- disarm() clears
        // ARMED unconditionally once armed, so this should never trigger.
        if (ARMING_FLAG(ARMED)) {
            mitigationState.currentAction = previousAction;
        } else {
            *wasLanded = true;
            fiuFaultDisarmLatched    = true;
            fiuFaultDisarmSourceMask |= familyMask;
        }
    }

    return *wasLanded;
}

static void mitigateBaro(void)
{
    const bool active = fiuDetectionIsFaultActive(FIU_FAULT_BARO_STUCK) ||
                         fiuDetectionIsFaultActive(FIU_FAULT_BARO_ANOMALY);

    if (active) {
        ENABLE_FLIGHT_MODE(ANGLE_MODE); // known no-op on this board -- see file header
    }

    mitigationState.baroAction = active ? FIU_MITIGATION_ACTION_MODE_RESTRICTION
                                         : FIU_MITIGATION_ACTION_NONE;
}

static void mitigateMotor(void)
{
    static bool wasActive = false;
    static bool wasLanded = false;

    const bool active = fiuDetectionIsFaultActive(FIU_FAULT_MOTOR_LOSS_ANY);
    const bool disarmed = mitigateWithForcedLanding(active, &wasActive, &wasLanded, FIU_FAULT_MOTOR_LOSS_ANY);

    mitigationState.motorAction = !active  ? FIU_MITIGATION_ACTION_NONE
                                 : disarmed ? FIU_MITIGATION_ACTION_DISARMED
                                            : FIU_MITIGATION_ACTION_LANDING;
}

static void mitigateBattery(void)
{
    static bool wasActive = false;
    static bool wasLanded = false;

    const bool active = fiuDetectionIsFaultActive(FIU_FAULT_BATT_CRITICAL);
    const bool disarmed = mitigateWithForcedLanding(active, &wasActive, &wasLanded, FIU_FAULT_BATT_CRITICAL);

    mitigationState.batteryAction = !active  ? FIU_MITIGATION_ACTION_NONE
                                   : disarmed ? FIU_MITIGATION_ACTION_DISARMED
                                              : FIU_MITIGATION_ACTION_LANDING;
}

static void mitigateGyro(void)
{
    static bool wasStructuralActive = false;
    static bool anomalyWasActive = false;
    static bool anomalyWasLanded = false;

    const bool structural = fiuDetectionIsFaultActive(FIU_FAULT_GYRO_STUCK) ||
                             fiuDetectionIsFaultActive(FIU_FAULT_GYRO_STUCK_AXIS_ANY) ||
                             fiuDetectionIsFaultActive(FIU_FAULT_GYRO_OVERRANGE);

    // Structural (sensor confirmed broken) always wins over anomaly
    // (transient) -- see file header. Immediate disarm, edge-triggered, no
    // landing attempt, no dependency on STATE(LANDING_DETECTED).
    if (structural && !wasStructuralActive) {
        // See mitigateWithForcedLanding() above -- must be set before
        // disarm() for the same reason (guidelines_fiu.md Punkt 35).
        const uint8_t previousAction = mitigationState.currentAction;
        mitigationState.currentAction = FIU_MITIGATION_ACTION_DISARMED;
        disarm(DISARM_FIU_FAULT);

        // See mitigateWithForcedLanding() above -- defensive verification,
        // disarm() has no return value.
        if (ARMING_FLAG(ARMED)) {
            mitigationState.currentAction = previousAction;
        } else {
            fiuFaultDisarmLatched    = true;
            fiuFaultDisarmSourceMask |= (FIU_FAULT_GYRO_STUCK | FIU_FAULT_GYRO_STUCK_AXIS_ANY | FIU_FAULT_GYRO_OVERRANGE);
        }
    }
    wasStructuralActive = structural;

    // Only engage the landing helper for anomaly when no structural bit is
    // active at the same time (see file header for why).
    const bool anomalyOnly = fiuDetectionIsFaultActive(FIU_FAULT_GYRO_ANOMALY) && !structural;
    const bool anomalyDisarmed = mitigateWithForcedLanding(anomalyOnly, &anomalyWasActive, &anomalyWasLanded, FIU_FAULT_GYRO_ANOMALY);

    if (structural) {
        mitigationState.gyroAction = FIU_MITIGATION_ACTION_DISARMED;
    } else if (anomalyOnly) {
        mitigationState.gyroAction = anomalyDisarmed ? FIU_MITIGATION_ACTION_DISARMED
                                                      : FIU_MITIGATION_ACTION_LANDING;
    } else {
        mitigationState.gyroAction = FIU_MITIGATION_ACTION_NONE;
    }
}

void fiuMitigationUpdate(void)
{
    mitigateBaro();
    mitigateMotor();
    mitigateBattery();
    mitigateGyro();

    // Highest action level wins (RED > YELLOW > GREEN > OFF). The numeric
    // encoding IS the priority order (NONE < MODE_RESTRICTION < LANDING <
    // DISARMED), so a plain max across the four independent peripherals
    // reproduces that priority with no separate lookup table.
    uint8_t action = mitigationState.baroAction;
    if (mitigationState.motorAction   > action) action = mitigationState.motorAction;
    if (mitigationState.batteryAction > action) action = mitigationState.batteryAction;
    if (mitigationState.gyroAction    > action) action = mitigationState.gyroAction;

    // Display latch: keeps DISARMED visible even after the triggering fault
    // flag(s) have already cleared. Clears only once the RC link is back AND
    // the arm switch itself is off -- see file header note below.
    if (fiuFaultDisarmLatched && failsafeIsReceivingRxData() && !IS_RC_MODE_ACTIVE(BOXARM)) {
        fiuFaultDisarmLatched    = false;
        fiuFaultDisarmSourceMask = 0;
    }
    if (fiuFaultDisarmLatched) {
        action = FIU_MITIGATION_ACTION_DISARMED;
    }

    mitigationState.currentAction    = action;
    mitigationState.disarmLatched    = fiuFaultDisarmLatched;
    mitigationState.disarmSourceMask = fiuFaultDisarmSourceMask;
}

const fiuMitigationState_t *fiuMitigationGetState(void)
{
    return &mitigationState;
}
