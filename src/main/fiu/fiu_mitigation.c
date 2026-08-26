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
 * Reads fault-detection state from fiu_detection.c (read-only) and triggers
 * a graduated safety response. Performs NO fault detection of its own.
 *
 * Called at 100 Hz from taskUpdateAux() -- same task as fiuDetectionUpdate(),
 * immediately after it, so mitigation always acts on the current cycle's
 * detection result.
 *
 * Stages (increasing severity, independently triggered -- see below):
 *
 *   Stage 1 -- Mode Restriction:
 *     Trigger: FIU_FAULT_BARO_STUCK or FIU_FAULT_BARO_ANOMALY active.
 *     Action:  ENABLE_FLIGHT_MODE(ANGLE_MODE) every cycle while active.
 *     KNOWN LIMITATION (deliberate thesis scope decision): this does NOT
 *     disable INAV's nav altitude-hold / RTH / poshold. ANGLE_MODE and
 *     NAV_ALTHOLD_MODE / NAV_POSHOLD_MODE / NAV_RTH_MODE are independent
 *     bits in flightModeFlags_e; nav throttle control is gated by the nav
 *     FSM state, never by ANGLE_MODE. Additionally processRx() (TASK_RX,
 *     runs more often than this 100 Hz task) unconditionally recomputes
 *     ANGLE_MODE every RX frame from the BOXANGLE switch position, so this
 *     forced flag does not persist beyond the next RX frame unless BOXANGLE
 *     is also selected by the pilot. Net real effect: forces roll/pitch
 *     self-leveling assist if the pilot happens to be in acro/manual at the
 *     moment of the fault; it is NOT a substitute for disabling altitude
 *     hold. The pilot must toggle the altitude-hold switch manually if nav
 *     altitude-hold needs to be exited during a baro fault. Actually
 *     exiting nav altitude-hold would require driving the nav FSM directly
 *     (e.g. navProcessFSMEvents(NAV_FSM_EVENT_SWITCH_TO_IDLE)), which was
 *     evaluated and explicitly deferred for this thesis session to control
 *     scope.
 *
 *   Stage 2 -- Emergency Landing:
 *     Trigger: FIU_FAULT_GYRO_STUCK, FIU_FAULT_GYRO_ANOMALY,
 *              FIU_FAULT_GYRO_OVERRANGE, any FIU_FAULT_MOTOR_LOSS_ANY bit,
 *              or FIU_FAULT_BATT_CRITICAL active.
 *     Action:  activateForcedEmergLanding() / abortForcedEmergLanding()
 *              (navigation/navigation.h) -- the same RC-link-independent API
 *              INAV's own failsafe code uses to enter its landing phase.
 *              NOTE: failsafeOnValidDataFailed() was evaluated first and
 *              rejected -- it only affects failsafeState.rxLinkState, which
 *              is re-stamped from live RX data on every RC frame inside
 *              processRx() (the same function that evaluates it), so it can
 *              never reach FAILSAFE_LANDING for a fault unrelated to RC
 *              link loss. activateForcedEmergLanding() instead sets a
 *              standalone flag (posControl.flags.forcedEmergLandingActivated)
 *              that rx.c never touches. Only takes effect while ARMED.
 *
 *              IMPORTANT: both calls are edge-triggered (only on the
 *              active/inactive transition), NOT called every cycle while
 *              (in)active -- mirroring exactly how failsafe.c itself calls
 *              them (failsafe.c:470/538/552/571). abortForcedEmergLanding()
 *              internally fires NAV_FSM_EVENT_SWITCH_TO_IDLE, which nearly
 *              every nav state (ALTHOLD, POSHOLD, RTH*, WAYPOINT*, CRUISE,
 *              COURSE_HOLD) accepts as a transition straight to NAV_STATE_IDLE.
 *              Calling it unconditionally every cycle while the fault is
 *              NOT active (i.e. almost all the time) would force any
 *              legitimate, unrelated nav mode back to IDLE continuously --
 *              not just during a fault. Edge-triggering avoids that.
 *
 *   Stage 3 -- Immediate Disarm:
 *     Trigger: the same fault set that drives Stage 2 (see above) is active
 *              AND STATE(LANDING_DETECTED) is set (INAV's own multirotor/
 *              fixed-wing landing detector, navigation.c:updateLandingStatus()
 *              / isLandingDetected() -- the same detector INAV's own
 *              disarm_on_landing feature reads, navigation.c:3556-3561).
 *              Ground-detection design decision (2026-08-04): reuse INAV's
 *              existing landing detector rather than a new altitude/throttle
 *              heuristic -- no new detection code, same precedent as
 *              disarm_on_landing. Reusing mitigationState.stage2Active (set
 *              by mitigateStage2() immediately before this runs, same cycle)
 *              as the fault-side condition keeps the two stages' trigger
 *              sets identical by construction -- no separate fault list to
 *              maintain in sync.
 *     Action:  disarm(DISARM_FIU_FAULT) (fc/fc_core.h) -- idempotent,
 *              ARMED-guarded, NOT raw DISABLE_ARMING_FLAG(ARMED) (skips
 *              Blackbox-finish/beeper/stats cleanup).
 *              Edge-triggered like Stage 2, for the same reason: only fire
 *              on the inactive->active transition, not every cycle. Self-
 *              clearing in practice -- once disarm() runs, ARMING_FLAG(ARMED)
 *              drops, and updateLandingStatus() resets STATE(LANDING_DETECTED)
 *              on the very next 100 Hz cycle while disarmed, so `active` goes
 *              false on its own without extra bookkeeping. No opt-out
 *              setting and no re-arm lock (ARMING_DISABLED_*) by deliberate
 *              choice -- kept minimal, matching Stage 1/2 which also have no
 *              CLI opt-out. Considered adding fiu_stage3_disarm (CLI toggle)
 *              + ARMING_DISABLED_FIU_FAULT (re-arm lock, mirroring
 *              ARMING_DISABLED_LANDING_DETECTED) on 2026-08-04, but reverted
 *              at the owner's request to keep the footprint outside fiu/ to
 *              the bare minimum (DISARM_FIU_FAULT enum value + its forced
 *              osd.c consequence only). See guidelines_fiu.md "Fault
 *              Mitigation Plan" for the reverted design and rationale, kept
 *              there as a documented future-work candidate.
 */

#include <stdint.h>
#include <stdbool.h>

#include "platform.h"

#include "fiu/fiu_mitigation.h"
#include "fiu/fiu_detection.h"
#include "fc/fc_core.h"
#include "fc/runtime_config.h"
#include "navigation/navigation.h"

static fiuMitigationState_t mitigationState;

static void mitigateStage1(void)
{
    const bool active = fiuDetectionIsFaultActive(FIU_FAULT_BARO_STUCK) ||
                         fiuDetectionIsFaultActive(FIU_FAULT_BARO_ANOMALY);

    if (active) {
        ENABLE_FLIGHT_MODE(ANGLE_MODE);
    }

    mitigationState.stage1Active = active;
}

static void mitigateStage2(void)
{
    const bool active = fiuDetectionIsFaultActive(FIU_FAULT_GYRO_STUCK) ||
                         fiuDetectionIsFaultActive(FIU_FAULT_GYRO_ANOMALY) ||
                         fiuDetectionIsFaultActive(FIU_FAULT_GYRO_OVERRANGE) ||
                         ((fiuDetectionGetState()->faultFlags & FIU_FAULT_MOTOR_LOSS_ANY) != 0) ||
                         fiuDetectionIsFaultActive(FIU_FAULT_BATT_CRITICAL);

    if (active && !mitigationState.stage2Active) {
        // Rising edge: fault just appeared -- force emergency landing once.
        activateForcedEmergLanding();
    } else if (!active && mitigationState.stage2Active) {
        // Falling edge: fault cleared -- release the forced override once.
        abortForcedEmergLanding();
    }

    mitigationState.stage2Active = active;
}

static void mitigateStage3(void)
{
    // Reuses mitigationState.stage2Active (just computed above, same cycle)
    // as the fault-side condition -- keeps Stage 3's fault set identical to
    // Stage 2's by construction, no separate list to maintain.
    const bool active = mitigationState.stage2Active && STATE(LANDING_DETECTED);

    if (active && !mitigationState.stage3Active) {
        // Rising edge: Stage-2-severity fault while grounded -- disarm once.
        disarm(DISARM_FIU_FAULT);
    }

    mitigationState.stage3Active = active;
}

void fiuMitigationUpdate(void)
{
    mitigateStage1();
    mitigateStage2();
    mitigateStage3();

    if (mitigationState.stage3Active) {
        mitigationState.activeStage = FIU_MITIGATION_STAGE_3;
    } else if (mitigationState.stage2Active) {
        mitigationState.activeStage = FIU_MITIGATION_STAGE_2;
    } else if (mitigationState.stage1Active) {
        mitigationState.activeStage = FIU_MITIGATION_STAGE_1;
    } else {
        mitigationState.activeStage = FIU_MITIGATION_STAGE_NONE;
    }
}

const fiuMitigationState_t *fiuMitigationGetState(void)
{
    return &mitigationState;
}
