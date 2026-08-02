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
    //
    // === LAYOUT ARCHITECTURE ===
    // CH7 (3-way switch) selects the active fault layout:
    //   CH7 ~1000 → Layout 1: Motor Fault   (CH5/CH6 = motor combos)
    //   CH7 ~1500 → Layout 2: Bus Fault     (CH5=I2C, CH6=SPI, CH9=I2C rate, CH10=SPI rate)
    //   CH7 ~2000 → Layout 3: RC Loss + Battery (CH5=RC loss, CH6+CH10=battery fault)
    //
    // LC 0–6 reset ALL GVs to OFF every cycle (fires first).
    // Layout-specific LCs at higher indices override the reset values.
    // Mutual exclusion: switching layout immediately clears previous GVs.

    ////////////////////////////////////////////////////////////////////
    // RESET PHASE (LC 0–6) — always evaluated, resets all GVs to OFF
    // These fire first; layout-specific LCs at higher indices override them.
    ////////////////////////////////////////////////////////////////////

    // LC0: GV0 = 0 (motor fault OFF)
    logicConditionsMutable(0)->enabled        = 1;
    logicConditionsMutable(0)->activatorId    = -1;
    logicConditionsMutable(0)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(0)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(0)->operandA.value = 0;   // GV0
    logicConditionsMutable(0)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(0)->operandB.value = 0;
    logicConditionsMutable(0)->flags          = 0;

    // LC1: GV1 = 0 (I2C bus select OFF)
    logicConditionsMutable(1)->enabled        = 1;
    logicConditionsMutable(1)->activatorId    = -1;
    logicConditionsMutable(1)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(1)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(1)->operandA.value = 1;   // GV1
    logicConditionsMutable(1)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(1)->operandB.value = 0;
    logicConditionsMutable(1)->flags          = 0;

    // LC2: GV2 = 0 (SPI bus select OFF)
    logicConditionsMutable(2)->enabled        = 1;
    logicConditionsMutable(2)->activatorId    = -1;
    logicConditionsMutable(2)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(2)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(2)->operandA.value = 2;   // GV2
    logicConditionsMutable(2)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(2)->operandB.value = 0;
    logicConditionsMutable(2)->flags          = 0;

    // LC3: GV3 = 1000 (I2C rate OFF — fiu.c clamps to 1000, maps to 0%)
    logicConditionsMutable(3)->enabled        = 1;
    logicConditionsMutable(3)->activatorId    = -1;
    logicConditionsMutable(3)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(3)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(3)->operandA.value = 3;   // GV3
    logicConditionsMutable(3)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(3)->operandB.value = 1000;
    logicConditionsMutable(3)->flags          = 0;

    // LC4: GV4 = 1000 (SPI rate OFF)
    logicConditionsMutable(4)->enabled        = 1;
    logicConditionsMutable(4)->activatorId    = -1;
    logicConditionsMutable(4)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(4)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(4)->operandA.value = 4;   // GV4
    logicConditionsMutable(4)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(4)->operandB.value = 1000;
    logicConditionsMutable(4)->flags          = 0;

    // LC5: GV5 = 0 (RC loss OFF)
    logicConditionsMutable(5)->enabled        = 1;
    logicConditionsMutable(5)->activatorId    = -1;
    logicConditionsMutable(5)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(5)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(5)->operandA.value = 5;   // GV5
    logicConditionsMutable(5)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(5)->operandB.value = 0;
    logicConditionsMutable(5)->flags          = 0;

    // LC6: GV6 = 1000 (battery fault OFF — fiu.c: <1250 → level 0)
    logicConditionsMutable(6)->enabled        = 1;
    logicConditionsMutable(6)->activatorId    = -1;
    logicConditionsMutable(6)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(6)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(6)->operandA.value = 6;   // GV6
    logicConditionsMutable(6)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(6)->operandB.value = 1000;
    logicConditionsMutable(6)->flags          = 0;

    ////////////////////////////////////////////////////////////////////
    // LAYOUT DETECTION (LC 7–10)
    // CH7 3-way switch position determines the active fault layout
    ////////////////////////////////////////////////////////////////////

    // LC7: CH7 < 1250 → Layout 1 active (motor fault)
    logicConditionsMutable(7)->enabled        = 1;
    logicConditionsMutable(7)->activatorId    = -1;
    logicConditionsMutable(7)->operation      = LOGIC_CONDITION_LOWER_THAN;
    logicConditionsMutable(7)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(7)->operandA.value = 7;   // CH7
    logicConditionsMutable(7)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(7)->operandB.value = 1250;
    logicConditionsMutable(7)->flags          = 0;

    // LC8: CH7 > 1750 → Layout 3 active (RC loss + battery)
    logicConditionsMutable(8)->enabled        = 1;
    logicConditionsMutable(8)->activatorId    = -1;
    logicConditionsMutable(8)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(8)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(8)->operandA.value = 7;   // CH7
    logicConditionsMutable(8)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(8)->operandB.value = 1750;
    logicConditionsMutable(8)->flags          = 0;

    // LC9: CH7 > 1250 (lower bound for Layout 2 detection)
    logicConditionsMutable(9)->enabled        = 1;
    logicConditionsMutable(9)->activatorId    = -1;
    logicConditionsMutable(9)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(9)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(9)->operandA.value = 7;   // CH7
    logicConditionsMutable(9)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(9)->operandB.value = 1250;
    logicConditionsMutable(9)->flags          = 0;

    // LC10: if LC9 AND CH7 < 1750 → Layout 2 active (bus fault)
    logicConditionsMutable(10)->enabled        = 1;
    logicConditionsMutable(10)->activatorId    = 9;
    logicConditionsMutable(10)->operation      = LOGIC_CONDITION_LOWER_THAN;
    logicConditionsMutable(10)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(10)->operandA.value = 7;   // CH7
    logicConditionsMutable(10)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(10)->operandB.value = 1750;
    logicConditionsMutable(10)->flags          = 0;

    ////////////////////////////////////////////////////////////////////
    // LAYOUT 1: MOTOR FAULT (LC 11–16)
    // CH5 alone ON → GV0 = 1  (motor 0 off, bit 0)
    // CH6 alone ON → GV0 = 48 (motors 4+5 off, bit4|bit5)
    // Both ON      → GV0 stays 0 (placeholder — future third combo)
    ////////////////////////////////////////////////////////////////////

    // LC11: if L1(LC7) AND CH5 > 1500
    logicConditionsMutable(11)->enabled        = 1;
    logicConditionsMutable(11)->activatorId    = 7;
    logicConditionsMutable(11)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(11)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(11)->operandA.value = 5;  // CH5
    logicConditionsMutable(11)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(11)->operandB.value = 1500;
    logicConditionsMutable(11)->flags          = 0;

    // LC12: if LC11 AND CH6 < 1500 → CH5 alone ON
    logicConditionsMutable(12)->enabled        = 1;
    logicConditionsMutable(12)->activatorId    = 11;
    logicConditionsMutable(12)->operation      = LOGIC_CONDITION_LOWER_THAN;
    logicConditionsMutable(12)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(12)->operandA.value = 6;  // CH6
    logicConditionsMutable(12)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(12)->operandB.value = 1500;
    logicConditionsMutable(12)->flags          = 0;

    // LC13: if LC12 → GV0 = 1 (motor 0 off)
    logicConditionsMutable(13)->enabled        = 1;
    logicConditionsMutable(13)->activatorId    = 12;
    logicConditionsMutable(13)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(13)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(13)->operandA.value = 0;  // GV0
    logicConditionsMutable(13)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(13)->operandB.value = 1;
    logicConditionsMutable(13)->flags          = 0;

    // LC14: if L1(LC7) AND CH6 > 1500
    logicConditionsMutable(14)->enabled        = 1;
    logicConditionsMutable(14)->activatorId    = 7;
    logicConditionsMutable(14)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(14)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(14)->operandA.value = 6;  // CH6
    logicConditionsMutable(14)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(14)->operandB.value = 1500;
    logicConditionsMutable(14)->flags          = 0;

    // LC15: if LC14 AND CH5 < 1500 → CH6 alone ON
    logicConditionsMutable(15)->enabled        = 1;
    logicConditionsMutable(15)->activatorId    = 14;
    logicConditionsMutable(15)->operation      = LOGIC_CONDITION_LOWER_THAN;
    logicConditionsMutable(15)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(15)->operandA.value = 5;  // CH5
    logicConditionsMutable(15)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(15)->operandB.value = 1500;
    logicConditionsMutable(15)->flags          = 0;

    // LC16: if LC15 → GV0 = 48 (motors 4+5 off)
    logicConditionsMutable(16)->enabled        = 1;
    logicConditionsMutable(16)->activatorId    = 15;
    logicConditionsMutable(16)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(16)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(16)->operandA.value = 0;  // GV0
    logicConditionsMutable(16)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(16)->operandB.value = 48;
    logicConditionsMutable(16)->flags          = 0;

    ////////////////////////////////////////////////////////////////////
    // LAYOUT 2: BUS FAULT (LC 17–22)
    // CH5 → I2C bus select (GV1=1), CH9 (knob1) → I2C error rate (GV3)
    // CH6 → SPI bus select (GV2=4), CH10 (knob2) → SPI error rate (GV4)
    // Rates pass through always in L2; bus select gates actual injection.
    ////////////////////////////////////////////////////////////////////

    // LC17: if L2(LC10) AND CH5 > 1500 → I2C bus enable
    logicConditionsMutable(17)->enabled        = 1;
    logicConditionsMutable(17)->activatorId    = 10;
    logicConditionsMutable(17)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(17)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(17)->operandA.value = 5;  // CH5
    logicConditionsMutable(17)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(17)->operandB.value = 1500;
    logicConditionsMutable(17)->flags          = 0;

    // LC18: if LC17 → GV1 = 1 (I2C bus 1 selected)
    logicConditionsMutable(18)->enabled        = 1;
    logicConditionsMutable(18)->activatorId    = 17;
    logicConditionsMutable(18)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(18)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(18)->operandA.value = 1;  // GV1
    logicConditionsMutable(18)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(18)->operandB.value = 1;
    logicConditionsMutable(18)->flags          = 0;

    // LC19: if L2(LC10) → GV3 = CH9 (I2C rate passthrough, always active in L2)
    logicConditionsMutable(19)->enabled        = 1;
    logicConditionsMutable(19)->activatorId    = 10;
    logicConditionsMutable(19)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(19)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(19)->operandA.value = 3;  // GV3
    logicConditionsMutable(19)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(19)->operandB.value = 9;  // CH9 (knob1)
    logicConditionsMutable(19)->flags          = 0;

    // LC20: if L2(LC10) AND CH6 > 1500 → SPI bus enable
    logicConditionsMutable(20)->enabled        = 1;
    logicConditionsMutable(20)->activatorId    = 10;
    logicConditionsMutable(20)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(20)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(20)->operandA.value = 6;  // CH6
    logicConditionsMutable(20)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(20)->operandB.value = 1500;
    logicConditionsMutable(20)->flags          = 0;

    // LC21: if LC20 → GV2 = 4 (SPI3 selected, bit2=4)
    logicConditionsMutable(21)->enabled        = 1;
    logicConditionsMutable(21)->activatorId    = 20;
    logicConditionsMutable(21)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(21)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(21)->operandA.value = 2;  // GV2
    logicConditionsMutable(21)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(21)->operandB.value = 4;
    logicConditionsMutable(21)->flags          = 0;

    // LC22: if L2(LC10) → GV4 = CH10 (SPI rate passthrough, always active in L2)
    logicConditionsMutable(22)->enabled        = 1;
    logicConditionsMutable(22)->activatorId    = 10;
    logicConditionsMutable(22)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(22)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(22)->operandA.value = 4;  // GV4
    logicConditionsMutable(22)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(22)->operandB.value = 10; // CH10 (knob2)
    logicConditionsMutable(22)->flags          = 0;

    ////////////////////////////////////////////////////////////////////
    // LAYOUT 3: RC LOSS + BATTERY FAULT (LC 23–26)
    // CH5 → RC link loss (GV5=1 when ON)
    // CH6 ON + CH10 (knob2) → battery fault level (GV6 = CH10 raw)
    //   fiu.c converts: <1250=off, 1250–1750=warning, >1750=critical
    ////////////////////////////////////////////////////////////////////

    // LC23: if L3(LC8) AND CH5 > 1500 → RC loss enable
    logicConditionsMutable(23)->enabled        = 1;
    logicConditionsMutable(23)->activatorId    = 8;
    logicConditionsMutable(23)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(23)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(23)->operandA.value = 5;  // CH5
    logicConditionsMutable(23)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(23)->operandB.value = 1500;
    logicConditionsMutable(23)->flags          = 0;

    // LC24: if LC23 → GV5 = 1 (RC loss active)
    logicConditionsMutable(24)->enabled        = 1;
    logicConditionsMutable(24)->activatorId    = 23;
    logicConditionsMutable(24)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(24)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(24)->operandA.value = 5;  // GV5
    logicConditionsMutable(24)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(24)->operandB.value = 1;
    logicConditionsMutable(24)->flags          = 0;

    // LC25: if L3(LC8) AND CH6 > 1500 → battery fault enable
    logicConditionsMutable(25)->enabled        = 1;
    logicConditionsMutable(25)->activatorId    = 8;
    logicConditionsMutable(25)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(25)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(25)->operandA.value = 6;  // CH6
    logicConditionsMutable(25)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(25)->operandB.value = 1500;
    logicConditionsMutable(25)->flags          = 0;

    // LC26: if LC25 → GV6 = CH10 (battery fault level from knob2)
    logicConditionsMutable(26)->enabled        = 1;
    logicConditionsMutable(26)->activatorId    = 25;
    logicConditionsMutable(26)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(26)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(26)->operandA.value = 6;  // GV6
    logicConditionsMutable(26)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(26)->operandB.value = 10; // CH10 (knob2)
    logicConditionsMutable(26)->flags          = 0;

#endif
}