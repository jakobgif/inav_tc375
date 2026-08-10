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
 * FIU LED Strip Module
 *
 * Visualises active FIU injection, detection AND mitigation state on a WS2811/WS2812 strip.
 * Uses DMA via GTM ATOM timer -- requires USE_LED_STRIP and USE_FIU.
 * Enabled when USE_FIU is defined.
 *
 * Design rationale (2026-08-08): Injection is already physically layer-exclusive -- the
 * RC Logic-Condition reset phase (LC0-6) zeroes all GVs every cycle, only the active CH7
 * layout layer re-sets its own GVs, and I2C/SPI additionally have their own mutual-exclusion
 * safety reset (fiu.c). So LEDs 0-4 can safely show "whatever is currently active" without
 * any layer-awareness in this file. Detection, however, runs unconditionally regardless of
 * CH7 position, so LED 5 must stay a global catch-all -- a real fault outside the layer
 * currently under test must never be hidden.
 *
 * LEDs 3/4 additionally reuse the same layer-exclusivity across Layer 2 (Bus) and Layer 3
 * (RC+Batt): I2C pairs with RC loss on LED 3, SPI pairs with battery on LED 4. This specific
 * pairing (rather than I2C+SPI / Batt+RC) keeps MAGENTA meaning exactly one thing per LED
 * position -- LED 3's MAGENTA is always "RC loss", LED 4's MAGENTA is always "SPI overrange
 * XYZ" -- battery never uses MAGENTA, so the two meanings never collide on the same LED.
 *
 * LED layout (8 LEDs):
 *   0    Y6 arm A (motor 0 upper / motor 3 lower)  GREEN=upper off / BLUE=lower off / RED=both
 *   1    Y6 arm B (motor 1 upper / motor 4 lower)  same
 *   2    Y6 arm C (motor 2 upper / motor 5 lower)  same
 *   3    I2C injection (Layer 2) OR RC loss (Layer 3):
 *          I2C       OFF | GREEN(0%) -> YELLOW -> RED(100%)
 *          RC loss   MAGENTA (binary)
 *   4    SPI injection (Layer 2) OR battery (Layer 3):
 *          SPI error-rate  OFF | GREEN(0%) -> YELLOW -> RED(100%)
 *          SPI overrange   CYAN=X / BLUE=Y / PURPLE=Z / MAGENTA=XYZ, brightness=intensity
 *          Battery         OFF | YELLOW=warning | RED=critical
 *   5    Detection catch-all (any active fault family, global, not layer-gated):
 *          OFF | GREEN=motor | YELLOW=baro | CYAN=gyro | BLUE=battery | MAGENTA=RC loss
 *          PURPLE = 2+ families active simultaneously (see Blackbox fiuDetFlags for detail)
 *   6    Mitigation stage        OFF | GREEN=stage1 | YELLOW=stage2 | RED=stage3
 *   7    Mitigation source family -- same colour code as LED 5, restricted to the fault(s)
 *        driving the currently active stage
 */

#pragma once

#ifdef USE_FIU

void fiuLedInit(void);
void fiuLedUpdate(void);

#endif // USE_FIU