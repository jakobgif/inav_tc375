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

/**
 * @file timer_impl_tc375.c
 * @author Jakob Frenzel (jakob.frenzel@hotmail.com)
 * @brief implementation of timer related functions
 * @version 0.1
 * @date 2025-04-03
 * 
 * @copyright Copyright (c) 2025
 * 
 */

//TODO: finish timer implementation

// #include <stdbool.h>
// #include <stdint.h>
// #include <math.h>

#include "platform.h"

// #include "build/atomic.h"
// #include "build/debug.h"

// #include "common/utils.h"

// #include "drivers/io.h"
// #include "drivers/rcc.h"
// #include "drivers/time.h"
// #include "drivers/nvic.h"
// #include "drivers/dma.h"
#include "drivers/timer.h"
#include "drivers/timer_impl.h"

#if !defined(UNUSED)
#define UNUSED(x) ((void)(x))
#endif

void impl_timerInitContext(TCH_t * tch){
    tch->timHw->config->triggerOut = tch->timHw->triggerOut;
    return;
}

volatile timCCR_t * impl_timerCCR(TCH_t * tch){
    Ifx_GTM_TOM_CH *tomCh = IfxGtm_Tom_Ch_getChannelPointer(tch->timHw->tim->tom, tch->timHw->tim->timerChannel);
    return tomCh->CM1.U;
}

void impl_timerNVICConfigure(TCH_t * tch, int irqPriority){
    UNUSED(irqPriority);
    return;
}

void impl_timerConfigBase(TCH_t * tch, uint16_t period, uint32_t hz){
    UNUSED(tch);
    UNUSED(period);
    UNUSED(hz);
    return;
}

void impl_enableTimer(TCH_t * tch){
    IfxGtm_Tom_Timer_run(tch->timHw->tim);
}

void impl_timerEnableIT(TCH_t * tch, uint32_t interrupt){
    UNUSED(tch);
    UNUSED(interrupt);
    return;
}

void impl_timerDisableIT(TCH_t * tch, uint32_t interrupt){
    UNUSED(tch);
    UNUSED(interrupt);
    return;
}

void impl_timerClearFlag(TCH_t * tch, uint32_t flag){
    UNUSED(tch);
    UNUSED(flag);
    return;
}

void impl_timerChConfigIC(TCH_t * tch, bool polarityRising, unsigned inputFilterTicks){
    UNUSED(tch);
    UNUSED(polarityRising);
    UNUSED(inputFilterTicks);
    return;
}

void impl_timerChCaptureCompareEnable(TCH_t * tch, bool enable){
    UNUSED(tch);
    UNUSED(enable);
    return;
}

void impl_timerPWMConfigChannel(TCH_t * tch, uint16_t value){
    UNUSED(tch);
    UNUSED(value);
    return;
}

void impl_timerPWMStart(TCH_t * tch){
    //copied from IfxGtm_Tom_Pwm_start();
    IfxGtm_Tom_Tgc_enableChannel(tch->timHw->tim->tgc[0], tch->timHw->tim->timerChannel, TRUE, TRUE);
    IfxGtm_Tom_Tgc_enableChannelOutput(tch->timHw->tim->tgc[0], tch->timHw->tim->timerChannel, TRUE, TRUE);
    IfxGtm_Tom_Tgc_trigger(tch->timHw->tim->tgc[0]);
    return;
}

bool impl_timerPWMConfigChannelDMA(TCH_t * tch, void * dmaBuffer, uint8_t dmaBufferElementSize, uint32_t dmaBufferElementCount){
    UNUSED(tch);
    UNUSED(dmaBuffer);
    UNUSED(dmaBufferElementSize);
    UNUSED(dmaBufferElementCount);
    return 0;
}
void impl_timerPWMPrepareDMA(TCH_t * tch, uint32_t dmaBufferElementCount){
    UNUSED(tch);
    UNUSED(dmaBufferElementCount);
    return;
}
void impl_timerPWMStartDMA(TCH_t * tch){
    UNUSED(tch);
    return;
}
void impl_timerPWMStopDMA(TCH_t * tch){
    UNUSED(tch);
    return;
}