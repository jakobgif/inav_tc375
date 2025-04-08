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
    [0] = { .tim=&tomDriver[0], .config=&tomConfig[0], .rcc=NULL, .irq=NULL, .secondIrq=NULL },
};

//return clock speed of this timer
uint32_t timerClock(IfxGtm_Tom_Timer *tim){
    return IfxGtm_Tom_Ch_getClockFrequency(tim->gtm, tim->tom, tim->timerChannel);
}
