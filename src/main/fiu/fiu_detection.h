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

// Number of consecutive identical readings required to declare sensor stuck.
// Baro runs at 20 Hz -> 10 readings = 500 ms
#define FIU_DETECT_BARO_STUCK_THRESHOLD  10
// Gyro detection runs at 100 Hz -> 5 readings = 50 ms
#define FIU_DETECT_GYRO_STUCK_THRESHOLD   5
// Delta between consecutive 100 Hz samples (10 ms apart).
// Physical limit of a multirotor: ~50 °/s per 10 ms. 150 °/s signals a fault-induced jump.
// Only fires if drone is actually rotating when fault is active. Tune after flight tests.
#define FIU_DETECT_GYRO_ANOMALY_DELTA_THRESHOLD  150.0f
// Require 3 consecutive large-delta readings (30 ms) to suppress single-sample noise
#define FIU_DETECT_GYRO_ANOMALY_COUNT     3

// Baro anomaly: impossible pressure jump between two consecutive baro updates (20 Hz).
// Normal flight max delta: ~3 Pa per 50 ms (5 m/s climb). 50 Pa has 16x margin.
// Count applies to consecutive baro updates (not detection cycles) -> 3 updates = 150 ms.
#define FIU_DETECT_BARO_ANOMALY_DELTA_THRESHOLD  50
#define FIU_DETECT_BARO_ANOMALY_COUNT             3

// Fault detection flags - one bit per fault type
// Baro: bits 0-1, Gyro: bits 2-3, Battery: bits 4-5, RC: bit 6, Gyro overrange: bit 7
// Motor: bit 8 (requires uint16_t faultFlags — future extension)
typedef enum {
    FIU_FAULT_NONE           = 0,
    FIU_FAULT_BARO_STUCK     = (1 << 0),  // baroPressure identical for N consecutive readings
    FIU_FAULT_BARO_ANOMALY   = (1 << 1),  // |baroPressure delta| > threshold for N consecutive baro updates
    FIU_FAULT_GYRO_STUCK     = (1 << 2),  // gyroRaw[] identical for N consecutive readings
    FIU_FAULT_GYRO_ANOMALY   = (1 << 3),  // |gyroRaw delta| > threshold for N consecutive readings
    FIU_FAULT_BATT_WARNING   = (1 << 4),  // INAV battery state == BATTERY_WARNING
    FIU_FAULT_BATT_CRITICAL  = (1 << 5),  // INAV battery state == BATTERY_CRITICAL
    FIU_FAULT_RC_LOSS        = (1 << 6),  // rxIsReceivingSignal() == false
} fiuFaultFlags_e;

// Snapshot written to Blackbox each frame
typedef struct {
    uint8_t   faultFlags;                // bitmask of fiuFaultFlags_e
    uint32_t  baroDetectedAtMs;          // millis() when baro stuck was first detected (0 = not detected)
    uint32_t  gyroDetectedAtMs;          // millis() when gyro stuck was first detected (0 = not detected)
    uint32_t  gyroAnomalyDetectedAtMs;   // millis() when gyro anomaly was first detected (0 = not detected)
    uint32_t  baroAnomalyDetectedAtMs;   // millis() when baro anomaly was first detected (0 = not detected)
    uint32_t  battDetectedAtMs;          // millis() when battery warning/critical was first detected (0 = not detected)
    uint32_t  rcLossDetectedAtMs;        // millis() when RC loss was first detected (0 = not detected)
} fiuDetectionState_t;

void fiuDetectionUpdate(void);

const fiuDetectionState_t *fiuDetectionGetState(void);
bool fiuDetectionIsFaultActive(fiuFaultFlags_e flag);