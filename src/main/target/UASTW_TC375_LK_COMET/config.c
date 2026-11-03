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
#include "flight/pid.h"
#include "config/config_master.h"
#include "config/general_settings.h"
#include "drivers/pwm_mapping.h"
#include "sensors/acceleration.h"
#include "sensors/barometer.h"
#include "sensors/gyro.h"
#include "fc/rc_modes.h"
#include "fc/rc_controls.h"
#include "fc/control_profile.h"
#include "sensors/battery.h"
#include "blackbox.h"

void targetConfiguration(void){
    serialConfigMutable()->portConfigs[findSerialPortIndexByIdentifier(SERIAL_PORT_USART1)].functionMask = FUNCTION_MSP;

    //blackbox
    serialConfigMutable()->portConfigs[findSerialPortIndexByIdentifier(SERIAL_PORT_USART3)].functionMask = FUNCTION_BLACKBOX;
    serialConfigMutable()->portConfigs[findSerialPortIndexByIdentifier(SERIAL_PORT_USART3)].peripheral_baudrateIndex = BAUD_250000;
    blackboxConfigMutable()->rate_denom = 4; //log 1/4 loop iterations
    blackboxIncludeFlagClear(UINT32_MAX); //clear all flags, only log minimum
    blackboxIncludeFlagSet(BLACKBOX_FEATURE_MOTORS);

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
        profile->motor.throttleIdle = 8;
    }

    //air mode only active above threshold
    rcControlsConfigMutable()->airmodeHandlingType = THROTTLE_THRESHOLD;

    //gyro based on micoair
    //set gyro_main_lpf_hz = 110
    gyroConfigMutable()->gyro_main_lpf_hz = 110;
    //set gyro_dyn_lpf_min_hz = 85
    gyroConfigMutable()->gyroDynamicLpfMinHz = 85;
    //set gyro_dyn_lpf_max_hz = 300
    gyroConfigMutable()->gyroDynamicLpfMaxHz = 300;
    //set gyro_dyn_lpf_curve_expo = 3
    gyroConfigMutable()->gyroDynamicLpfCurveExpo = 3;
    //set setpoint_kalman_q = 200
    gyroConfigMutable()->kalman_q = 200;

    //PID based on micoair
    //set mc_p_pitch = 40
    pidProfileMutable()->bank_mc.pid[PID_PITCH].P = 40;
    //set mc_i_pitch = 90
    pidProfileMutable()->bank_mc.pid[PID_PITCH].I = 90;
    //set mc_d_pitch = 27
    pidProfileMutable()->bank_mc.pid[PID_PITCH].D = 27;
    //set mc_cd_pitch = 88
    pidProfileMutable()->bank_mc.pid[PID_PITCH].FF = 88;
    //set mc_p_roll = 36
    pidProfileMutable()->bank_mc.pid[PID_ROLL].P = 36;
    //set mc_i_roll = 82
    pidProfileMutable()->bank_mc.pid[PID_ROLL].I = 82;
    //set mc_d_roll = 24
    pidProfileMutable()->bank_mc.pid[PID_ROLL].D = 24;
    //set mc_cd_roll = 80
    pidProfileMutable()->bank_mc.pid[PID_ROLL].FF = 80;
    //set mc_p_yaw = 43
    pidProfileMutable()->bank_mc.pid[PID_YAW].P = 43;
    //set mc_i_yaw = 84
    pidProfileMutable()->bank_mc.pid[PID_YAW].I = 84;
    //set mc_cd_yaw = 90
    pidProfileMutable()->bank_mc.pid[PID_YAW].FF = 90;
    //set d_boost_max =  1.000
    pidProfileMutable()->dBoostMax = 1.000;
    //set antigravity_gain =  2.000
    pidProfileMutable()->antigravityGain = 2.000;
    //set antigravity_accelerator =  5.000
    pidProfileMutable()->antigravityAccelerator = 5.000;

    //controlRateProfiles based on micoair
    for (uint8_t i = 0; i < MAX_CONTROL_PROFILE_COUNT; ++i) {
        controlConfig_t *config = (controlConfig_t *)controlProfiles(i);
        //set tpa_rate = 20
        config->throttle.dynPID = 20;
        //set tpa_breakpoint = 1200
        config->throttle.pa_breakpoint = 1200;
        //set rc_expo = 80
        config->stabilized.rcExpo8 = 80;
        //set rc_yaw_expo = 80
        config->stabilized.rcYawExpo8 = 80;
        //set roll_rate = 70
        config->stabilized.rates[FD_ROLL] = 70;
        //set pitch_rate = 70
        config->stabilized.rates[FD_PITCH] = 70;
        //set yaw_rate = 60
        config->stabilized.rates[FD_YAW] = 60;
    }

    //Ez tune based on micoair
    ezTuneMutable()->enabled = TRUE;
    //set ez_response = 92
    ezTuneMutable()->response = 92;
    //set ez_damping = 108
    ezTuneMutable()->damping = 108;
    //set ez_stability = 110
    ezTuneMutable()->stability = 110;
    //set ez_aggressiveness = 80
    ezTuneMutable()->aggressiveness = 80;
    //set ez_rate = 134
    ezTuneMutable()->rate = 134;
    //set ez_expo = 118
    ezTuneMutable()->expo = 118;

    //indicate that defaults are applied
    generalSettingsMutable()->appliedDefaults = APPLIED_DEFAULTS_CUSTOM;
}