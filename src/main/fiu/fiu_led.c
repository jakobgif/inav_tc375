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
 * LED layout:
 *   0-5  Motor 0-5  RED    when motor disabled by FIU
 *   6    I2C fault  ORANGE when any I2C bus is blocked
 *   7    SPI fault  YELLOW when any SPI bus is blocked
 *   8    Baro stuck PURPLE when barometer stuck detected
 */

#include "platform.h"

#ifdef USE_FIU

#include <stdint.h>
#include <stdbool.h>

#include "fiu/fiu_led.h"
#include "common/color.h"
#include "drivers/light_ws2811strip.h"

#define FIU_LED_COUNT   9

static const hsvColor_t COLOR_OFF = {  0,   0,   0};
static const hsvColor_t COLOR_RED = {  0,   0, 255};

// ---------------------------------------------------------------------------
// fiuLedInit -- called once from fc_init.c
// ---------------------------------------------------------------------------
void fiuLedInit(void)
{
    ws2811LedStripInit();
}

// ---------------------------------------------------------------------------
// fiuLedUpdate -- blink test: all 9 LEDs RED at ~0.5 Hz (100 Hz call rate)
// ws2811UpdateStrip() skips silently if DMA still busy -- no race condition.
// ---------------------------------------------------------------------------
void fiuLedUpdate(void)
{
    static uint8_t counter = 0;

    const hsvColor_t *color = (counter < 50) ? &COLOR_RED : &COLOR_OFF;

    for (int i = 0; i < FIU_LED_COUNT; i++) {
        setLedHsv(i, color);
    }

    ws2811UpdateStrip();

    if (++counter >= 100) {
        counter = 0;
    }
}

#endif // USE_FIU