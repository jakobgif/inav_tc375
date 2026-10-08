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
 * FIU Fault Detection Module
 *
 * Detects sensor faults injected by the FIU that INAV's built-in health checks
 * cannot see (because FIU operates below the sensor abstraction layer).
 *
 * Called at 100 Hz from taskUpdateAux() -- same task as fiuUpdateFromGlobalVars().
 *
 * Detections grouped by fault source:
 *
 *   I2C / Baro:
 *     - Baro stuck:    baroPressure identical for N readings (I2C full-block)
 *     - Baro anomaly:  |baroPressure delta| > threshold (I2C rate-fault)
 *
 *   SPI / Gyro:
 *     - Gyro stuck:    gyroRaw[] identical for N readings (SPI full-block, error-rate mode)
 *     - Gyro anomaly:  |gyroRaw delta| > threshold (SPI rate-fault, drone rotating)
 *     - Gyro overrange:|gyroRaw| > 900 dps (SPI overrange mode, fill=0x40-0x7F)
 *
 *   Battery:
 *     - Batt warning:  INAV battery state == BATTERY_WARNING
 *     - Batt critical: INAV battery state == BATTERY_CRITICAL
 *
 *   RC Loss:
 *     - RC loss:       rxIsReceivingSignal() == false
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "platform.h"

#include "drivers/time.h"
#include "drivers/pwm_output.h"
#include "rx/rx.h"
#include "sensors/barometer.h"
#include "sensors/battery.h"
#include "sensors/gyro.h"
#include "fc/runtime_config.h"
#include "flight/mixer.h"

#include "fiu/fiu_detection.h"
#include "fiu/fiu_led.h"

static fiuDetectionState_t detState;

// --- I2C / Baro detection state ---
static int32_t  baroLastPressure  = 0;
static uint8_t  baroStuckCount    = 0;
static int32_t  baroAnomalyPrev   = 0;
static uint8_t  baroAnomalyCount  = 0;

// --- SPI / Gyro detection state ---
static float    gyroLastRaw[XYZ_AXIS_COUNT]      = {0.0f, 0.0f, 0.0f};
static uint8_t  gyroStuckCount                   = 0;
static float    gyroAnomalyPrev[XYZ_AXIS_COUNT]  = {0.0f, 0.0f, 0.0f};
static uint8_t  gyroAnomalyCount                 = 0;
static uint8_t  gyroOverrangeCount               = 0;

// --- Motor detection state ---
#ifdef USE_FIU
static uint8_t  motorLossCount = 0;
#endif

// ---------------------------------------------------------------------------
// I2C / Baro — stuck detection
//
// If baroPressure is identical to the previous reading, increment a counter.
// Once it reaches FIU_DETECT_BARO_STUCK_THRESHOLD the sensor is declared stuck.
// Clears as soon as readings differ again (sensor recovered or FIU deactivated).
// ---------------------------------------------------------------------------
static void detectBaroStuck(void)
{
#ifdef USE_BARO
    int32_t currentPressure = baro.baroPressure;

    if (currentPressure == baroLastPressure) {
        if (baroStuckCount < FIU_DETECT_BARO_STUCK_THRESHOLD) {
            baroStuckCount++;
        }
    } else {
        baroStuckCount   = 0;
        baroLastPressure = currentPressure;
    }

    if (baroStuckCount >= FIU_DETECT_BARO_STUCK_THRESHOLD) {
        if (!(detState.faultFlags & FIU_FAULT_BARO_STUCK)) {
            detState.faultFlags      |= FIU_FAULT_BARO_STUCK;
            detState.baroDetectedAtMs = millis();
        }
    } else {
        detState.faultFlags      &= ~FIU_FAULT_BARO_STUCK;
        detState.baroDetectedAtMs = 0;
    }
#endif
}

// ---------------------------------------------------------------------------
// I2C / Baro — anomaly (delta) detection
//
// Only evaluates when baroPressure actually changes (baro at 20 Hz, detection
// at 100 Hz -- 5 of 6 calls see the same value).
//
// With I2C rate-fault: zero bytes -> Praw=0, Traw=0 -> pressure = c00
// (chip-specific calibration offset). Jump from real ~101325 Pa to c00 >> 50 Pa.
// Count resets on any small-delta update -> clears when fault deactivated.
// ---------------------------------------------------------------------------
static void detectBaroAnomaly(void)
{
#ifdef USE_BARO
    int32_t current = baro.baroPressure;

    if (current == baroAnomalyPrev) {
        return;  // no new baro reading yet, nothing to evaluate
    }

    int32_t delta = current - baroAnomalyPrev;
    if (delta < 0) delta = -delta;
    baroAnomalyPrev = current;

    if (delta > FIU_DETECT_BARO_ANOMALY_DELTA_THRESHOLD) {
        if (baroAnomalyCount < FIU_DETECT_BARO_ANOMALY_COUNT) {
            baroAnomalyCount++;
        }
    } else {
        baroAnomalyCount = 0;
    }

    if (baroAnomalyCount >= FIU_DETECT_BARO_ANOMALY_COUNT) {
        if (!(detState.faultFlags & FIU_FAULT_BARO_ANOMALY)) {
            detState.faultFlags              |= FIU_FAULT_BARO_ANOMALY;
            detState.baroAnomalyDetectedAtMs  = millis();
        }
    } else {
        detState.faultFlags              &= ~FIU_FAULT_BARO_ANOMALY;
        detState.baroAnomalyDetectedAtMs  = 0;
    }
#endif
}

// ---------------------------------------------------------------------------
// SPI / Gyro — stuck detection
//
// If all three gyroRaw axes are identical to the previous reading, increment a
// counter. Once it reaches FIU_DETECT_GYRO_STUCK_THRESHOLD the gyro is declared
// stuck. Clears as soon as any axis changes again.
//
// gyro.gyroRaw[] is updated at ~1 kHz in the PID task and is safe to read from
// taskUpdateAux() without a lock (same pattern as blackbox.c).
// ---------------------------------------------------------------------------
static void detectGyroStuck(void)
{
    bool allSame = (gyro.gyroRaw[X] == gyroLastRaw[X]) &&
                   (gyro.gyroRaw[Y] == gyroLastRaw[Y]) &&
                   (gyro.gyroRaw[Z] == gyroLastRaw[Z]);

    if (allSame) {
        if (gyroStuckCount < FIU_DETECT_GYRO_STUCK_THRESHOLD) {
            gyroStuckCount++;
        }
    } else {
        gyroStuckCount = 0;
        gyroLastRaw[X] = gyro.gyroRaw[X];
        gyroLastRaw[Y] = gyro.gyroRaw[Y];
        gyroLastRaw[Z] = gyro.gyroRaw[Z];
    }

    if (gyroStuckCount >= FIU_DETECT_GYRO_STUCK_THRESHOLD) {
        if (!(detState.faultFlags & FIU_FAULT_GYRO_STUCK)) {
            detState.faultFlags      |= FIU_FAULT_GYRO_STUCK;
            detState.gyroDetectedAtMs = millis();
        }
    } else {
        detState.faultFlags      &= ~FIU_FAULT_GYRO_STUCK;
        detState.gyroDetectedAtMs = 0;
    }
}

// ---------------------------------------------------------------------------
// SPI / Gyro — anomaly (delta) detection
//
// Compares each gyroRaw axis against the previous 100 Hz sample. A jump larger
// than FIU_DETECT_GYRO_ANOMALY_DELTA_THRESHOLD dps in 10 ms is physically
// impossible for a multirotor and indicates a fault-induced reading alternating
// between the real rotation rate and the injected ~0 dps.
//
// Requires N consecutive large-delta readings to avoid single-sample noise.
// NOTE: only triggers when the drone is actually rotating at fault time.
// ---------------------------------------------------------------------------
static void detectGyroAnomaly(void)
{
    bool largeDelta = (fabsf(gyro.gyroRaw[X] - gyroAnomalyPrev[X]) > FIU_DETECT_GYRO_ANOMALY_DELTA_THRESHOLD) ||
                      (fabsf(gyro.gyroRaw[Y] - gyroAnomalyPrev[Y]) > FIU_DETECT_GYRO_ANOMALY_DELTA_THRESHOLD) ||
                      (fabsf(gyro.gyroRaw[Z] - gyroAnomalyPrev[Z]) > FIU_DETECT_GYRO_ANOMALY_DELTA_THRESHOLD);

    gyroAnomalyPrev[X] = gyro.gyroRaw[X];
    gyroAnomalyPrev[Y] = gyro.gyroRaw[Y];
    gyroAnomalyPrev[Z] = gyro.gyroRaw[Z];

    if (largeDelta) {
        if (gyroAnomalyCount < FIU_DETECT_GYRO_ANOMALY_COUNT) {
            gyroAnomalyCount++;
        }
    } else {
        gyroAnomalyCount = 0;
    }

    if (gyroAnomalyCount >= FIU_DETECT_GYRO_ANOMALY_COUNT) {
        if (!(detState.faultFlags & FIU_FAULT_GYRO_ANOMALY)) {
            detState.faultFlags              |= FIU_FAULT_GYRO_ANOMALY;
            detState.gyroAnomalyDetectedAtMs  = millis();
        }
    } else {
        detState.faultFlags              &= ~FIU_FAULT_GYRO_ANOMALY;
        detState.gyroAnomalyDetectedAtMs  = 0;
    }
}

// ---------------------------------------------------------------------------
// SPI / Gyro — overrange (absolute threshold) detection
//
// Checks if any gyroRaw axis exceeds FIU_DETECT_GYRO_OVERRANGE_THRESHOLD (900 dps).
// Real acro flight max is ~800 dps, FIU overrange injects min ~1003 dps (fill=0x40),
// so the threshold sits clearly between normal operation and injected fault values.
//
// Requires N consecutive readings (30 ms) to suppress single-sample noise.
// Clears automatically when all axes drop back below the threshold.
// ---------------------------------------------------------------------------
static void detectGyroOverrange(void)
{
    bool overrange = (fabsf(gyro.gyroRaw[X]) > FIU_DETECT_GYRO_OVERRANGE_THRESHOLD) ||
                     (fabsf(gyro.gyroRaw[Y]) > FIU_DETECT_GYRO_OVERRANGE_THRESHOLD) ||
                     (fabsf(gyro.gyroRaw[Z]) > FIU_DETECT_GYRO_OVERRANGE_THRESHOLD);

    if (overrange) {
        if (gyroOverrangeCount < FIU_DETECT_GYRO_OVERRANGE_COUNT) {
            gyroOverrangeCount++;
        }
    } else {
        gyroOverrangeCount = 0;
    }

    if (gyroOverrangeCount >= FIU_DETECT_GYRO_OVERRANGE_COUNT) {
        if (!(detState.faultFlags & FIU_FAULT_GYRO_OVERRANGE)) {
            detState.faultFlags                |= FIU_FAULT_GYRO_OVERRANGE;
            detState.gyroOverrangeDetectedAtMs  = millis();
        }
    } else {
        detState.faultFlags                &= ~FIU_FAULT_GYRO_OVERRANGE;
        detState.gyroOverrangeDetectedAtMs  = 0;
    }
}

// ---------------------------------------------------------------------------
// Battery fault detection
//
// Reads INAV's own battery state machine (getBatteryState()), which already
// processes the FIU-injected voltage through its LPF and hysteresis logic.
// No additional debounce needed -- INAV's state machine is already stable.
//
// BATTERY_WARNING  -> FIU_FAULT_BATT_WARNING
// BATTERY_CRITICAL -> FIU_FAULT_BATT_WARNING | FIU_FAULT_BATT_CRITICAL
// ---------------------------------------------------------------------------
static void detectBatteryFault(void)
{
    batteryState_e state = getBatteryState();

    bool warning  = (state == BATTERY_WARNING || state == BATTERY_CRITICAL);
    bool critical = (state == BATTERY_CRITICAL);

    if (warning) {
        if (!(detState.faultFlags & FIU_FAULT_BATT_WARNING)) {
            detState.faultFlags      |= FIU_FAULT_BATT_WARNING;
            detState.battDetectedAtMs = millis();
        }
    } else {
        detState.faultFlags      &= ~FIU_FAULT_BATT_WARNING;
        detState.battDetectedAtMs = 0;
    }

    if (critical) {
        detState.faultFlags |= FIU_FAULT_BATT_CRITICAL;
    } else {
        detState.faultFlags &= ~FIU_FAULT_BATT_CRITICAL;
    }
}

// ---------------------------------------------------------------------------
// RC Loss detection
//
// Reads INAV's RX subsystem via rxIsReceivingSignal(), which returns false when
// no valid RC frames have been received for the configured signal timeout.
// INAV already validates and debounces the signal internally, so no extra count
// is needed here. Detection fires as soon as INAV considers the link lost.
//
// Works for both real RC loss and FIU-injected loss (rx.c fakes missing frames
// when fiuIsRcLossActive() is true) -- detection cannot distinguish between them.
// ---------------------------------------------------------------------------
static void detectRcLoss(void)
{
    if (!rxIsReceivingSignal()) {
        if (!(detState.faultFlags & FIU_FAULT_RC_LOSS)) {
            detState.faultFlags        |= FIU_FAULT_RC_LOSS;
            detState.rcLossDetectedAtMs = millis();
        }
    } else {
        detState.faultFlags        &= ~FIU_FAULT_RC_LOSS;
        detState.rcLossDetectedAtMs = 0;
    }
}

// ---------------------------------------------------------------------------
// Motor — loss (commanded vs. written divergence) detection
//
// Compares the value pwmWriteMotor() was asked to write (motorCommandedValue)
// with what it actually sent to the driver (motorWrittenValue). When FIU has
// hard-disabled a motor the commanded value is non-zero but motorWrittenValue
// is 0 — a divergence that is impossible in normal operation.
//
// Only evaluates while ARMED: disarmed DSHOT idle writes are legitimately 0,
// so checking without the ARMED guard would fire on every disarmed cycle.
//
// Requires FIU_DETECT_MOTOR_LOSS_COUNT consecutive readings (30 ms) to fire.
// Clears automatically once no motor shows commanded/written divergence.
// ---------------------------------------------------------------------------
#ifdef USE_FIU
static void detectMotorFault(void)
{
    // Motor bits occupy the upper byte of faultFlags, so at most 8 motors are tracked.
    int motorCount = getMotorCount();
    if (motorCount > FIU_FAULT_MOTOR_LOSS_MAX) {
        motorCount = FIU_FAULT_MOTOR_LOSS_MAX;
    }

    if (!ARMING_FLAG(ARMED)) {
        detState.faultFlags    &= ~FIU_FAULT_MOTOR_LOSS_ANY;
        detState.motorLossMask  = 0;
        motorLossCount          = 0;
        for (int i = 0; i < motorCount; i++) {
            detState.motorDetectedAtMs[i] = 0;
        }
        return;
    }

    uint8_t lossMask = 0;
    for (int i = 0; i < motorCount; i++) {
        if (pwmGetMotorCommanded(i) > 0 && pwmGetMotorWritten(i) == 0) {
            lossMask |= (1 << i);
        }
    }
    detState.motorLossMask = lossMask;
    bool anyLoss = (lossMask != 0);

    if (anyLoss) {
        if (motorLossCount < FIU_DETECT_MOTOR_LOSS_COUNT) {
            motorLossCount++;
        }
    } else {
        motorLossCount = 0;
    }

    if (motorLossCount >= FIU_DETECT_MOTOR_LOSS_COUNT) {
        // Rebuild the motor bits from scratch so recovered motors clear themselves.
        detState.faultFlags &= ~FIU_FAULT_MOTOR_LOSS_ANY;
        for (int i = 0; i < motorCount; i++) {
            if (lossMask & (1 << i)) {
                detState.faultFlags |= FIU_FAULT_MOTOR_LOSS(i);
                if (detState.motorDetectedAtMs[i] == 0) {
                    detState.motorDetectedAtMs[i] = millis();  // first time this motor failed
                }
            } else {
                detState.motorDetectedAtMs[i] = 0;  // motor recovered
            }
        }
    } else {
        detState.faultFlags &= ~FIU_FAULT_MOTOR_LOSS_ANY;
        for (int i = 0; i < motorCount; i++) {
            detState.motorDetectedAtMs[i] = 0;
        }
    }
}
#endif // USE_FIU

// ---------------------------------------------------------------------------
// Returns whichever of two "first detected at" timestamps fired earlier.
// A value of 0 means "not active", so a 0 argument is never preferred over
// a nonzero one (used to collapse a category's sub-fault timestamps into a
// single per-category value for Blackbox, e.g. baro stuck + anomaly -> I2C).
// ---------------------------------------------------------------------------
static uint32_t earliestNonZero(uint32_t a, uint32_t b)
{
    if (a == 0) return b;
    if (b == 0) return a;
    return (a < b) ? a : b;
}

// ---------------------------------------------------------------------------
// Main update — called at 100 Hz from taskUpdateAux()
// ---------------------------------------------------------------------------
void fiuDetectionUpdate(void)
{
    // I2C / Baro
    detectBaroStuck();
    detectBaroAnomaly();
    detState.i2cDetectedAtMs = earliestNonZero(detState.baroDetectedAtMs, detState.baroAnomalyDetectedAtMs);

    // SPI / Gyro
    detectGyroStuck();
    detectGyroAnomaly();
    detectGyroOverrange();
    detState.spiDetectedAtMs = earliestNonZero(
        earliestNonZero(detState.gyroDetectedAtMs, detState.gyroAnomalyDetectedAtMs),
        detState.gyroOverrangeDetectedAtMs);

    // Battery
    detectBatteryFault();

    // RC Loss
    detectRcLoss();

#ifdef USE_FIU
    // Motor
    detectMotorFault();
    uint32_t motorMs = 0;
    for (int i = 0; i < MAX_MOTORS; i++) {
        motorMs = earliestNonZero(motorMs, detState.motorDetectedAtMs[i]);
    }
    detState.motorAnyDetectedAtMs = motorMs;

    fiuLedUpdate();
#endif
}

const fiuDetectionState_t *fiuDetectionGetState(void)
{
    return &detState;
}

bool fiuDetectionIsFaultActive(fiuFaultFlags_e flag)
{
    return (detState.faultFlags & flag) != 0;
}
