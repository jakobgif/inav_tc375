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
 * Visualises active FIU injection and detection state on a WS2811/WS2812 strip.
 * Uses DMA via GTM ATOM timer -- requires USE_LED_STRIP and USE_FIU.
 * Enabled when USE_FIU is defined.
 *
 * LED layout (8 LEDs):
 *   0    Y6 arm A (motor 0 upper / motor 3 lower)  GREEN=upper off / BLUE=lower off / RED=both
 *   1    Y6 arm B (motor 1 upper / motor 4 lower)  same
 *   2    Y6 arm C (motor 2 upper / motor 5 lower)  same
 *   3    I2C injection rate    OFF | GREEN(0%) -> YELLOW -> RED(100%)
 *   4    SPI injection:
 *          error-rate mode     OFF | GREEN(0%) -> YELLOW -> RED(100%)
 *          overrange mode      CYAN=X / BLUE=Y / PURPLE=Z / MAGENTA=XYZ, brightness=intensity
 *   5    Batt + RC injection   OFF | YELLOW=warning | RED=critical | PURPLE=RC loss
 *   6    Baro detection        OFF | YELLOW=anomaly | RED=stuck
 *   7    Gyro detection        OFF | YELLOW=anomaly | RED=stuck | PURPLE=overrange
 */

#pragma once

#ifdef USE_FIU

void fiuLedInit(void);
void fiuLedUpdate(void);

#endif // USE_FIU