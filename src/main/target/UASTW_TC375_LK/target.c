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
 * @file target.c
 * @author Jakob Frenzel (jakob.frenzel@hotmail.com)
 * @brief target stuff for FHTW flight controller
 * @date 2025-04-03
 */

#include <stdint.h>

#include "platform.h"

#include "drivers/io.h"
#include "drivers/timer.h"

timerHardware_t timerHardware[] = {
    DEF_TIM(atomDriver[0], IfxGtm_ATOM0_0N_TOUT17_P00_8_OUT,    PWM_MOTOR_1_PIN, IOCFG_OUT_PP, TIM_USE_MOTOR),
    DEF_TIM(atomDriver[1], IfxGtm_ATOM1_1_TOUT10_P00_1_OUT,     PWM_MOTOR_2_PIN, IOCFG_OUT_PP, TIM_USE_MOTOR),
    DEF_TIM(atomDriver[2], IfxGtm_ATOM2_0_TOUT18_P00_9_OUT,     PWM_MOTOR_3_PIN, IOCFG_OUT_PP, TIM_USE_MOTOR),
    DEF_TIM(atomDriver[3], IfxGtm_ATOM3_0_TOUT26_P33_4_OUT,     PWM_MOTOR_4_PIN, IOCFG_OUT_PP, TIM_USE_MOTOR),
    DEF_TIM(atomDriver[4], IfxGtm_ATOM4_0_TOUT48_P22_1_OUT,     PWM_MOTOR_5_PIN, IOCFG_OUT_PP, TIM_USE_MOTOR),
    DEF_TIM(atomDriver[5], IfxGtm_ATOM3_4_TOUT34_P33_12_OUT,    PWM_MOTOR_6_PIN, IOCFG_OUT_PP, TIM_USE_MOTOR),
};

const int timerHardwareCount = sizeof(timerHardware) / sizeof(timerHardware[0]);
