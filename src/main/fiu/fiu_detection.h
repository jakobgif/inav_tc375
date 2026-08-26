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
#define FIU_FAULT_MOTOR_LOSS_MAX     8       // bits 8..15 — limit of the uint16_t faultFlags
#define FIU_FAULT_MOTOR_LOSS(i)      (1u << (FIU_FAULT_MOTOR_LOSS_SHIFT + (i)))
#define FIU_FAULT_MOTOR_LOSS_ANY     0xFF00u // "any motor lost" — for mitigation checks

// Fault detection flags — one bit per fault type, grouped by fault source.
// faultFlags is uint16_t to accommodate the motor bits in the upper byte.
// I2C/Baro: bits 0-1  |  SPI/Gyro: bits 2-3, 7  |  Battery: bits 4-5
// RC Loss:  bit 6     |  Motor:     bits 8-15 (one per motor, see FIU_FAULT_MOTOR_LOSS)
typedef enum {
    FIU_FAULT_NONE              = 0,

    // I2C / Baro faults
    FIU_FAULT_BARO_STUCK        = (1 << 0),  // baroPressure identical for N consecutive readings
    FIU_FAULT_BARO_ANOMALY      = (1 << 1),  // |baroPressure delta| > threshold for N consecutive baro updates

    // SPI / Gyro faults
    FIU_FAULT_GYRO_STUCK        = (1 << 2),  // gyroRaw[] identical for N consecutive readings
    FIU_FAULT_GYRO_ANOMALY      = (1 << 3),  // |gyroRaw delta| > threshold for N consecutive readings
    FIU_FAULT_GYRO_OVERRANGE    = (1 << 7),  // |gyroRaw| > threshold on any axis for N consecutive readings

    // Battery faults
    FIU_FAULT_BATT_WARNING      = (1 << 4),  // INAV battery state == BATTERY_WARNING
    FIU_FAULT_BATT_CRITICAL     = (1 << 5),  // INAV battery state == BATTERY_CRITICAL

    // RC Loss fault
    FIU_FAULT_RC_LOSS           = (1 << 6),  // rxIsReceivingSignal() == false

    // Motor faults live at bits 8-15 — see FIU_FAULT_MOTOR_LOSS(i) above
} fiuFaultFlags_e;

// Snapshot written to Blackbox each frame — grouped by fault source
typedef struct {
    uint16_t  faultFlags;                    // bitmask of fiuFaultFlags_e (uint16_t for motor bit 8)

    // I2C / Baro
    uint32_t  baroDetectedAtMs;              // millis() when baro stuck was first detected (0 = not detected)
    uint32_t  baroAnomalyDetectedAtMs;       // millis() when baro anomaly was first detected (0 = not detected)
    uint32_t  i2cDetectedAtMs;               // combined: earlier of baroDetectedAtMs/baroAnomalyDetectedAtMs (0 = neither active) -- logged as fiuDetI2cMs, mirrors fiuInjI2c

    // SPI / Gyro
    uint32_t  gyroDetectedAtMs;              // millis() when gyro stuck was first detected (0 = not detected)
    uint32_t  gyroAnomalyDetectedAtMs;       // millis() when gyro anomaly was first detected (0 = not detected)
    uint32_t  gyroOverrangeDetectedAtMs;     // millis() when gyro overrange was first detected (0 = not detected)
    uint32_t  spiDetectedAtMs;               // combined: earliest of the three gyro timestamps above (0 = none active) -- logged as fiuDetSpiMs, mirrors fiuInjSpi

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
