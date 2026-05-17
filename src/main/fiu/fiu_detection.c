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
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "platform.h"

#include "drivers/time.h"
#include "sensors/barometer.h"

#include "fiu/fiu_detection.h"
#include "fiu/fiu_led.h"

static fiuDetectionState_t detState;

// --- Baro stuck detection state ---
static int32_t  baroLastPressure   = 0;
static uint8_t  baroStuckCount     = 0;

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
// Main update - called at 100 Hz from taskUpdateAux()
// ---------------------------------------------------------------------------
void fiuDetectionUpdate(void)
{
    detectBaroStuck();

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