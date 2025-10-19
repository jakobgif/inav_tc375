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
 * @file config.c
 * @author Jakob Frenzel (jakob.frenzel@hotmail.com)
 * @brief default config for drone
 * @date 2025-09-27
 */

#include "platform.h"
#include "io/serial.h"
#include "log.h"
#include "navigation/navigation.h"
#include "navigation/navigation_pos_estimator_private.h"
#include "flight/mixer.h"
#include "flight/ez_tune.h"
#include "config/config_master.h"
#include "drivers/pwm_mapping.h"
#include "sensors/acceleration.h"
#include "sensors/barometer.h"
#include "sensors/gyro.h"
#include "fc/rc_modes.h"
#include "sensors/battery.h"
#include "blackbox.h"

void targetConfiguration(void){
    serialConfigMutable()->portConfigs[findSerialPortIndexByIdentifier(SERIAL_PORT_USART1)].functionMask = FUNCTION_MSP;

    //blackbox
    serialConfigMutable()->portConfigs[findSerialPortIndexByIdentifier(SERIAL_PORT_USART3)].functionMask = FUNCTION_BLACKBOX;
    serialConfigMutable()->portConfigs[findSerialPortIndexByIdentifier(SERIAL_PORT_USART3)].peripheral_baudrateIndex = BAUD_250000;
    blackboxConfigMutable()->rate_denom = 32; //log 1/32 loop iterations
    blackboxIncludeFlagClear(UINT32_MAX); //clear all flags, only log minimum

    //log
    serialConfigMutable()->portConfigs[findSerialPortIndexByIdentifier(SERIAL_PORT_USART4)].functionMask = FUNCTION_LOG;
    logConfigMutable()->level = LOG_LEVEL_DEBUG; //set to max
    logConfigMutable()->topics = 4294967295; //all topics

    //inav default is GPS but we dont have GPS
    positionEstimationConfigMutable()->default_alt_sensor = ALTITUDE_SOURCE_BARO_ONLY;

    //to show Y6 drone in configurator
    mixerConfigMutable()->appliedMixerPreset = 6;

    //set mixer for Y6
    *primaryMotorMixerMutable(0) = (motorMixer_t){ 1.000f,  0.000f,  1.333f,  1.000f };
    *primaryMotorMixerMutable(1) = (motorMixer_t){ 1.000f, -1.000f, -0.667f, -1.000f };
    *primaryMotorMixerMutable(2) = (motorMixer_t){ 1.000f,  1.000f, -0.667f, -1.000f };
    *primaryMotorMixerMutable(3) = (motorMixer_t){ 1.000f,  0.000f,  1.333f, -1.000f };
    *primaryMotorMixerMutable(4) = (motorMixer_t){ 1.000f, -1.000f, -0.667f,  1.000f };
    *primaryMotorMixerMutable(5) = (motorMixer_t){ 1.000f,  1.000f, -0.667f,  1.000f };

    //ESC
    mixerConfigMutable()->motorstopOnLow = FALSE;
    motorConfigMutable()->motorPwmProtocol = PWM_TYPE_DSHOT300;

    //RX
    serialConfigMutable()->portConfigs[findSerialPortIndexByIdentifier(SERIAL_PORT_USART2)].functionMask = FUNCTION_RX_SERIAL;
    rxConfigMutable()->receiverType = RX_TYPE_SERIAL;
    rxConfigMutable()->serialrx_provider = SERIALRX_IBUS;

    //Modes config
    //arm
    modeActivationCondition_t *mode = modeActivationConditionsMutable(0);
    mode->modeId = BOXARM;
    mode->auxChannelIndex = 3;
    mode->range.startStep = CHANNEL_VALUE_TO_STEP(1950);
    mode->range.endStep   = CHANNEL_VALUE_TO_STEP(2050);
    //angled mode
    mode = modeActivationConditionsMutable(1);
    mode->modeId = BOXANGLE;
    mode->auxChannelIndex = 3;
    mode->range.startStep = CHANNEL_VALUE_TO_STEP(900);
    mode->range.endStep   = CHANNEL_VALUE_TO_STEP(2100);

    //sensors
    accelerometerConfigMutable()->acc_hardware = 7; //BMI088
    barometerConfigMutable()->baro_hardware = 9; //DPS310

    //fail safe
    failsafeConfigMutable()->failsafe_procedure = FAILSAFE_PROCEDURE_DROP_IT;

    //motor arm idle throttle
    for (uint8_t i = 0; i < MAX_BATTERY_PROFILE_COUNT; ++i) {
        batteryProfile_t *profile = (batteryProfile_t *)batteryProfiles(i);
        profile->motor.throttleIdle = 7;
    }

    //PID
    ezTuneMutable()->enabled = TRUE;
}