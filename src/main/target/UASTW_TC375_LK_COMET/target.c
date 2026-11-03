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
 * @file target.c
 * @author Jakob Frenzel (jakob.frenzel@hotmail.com)
 * @brief target configuration for FHTW COMET drone
 * @date 2025-09-22
 */

#include <stdint.h>

#include "platform.h"

#include "drivers/io.h"
#include "drivers/timer.h"

timerHardware_t timerHardware[] = {
    DEF_TIM(atomDriver[0], IfxGtm_ATOM1_1_TOUT10_P00_1_OUT,     MOTOR_1_PIN, IOCFG_OUT_PP, TIM_USE_MOTOR),
    DEF_TIM(atomDriver[1], IfxGtm_ATOM1_5_TOUT15_P00_6_OUT,     MOTOR_2_PIN, IOCFG_OUT_PP, TIM_USE_MOTOR),
    DEF_TIM(atomDriver[2], IfxGtm_ATOM0_2_TOUT12_P00_3_OUT,     MOTOR_3_PIN, IOCFG_OUT_PP, TIM_USE_MOTOR),
    DEF_TIM(atomDriver[3], IfxGtm_ATOM2_0_TOUT18_P00_9_OUT,     MOTOR_4_PIN, IOCFG_OUT_PP, TIM_USE_MOTOR),
    DEF_TIM(atomDriver[4], IfxGtm_ATOM0_4_TOUT14_P00_5_OUT,     MOTOR_5_PIN, IOCFG_OUT_PP, TIM_USE_MOTOR),
    DEF_TIM(atomDriver[5], IfxGtm_ATOM1_6_TOUT16_P00_7_OUT,     MOTOR_6_PIN, IOCFG_OUT_PP, TIM_USE_MOTOR),
};

const int timerHardwareCount = sizeof(timerHardware) / sizeof(timerHardware[0]);
