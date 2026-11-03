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
 * @file timer_tc375.c
 * @author Jakob Frenzel (jakob.frenzel@hotmail.com)
 * @brief implementation for timer 
 * @date 2025-03-22
 */

#include <stdbool.h>
#include <stdint.h>

#include "platform.h"
#include "drivers/timer.h"

//we can create timers in runtime so we need to prepare structs for every available timer
IfxGtm_Atom_Timer atomDriver[HARDWARE_TIMER_DEFINITION_COUNT];
static IfxGtm_Atom_Timer_Config atomConfig[HARDWARE_TIMER_DEFINITION_COUNT];

timerDef_t const timerDefinitions[HARDWARE_TIMER_DEFINITION_COUNT] = {
    [0] = { .tim=&atomDriver[0], .config=&atomConfig[0], .isrPriority=INTPRIO_GTM_ATOM_00},
    [1] = { .tim=&atomDriver[1], .config=&atomConfig[1], .isrPriority=INTPRIO_GTM_ATOM_01},
    [2] = { .tim=&atomDriver[2], .config=&atomConfig[2], .isrPriority=INTPRIO_GTM_ATOM_02},
    [3] = { .tim=&atomDriver[3], .config=&atomConfig[3], .isrPriority=INTPRIO_GTM_ATOM_03},
    [4] = { .tim=&atomDriver[4], .config=&atomConfig[4], .isrPriority=INTPRIO_GTM_ATOM_04},
    [5] = { .tim=&atomDriver[5], .config=&atomConfig[5], .isrPriority=INTPRIO_GTM_ATOM_05},
    [6] = { .tim=&atomDriver[6], .config=&atomConfig[6], .isrPriority=INTPRIO_GTM_ATOM_06},
    [7] = { .tim=&atomDriver[7], .config=&atomConfig[7], .isrPriority=INTPRIO_GTM_ATOM_07},
    [8] = { .tim=&atomDriver[8], .config=&atomConfig[8], .isrPriority=INTPRIO_GTM_ATOM_08},
    [9] = { .tim=&atomDriver[9], .config=&atomConfig[9], .isrPriority=INTPRIO_GTM_ATOM_09},
    [10] = { .tim=&atomDriver[10], .config=&atomConfig[10], .isrPriority=INTPRIO_GTM_ATOM_10},
    [11] = { .tim=&atomDriver[11], .config=&atomConfig[11], .isrPriority=INTPRIO_GTM_ATOM_11},
    [12] = { .tim=&atomDriver[12], .config=&atomConfig[12], .isrPriority=INTPRIO_GTM_ATOM_12},
    [13] = { .tim=&atomDriver[13], .config=&atomConfig[13], .isrPriority=INTPRIO_GTM_ATOM_13},
    [14] = { .tim=&atomDriver[14], .config=&atomConfig[14], .isrPriority=INTPRIO_GTM_ATOM_14},
    [15] = { .tim=&atomDriver[15], .config=&atomConfig[15], .isrPriority=INTPRIO_GTM_ATOM_15}
};

//return clock speed of this timer
uint32_t timerClock(HAL_Timer_t *tim){
    return IfxGtm_Cmu_getGclkFrequency(&MODULE_GTM);
}
