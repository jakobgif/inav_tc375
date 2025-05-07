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
 * @version 0.1
 * @date 2025-03-22
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdbool.h>
#include <stdint.h>

#include "platform.h"

//#include "common/utils.h"

// #include "drivers/io.h"
// #include "drivers/rcc.h"
// #include "drivers/time.h"
// #include "drivers/nvic.h"
#include "drivers/timer.h"
// #include "drivers/timer_impl.h"

//we can create timers in runtime so we need to prepare structs for every available timer
IfxGtm_Tom_Timer tomDriver[HARDWARE_TIMER_DEFINITION_COUNT];
IfxGtm_Tom_Timer_Config tomConfig[HARDWARE_TIMER_DEFINITION_COUNT];

timerDef_t const timerDefinitions[HARDWARE_TIMER_DEFINITION_COUNT] = {
    [0] = { .tim=&tomDriver[0], .config=&tomConfig[0], .isrPriority=INTPRIO_GTM_TOM_00, .rcc=NULL, .irq=NULL, .secondIrq=NULL },
    [1] = { .tim=&tomDriver[1], .config=&tomConfig[1], .isrPriority=INTPRIO_GTM_TOM_01, .rcc=NULL, .irq=NULL, .secondIrq=NULL },
    [2] = { .tim=&tomDriver[2], .config=&tomConfig[2], .isrPriority=INTPRIO_GTM_TOM_02, .rcc=NULL, .irq=NULL, .secondIrq=NULL },
    [3] = { .tim=&tomDriver[3], .config=&tomConfig[3], .isrPriority=INTPRIO_GTM_TOM_03, .rcc=NULL, .irq=NULL, .secondIrq=NULL },
    [4] = { .tim=&tomDriver[4], .config=&tomConfig[4], .isrPriority=INTPRIO_GTM_TOM_04, .rcc=NULL, .irq=NULL, .secondIrq=NULL },
    [5] = { .tim=&tomDriver[5], .config=&tomConfig[5], .isrPriority=INTPRIO_GTM_TOM_05, .rcc=NULL, .irq=NULL, .secondIrq=NULL },
    [6] = { .tim=&tomDriver[6], .config=&tomConfig[6], .isrPriority=INTPRIO_GTM_TOM_06, .rcc=NULL, .irq=NULL, .secondIrq=NULL },
    [7] = { .tim=&tomDriver[7], .config=&tomConfig[7], .isrPriority=INTPRIO_GTM_TOM_07, .rcc=NULL, .irq=NULL, .secondIrq=NULL },
    [8] = { .tim=&tomDriver[8], .config=&tomConfig[8], .isrPriority=INTPRIO_GTM_TOM_08, .rcc=NULL, .irq=NULL, .secondIrq=NULL },
    [9] = { .tim=&tomDriver[9], .config=&tomConfig[9], .isrPriority=INTPRIO_GTM_TOM_09, .rcc=NULL, .irq=NULL, .secondIrq=NULL },
    [10] = { .tim=&tomDriver[10], .config=&tomConfig[10], .isrPriority=INTPRIO_GTM_TOM_10, .rcc=NULL, .irq=NULL, .secondIrq=NULL },
    [11] = { .tim=&tomDriver[11], .config=&tomConfig[11], .isrPriority=INTPRIO_GTM_TOM_11, .rcc=NULL, .irq=NULL, .secondIrq=NULL },
    [12] = { .tim=&tomDriver[12], .config=&tomConfig[12], .isrPriority=INTPRIO_GTM_TOM_12, .rcc=NULL, .irq=NULL, .secondIrq=NULL },
    [13] = { .tim=&tomDriver[13], .config=&tomConfig[13], .isrPriority=INTPRIO_GTM_TOM_13, .rcc=NULL, .irq=NULL, .secondIrq=NULL },
    [14] = { .tim=&tomDriver[14], .config=&tomConfig[14], .isrPriority=INTPRIO_GTM_TOM_14, .rcc=NULL, .irq=NULL, .secondIrq=NULL },
    [15] = { .tim=&tomDriver[15], .config=&tomConfig[15], .isrPriority=INTPRIO_GTM_TOM_15, .rcc=NULL, .irq=NULL, .secondIrq=NULL }
};

//return clock speed of this timer
uint32_t timerClock(timerDef_t *tim){
    return IfxGtm_Tom_Ch_getClockFrequency(tim->tim->gtm, tim->tim->tom, tim->tim->timerChannel);
}
