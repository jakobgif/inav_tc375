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
#include "drivers/system.h"

static void checkAndHandleResetReason(){
    return;
}

bool isMPUSoftReset(void){
    return false;
}

void systemInit(void){
    checkAndHandleResetReason();

    cycleCounterInit();

#if defined(AURIX_CLEAR_DFLASH_ON_SYSTEM_INIT)
    uint16_t endInitSafetyPassword = IfxScuWdt_getSafetyWatchdogPassword();

    IfxScuWdt_clearSafetyEndinit(endInitSafetyPassword);
    IfxFlash_eraseMultipleSectors(IfxFlash_dFlashTableEepLog[0].start, IFXFLASH_DFLASH_NUM_LOG_SECTORS);
    IfxScuWdt_setSafetyEndinit(endInitSafetyPassword);

    IfxFlash_waitUnbusy(0, IfxFlash_FlashType_D0);
#endif
}