/*
 * This file is part of ATbetaflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file adc_tc375.c
 * @author Jakob Frenzel (jakob.frenzel@hotmail.com)
 * @brief implementation of ADC code
 * @date 2025-06-29
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "platform.h"

#include "utils.h"

#include "adc.h"
#include "adc_impl.h"

//for this implementation we use one group with ADC_CHN_COUNT channels so it must be smaller than 16
//we also hard coded 6 DMA channels. so we cannot use more than 7 ADCs (adc 0 is a dummy)
STATIC_ASSERT(ADC_CHN_COUNT <= 7, adc_channel_limit_exceeded);

//must be compatible with aurix DMA implementation
STATIC_ASSERT(ADC_AVERAGE_N_SAMPLES == 16, adc_averaging_not_compatible);

static adcDevice_t adcHardware[ADCDEV_COUNT];
static IfxEvadc_Adc_Channel adcChannel[ADC_CHN_COUNT];
static IfxDma_Dma_Channel dmaChannel[ADC_CHN_COUNT];

ADCDevice adcDeviceByInstance(IfxEvadc_GroupId *instance)
{
    if (instance == IfxEvadc_GroupId_0)
        return ADCDEV_1;

    return ADCINVALID;
}

static void adcInstanceInit(ADCDevice adcDevice){
    adcDevice_t * adc = &adcHardware[adcDevice];
    /* Create configuration */
    IfxEvadc_Adc_Config adcConfig;
    IfxEvadc_Adc_initModuleConfig(&adcConfig, &MODULE_EVADC);

    /* Initialize module */
    IfxEvadc_Adc_initModule(&adc->evadc, &adcConfig);

    /* Create and initialize group configuration with default values */
    IfxEvadc_Adc_GroupConfig adcGroupConfig;
    IfxEvadc_Adc_initGroupConfig(&adcGroupConfig, &adc->evadc);

    /* Setting user configuration using group 2 */
    adcGroupConfig.groupId = adcDevice;
    adcGroupConfig.master = adcDevice;
    adcGroupConfig.startupCalibration = TRUE;

    /* Enable queued source */
    adcGroupConfig.arbiter.requestSlotQueue0Enabled = TRUE;

    /* Enable all gates in "always" mode (no edge detection) */
    adcGroupConfig.queueRequest[0].triggerConfig.gatingMode = IfxEvadc_GatingMode_always;

    /* Initialize the group */
    IfxEvadc_Adc_initGroup(&adc->adcGroup, &adcGroupConfig);

    return;
}

void adcHardwareInit(drv_adc_config_t *init){
    UNUSED(init);

    //config adc instance 
    for (int i = 0; i < ADCDEV_COUNT; i++) {
        adcInstanceInit(i);
    }

    //config channels
    for (int i = ADC_CHN_1; i < ADC_CHN_COUNT; i++) {
        if (adcConfig[i].analogPinId == 0xFF) //channel is unused
            continue;

        // only use adc1 for now
        adcDevice_t *adc = &(adcHardware[adcConfig[i].adcDevice]);
        
        adc->enabled = true;
        adc->usedChannelCount++;
        adcConfig[i].dmaIndex = adc->usedChannelCount;
        adcConfig[i].enabled = true;

        /* Initialize the configuration with default values */
        IfxEvadc_Adc_ChannelConfig adcChannelConfig;
        IfxEvadc_Adc_initChannelConfig(&adcChannelConfig, &adc->adcGroup);

        /* Select the channel ID and the respective result register */
        adcChannelConfig.channelId = (IfxEvadc_ChannelId)(adcConfig[i].analogPinId);
        adcChannelConfig.resultRegister = (IfxEvadc_ChannelResult)(adcConfig[i].dmaIndex-1);

        /* Assign the DMA channel as priority of the interrupt in order to trigger the DMA when a new result is
        * available.
        */
        adcChannelConfig.resultPriority = DMA_CHANNEL_ADC_1 + (adcConfig[i].dmaIndex-1);
        /* Assign the DMA as service provider for the interrupt */
        adcChannelConfig.resultServProvider = IfxSrc_Tos_dma;

        /* Initialize the channel */
        IfxEvadc_Adc_initChannel(&adcChannel[i], &adcChannelConfig);

        //add channel to queue
        IfxEvadc_Adc_addToQueue(&adcChannel[i], IfxEvadc_RequestSource_queue0, IFXEVADC_QUEUE_REFILL);

        //now DMA config starts
        /* Create module configuration */
        IfxDma_Dma_Config dmaConfig;
        IfxDma_Dma_initModuleConfig(&dmaConfig, &MODULE_DMA);

        /* Initialize module */
        IfxDma_Dma dma;
        IfxDma_Dma_initModule(&dma, &dmaConfig);

        /* Initial configuration for the channel */
        IfxDma_Dma_ChannelConfig dmaChnsCfg;
        IfxDma_Dma_initChannelConfig(&dmaChnsCfg, &dma);

        dmaChnsCfg.transferCount = 1;
        dmaChnsCfg.requestMode = IfxDma_ChannelRequestMode_completeTransactionPerRequest;
        dmaChnsCfg.moveSize = IfxDma_ChannelMoveSize_16bit;
        dmaChnsCfg.operationMode = IfxDma_ChannelOperationMode_continuous;
        dmaChnsCfg.hardwareRequestEnabled = TRUE;

        /* Source and destination addresses */
        dmaChnsCfg.sourceCircularBufferEnabled = TRUE;
        dmaChnsCfg.sourceAddressCircularRange = IfxDma_ChannelIncrementCircular_none;

        //NOTE: on aurix we dont have variable step size for DMA therefore the adc values buffer is setup differently
        dmaChnsCfg.destinationCircularBufferEnabled = TRUE;
        dmaChnsCfg.destinationAddressCircularRange = IfxDma_ChannelIncrementCircular_32; //because 16x uint16(=2byte) = 32byte
        dmaChnsCfg.destinationAddressIncrementDirection = IfxDma_ChannelIncrementDirection_positive;

        /* Interrupt generated after transactions */
        //dmaChnsCfg.channelInterruptEnabled = TRUE;
        //dmaChnsCfg.channelInterruptControl = IfxDma_ChannelInterruptControl_thresholdLimitMatch;
        //dmaChnsCfg.channelInterruptTypeOfService = IfxCpu_Irq_getTos(IfxCpu_getCoreIndex());
        //dmaChnsCfg.channelInterruptPriority = INTPRIO_DISABLED;

        /* Channel specific configurations */
        dmaChnsCfg.channelId = DMA_CHANNEL_ADC_1 + (adcConfig[i].dmaIndex-1);
        dmaChnsCfg.sourceAddress = 0xF0020700u + (sizeof(Ifx_EVADC_GLOB_RES_Bits) * (adcConfig[i].dmaIndex-1)); //Address of EVADC Group 0 Result Register 0 (EVADC_G0_RES0) + offset per channel
        dmaChnsCfg.destinationAddress = (uint32_t)&(adcValues[adcConfig[i].adcDevice][adcConfig[i].dmaIndex * ADC_AVERAGE_N_SAMPLES]);

        /* Initialize the DMA channel */
        IfxDma_Dma_initChannel(&dmaChannel[adcConfig[i].dmaIndex-1], &dmaChnsCfg);

        IfxEvadc_Adc_startQueue(&adc->adcGroup, IfxEvadc_RequestSource_queue0);
    }
}
