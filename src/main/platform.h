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
#include "Platform_Types.h"
typedef   uint8     uint8_t;
typedef   uint16    uint16_t;
typedef   sint16    int16_t;
typedef   sint32    int32_t;
typedef   uint32    uint32_t;
typedef   uint64    uint64_t;
typedef   boolean   bool;

//GPIOs
#include "IfxPort.h"
#include "IfxPort_PinMap.h"

//flash
#include "IfxFlash.h"

//timer
#include "IfxStm.h"
#include "IfxGtm_Tom_Timer.h"

//uart
#include "IfxAsclin_Asc.h"

//interrupt prios start
#include "target.h"
#define INTPRIO_DISABLED    0   //0 will never be served, 1 is the minimum level

#define INTPRIO_ASCLIN7_TX  66
#define INTPRIO_ASCLIN7_RX  65
#define INTPRIO_ASCLIN6_TX  64
#define INTPRIO_ASCLIN6_RX  63
#define INTPRIO_ASCLIN5_TX  62
#define INTPRIO_ASCLIN5_RX  61
#define INTPRIO_ASCLIN4_TX  60
#define INTPRIO_ASCLIN4_RX  59
#define INTPRIO_ASCLIN3_TX  58
#define INTPRIO_ASCLIN3_RX  56
#define INTPRIO_ASCLIN2_TX  55
#define INTPRIO_ASCLIN2_RX  54
#define INTPRIO_ASCLIN1_TX  53
#define INTPRIO_ASCLIN1_RX  52
#define INTPRIO_ASCLIN0_TX  51
#define INTPRIO_ASCLIN0_RX  50

#define INTPRIO_GTM_TOM_15   100
#define INTPRIO_GTM_TOM_14   101
#define INTPRIO_GTM_TOM_13   102
#define INTPRIO_GTM_TOM_12   103
#define INTPRIO_GTM_TOM_11   104
#define INTPRIO_GTM_TOM_10   105
#define INTPRIO_GTM_TOM_09   106
#define INTPRIO_GTM_TOM_08   107
#define INTPRIO_GTM_TOM_07   108
#define INTPRIO_GTM_TOM_06   109
#define INTPRIO_GTM_TOM_05   110
#define INTPRIO_GTM_TOM_04   111
#define INTPRIO_GTM_TOM_03   112
#define INTPRIO_GTM_TOM_02   113
#define INTPRIO_GTM_TOM_01   114
#define INTPRIO_GTM_TOM_00   115

#define INTPRIO_MAX          255 //max will be served first
//interrupt prios end

//copied from reference project
#define U_ID_0 (((*(uint32_t*)&SCU_CHIPID) & IFX_SCU_CHIPID_CHREV_MSK) >> IFX_SCU_CHIPID_CHREV_OFF)
#define U_ID_1 (((*(uint32_t*)&SCU_CHIPID) & IFX_SCU_CHIPID_CHTEC_MSK) >> IFX_SCU_CHIPID_CHTEC_OFF)
#define U_ID_2 (((*(uint32_t*)&SCU_CHIPID) & IFX_SCU_CHIPID_CHID_MSK) >> IFX_SCU_CHIPID_CHID_OFF)

#define __NOP(n) __nop(n) //some inav functions use __NOP() but aurix only has __nop()

#define AURIX_CLEAR_DFLASH_ON_SYSTEM_INIT

#define MCU_FLASH_SIZE  IFXFLASH_PFLASH_SIZE

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
