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
 * @file config_streamer_tc375.c
 * @author Jakob Frenzel (jakob.frenzel@hotmail.com)
 * @brief store data to flash, based on https://github.com/Infineon/AURIX_code_examples/tree/master/code_examples/Flash_Programming_1_KIT_TC375_LK
 * @version 0.1
 * @date 2025-03-19
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>
#include "platform.h"
#include "drivers/system.h"
#include "config/config_streamer.h"

#if defined(TC375) && !defined(CONFIG_IN_RAM) && !defined(CONFIG_IN_EXTERNAL_FLASH)

#define FLASH_PAGE_SIZE     IFXFLASH_DFLASH_PAGE_LENGTH       

#define FLASH_MODULE        0   // Macro to select the flash (PMU) module
#define DATA_FLASH          0   // Define the Data Flash Bank to be used, see IfxFlash_FlashType

#define DFLASH_START_ADDRESS    0xAF000000  //info from Libraries\iLLD\TC37A\Tricore\_Impl\IfxFlash_cfg.c IfxFlash_dFlashTableEepLog
#define DFLASH_END_ADDRESS      0xAF03FFFF

static uint32_t getFLASHSectorForEEPROM(uint32_t address){
    //info from Libraries\iLLD\TC37A\Tricore\_Impl\IfxFlash_cfg.c IfxFlash_dFlashTableEepLog
    if (address < DFLASH_START_ADDRESS){
        //below start address
        while (1) {
            failureMode(FAILURE_FLASH_WRITE_FAILED);
        }
    }

    //find in which sector the address is
    for(uint8_t i = 0; i < IFXFLASH_DFLASH_NUM_LOG_SECTORS; i++){
        if (address <= IfxFlash_dFlashTableEepLog[i].end)
            return IfxFlash_dFlashTableEepLog[i].start;
    }

    //should not reach this point
    while (1) {
        failureMode(FAILURE_FLASH_WRITE_FAILED);
    }
}

void config_streamer_impl_unlock(void){
    //in aurix this is handled by library
    return;
}

void config_streamer_impl_lock(void){
    //in aurix this is handled by library
    return;
}

int config_streamer_impl_write_word(config_streamer_t *c, config_streamer_buffer_align_type_t *buffer){
    if (c->err != 0) {
        return c->err;
    }

    uint32_t sector = (uint32_t)NULL;
    if (c->address % FLASH_PAGE_SIZE == 0) {
        sector = getFLASHSectorForEEPROM(c->address);

        /* Erase the sector */
        IfxFlash_eraseSector(sector);

        /* Wait until the sector is erased */
        IfxFlash_waitUnbusy(FLASH_MODULE, DATA_FLASH);
    }

    /* Enter in page mode */
    if (IfxFlash_enterPageMode(c->address) != 0) {
        return -1;
    }

    /* Wait until page mode is entered */
    IfxFlash_waitUnbusy(FLASH_MODULE, DATA_FLASH);
    
    //in inav we can configure the buffer size. I configure to 8Bytes = 64bit = 2x32bit 
    //load data to write into page buffer
    IfxFlash_loadPage2X32(c->address, buffer[0], buffer[1]);

    /* Write the loaded page */
    IfxFlash_writePage(c->address); /* Write the page */

    /* Wait until the data is written in the Data Flash memory */
    IfxFlash_waitUnbusy(FLASH_MODULE, DATA_FLASH);

    c->address += CONFIG_STREAMER_BUFFER_SIZE;
    return 0;
}

#endif
