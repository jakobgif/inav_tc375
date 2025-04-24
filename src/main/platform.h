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

#pragma once

#if defined(STM32H7)
#include "stm32h7xx.h"
#include "stm32h7xx_hal.h"
#include "system_stm32h7xx.h"

#include "stm32h7xx_ll_spi.h"
#include "stm32h7xx_ll_gpio.h"
#include "stm32h7xx_ll_dma.h"
#include "stm32h7xx_ll_rcc.h"
#include "stm32h7xx_ll_bus.h"
#include "stm32h7xx_ll_tim.h"
#include "stm32h7xx_ll_system.h"

// Chip Unique ID on H7
#define U_ID_0 (*(uint32_t*)UID_BASE)
#define U_ID_1 (*(uint32_t*)(UID_BASE + 4))
#define U_ID_2 (*(uint32_t*)(UID_BASE + 8))

#elif defined(STM32F7)
#include "stm32f7xx.h"
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_rtc.h"
#include "stm32f7xx_ll_spi.h"
#include "stm32f7xx_ll_gpio.h"
#include "stm32f7xx_ll_dma.h"
#include "stm32f7xx_ll_rcc.h"
#include "stm32f7xx_ll_bus.h"
#include "stm32f7xx_ll_tim.h"

// Chip Unique ID on F7
#if defined(STM32F722xx)
#define U_ID_0 (*(uint32_t*)0x1ff07a10)
#define U_ID_1 (*(uint32_t*)0x1ff07a14)
#define U_ID_2 (*(uint32_t*)0x1ff07a18)
#else
#define U_ID_0 (*(uint32_t*)0x1ff0f420)
#define U_ID_1 (*(uint32_t*)0x1ff0f424)
#define U_ID_2 (*(uint32_t*)0x1ff0f428)
#endif

#elif defined(AT32F43x)
#include "at32f435_437.h"  

#define U_ID_0 (*(uint32_t*)0x1FFFF7E8)
#define U_ID_1 (*(uint32_t*)0x1FFFF7EC)
#define U_ID_2 (*(uint32_t*)0x1FFFF7F0)
typedef enum
{
  DISABLE = 0,
  ENABLE = !DISABLE
} FunctionalState;
#define IS_FUNCTIONAL_STATE(STATE) (((STATE) == DISABLE) || ((STATE) == ENABLE))

#elif defined(STM32F4)
#include "stm32f4xx.h"

// Chip Unique ID on F405
#define U_ID_0 (*(uint32_t*)0x1fff7a10)
#define U_ID_1 (*(uint32_t*)0x1fff7a14)
#define U_ID_2 (*(uint32_t*)0x1fff7a18)

#elif defined(TC375)
//#warning Building for TC375
//types
#include "Libraries\iLLD\TC37A\Tricore\Cpu\Std\Platform_Types.h"
typedef   uint8     uint8_t;
typedef   uint16    uint16_t;
typedef   uint32    uint32_t;
typedef   uint64    uint64_t;
//typedef   boolean   bool;

//GPIOs
#include "Libraries\iLLD\TC37A\Tricore\Port\Std\IfxPort.h"
#include "Libraries\iLLD\TC37A\Tricore\_PinMap\IfxPort_PinMap.h"

//flash
#include "Libraries\iLLD\TC37A\Tricore\Flash\Std\IfxFlash.h"

//timer
#include "Libraries\iLLD\TC37A\Tricore\Stm\Std\IfxStm.h"
#include "Libraries\iLLD\TC37A\Tricore\Gtm\Tom\Timer\IfxGtm_Tom_Timer.h"

//uart
#include "Libraries\iLLD\TC37A\Tricore\Asclin\Asc\IfxAsclin_Asc.h"

#include "target.h"
#define INTPRIO_DISABLED    0   //0 will never be served, 1 is the minimum level

#ifdef USE_UART1
#define INTPRIO_ASCLIN0_TX  50
#define INTPRIO_ASCLIN0_RX  51
#endif

#define INTPRIO_GTM_TOM_0   100

#define INTPRIO_MAX         255 //max will be served first

#endif

#include "target/common.h"
#include "target.h"
#include "target/sanity_check.h"
#include "target/common_post.h"

// Remove the unaligned packed structure member pointer access warning
// The compiler guarantees that unaligned access is safe for packed structures.

#if (__GNUC__ >= 9)
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#endif
