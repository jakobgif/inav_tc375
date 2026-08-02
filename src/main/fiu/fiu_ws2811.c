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
 * FIU standalone WS2811 driver -- see fiu_ws2811.h for the why.
 *
 * Uses only the generic GTM/DMA timer primitives (timerConfigBase,
 * timerPWMConfigChannelDMA, timerPWMPrepareDMA/StartDMA, timerPWMDMAInProgress) that
 * are already proven on this target by the DSHOT motor outputs. No dependency on
 * USE_LED_STRIP.
 */

#include "platform.h"

#ifdef USE_FIU

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "build/build_config.h"
#include "common/color.h"
#include "common/colorconversion.h"
#include "drivers/io.h"
#include "drivers/timer.h"

#include "fiu/fiu_ws2811.h"

// WS2811 timing: 2.4 MHz timer base, 800 kHz carrier -> 3 timer ticks per bit.
//   bit "1" = 2/3 high, bit "0" = 1/3 high (compare values loaded per timer period).
#define FIU_WS2811_TIMER_HZ     2400000
#define FIU_WS2811_CARRIER_HZ   800000
#define FIU_WS2811_PERIOD       (FIU_WS2811_TIMER_HZ / FIU_WS2811_CARRIER_HZ)   // 3
#define FIU_WS2811_BIT_1        ((FIU_WS2811_PERIOD * 2) / 3)                   // 2
#define FIU_WS2811_BIT_0        (FIU_WS2811_PERIOD / 3)                         // 1

#define FIU_WS2811_RESET_LEN    42                                             // leading low = latch/reset gap
#define FIU_WS2811_BITS_PER_LED 24
#define FIU_WS2811_BUFFER_SIZE  (FIU_WS2811_RESET_LEN + FIU_WS2811_LED_COUNT * FIU_WS2811_BITS_PER_LED + 1)

// DMA source buffer of per-timer-period compare values. DMA_RAM keeps it in a
// DMA-reachable section (same as the motor DSHOT buffers). Only ~940 bytes.
static DMA_RAM timerDMASafeType_t fiuLedDmaBuffer[FIU_WS2811_BUFFER_SIZE];

static hsvColor_t fiuLedColor[FIU_WS2811_LED_COUNT];
static TCH_t     *fiuLedTCH   = NULL;
static bool       fiuLedReady = false;

void fiuWs2811Init(void)
{
    // Find the timer channel marked TIM_USE_LED in the target timer table (P00.12 / ATOM3).
    const timerHardware_t *timHw = timerGetByTag(IO_TAG(FIU_LED_PIN), TIM_USE_LED);
    if (timHw == NULL) {
        timHw = timerGetByUsageFlag(TIM_USE_LED);
    }
    if (timHw == NULL) {
        return;
    }

    fiuLedTCH = timerGetTCH(timHw);
    if (fiuLedTCH == NULL) {
        return;
    }

    IO_t io = IOGetByTag(timHw->tag);
    IOInit(io, OWNER_LED_STRIP, RESOURCE_OUTPUT, 0);
    IOConfigGPIOAF(io, IOCFG_AF_PP_FAST, timHw->alternateFunction);

    timerConfigBase(fiuLedTCH, FIU_WS2811_PERIOD, FIU_WS2811_TIMER_HZ);
    timerPWMConfigChannel(fiuLedTCH, 0);
    if (!timerPWMConfigChannelDMA(fiuLedTCH, fiuLedDmaBuffer, sizeof(fiuLedDmaBuffer[0]), FIU_WS2811_BUFFER_SIZE)) {
        fiuLedReady = false;
        return;
    }

    memset(fiuLedDmaBuffer, 0, sizeof(fiuLedDmaBuffer));
    fiuLedReady = true;

    fiuWs2811Update();  // push an initial (all-off) frame
}

void fiuWs2811SetHsv(uint8_t index, const hsvColor_t *color)
{
    if (index < FIU_WS2811_LED_COUNT) {
        fiuLedColor[index] = *color;
    }
}

void fiuWs2811Update(void)
{
    if (!fiuLedReady || fiuLedTCH == NULL) {
        return;
    }

    // Non-blocking: if the previous frame is still being sent, skip and retry next call.
    if (timerPWMDMAInProgress(fiuLedTCH)) {
        return;
    }

    uint16_t offset = FIU_WS2811_RESET_LEN;
    for (int led = 0; led < FIU_WS2811_LED_COUNT; led++) {
        const rgbColor24bpp_t *rgb = hsvToRgb24(&fiuLedColor[led]);
        // WS2811 wants GRB order, MSB first.
        uint32_t grb = ((uint32_t)rgb->rgb.g << 16) | ((uint32_t)rgb->rgb.r << 8) | (uint32_t)rgb->rgb.b;
        for (int bit = FIU_WS2811_BITS_PER_LED - 1; bit >= 0; bit--) {
            fiuLedDmaBuffer[offset++] = (grb & (1u << bit)) ? FIU_WS2811_BIT_1 : FIU_WS2811_BIT_0;
        }
    }

    timerPWMPrepareDMA(fiuLedTCH, FIU_WS2811_BUFFER_SIZE);
    timerPWMStartDMA(fiuLedTCH);
}

#endif // USE_FIU
