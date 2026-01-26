/*
 * This file is part of Cleanflight.
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
 * @file system_tc375.c
 * @author Jakob Frenzel (jakob.frenzel@hotmail.com)
 * @brief perform startup init
 * @date 2025-05-12
 */

#include "platform.h"
#include "build/build_config.h"
#include "drivers/system.h"
#include "drivers/time.h"
#include "log.h"

void checkAndHandleResetReason(){
    IfxScuRcu_ResetCode lastReset;

    // /* Evaluate the last reset cause/details */
    lastReset = IfxScuRcu_evaluateReset();

    LOG_DEBUG(SYSTEM, "Reset trigger: %d", lastReset.resetTrigger);
    LOG_DEBUG(SYSTEM, "Reset type: %d", lastReset.resetType);

    // switch (lastReset.resetType){
    //     case IfxScuRcu_ResetType_application:       
    //         break;

    //     case IfxScuRcu_ResetType_system:
    //         break;

    //     case IfxScuRcu_ResetType_warmpoweron:
    //         break;

    //     case IfxScuRcu_ResetType_coldpoweron:
    //         LOG_INFO(SYSTEM, "Reset reason: cold power on");
    //         break;

    //     default:
    //         break;
    // }

    /* Clear Cold Power-On Reset sticky bits */
    IfxScuRcu_clearColdResetStatus();

    return;
}

bool isMPUSoftReset(void){
    IfxScuRcu_ResetCode lastReset;
    lastReset = IfxScuRcu_evaluateReset();
    return (lastReset.resetType != IfxScuRcu_ResetType_coldpoweron);
}

void systemReset(void){
    IfxCpu_disableInterrupts();

    //from example code
    /* Get the CPU EndInit password */
    uint16_t CPUEndinitPw = IfxScuWdt_getCpuWatchdogPassword();

    /* Configure the request trigger in the Reset Configuration Register */
    IfxScuRcu_configureResetRequestTrigger(IfxScuRcu_Trigger_sw, IfxScuRcu_ResetType_system);

    /* Clear CPU EndInit protection to write in the SWRSTCON register of SCU */
    IfxScuWdt_clearCpuEndinit(CPUEndinitPw);

    /* Trigger a software reset based on the configuration of RSTCON register */
    IfxCpu_triggerSwReset();

    /* The following instructions are not executed if a SW reset occurs */
    /* Set CPU EndInit protection */
    IfxScuWdt_setCpuEndinit(CPUEndinitPw);
}

void cycleCounterInit(void){
    extern ticks_t usTicks;
    usTicks = IfxStm_getFrequency(IFXSTM_DEFAULT_TIMER) / 1000000;
}

void systemResetToBootloader(void){
    systemResetRequest(0); //RESET_BOOTLOADER_REQUEST_ROM
}

void systemResetRequest(uint32_t requestId){
    //IfxScuRcu_configureResetRequestTrigger(IfxScuRcu_Trigger_sw, IfxScuRcu_ResetType_system);
    systemReset();
}

void systemInit(void){
    cycleCounterInit();
    
    //needed for timers
    IfxGtm_enable(&MODULE_GTM);

    //init die temperature measurement
    IfxDts_Dts_Config dtsConf;
    IfxDts_Dts_initModuleConfig(&dtsConf);    
    //no other settings needed. default settings work          
    IfxDts_Dts_initModule(&dtsConf);

#if defined(AURIX_CLEAR_DFLASH_ON_SYSTEM_INIT)
    uint16_t endInitSafetyPassword = IfxScuWdt_getSafetyWatchdogPassword();

    IfxScuWdt_clearSafetyEndinit(endInitSafetyPassword);
    IfxFlash_eraseMultipleSectors(IfxFlash_dFlashTableEepLog[0].start, IFXFLASH_DFLASH_NUM_LOG_SECTORS);
    IfxScuWdt_setSafetyEndinit(endInitSafetyPassword);

    IfxFlash_waitUnbusy(0, IfxFlash_FlashType_D0);
#endif
}