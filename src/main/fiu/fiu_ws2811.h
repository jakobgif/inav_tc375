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
 * FIU standalone WS2811/WS2812 driver.
 *
 * Self-contained WS2811 driver for the FIU fault LED strip. Deliberately does NOT
 * use INAV's USE_LED_STRIP subsystem (io/ledstrip.c + drivers/light_ws2811strip.c) --
 * enabling that subsystem wedges the TC375 board before the scheduler/MSP start. This
 * driver only uses the GTM ATOM3 timer + DMA primitives (drivers/timer.c), which are
 * the same ones the DSHOT motors use and are known to be safe on this target.
 *
 * Drives the pin marked TIM_USE_LED in the target timer table (FIU_LED_PIN / P00.12).
 */

#pragma once

#ifdef USE_FIU

#include "common/color.h"

#define FIU_WS2811_LED_COUNT    8

// Initialise the GTM ATOM3 channel + DMA for the fault LED strip.
void fiuWs2811Init(void);

// Set one LED's colour in the internal buffer (index 0..FIU_WS2811_LED_COUNT-1).
void fiuWs2811SetHsv(uint8_t index, const hsvColor_t *color);

// Encode the buffer to WS2811 timing and kick off a (non-blocking) DMA frame.
void fiuWs2811Update(void);

#endif // USE_FIU
