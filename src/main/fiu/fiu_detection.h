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

// Fault detection flags - one bit per fault type
typedef enum {
    FIU_FAULT_NONE        = 0,
    FIU_FAULT_BARO_STUCK  = (1 << 0),  // baroPressure identical for N consecutive readings
} fiuFaultFlags_e;

// Snapshot written to Blackbox each frame
typedef struct {
    uint8_t   faultFlags;           // bitmask of fiuFaultFlags_e
    uint32_t  baroDetectedAtMs;     // millis() when baro fault was first detected (0 = not detected)
} fiuDetectionState_t;

void fiuDetectionUpdate(void);

const fiuDetectionState_t *fiuDetectionGetState(void);
bool fiuDetectionIsFaultActive(fiuFaultFlags_e flag);