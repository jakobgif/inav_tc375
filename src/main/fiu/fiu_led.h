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
 * Visualises active FIU faults on an addressable LED strip (WS2811/WS2812).
 * Uses software bit-bang via GPIO -- no DMA or timer required.
 * Enabled when USE_FIU is defined.
 *
 * LED layout (indices):
 *   0-5 : Motor 0-5  -- RED when motor is disabled via FIU
 *   6   : I2C fault  -- ORANGE when any I2C bus is blocked
 *   7   : SPI fault  -- YELLOW when any SPI bus is blocked
 *   8   : Baro stuck -- PURPLE when barometer stuck fault detected
 */

#pragma once

#ifdef USE_FIU

void fiuLedInit(void);
void fiuLedUpdate(void);

#endif // USE_FIU