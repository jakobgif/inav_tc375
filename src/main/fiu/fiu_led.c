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
 * LED layout (8 LEDs):
 *   0    Y6 arm A (motor 0 upper / motor 3 lower)
 *   1    Y6 arm B (motor 1 upper / motor 4 lower)
 *   2    Y6 arm C (motor 2 upper / motor 5 lower)
 *        GREEN = upper motor disabled, BLUE = lower, RED = both
 *   3    I2C injection rate    GREEN(0%) -> YELLOW -> RED(100%), OFF if mask=0
 *   4    SPI injection:
 *          Error-rate mode     GREEN(0%) -> YELLOW -> RED(100%)
 *          Overrange mode      Hue = axis (CYAN=X, BLUE=Y, PURPLE=Z, MAGENTA=XYZ)
 *                              Brightness = intensity (spiRate 0-100)
 *   5    Batt + RC injection   OFF / YELLOW=warning / RED=critical / PURPLE=RC loss
 *   6    Baro detection        OFF / YELLOW=anomaly / RED=stuck
 *   7    Gyro detection        OFF / YELLOW=anomaly / RED=stuck / PURPLE=overrange
 */

#include "platform.h"

#ifdef USE_FIU

#include <stdint.h>
#include <stdbool.h>

#include "fiu/fiu.h"
#include "fiu/fiu_led.h"
#include "fiu/fiu_detection.h"
#include "common/color.h"
#include "drivers/light_ws2811strip.h"

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
static hsvColor_t COLOR_BLUE;
static hsvColor_t COLOR_PURPLE;

// Rate 0-100 -> hue 120 (green) through 60 (yellow) to 0 (red)
static hsvColor_t rateToColor(uint8_t rate)
{
    uint16_t hue = (uint16_t)(120U - ((uint16_t)rate * 120U) / 100U);
    return makeColor(hue, FIU_LED_BRIGHTNESS);
}

// ---------------------------------------------------------------------------
// fiuLedInit -- called once from fc_init.c
// ---------------------------------------------------------------------------
void fiuLedInit(void)
{
    COLOR_OFF    = (hsvColor_t){0,   0, 0};
    COLOR_RED    = makeColor(  0, FIU_LED_BRIGHTNESS);
    COLOR_YELLOW = makeColor( 60, FIU_LED_BRIGHTNESS);
    COLOR_GREEN  = makeColor(120, FIU_LED_BRIGHTNESS);
    COLOR_BLUE   = makeColor(240, FIU_LED_BRIGHTNESS);
    COLOR_PURPLE = makeColor(270, FIU_LED_BRIGHTNESS / 2);

    ws2811LedStripInit();
}

// ---------------------------------------------------------------------------
// fiuLedUpdate -- maps active FIU faults to LED colours.
// Called at 100 Hz from fiuDetectionUpdate() after detection state is updated.
// ws2811UpdateStrip() skips silently if DMA still busy -- no race condition.
// ---------------------------------------------------------------------------
void fiuLedUpdate(void)
{
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
        setLedHsv(arm, color);
    }

    // LED 3: I2C injection rate -- OFF if no bus selected, else GREEN->YELLOW->RED
    if (state->i2cMask == 0) {
        setLedHsv(3, &COLOR_OFF);
    } else {
        hsvColor_t c = rateToColor(state->i2cRate);
        setLedHsv(3, &c);
    }

    // LED 4: SPI injection
    //   Error-rate mode: GREEN->YELLOW->RED gradient (spiRate = error rate 0-100)
    //   Overrange mode:  hue = axis (CYAN=X, BLUE=Y, PURPLE=Z, MAGENTA=XYZ)
    //                    brightness = intensity (spiRate 0-100)
    if (state->spiMask == 0) {
        setLedHsv(4, &COLOR_OFF);
    } else if (state->spiOverrange) {
        uint16_t hue;
        switch (state->spiAxisMask) {
            case FIU_SPI_AXIS_X:  hue = 180; break;  // CYAN
            case FIU_SPI_AXIS_Y:  hue = 240; break;  // BLUE
            case FIU_SPI_AXIS_Z:  hue = 270; break;  // PURPLE
            default:              hue = 300; break;  // MAGENTA (XYZ)
        }
        uint8_t v = (uint8_t)((uint32_t)FIU_LED_BRIGHTNESS * state->spiRate / 100);
        if (v < 3) v = 3;  // minimum visible when overrange active at low intensity
        hsvColor_t c = makeColor(hue, v);
        setLedHsv(4, &c);
    } else {
        hsvColor_t c = rateToColor(state->spiRate);
        setLedHsv(4, &c);
    }

    // LED 5: Battery + RC injection
    //   Priority: RC loss > batt critical > batt warning
    if (state->rcLossFault) {
        setLedHsv(5, &COLOR_PURPLE);
    } else if (state->battFault == 2) {
        setLedHsv(5, &COLOR_RED);
    } else if (state->battFault == 1) {
        setLedHsv(5, &COLOR_YELLOW);
    } else {
        setLedHsv(5, &COLOR_OFF);
    }

    // LED 6: Baro detection -- priority: stuck > anomaly
    if (fiuDetectionIsFaultActive(FIU_FAULT_BARO_STUCK)) {
        setLedHsv(6, &COLOR_RED);
    } else if (fiuDetectionIsFaultActive(FIU_FAULT_BARO_ANOMALY)) {
        setLedHsv(6, &COLOR_YELLOW);
    } else {
        setLedHsv(6, &COLOR_OFF);
    }

    // LED 7: Gyro detection -- priority: overrange > stuck > anomaly
    if (fiuDetectionIsFaultActive(FIU_FAULT_GYRO_OVERRANGE)) {
        setLedHsv(7, &COLOR_PURPLE);
    } else if (fiuDetectionIsFaultActive(FIU_FAULT_GYRO_STUCK)) {
        setLedHsv(7, &COLOR_RED);
    } else if (fiuDetectionIsFaultActive(FIU_FAULT_GYRO_ANOMALY)) {
        setLedHsv(7, &COLOR_YELLOW);
    } else {
        setLedHsv(7, &COLOR_OFF);
    }

    ws2811UpdateStrip();
}

#endif // USE_FIU
