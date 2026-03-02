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
#include "fc/controlrate_profile.h"
#include "sensors/battery.h"
#include "blackbox.h"
#ifdef USE_FIU
#include "programming/logic_condition.h"
#endif

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
    for (uint8_t i = 0; i < MAX_CONTROL_RATE_PROFILE_COUNT; ++i) {
        controlRateConfig_t *config = (controlRateConfig_t *)controlRateProfiles(i);
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

#ifdef USE_FIU
    // Logic conditions for FIU RC-channel activation
    //
    // activatorId = -1  → condition is ALWAYS evaluated (no parent LC required)
    // activatorId =  N  → condition is only evaluated when LC[N] is TRUE
    //
    // operandA/B type:
    //   LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL → operand value = RC channel index
    //   LOGIC_CONDITION_OPERAND_TYPE_VALUE      → operand value = raw integer
    //
    // LOGIC_CONDITION_GVAR_SET: operandA = GVAR index to write,
    //                           operandB = value to write into that GVAR

    ////////////////////////////////////////////////////////////////////
    // MOTOR FAULT  |  GV0 = motor bitmask  |  RC CH7 (AUX6)
    // CH7 > 1500 → GV0 = 1 (fault ON)
    // CH7 < 1500 → GV0 = 0 (fault OFF)
    ////////////////////////////////////////////////////////////////////

    // LC0: CH7 > 1500 → motor fault trigger condition
    logicConditionsMutable(0)->enabled        = 1;
    logicConditionsMutable(0)->activatorId    = -1;   // always evaluated
    logicConditionsMutable(0)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(0)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(0)->operandA.value = 7;    // RC channel 7 (AUX6)
    logicConditionsMutable(0)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(0)->operandB.value = 1500; // threshold: switch HIGH > 1500µs
    logicConditionsMutable(0)->flags          = 0;

    // LC1: Set GV0=1 when LC0 is true (motor fault ON)
    logicConditionsMutable(1)->enabled        = 1;
    logicConditionsMutable(1)->activatorId    = 0;    // only active when LC0 is true
    logicConditionsMutable(1)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(1)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(1)->operandA.value = 0;    // target GVAR index: GV0
    logicConditionsMutable(1)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(1)->operandB.value = 1;    // write value: 1 (fault active)
    logicConditionsMutable(1)->flags          = 0;

    // LC2: CH7 < 1500 → motor fault reset condition
    logicConditionsMutable(2)->enabled        = 1;
    logicConditionsMutable(2)->activatorId    = -1;   // always evaluated
    logicConditionsMutable(2)->operation      = LOGIC_CONDITION_LOWER_THAN;
    logicConditionsMutable(2)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(2)->operandA.value = 7;    // RC channel 7 (AUX6)
    logicConditionsMutable(2)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(2)->operandB.value = 1500; // threshold: switch LOW < 1500µs
    logicConditionsMutable(2)->flags          = 0;

    // LC3: Set GV0=0 when LC2 is true (motor fault OFF)
    logicConditionsMutable(3)->enabled        = 1;
    logicConditionsMutable(3)->activatorId    = 2;    // only active when LC2 is true
    logicConditionsMutable(3)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(3)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(3)->operandA.value = 0;    // target GVAR index: GV0
    logicConditionsMutable(3)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(3)->operandB.value = 0;    // write value: 0 (no fault)
    logicConditionsMutable(3)->flags          = 0;

    ////////////////////////////////////////////////////////////////////
    // I2C BUS FAULT  |  GV1 = I2C bus number  |  RC CH6 (AUX5)
    // CH6 > 1500 → GV1 = 1 (fault I2C bus 1)
    // CH6 < 1500 → GV1 = 0 (no fault)
    ////////////////////////////////////////////////////////////////////

    // LC4: CH6 > 1500 → I2C bus fault trigger condition
    logicConditionsMutable(4)->enabled        = 1;
    logicConditionsMutable(4)->activatorId    = -1;   // always evaluated
    logicConditionsMutable(4)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(4)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(4)->operandA.value = 6;    // RC channel 6 (AUX5)
    logicConditionsMutable(4)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(4)->operandB.value = 1500; // threshold: switch HIGH > 1500µs
    logicConditionsMutable(4)->flags          = 0;

    // LC5: Set GV1=1 when LC4 is true (fault I2C bus 1)
    logicConditionsMutable(5)->enabled        = 1;
    logicConditionsMutable(5)->activatorId    = 4;    // only active when LC4 is true
    logicConditionsMutable(5)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(5)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(5)->operandA.value = 1;    // target GVAR index: GV1
    logicConditionsMutable(5)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(5)->operandB.value = 1;    // write value: 1 (I2C bus 1)
    logicConditionsMutable(5)->flags          = 0;

    // LC6: CH6 < 1500 → I2C bus fault reset condition
    logicConditionsMutable(6)->enabled        = 1;
    logicConditionsMutable(6)->activatorId    = -1;   // always evaluated
    logicConditionsMutable(6)->operation      = LOGIC_CONDITION_LOWER_THAN;
    logicConditionsMutable(6)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(6)->operandA.value = 6;    // RC channel 6 (AUX5)
    logicConditionsMutable(6)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(6)->operandB.value = 1500; // threshold: switch LOW < 1500µs
    logicConditionsMutable(6)->flags          = 0;

    // LC7: Set GV1=0 when LC6 is true (I2C bus fault OFF)
    logicConditionsMutable(7)->enabled        = 1;
    logicConditionsMutable(7)->activatorId    = 6;    // only active when LC6 is true
    logicConditionsMutable(7)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(7)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(7)->operandA.value = 1;    // target GVAR index: GV1
    logicConditionsMutable(7)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(7)->operandB.value = 0;    // write value: 0 (no fault)
    logicConditionsMutable(7)->flags          = 0;

    ////////////////////////////////////////////////////////////////////
    // SPI BUS FAULT  |  GV2 = SPI bus number  |  RC CH5 (AUX4)
    // CH5 > 1500 → GV2 = 4 (fault SPI bus 4)
    // CH5 < 1500 → GV2 = 0 (no fault)
    ////////////////////////////////////////////////////////////////////

    // LC8: CH5 > 1500 → SPI bus fault trigger condition
    logicConditionsMutable(8)->enabled        = 1;
    logicConditionsMutable(8)->activatorId    = -1;   // always evaluated
    logicConditionsMutable(8)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(8)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(8)->operandA.value = 5;    // RC channel 5 (AUX4)
    logicConditionsMutable(8)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(8)->operandB.value = 1500; // threshold: switch HIGH > 1500µs
    logicConditionsMutable(8)->flags          = 0;

    // LC9: Set GV2=4 when LC8 is true (fault SPI bus 4)
    logicConditionsMutable(9)->enabled        = 1;
    logicConditionsMutable(9)->activatorId    = 8;    // only active when LC8 is true
    logicConditionsMutable(9)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(9)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(9)->operandA.value = 2;    // target GVAR index: GV2
    logicConditionsMutable(9)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(9)->operandB.value = 4;    // write value: 4 (SPI bus 4)
    logicConditionsMutable(9)->flags          = 0;

    // LC10: CH5 < 1500 → SPI bus fault reset condition
    logicConditionsMutable(10)->enabled        = 1;
    logicConditionsMutable(10)->activatorId    = -1;   // always evaluated
    logicConditionsMutable(10)->operation      = LOGIC_CONDITION_LOWER_THAN;
    logicConditionsMutable(10)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(10)->operandA.value = 5;    // RC channel 5 (AUX4)
    logicConditionsMutable(10)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(10)->operandB.value = 1500; // threshold: switch LOW < 1500µs
    logicConditionsMutable(10)->flags          = 0;

    // LC11: Set GV2=0 when LC10 is true (SPI bus fault OFF)
    logicConditionsMutable(11)->enabled        = 1;
    logicConditionsMutable(11)->activatorId    = 10;   // only active when LC10 is true
    logicConditionsMutable(11)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(11)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(11)->operandA.value = 2;    // target GVAR index: GV2
    logicConditionsMutable(11)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(11)->operandB.value = 0;    // write value: 0 (no fault)
    logicConditionsMutable(11)->flags          = 0;
#endif
}