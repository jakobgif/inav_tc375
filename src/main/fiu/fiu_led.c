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
 * FIU LED Strip Module -- WS2811/WS2812 via GTM ATOM DMA.
 *
 * LED layout (8 LEDs) -- see fiu_led.h for full design rationale:
 *   0    Y6 arm A (motor 0 upper / motor 3 lower)
 *   1    Y6 arm B (motor 1 upper / motor 4 lower)
 *   2    Y6 arm C (motor 2 upper / motor 5 lower)
 *        GREEN = upper motor disabled, BLUE = lower, RED = both
 *   3    I2C injection (Layer 2) OR RC loss (Layer 3) -- layers never overlap (LC0-6 reset
 *        phase, see guidelines_lc.md), so no priority logic needed
 *          I2C       OFF | GREEN(0%) -> YELLOW -> RED(100%)
 *          RC loss   MAGENTA (binary)
 *   4    SPI injection (Layer 2) OR battery (Layer 3) -- same layer-exclusivity argument
 *          SPI error-rate mode  GREEN(0%) -> YELLOW -> RED(100%)
 *          SPI overrange mode   Hue = axis (CYAN=X, BLUE=Y, PURPLE=Z, MAGENTA=XYZ)
 *                               Brightness = intensity (spiRate 0-100)
 *          Battery              OFF | YELLOW=warning | RED=critical
 *   5    Detection catch-all (global, not layer-gated -- a real fault must never be hidden
 *        just because a different CH7 layer is under test)
 *          OFF / GREEN=motor / YELLOW=baro / CYAN=gyro / BLUE=battery / MAGENTA=RC loss
 *          PURPLE = 2+ fault families active simultaneously (see Blackbox fiuDetFlags)
 *   6    Mitigation stage      OFF / GREEN=stage1 / YELLOW=stage2 / RED=stage3
 *   7    Mitigation source family -- same colour code as LED 5, restricted to the fault(s)
 *        feeding the currently active stage
 */

#include "platform.h"

#ifdef USE_FIU

#include <stdint.h>
#include <stdbool.h>

#include "fiu/fiu.h"
#include "fiu/fiu_led.h"
#include "fiu/fiu_detection.h"
#include "fiu/fiu_mitigation.h"
#include "common/color.h"
#include "fiu/fiu_ws2811.h"

#define FIU_LED_COUNT   8

// Y6 coaxial pairs: motors 0-2 = upper, motors 3-5 = lower
static const uint8_t MOTOR_UPPER[3] = {0, 1, 2};
static const uint8_t MOTOR_LOWER[3] = {3, 4, 5};

// Overall brightness 0-255. Adjust this one value to change all LEDs at once.
#define FIU_LED_BRIGHTNESS  10

// Perceptual correction per 60-degree hue sector (percent of FIU_LED_BRIGHTNESS).
// Green is perceived ~2x brighter than red; blue ~4x darker.
// Tune if your strip looks unbalanced.
static const uint8_t LUMINANCE_CORRECTION[6] = {
    100,  // 0°   red
     30,  // 60°  yellow
     35,  // 120° green
     32,  // 180° cyan
    180,  // 240° blue / purple
     80,  // 300° magenta
};

// Returns an HSV colour at the given hue with perceptually corrected brightness.
// Adding a new colour: just call makeColor(hue, FIU_LED_BRIGHTNESS).
static hsvColor_t makeColor(uint16_t hue, uint8_t baseV)
{
    uint8_t  sector = (uint8_t)((hue % 360U) / 60U);
    uint32_t v      = ((uint32_t)baseV * LUMINANCE_CORRECTION[sector]) / 100U;
    hsvColor_t c    = {hue, 0, (uint8_t)(v > 255U ? 255U : v)};
    return c;
}

static hsvColor_t COLOR_OFF;
static hsvColor_t COLOR_RED;
static hsvColor_t COLOR_YELLOW;
static hsvColor_t COLOR_GREEN;
static hsvColor_t COLOR_CYAN;
static hsvColor_t COLOR_BLUE;
static hsvColor_t COLOR_PURPLE;
static hsvColor_t COLOR_MAGENTA;

static bool ledHwInitDone = false;

// Rate 0-100 -> hue 120 (green) through 60 (yellow) to 0 (red)
static hsvColor_t rateToColor(uint8_t rate)
{
    uint16_t hue = (uint16_t)(120U - ((uint16_t)rate * 120U) / 100U);
    return makeColor(hue, FIU_LED_BRIGHTNESS);
}

// SPI overrange intensity gradient (rate 0-100 -> OVERRANGE_V_MIN..OVERRANGE_V_MAX), applied
// directly in POST-correction units instead of feeding a linear 0..FIU_LED_BRIGHTNESS baseV
// into makeColor(). A pre-correction floor doesn't work for this: low-correction sectors
// (CYAN=32%, the default axis X) squeeze the whole 0..FIU_LED_BRIGHTNESS input range into
// just 0-3 post-correction, so any "always >= 3" floor wipes out the entire gradient instead
// of just the bottom end. Inverting the correction (with a ceiling division to counter
// truncation) makes every axis hue reach the same target range.
#define OVERRANGE_V_MIN  3
#define OVERRANGE_V_MAX  FIU_LED_BRIGHTNESS

static hsvColor_t makeOverrangeColor(uint16_t hue, uint8_t rate)
{
    uint8_t sector = (uint8_t)((hue % 360U) / 60U);
    uint16_t targetV = (uint16_t)(OVERRANGE_V_MIN +
        ((uint16_t)(OVERRANGE_V_MAX - OVERRANGE_V_MIN) * rate) / 100U);
    uint32_t baseV = ((uint32_t)targetV * 100U + LUMINANCE_CORRECTION[sector] - 1U)
        / LUMINANCE_CORRECTION[sector];
    if (baseV > 255U) baseV = 255U;
    return makeColor(hue, (uint8_t)baseV);
}

// Fault-family colour code, shared by LED 5 (detection catch-all) and LED 7
// (mitigation source): one hue per family, OFF if none active, PURPLE if 2+
// families are active at once (an exact combination doesn't fit in one LED --
// see Blackbox fiuDetFlags for the precise bitmask).
#define FAMILY_MASK_MOTOR  (FIU_FAULT_MOTOR_LOSS_ANY)
#define FAMILY_MASK_BARO   (FIU_FAULT_BARO_STUCK | FIU_FAULT_BARO_ANOMALY)
#define FAMILY_MASK_GYRO   (FIU_FAULT_GYRO_STUCK | FIU_FAULT_GYRO_ANOMALY | FIU_FAULT_GYRO_OVERRANGE)
#define FAMILY_MASK_BATT   (FIU_FAULT_BATT_WARNING | FIU_FAULT_BATT_CRITICAL)
#define FAMILY_MASK_RC     (FIU_FAULT_RC_LOSS)

static hsvColor_t familyColor(uint16_t flags)
{
    uint8_t count = 0;
    hsvColor_t color = COLOR_OFF;

    if (flags & FAMILY_MASK_MOTOR) { count++; color = COLOR_GREEN; }
    if (flags & FAMILY_MASK_BARO)  { count++; color = COLOR_YELLOW; }
    if (flags & FAMILY_MASK_GYRO)  { count++; color = COLOR_CYAN; }
    if (flags & FAMILY_MASK_BATT)  { count++; color = COLOR_BLUE; }
    if (flags & FAMILY_MASK_RC)    { count++; color = COLOR_MAGENTA; }

    if (count == 0) return COLOR_OFF;
    if (count > 1)  return COLOR_PURPLE;
    return color;
}

// ---------------------------------------------------------------------------
// fiuLedInit -- called once from fc_init.c
// ---------------------------------------------------------------------------
void fiuLedInit(void)
{
    COLOR_OFF     = (hsvColor_t){0,   0, 0};
    COLOR_RED     = makeColor(  0, FIU_LED_BRIGHTNESS);
    COLOR_YELLOW  = makeColor( 60, FIU_LED_BRIGHTNESS);
    COLOR_GREEN   = makeColor(120, FIU_LED_BRIGHTNESS);
    COLOR_CYAN    = makeColor(180, FIU_LED_BRIGHTNESS);
    COLOR_BLUE    = makeColor(240, FIU_LED_BRIGHTNESS);
    COLOR_PURPLE  = makeColor(270, FIU_LED_BRIGHTNESS / 2);
    COLOR_MAGENTA = makeColor(300, FIU_LED_BRIGHTNESS);
    // fiuWs2811Init() deferred to first fiuLedUpdate() — runs after scheduler start
}

// ---------------------------------------------------------------------------
// fiuLedUpdate -- maps active FIU faults to LED colours.
// Called at 100 Hz from taskUpdateAux() (fc_tasks.c), after fiuDetectionUpdate()
// AND fiuMitigationUpdate() so LEDs 5-7 read this cycle's state, not last cycle's.
// fiuWs2811Update() skips silently if DMA still busy -- no race condition.
// ---------------------------------------------------------------------------
void fiuLedUpdate(void)
{
    if (!ledHwInitDone) {
        fiuWs2811Init();
        ledHwInitDone = true;
    }

    const fiuState_t *state = fiuGetState();

    // LEDs 0-2: one per Y6 arm
    //   GREEN = upper motor disabled, BLUE = lower motor disabled, RED = both
    for (int arm = 0; arm < 3; arm++) {
        bool upper = (state->motorMask & (1U << MOTOR_UPPER[arm])) != 0;
        bool lower = (state->motorMask & (1U << MOTOR_LOWER[arm])) != 0;

        const hsvColor_t *color;
        if (upper && lower) {
            color = &COLOR_RED;
        } else if (upper) {
            color = &COLOR_GREEN;
        } else if (lower) {
            color = &COLOR_BLUE;
        } else {
            color = &COLOR_OFF;
        }
        fiuWs2811SetHsv(arm, color);
    }

    // LED 3: I2C (Layer 2) or RC loss (Layer 3) -- never both non-zero at once, the LC0-6
    // reset phase zeroes every layer's GVs except the one the active CH7 position re-sets
    // (see guidelines_lc.md), so no priority/collision logic is needed here.
    if (state->i2cMask != 0) {
        hsvColor_t c = rateToColor(state->i2cRate);
        fiuWs2811SetHsv(3, &c);
    } else if (state->rcLossFault) {
        fiuWs2811SetHsv(3, &COLOR_MAGENTA);
    } else {
        fiuWs2811SetHsv(3, &COLOR_OFF);
    }

    // LED 4: SPI (Layer 2) or battery (Layer 3) -- same layer-exclusivity argument as LED 3.
    // Grouping SPI with battery (rather than with I2C) keeps MAGENTA meaning only one thing
    // per LED: on LED 3 it's always "RC loss", on LED 4 it's always "SPI overrange XYZ" --
    // battery never uses MAGENTA, so the two never collide in meaning on the same LED.
    //   SPI error-rate mode: GREEN->YELLOW->RED gradient
    //   SPI overrange mode:  hue = axis (CYAN=X, BLUE=Y, PURPLE=Z, MAGENTA=XYZ)
    //                        brightness = intensity (spiRate 0-100)
    if (state->spiMask != 0) {
        if (state->spiOverrange) {
            uint16_t hue;
            switch (state->spiAxisMask) {
                case FIU_SPI_AXIS_X:  hue = 180; break;  // CYAN
                case FIU_SPI_AXIS_Y:  hue = 240; break;  // BLUE
                case FIU_SPI_AXIS_Z:  hue = 270; break;  // PURPLE
                default:              hue = 300; break;  // MAGENTA (XYZ)
            }
            hsvColor_t c = makeOverrangeColor(hue, state->spiRate);
            fiuWs2811SetHsv(4, &c);
        } else {
            hsvColor_t c = rateToColor(state->spiRate);
            fiuWs2811SetHsv(4, &c);
        }
    } else if (state->battFault == 2) {
        fiuWs2811SetHsv(4, &COLOR_RED);
    } else if (state->battFault == 1) {
        fiuWs2811SetHsv(4, &COLOR_YELLOW);
    } else {
        fiuWs2811SetHsv(4, &COLOR_OFF);
    }

    const fiuDetectionState_t *detState = fiuDetectionGetState();

    // LED 5: Detection catch-all -- global, NOT layer-gated. fiuDetectionUpdate() runs
    // unconditionally regardless of which CH7 layer is under test, so a real fault must
    // always surface here even if it's outside the family currently being injected.
    hsvColor_t detColor = familyColor(detState->faultFlags);
    fiuWs2811SetHsv(5, &detColor);

    // LED 6: Mitigation stage -- GREEN=1 (mode restriction), YELLOW=2 (emergency landing),
    // RED=3 (immediate disarm).
    const fiuMitigationState_t *mit = fiuMitigationGetState();
    hsvColor_t stageColor;
    switch (mit->activeStage) {
        case FIU_MITIGATION_STAGE_1: stageColor = COLOR_GREEN;  break;
        case FIU_MITIGATION_STAGE_2: stageColor = COLOR_YELLOW; break;
        case FIU_MITIGATION_STAGE_3: stageColor = COLOR_RED;    break;
        default:                     stageColor = COLOR_OFF;    break;
    }
    fiuWs2811SetHsv(6, &stageColor);

    // LED 7: Mitigation source family -- same colour code as LED 5, restricted to the
    // fault set that actually feeds the currently active stage (stage1 = baro only,
    // stage2/3 = gyro/motor/batt-critical -- see fiu_mitigation.c).
    hsvColor_t mitColor;
    if (mit->activeStage == FIU_MITIGATION_STAGE_NONE) {
        mitColor = COLOR_OFF;
    } else if (mit->activeStage == FIU_MITIGATION_STAGE_1) {
        mitColor = familyColor(detState->faultFlags & FAMILY_MASK_BARO);
    } else {
        mitColor = familyColor(detState->faultFlags &
            (FAMILY_MASK_GYRO | FAMILY_MASK_MOTOR | FIU_FAULT_BATT_CRITICAL));
    }
    fiuWs2811SetHsv(7, &mitColor);

    fiuWs2811Update();
}

#endif // USE_FIU
