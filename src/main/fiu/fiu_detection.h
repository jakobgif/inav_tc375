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
#include "common/time.h"
#include "common/axis.h"
#include "drivers/pwm_mapping.h"

// --- I2C / Baro detection thresholds ---

// Baro stuck: baroPressure AND baroTemperature identical for N consecutive
// *samples* (gated on baro.baroSampleSeq, not on the 100 Hz detection call
// rate -- see detectBaroStuck() in fiu_detection.c). Deliberately makes no
// assumption about *what* the frozen value looks like (a jump-from-baseline
// heuristic was tried and dropped -- it only covers this FIU's specific
// injection signature, not a real-world stuck sensor freezing quietly at its
// last valid reading) -- N is the only lever against false positives from a
// motionless sensor coincidentally repeating a plausible value. Raised from
// 10 to 20 on 2026-08-10 after HW testing still showed occasional false
// positives at 10 (500 ms); 20 @ 20 Hz = 1 s. Revisit again after the next
// HW test if still needed.
#define FIU_DETECT_BARO_STUCK_THRESHOLD          20

// Baro anomaly: impossible pressure jump between two consecutive baro updates (20 Hz).
// Normal flight max delta: ~3 Pa per 50 ms (5 m/s climb). 50 Pa has 16x margin.
// Count applies to consecutive baro updates (not detection cycles) -> 3 updates = 150 ms.
#define FIU_DETECT_BARO_ANOMALY_DELTA_THRESHOLD  50
#define FIU_DETECT_BARO_ANOMALY_COUNT             3

// --- SPI / Gyro detection thresholds ---

// Gyro stuck: all three gyroRaw axes identical for N consecutive readings.
// Gyro detection runs at 100 Hz -> 5 readings = 50 ms
#define FIU_DETECT_GYRO_STUCK_THRESHOLD           5

// Gyro stuck (single axis): the check above requires ALL THREE axes frozen
// simultaneously (the full SPI-block injection signature) and clears the instant
// any one axis moves -- a real single-axis fault (only one axis frozen, the
// others move normally) never satisfies that condition and was structurally
// undetectable (2026-08-22 incident, LOG00292_2: gyroADC[0] frozen at 0 for the
// full 4.67s flight while Y/Z moved normally, no FIU injection active -> crash,
// see guidelines_fiu.md "HW-Test Erkenntnisse" Punkt 23). This threshold drives
// an independent, per-axis check (detectGyroStuckAxis() in fiu_detection.c).
// Deliberately much higher than the 3-axis threshold above (50 vs. 5): a single
// axis legitimately reads the same raw value for a while during genuine
// near-zero rotation (e.g. yaw in a stable hover), far more likely than all
// three axes doing so at once -- same false-positive-vs-latency trade-off
// already used for FIU_DETECT_BARO_STUCK_THRESHOLD. Not HW-validated (hardware
// testing ended with the incident above).
#define FIU_DETECT_GYRO_STUCK_AXIS_THRESHOLD     50   // 50 @ 100Hz = 500ms

// Gyro anomaly: impossible rate jump between consecutive 100 Hz samples (10 ms apart).
// Physical limit of a multirotor: ~50 dps per 10 ms. 150 dps signals a fault-induced jump.
// Only fires if drone is actually rotating when fault is active. Tune after flight tests.
#define FIU_DETECT_GYRO_ANOMALY_DELTA_THRESHOLD  150.0f
#define FIU_DETECT_GYRO_ANOMALY_COUNT              3

// Flight tests (2026-08-16, LOG00217/LOG00224) showed this delta threshold gets crossed
// by real ground-bounce/vibration after landing, not just by the injected fault. Since the
// flag clears the instant a single sample drops back under threshold, it could immediately
// re-arm on the very next 30 ms burst -- causing a rapid activate/abort storm in
// mitigateStage2() (repeated forced-emergency-landing engagement) that itself keeps the
// airframe from settling, so it also blocked Stage 3's landing-stillness window. This
// cooldown blocks re-arming for a fixed dead time after the flag actually clears; it does
// not affect first-time detection latency (still ~30 ms) or any fault that stays
// continuously active (delta never drops below threshold, so the flag never clears and the
// cooldown path is never entered).
#define FIU_DETECT_GYRO_ANOMALY_COOLDOWN_MS       800

// Gyro overrange: absolute axis value exceeds physical multirotor maximum.
// Real acro flight max: ~800 dps. FIU overrange injects min ~1003 dps (fill=0x40).
// Threshold 900 dps sits between both -> no false positives in normal flight.
// Require 3 consecutive readings (30 ms) to suppress single-sample noise.
#define FIU_DETECT_GYRO_OVERRANGE_THRESHOLD      900.0f
#define FIU_DETECT_GYRO_OVERRANGE_COUNT            3

// --- Motor detection threshold ---

// Motor loss: pwmWriteMotor() commanded a non-zero value but motorWritePtr received 0.
// Requires ARMED to exclude legitimate disarmed-idle zero writes (DSHOT idle = 0).
// 3 consecutive readings at 100 Hz = 30 ms debounce.
#define FIU_DETECT_MOTOR_LOSS_COUNT  3

// Motor loss occupies the upper byte of faultFlags: bit (8 + i) = motor i lost.
// Encoding which motor failed into faultFlags keeps the fault visible in the
// existing fiuDetFlags Blackbox field — no extra log field is needed.
#define FIU_FAULT_MOTOR_LOSS_SHIFT   8
#define FIU_FAULT_MOTOR_LOSS_MAX     8       // bits 8..15
#define FIU_FAULT_MOTOR_LOSS(i)      (1u << (FIU_FAULT_MOTOR_LOSS_SHIFT + (i)))
#define FIU_FAULT_MOTOR_LOSS_ANY     0xFF00u // "any motor lost" — for mitigation checks

// Gyro per-axis stuck: bit (16 + axis) = that axis individually stuck, axis is
// X=0/Y=1/Z=2 (see common/axis.h). Independent of FIU_FAULT_GYRO_STUCK (bit 2,
// unchanged, still requires all three axes frozen at once) — see
// detectGyroStuckAxis() in fiu_detection.c and FIU_DETECT_GYRO_STUCK_AXIS_THRESHOLD
// above for why this needed its own bits rather than reusing bit 2.
#define FIU_FAULT_GYRO_STUCK_AXIS_SHIFT   16
#define FIU_FAULT_GYRO_STUCK_AXIS(axis)   (1u << (FIU_FAULT_GYRO_STUCK_AXIS_SHIFT + (axis)))
#define FIU_FAULT_GYRO_STUCK_AXIS_ANY     (7u << FIU_FAULT_GYRO_STUCK_AXIS_SHIFT) // any single axis stuck — for mitigation checks

// Fault detection flags — one bit per fault type, grouped by fault source.
// faultFlags is uint32_t: bits 0-15 were already fully allocated (8 motor bits
// in the upper byte left no room for the 3 new per-axis gyro bits below), same
// widening pattern as the earlier uint8_t->uint16_t motor-bit fix.
// I2C/Baro: bits 0-1  |  SPI/Gyro: bits 2-3, 7, 16-18  |  Battery: bits 4-5
// RC Loss:  bit 6     |  Motor: bits 8-15 (one per motor, see FIU_FAULT_MOTOR_LOSS)
typedef enum {
    FIU_FAULT_NONE              = 0,

    // I2C / Baro faults
    FIU_FAULT_BARO_STUCK        = (1 << 0),  // baroPressure identical for N consecutive readings
    FIU_FAULT_BARO_ANOMALY      = (1 << 1),  // |baroPressure delta| > threshold for N consecutive baro updates

    // SPI / Gyro faults
    FIU_FAULT_GYRO_STUCK        = (1 << 2),  // gyroRaw[] identical for N consecutive readings (all 3 axes at once)
    FIU_FAULT_GYRO_ANOMALY      = (1 << 3),  // |gyroRaw delta| > threshold for N consecutive readings
    FIU_FAULT_GYRO_OVERRANGE    = (1 << 7),  // |gyroRaw| > threshold on any axis for N consecutive readings

    // Gyro per-axis stuck faults (new) -- one axis frozen while the other two move
    // normally; independent of FIU_FAULT_GYRO_STUCK above, see detectGyroStuckAxis()
    // in fiu_detection.c. Values equal FIU_FAULT_GYRO_STUCK_AXIS(0/1/2) -- named here
    // so they show up directly in this enum instead of only existing as generated bits.
    FIU_FAULT_GYRO_STUCK_X      = FIU_FAULT_GYRO_STUCK_AXIS(0),  // (1 << 16)
    FIU_FAULT_GYRO_STUCK_Y      = FIU_FAULT_GYRO_STUCK_AXIS(1),  // (1 << 17)
    FIU_FAULT_GYRO_STUCK_Z      = FIU_FAULT_GYRO_STUCK_AXIS(2),  // (1 << 18)

    // Battery faults
    FIU_FAULT_BATT_WARNING      = (1 << 4),  // INAV battery state == BATTERY_WARNING
    FIU_FAULT_BATT_CRITICAL     = (1 << 5),  // INAV battery state == BATTERY_CRITICAL

    // RC Loss fault
    FIU_FAULT_RC_LOSS           = (1 << 6),  // rxIsReceivingSignal() == false

    // Motor faults live at bits 8-15 — see FIU_FAULT_MOTOR_LOSS(i) above
} fiuFaultFlags_e;

// Snapshot written to Blackbox each frame — grouped by fault source
typedef struct {
    uint32_t  faultFlags;                    // bitmask of fiuFaultFlags_e (uint32_t: bits 0-15 were full, per-axis gyro bits need 16-18)

    // I2C / Baro
    uint32_t  baroDetectedAtMs;              // millis() when baro stuck was first detected (0 = not detected)
    uint32_t  baroAnomalyDetectedAtMs;       // millis() when baro anomaly was first detected (0 = not detected)
    uint32_t  i2cDetectedAtMs;               // combined: earlier of baroDetectedAtMs/baroAnomalyDetectedAtMs (0 = neither active) -- logged as fiuDetI2cMs, mirrors fiuInjI2c

    // SPI / Gyro
    uint32_t  gyroDetectedAtMs;              // millis() when gyro stuck (all 3 axes) was first detected (0 = not detected)
    uint32_t  gyroStuckAxisDetectedAtMs[XYZ_AXIS_COUNT]; // millis() when each axis was first detected individually stuck (0 = not detected) -- independent of gyroDetectedAtMs, see FIU_FAULT_GYRO_STUCK_AXIS(axis)
    uint32_t  gyroAnomalyDetectedAtMs;       // millis() when gyro anomaly was first detected (0 = not detected)
    uint32_t  gyroOverrangeDetectedAtMs;     // millis() when gyro overrange was first detected (0 = not detected)
    uint32_t  spiDetectedAtMs;               // combined: earliest of the gyro timestamps above, incl. per-axis (0 = none active) -- logged as fiuDetSpiMs, mirrors fiuInjSpi

    // Battery
    uint32_t  battDetectedAtMs;              // millis() when battery warning/critical was first detected (0 = not detected)

    // RC Loss
    uint32_t  rcLossDetectedAtMs;            // millis() when RC loss was first detected (0 = not detected)

    // Motor
    uint32_t  motorDetectedAtMs[MAX_MOTORS]; // millis() when each motor was first detected lost (0 = not detected)
    uint32_t  motorAnyDetectedAtMs;          // combined: earliest motorDetectedAtMs[] entry (0 = no motor lost) -- logged as fiuDetMotorMs, mirrors fiuInjMotor
    uint8_t   motorLossMask;                            // bitmask of motors currently lost: bit i = motor i
} fiuDetectionState_t;

void fiuDetectionUpdate(void);

const fiuDetectionState_t *fiuDetectionGetState(void);
bool fiuDetectionIsFaultActive(fiuFaultFlags_e flag);
