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
#include "build/build_config.h"

#if !defined(UNUSED)
#define UNUSED(x) ((void)(x))
#endif

#define PSPR_FUNCTION CPU0_PSPR_FUNCTION

void impl_timerInitContext(TCH_t * tch){
    UNUSED(tch);
    return;
}

volatile timCCR_t * impl_timerCCR(TCH_t * tch){
    Ifx_GTM_ATOM_CH *atomCh = IfxGtm_Atom_Ch_getChannelPointer(tch->timHw->tim->atom, tch->timHw->triggerOut->channel);
    return (volatile timCCR_t *)&(atomCh->SR1.U);
}

void impl_timerNVICConfigure(TCH_t * tch, int irqPriority){
    UNUSED(irqPriority);
    return;
}

void impl_timerConfigBase(TCH_t * tch, uint16_t period, uint32_t hz){
    timerDef_t timerDef = timerDefinitions[timer2id(tch->timHw->tim)];
    IfxGtm_Atom_Timer_Config *atomConfig = (IfxGtm_Atom_Timer_Config*)timerDef.config;
                
    IfxGtm_Atom_Timer_initConfig(atomConfig, &MODULE_GTM); // Initialize default parameters

    atomConfig->atom = tch->timHw->triggerOut->atom;
    atomConfig->timerChannel = tch->timHw->triggerOut->channel;

    IfxGtm_Atom_Timer_init(tch->timHw->tim, atomConfig); //init ATOM

    //init has to be done before because pointer in timHw->tim not set yet
    Ifx_GTM_ATOM *atom = tch->timHw->tim->atom;
    IfxGtm_Atom_Ch channel = tch->timHw->tim->timerChannel;

    switch(tch->timHw->tim->atomIndex)
    {
        case IfxGtm_Atom_0:
            atomConfig->clock = IfxGtm_Cmu_Clk_0; // Select the CMU clock
            IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_0, (float32)hz); // Set the clock frequency
            IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK0); // Enable the CMU clock 0
            IfxGtm_Atom_Ch_setClockSource(atom, channel, IfxGtm_Cmu_Clk_0);
            break;
        case IfxGtm_Atom_1:
            atomConfig->clock = IfxGtm_Cmu_Clk_1;
            IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_1, (float32)hz);
            IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK1);
            IfxGtm_Atom_Ch_setClockSource(atom, channel, IfxGtm_Cmu_Clk_1);
            break;
        case IfxGtm_Atom_2:
            atomConfig->clock = IfxGtm_Cmu_Clk_2;
            IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_2, (float32)hz);
            IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK2);
            IfxGtm_Atom_Ch_setClockSource(atom, channel, IfxGtm_Cmu_Clk_2);
            break;
        case IfxGtm_Atom_3:
            atomConfig->clock = IfxGtm_Cmu_Clk_3;
            IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_3, (float32)hz);
            IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK3);
            IfxGtm_Atom_Ch_setClockSource(atom, channel, IfxGtm_Cmu_Clk_3);
            break;
        case IfxGtm_Atom_4:
            atomConfig->clock = IfxGtm_Cmu_Clk_4;
            IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_4, (float32)hz);
            IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK4);
            IfxGtm_Atom_Ch_setClockSource(atom, channel, IfxGtm_Cmu_Clk_4);
            break;
        case IfxGtm_Atom_5:
            atomConfig->clock = IfxGtm_Cmu_Clk_5;
            IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_5, (float32)hz);
            IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK5);
            IfxGtm_Atom_Ch_setClockSource(atom, channel, IfxGtm_Cmu_Clk_5);
            break;
    }

    IfxGtm_Atom_Ch_setCompareZero(atom, channel, period);
    IfxGtm_Atom_Ch_setCompareZeroShadow(atom, channel, period);
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

void PSPR_FUNCTION impl_timerPWMConfigChannel(TCH_t * tch, uint16_t value){
    Ifx_GTM_ATOM *atom = tch->timHw->tim->atom;
    IfxGtm_Atom_Ch channel = tch->timHw->tim->timerChannel;

    IfxGtm_Atom_Ch_setSignalLevel(atom, channel, Ifx_ActiveState_high);
    IfxGtm_Atom_Ch_setMode(atom, channel, IfxGtm_Atom_Mode_outputPwm);
    IfxGtm_PinMap_setAtomTout(tch->timHw->triggerOut, IfxPort_OutputMode_pushPull, IfxPort_PadDriver_cmosAutomotiveSpeed1);
    IfxGtm_Atom_Ch_setCompareOneShadow(atom, channel, (uint32_t)value);
    IfxGtm_Atom_Agc_enableChannelUpdate(tch->timHw->tim->agc, channel, TRUE); //CM0, CM1 and CLK_SRC will be updated from shadow registers when CN0 = CM0
    return;
}

void PSPR_FUNCTION impl_timerPWMStart(TCH_t * tch){
    IfxGtm_Atom_Agc_enableChannel(tch->timHw->tim->agc, tch->timHw->tim->timerChannel, TRUE, FALSE);
    IfxGtm_Atom_Agc_enableChannelOutput(tch->timHw->tim->agc, tch->timHw->tim->timerChannel, TRUE, FALSE);
    IfxGtm_Atom_Agc_trigger(tch->timHw->tim->agc);
    return;
}

bool PSPR_FUNCTION impl_timerPWMConfigChannelDMA(TCH_t * tch, void * dmaBuffer, uint8_t dmaBufferElementSize, uint32_t dmaBufferElementCount){
    UNUSED(dmaBufferElementCount);

    if(dmaBuffer == NULL_PTR){
        return false;
    }

    if(dmaBufferElementSize != sizeof(timerDMASafeType_t)){
        return false;
    }

    tch->dmaBuffer = dmaBuffer;

    timerDef_t timerDef = timerDefinitions[timer2id(tch->timHw->tim)];
    IfxGtm_Atom_Timer_Config *atomConfig = (IfxGtm_Atom_Timer_Config *)(timerDef.config);
    volatile Ifx_GTM_ATOM_AGC * agc = tch->timHw->tim->agc;
    //disable channel
    IfxGtm_Atom_Agc_enableChannel(agc, tch->timHw->triggerOut->channel, FALSE, FALSE);
    IfxGtm_Atom_Agc_enableChannelOutput(agc, tch->timHw->triggerOut->channel, FALSE, FALSE);
    IfxGtm_Atom_Agc_trigger(agc);

    IfxDma_ChannelId channelId = DMA_CHANNEL_TIMER_0 + timer2id(tch->timHw->tim);

    //configure DMA vector
    atomConfig->base.isrPriority = channelId;
    atomConfig->base.isrProvider = IfxSrc_Tos_dma;
    
    //enable interrupt
    volatile Ifx_SRC_SRCR *src;
    //this can cause race conditions when CM1 comes close to CM0. However, in DSHOT this is not the case
    IfxGtm_Atom_Ch_setNotification(tch->timHw->tim->atom, tch->timHw->tim->timerChannel, IfxGtm_IrqMode_pulseNotify, TRUE, FALSE);
    src = IfxGtm_Atom_Ch_getSrcPointer(tch->timHw->tim->gtm, atomConfig->atom, tch->timHw->tim->timerChannel);
    IfxSrc_init(src, IfxSrc_Tos_dma, channelId);
    IfxSrc_enable(src);

    //now DMA config starts
    IfxDma_Dma_Config dmaConfig;
    IfxDma_Dma_initModuleConfig(&dmaConfig, &MODULE_DMA);
    
    IfxDma_Dma_initModule(&(tch->dma), &dmaConfig);

    IfxDma_Dma_ChannelConfig * dmaChnCfg = &(tch->dmaChnCfg);
    IfxDma_Dma_initChannelConfig(dmaChnCfg, &(tch->dma));

    dmaChnCfg->transferCount = 18; //equals DSHOT_DMA_BUFFER_SIZE
    dmaChnCfg->requestMode = IfxDma_ChannelRequestMode_oneTransferPerRequest; //a request initiates a single transfer = 32bit
    dmaChnCfg->moveSize = IfxDma_ChannelMoveSize_32bit; //equals timerDMASafeType_t
    dmaChnCfg->operationMode = IfxDma_ChannelOperationMode_single; //channel disabled after transaction
    dmaChnCfg->hardwareRequestEnabled = TRUE;
    dmaChnCfg->channelId = channelId;
    dmaChnCfg->sourceAddressIncrementStep = IfxDma_ChannelIncrementStep_1; //count up one move size (32bit)
    dmaChnCfg->sourceAddressIncrementDirection = IfxDma_ChannelIncrementDirection_positive;
    dmaChnCfg->destinationAddressCircularRange = IfxDma_ChannelIncrementCircular_none; //destination address stays the same
    dmaChnCfg->destinationCircularBufferEnabled = TRUE;     
    
    return true;
}

void PSPR_FUNCTION impl_timerPWMPrepareDMA(TCH_t * tch, uint32_t dmaBufferElementCount){
    if(IfxDma_getChannelTransferCount((Ifx_DMA *)&(tch->dma), tch->dmaChannel.channelId) != 0){
        //ignore if transfer is already in progress
        //firmware will call this again anyways
        //one missed frame does not matter too much
        return;
    }

    IfxDma_Dma_ChannelConfig * dmaChnCfg = &(tch->dmaChnCfg);
    dmaChnCfg->sourceAddress = (uint32_t)(tch->dmaBuffer);
    dmaChnCfg->destinationAddress = (uint32_t)impl_timerCCR(tch);

    IfxDma_Dma_initChannel(&tch->dmaChannel, (const IfxDma_Dma_ChannelConfig *)dmaChnCfg);

    tch->dmaState = TCH_DMA_READY;
    return;
}

void PSPR_FUNCTION impl_timerPWMStartDMA(TCH_t * tch){
    if(tch->dmaState != TCH_DMA_READY){
        return;
    }

    //start timer
    impl_timerPWMStart(tch);

    tch->dmaState = TCH_DMA_ACTIVE;
    return;
}

void PSPR_FUNCTION impl_timerPWMStopDMA(TCH_t * tch){
    //terminate if any transaction is in progress
    IfxDma_disableChannelTransaction((Ifx_DMA *)&(tch->dma), tch->dmaChannel.channelId);

    volatile Ifx_GTM_ATOM_AGC * agc = tch->timHw->tim->agc;
    //disable channel
    IfxGtm_Atom_Agc_enableChannel(agc, tch->timHw->triggerOut->channel, FALSE, FALSE);
    IfxGtm_Atom_Agc_enableChannelOutput(agc, tch->timHw->triggerOut->channel, FALSE, FALSE);
    IfxGtm_Atom_Agc_trigger(agc);

    tch->dmaState = TCH_DMA_IDLE;
    return;
}
