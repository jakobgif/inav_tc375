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
 * Current detections:
 *   - Baro stuck: baroPressure identical for FIU_DETECT_BARO_STUCK_THRESHOLD
 *                 consecutive readings (triggered by I2C full-block fault).
 *   - Gyro stuck: all three gyroRaw axes identical for FIU_DETECT_GYRO_STUCK_THRESHOLD
 *                 consecutive readings (triggered by SPI full-block fault).
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "platform.h"

#include "drivers/time.h"
#include "rx/rx.h"
#include "sensors/barometer.h"
#include "sensors/battery.h"
#include "sensors/gyro.h"

#include "fiu/fiu_detection.h"
#include "fiu/fiu_led.h"

static fiuDetectionState_t detState;

// --- Baro stuck detection state ---
static int32_t  baroLastPressure   = 0;
static uint8_t  baroStuckCount     = 0;

// --- Gyro stuck detection state ---
static float    gyroLastRaw[XYZ_AXIS_COUNT] = {0.0f, 0.0f, 0.0f};
static uint8_t  gyroStuckCount             = 0;

// --- Gyro anomaly (delta) detection state ---
static float    gyroAnomalyPrev[XYZ_AXIS_COUNT] = {0.0f, 0.0f, 0.0f};
static uint8_t  gyroAnomalyCount               = 0;

// --- Baro anomaly (delta) detection state ---
static int32_t  baroAnomalyPrev  = 0;
static uint8_t  baroAnomalyCount = 0;

// ---------------------------------------------------------------------------
// Baro stuck detection
//
// Logic: if baroPressure is identical to the previous reading, increment a
// counter. Once the counter reaches FIU_DETECT_BARO_STUCK_THRESHOLD the
// sensor is declared stuck and FIU_FAULT_BARO_STUCK is set.
// The flag and timestamp are cleared as soon as the readings differ again
// (sensor recovered or FIU deactivated).
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
            // First cycle we cross the threshold: record detection timestamp
            detState.faultFlags      |= FIU_FAULT_BARO_STUCK;
            detState.baroDetectedAtMs = millis();
        }
    } else {
        // Readings are changing again -> clear fault
        detState.faultFlags      &= ~FIU_FAULT_BARO_STUCK;
        detState.baroDetectedAtMs = 0;
    }
#endif
}

// ---------------------------------------------------------------------------
// Gyro stuck detection
//
// Logic: if all three gyroRaw axes are identical to the previous reading,
// increment a counter. Once it reaches FIU_DETECT_GYRO_STUCK_THRESHOLD the
// gyro is declared stuck and FIU_FAULT_GYRO_STUCK is set.
// The flag and timestamp clear as soon as any axis changes again.
//
// gyro.gyroRaw[] is updated at ~1 kHz via gyroGetUpdatedData() in the PID
// task and is safe to read from taskUpdateAux() without a lock (same pattern
// as blackbox.c and baro detection above).
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
        gyroStuckCount   = 0;
        gyroLastRaw[X]   = gyro.gyroRaw[X];
        gyroLastRaw[Y]   = gyro.gyroRaw[Y];
        gyroLastRaw[Z]   = gyro.gyroRaw[Z];
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
// Gyro anomaly (delta) detection
//
// Logic: compare each gyroRaw axis against the previous 100 Hz sample.
// A jump larger than FIU_DETECT_GYRO_ANOMALY_DELTA_THRESHOLD °/s in 10 ms
// is physically impossible for a multirotor and indicates a fault-induced
// reading alternating between the real rotation rate and the injected ~0 °/s.
//
// Requires N consecutive large-delta readings to avoid single-sample noise.
// NOTE: only triggers when the drone is actually rotating at fault time.
//       Detection of the "extreme values" variant (memset 0x7F injection)
//       is a separate future task -- see project notes.
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
            detState.faultFlags            |= FIU_FAULT_GYRO_ANOMALY;
            detState.gyroAnomalyDetectedAtMs = millis();
        }
    } else {
        detState.faultFlags            &= ~FIU_FAULT_GYRO_ANOMALY;
        detState.gyroAnomalyDetectedAtMs = 0;
    }
}

// ---------------------------------------------------------------------------
// Baro anomaly (delta) detection
//
// Logic: only evaluate when baroPressure actually changes (baro runs at 20 Hz,
// detection at 100 Hz -- 5 of 6 calls see the same value). When a new reading
// arrives, check the delta against the previous reading.
//
// With I2C rate-fault: zero bytes -> Praw=0, Traw=0 -> pressure = c00
// (chip-specific calibration offset, NOT a real pressure value). The jump
// between real ~101325 Pa and c00 is expected to be >> 50 Pa.
//
// Count resets on any legitimate small-delta baro update, so the flag clears
// automatically once the fault is deactivated.
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
            detState.faultFlags           |= FIU_FAULT_BARO_ANOMALY;
            detState.baroAnomalyDetectedAtMs = millis();
        }
    } else {
        detState.faultFlags           &= ~FIU_FAULT_BARO_ANOMALY;
        detState.baroAnomalyDetectedAtMs = 0;
    }
#endif
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
// Main update - called at 100 Hz from taskUpdateAux()
// ---------------------------------------------------------------------------
void fiuDetectionUpdate(void)
{
    detectBaroStuck();
    detectBaroAnomaly();
    detectGyroStuck();
    detectGyroAnomaly();
    detectBatteryFault();
    detectRcLoss();

#ifdef USE_FIU
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