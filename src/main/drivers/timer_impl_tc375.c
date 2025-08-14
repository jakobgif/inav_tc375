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
 * @date 2025-04-03
 */

#include "platform.h"

#include "drivers/timer.h"
#include "drivers/timer_impl.h"

#if !defined(UNUSED)
#define UNUSED(x) ((void)(x))
#endif

void impl_timerInitContext(TCH_t * tch){
    UNUSED(tch);
    return;
}

volatile timCCR_t * impl_timerCCR(TCH_t * tch){
    Ifx_GTM_ATOM_CH *atomCh = IfxGtm_Atom_Ch_getChannelPointer(tch->timHw->tim->atom, tch->timHw->triggerOut->channel);
    return (volatile timCCR_t *)&(atomCh->CM1.U);
}

void impl_timerNVICConfigure(TCH_t * tch, int irqPriority){
    UNUSED(irqPriority);
    return;
}

void impl_timerConfigBase(TCH_t * tch, uint16_t period, uint32_t hz){
    timerDef_t timerDef = timerDefinitions[timer2id(tch->timHw->tim)];
    IfxGtm_Atom_Timer_Config *atomConfig = (IfxGtm_Atom_Timer_Config*)timerDef.config;
                
    IfxGtm_Atom_Timer_initConfig(atomConfig, &MODULE_GTM); // Initialize default parameters

    switch(tch->timHw->triggerOut->atom)
    {
        case IfxGtm_Atom_0:
            atomConfig->clock = IfxGtm_Cmu_Clk_0; // Select the CMU clock
            IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_0, (float32)hz); // Set the clock frequency
            IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK0); // Enable the CMU clock 0
            break;
        case IfxGtm_Atom_1:
            atomConfig->clock = IfxGtm_Cmu_Clk_1;
            IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_1, (float32)hz);
            IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK1);
            break;
        case IfxGtm_Atom_2:
            atomConfig->clock = IfxGtm_Cmu_Clk_2;
            IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_2, (float32)hz);
            IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK2);
            break;
        case IfxGtm_Atom_3:
            atomConfig->clock = IfxGtm_Cmu_Clk_3;
            IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_3, (float32)hz);
            IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK3);
            break;
        case IfxGtm_Atom_4:
            atomConfig->clock = IfxGtm_Cmu_Clk_4;
            IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_4, (float32)hz);
            IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK4);
            break;
        case IfxGtm_Atom_5:
            atomConfig->clock = IfxGtm_Cmu_Clk_5;
            IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_5, (float32)hz);
            IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK5);
            break;
    }

    atomConfig->atom = tch->timHw->triggerOut->atom;
    atomConfig->timerChannel = tch->timHw->triggerOut->channel;
    //atomConfig->base.isrPriority = 0; // Set interrupt priority
    //atomConfig->base.isrProvider = IfxSrc_Tos_cpu0; // Set interrupt provider

    IfxGtm_Atom_Timer_init(tch->timHw->tim, atomConfig); //init ATOM
    IfxGtm_Atom_Ch_setCompareZero(tch->timHw->tim->atom, tch->timHw->triggerOut->channel, period);
}

void impl_enableTimer(TCH_t * tch){
    UNUSED(tch);
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
    // PWM Part of ATOM TIMER
    IfxGtm_Atom_Ch_setSignalLevel(tch->timHw->tim->atom, tch->timHw->triggerOut->channel, Ifx_ActiveState_high);

    IfxGtm_Atom_Ch_setMode(tch->timHw->tim->atom, tch->timHw->triggerOut->channel, IfxGtm_Atom_Mode_outputPwm);

    IfxGtm_PinMap_setAtomTout(tch->timHw->triggerOut, IfxPort_OutputMode_pushPull, IfxPort_PadDriver_cmosAutomotiveSpeed1);

    IfxGtm_Atom_Ch_setCompareOne(tch->timHw->tim->atom, tch->timHw->triggerOut->channel, (uint32_t)value);
    return;
}

void impl_timerPWMStart(TCH_t * tch){
    IfxGtm_Atom_Agc_enableChannel(tch->timHw->tim->agc, tch->timHw->triggerOut->channel, TRUE, FALSE);
    IfxGtm_Atom_Agc_enableChannelOutput(tch->timHw->tim->agc, tch->timHw->triggerOut->channel, TRUE, FALSE);
    IfxGtm_Atom_Agc_trigger(tch->timHw->tim->agc);
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