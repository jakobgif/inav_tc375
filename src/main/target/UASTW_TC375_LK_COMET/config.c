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
    // === SLOT ARCHITECTURE (2026 redesign) ===
    // 6 physical controller slots: 3 RC layers (CH7 3-way switch selects
    // which) x 2 switches (CH5/CH6) each. Every slot GV here is a plain 0/1
    // activation flag -- WHICH fault type a slot triggers, and (for Motor)
    // which motor(s) it targets, is a runtime CLI setting (fiu_slot1..
    // fiu_slot6 / fiu_slot1_motor..fiu_slot6_motor, fiu/fiu_config.h), looked
    // up by fiuUpdateFromGlobalVars() (fiu.c) -- NOT encoded here. This lets
    // the 6 slots be reassigned to any of the 8 fault types via `set`/`diff`
    // without touching this file or reflashing. See guidelines_lc.md.
    //
    //   CH7 ~1000 → Layer 1: slots GV0 (CH5), GV1 (CH6)
    //   CH7 ~1500 → Layer 2: slots GV2 (CH5), GV3 (CH6)
    //   CH7 ~2000 → Layer 3: slots GV4 (CH5), GV5 (CH6)
    //   CH9 / CH10 → GV6 / GV7, unconditional raw passthrough (the two
    //   shared knobs; meaning depends entirely on which slot type is active)
    //
    // LC 0–5 reset all 6 slot GVs to OFF every cycle (fires first).
    // Per-slot LCs at higher indices override the reset value for exactly
    // one slot. Within a layer, both switches ON (or both OFF) leaves both
    // slot GVs at 0 -- fiu.c treats that as "nothing active in this layer"
    // (the per-layer mutual-exclusion safety reset).

    ////////////////////////////////////////////////////////////////////
    // RESET PHASE (LC 0–5) — always evaluated, resets all 6 slot GVs to OFF
    // These fire first; per-slot LCs at higher indices override them.
    ////////////////////////////////////////////////////////////////////

    // LC0: GV0 = 0 (slot: layer 1, switch 1 / CH5)
    logicConditionsMutable(0)->enabled        = 1;
    logicConditionsMutable(0)->activatorId    = -1;
    logicConditionsMutable(0)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(0)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(0)->operandA.value = 0;   // GV0
    logicConditionsMutable(0)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(0)->operandB.value = 0;
    logicConditionsMutable(0)->flags          = 0;

    // LC1: GV1 = 0 (slot: layer 1, switch 2 / CH6)
    logicConditionsMutable(1)->enabled        = 1;
    logicConditionsMutable(1)->activatorId    = -1;
    logicConditionsMutable(1)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(1)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(1)->operandA.value = 1;   // GV1
    logicConditionsMutable(1)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(1)->operandB.value = 0;
    logicConditionsMutable(1)->flags          = 0;

    // LC2: GV2 = 0 (slot: layer 2, switch 1 / CH5)
    logicConditionsMutable(2)->enabled        = 1;
    logicConditionsMutable(2)->activatorId    = -1;
    logicConditionsMutable(2)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(2)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(2)->operandA.value = 2;   // GV2
    logicConditionsMutable(2)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(2)->operandB.value = 0;
    logicConditionsMutable(2)->flags          = 0;

    // LC3: GV3 = 0 (slot: layer 2, switch 2 / CH6)
    logicConditionsMutable(3)->enabled        = 1;
    logicConditionsMutable(3)->activatorId    = -1;
    logicConditionsMutable(3)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(3)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(3)->operandA.value = 3;   // GV3
    logicConditionsMutable(3)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(3)->operandB.value = 0;
    logicConditionsMutable(3)->flags          = 0;

    // LC4: GV4 = 0 (slot: layer 3, switch 1 / CH5)
    logicConditionsMutable(4)->enabled        = 1;
    logicConditionsMutable(4)->activatorId    = -1;
    logicConditionsMutable(4)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(4)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(4)->operandA.value = 4;   // GV4
    logicConditionsMutable(4)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(4)->operandB.value = 0;
    logicConditionsMutable(4)->flags          = 0;

    // LC5: GV5 = 0 (slot: layer 3, switch 2 / CH6)
    logicConditionsMutable(5)->enabled        = 1;
    logicConditionsMutable(5)->activatorId    = -1;
    logicConditionsMutable(5)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(5)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(5)->operandA.value = 5;   // GV5
    logicConditionsMutable(5)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(5)->operandB.value = 0;
    logicConditionsMutable(5)->flags          = 0;

    ////////////////////////////////////////////////////////////////////
    // LAYER DETECTION (LC 6–9)
    // CH7 3-way switch position determines the active layer
    ////////////////////////////////////////////////////////////////////

    // LC6: CH7 < 1250 → Layer 1 active
    logicConditionsMutable(6)->enabled        = 1;
    logicConditionsMutable(6)->activatorId    = -1;
    logicConditionsMutable(6)->operation      = LOGIC_CONDITION_LOWER_THAN;
    logicConditionsMutable(6)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(6)->operandA.value = 7;   // CH7
    logicConditionsMutable(6)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(6)->operandB.value = 1250;
    logicConditionsMutable(6)->flags          = 0;

    // LC7: CH7 > 1750 → Layer 3 active
    logicConditionsMutable(7)->enabled        = 1;
    logicConditionsMutable(7)->activatorId    = -1;
    logicConditionsMutable(7)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(7)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(7)->operandA.value = 7;   // CH7
    logicConditionsMutable(7)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(7)->operandB.value = 1750;
    logicConditionsMutable(7)->flags          = 0;

    // LC8: CH7 > 1250 (lower bound for Layer 2 detection)
    logicConditionsMutable(8)->enabled        = 1;
    logicConditionsMutable(8)->activatorId    = -1;
    logicConditionsMutable(8)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(8)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(8)->operandA.value = 7;   // CH7
    logicConditionsMutable(8)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(8)->operandB.value = 1250;
    logicConditionsMutable(8)->flags          = 0;

    // LC9: if LC8 AND CH7 < 1750 → Layer 2 active
    logicConditionsMutable(9)->enabled        = 1;
    logicConditionsMutable(9)->activatorId    = 8;
    logicConditionsMutable(9)->operation      = LOGIC_CONDITION_LOWER_THAN;
    logicConditionsMutable(9)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(9)->operandA.value = 7;   // CH7
    logicConditionsMutable(9)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(9)->operandB.value = 1750;
    logicConditionsMutable(9)->flags          = 0;

    ////////////////////////////////////////////////////////////////////
    // SLOT ACTIVATION (LC 10–21) — 2 LCs per slot: a condition (layer
    // active AND its switch ON) and, chained off it, the GVAR_SET that
    // flips the slot's GV to 1. No "OFF" LC needed -- the reset phase
    // above already provides the 0 default.
    ////////////////////////////////////////////////////////////////////

    // LC10: if Layer1(LC6) AND CH5 > 1500 → slot L1-SW1 condition
    logicConditionsMutable(10)->enabled        = 1;
    logicConditionsMutable(10)->activatorId    = 6;
    logicConditionsMutable(10)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(10)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(10)->operandA.value = 5;  // CH5
    logicConditionsMutable(10)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(10)->operandB.value = 1500;
    logicConditionsMutable(10)->flags          = 0;

    // LC11: if LC10 → GV0 = 1
    logicConditionsMutable(11)->enabled        = 1;
    logicConditionsMutable(11)->activatorId    = 10;
    logicConditionsMutable(11)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(11)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(11)->operandA.value = 0;  // GV0
    logicConditionsMutable(11)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(11)->operandB.value = 1;
    logicConditionsMutable(11)->flags          = 0;

    // LC12: if Layer1(LC6) AND CH6 > 1500 → slot L1-SW2 condition
    logicConditionsMutable(12)->enabled        = 1;
    logicConditionsMutable(12)->activatorId    = 6;
    logicConditionsMutable(12)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(12)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(12)->operandA.value = 6;  // CH6
    logicConditionsMutable(12)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(12)->operandB.value = 1500;
    logicConditionsMutable(12)->flags          = 0;

    // LC13: if LC12 → GV1 = 1
    logicConditionsMutable(13)->enabled        = 1;
    logicConditionsMutable(13)->activatorId    = 12;
    logicConditionsMutable(13)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(13)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(13)->operandA.value = 1;  // GV1
    logicConditionsMutable(13)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(13)->operandB.value = 1;
    logicConditionsMutable(13)->flags          = 0;

    // LC14: if Layer2(LC9) AND CH5 > 1500 → slot L2-SW1 condition
    logicConditionsMutable(14)->enabled        = 1;
    logicConditionsMutable(14)->activatorId    = 9;
    logicConditionsMutable(14)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(14)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(14)->operandA.value = 5;  // CH5
    logicConditionsMutable(14)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(14)->operandB.value = 1500;
    logicConditionsMutable(14)->flags          = 0;

    // LC15: if LC14 → GV2 = 1
    logicConditionsMutable(15)->enabled        = 1;
    logicConditionsMutable(15)->activatorId    = 14;
    logicConditionsMutable(15)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(15)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(15)->operandA.value = 2;  // GV2
    logicConditionsMutable(15)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(15)->operandB.value = 1;
    logicConditionsMutable(15)->flags          = 0;

    // LC16: if Layer2(LC9) AND CH6 > 1500 → slot L2-SW2 condition
    logicConditionsMutable(16)->enabled        = 1;
    logicConditionsMutable(16)->activatorId    = 9;
    logicConditionsMutable(16)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(16)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(16)->operandA.value = 6;  // CH6
    logicConditionsMutable(16)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(16)->operandB.value = 1500;
    logicConditionsMutable(16)->flags          = 0;

    // LC17: if LC16 → GV3 = 1
    logicConditionsMutable(17)->enabled        = 1;
    logicConditionsMutable(17)->activatorId    = 16;
    logicConditionsMutable(17)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(17)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(17)->operandA.value = 3;  // GV3
    logicConditionsMutable(17)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(17)->operandB.value = 1;
    logicConditionsMutable(17)->flags          = 0;

    // LC18: if Layer3(LC7) AND CH5 > 1500 → slot L3-SW1 condition
    logicConditionsMutable(18)->enabled        = 1;
    logicConditionsMutable(18)->activatorId    = 7;
    logicConditionsMutable(18)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(18)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(18)->operandA.value = 5;  // CH5
    logicConditionsMutable(18)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(18)->operandB.value = 1500;
    logicConditionsMutable(18)->flags          = 0;

    // LC19: if LC18 → GV4 = 1
    logicConditionsMutable(19)->enabled        = 1;
    logicConditionsMutable(19)->activatorId    = 18;
    logicConditionsMutable(19)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(19)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(19)->operandA.value = 4;  // GV4
    logicConditionsMutable(19)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(19)->operandB.value = 1;
    logicConditionsMutable(19)->flags          = 0;

    // LC20: if Layer3(LC7) AND CH6 > 1500 → slot L3-SW2 condition
    logicConditionsMutable(20)->enabled        = 1;
    logicConditionsMutable(20)->activatorId    = 7;
    logicConditionsMutable(20)->operation      = LOGIC_CONDITION_GREATER_THAN;
    logicConditionsMutable(20)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(20)->operandA.value = 6;  // CH6
    logicConditionsMutable(20)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(20)->operandB.value = 1500;
    logicConditionsMutable(20)->flags          = 0;

    // LC21: if LC20 → GV5 = 1
    logicConditionsMutable(21)->enabled        = 1;
    logicConditionsMutable(21)->activatorId    = 20;
    logicConditionsMutable(21)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(21)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(21)->operandA.value = 5;  // GV5
    logicConditionsMutable(21)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(21)->operandB.value = 1;
    logicConditionsMutable(21)->flags          = 0;

    ////////////////////////////////////////////////////////////////////
    // KNOB PASSTHROUGH (LC 22–23) — unconditional, not layer-gated.
    // Both knobs are always forwarded raw; only the currently active
    // slot's type (fiu.c) decides whether/how they are used.
    ////////////////////////////////////////////////////////////////////

    // LC22: GV6 = CH9 (Knob A, always)
    logicConditionsMutable(22)->enabled        = 1;
    logicConditionsMutable(22)->activatorId    = -1;
    logicConditionsMutable(22)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(22)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(22)->operandA.value = 6;  // GV6
    logicConditionsMutable(22)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(22)->operandB.value = 9;  // CH9 (Knob A)
    logicConditionsMutable(22)->flags          = 0;

    // LC23: GV7 = CH10 (Knob B, always)
    logicConditionsMutable(23)->enabled        = 1;
    logicConditionsMutable(23)->activatorId    = -1;
    logicConditionsMutable(23)->operation      = LOGIC_CONDITION_GVAR_SET;
    logicConditionsMutable(23)->operandA.type  = LOGIC_CONDITION_OPERAND_TYPE_VALUE;
    logicConditionsMutable(23)->operandA.value = 7;  // GV7
    logicConditionsMutable(23)->operandB.type  = LOGIC_CONDITION_OPERAND_TYPE_RC_CHANNEL;
    logicConditionsMutable(23)->operandB.value = 10; // CH10 (Knob B)
    logicConditionsMutable(23)->flags          = 0;

#endif
}