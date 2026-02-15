# Fault Insertion Unit (FIU)

The FIU is a software-based fault injection module for the AURIX TC375 flight controller, developed as part of a Master Thesis project on embedded fault insertion for unmanned aerial systems (UAS).

## Purpose

The FIU allows selective disabling of individual motors during flight. This enables controlled fault injection testing to evaluate how the flight controller handles motor failures, without requiring hardware modifications.

## How it works

The FIU operates at the PWM driver level (`drivers/pwm_output.c`). When a motor is marked as disabled, the `pwmWriteMotor()` function intercepts the motor command and sends a DSHOT disarm command (value 0) instead of the calculated mixer output. The PID controller and mixer remain unmodified, preserving INAV upgrade compatibility.

### Motor disable via Global Variables

The FIU uses INAV Global Variable **GV0** as a bitmask to select which motors to disable:

| Bit | Motor |
|-----|-------|
| 0   | Motor 0 |
| 1   | Motor 1 |
| 2   | Motor 2 |
| 3   | Motor 3 |
| 4   | Motor 4 |
| 5   | Motor 5 |

**Examples:**
- `GV0 = 7` (0b000111) - disable motors 0, 1, 2
- `GV0 = 56` (0b111000) - disable motors 3, 4, 5
- `GV0 = 63` (0b111111) - disable all 6 motors

GV0 is configured via **Logic Conditions** in the INAV Configurator, allowing runtime activation without recompilation.

### Update frequency

`fiuUpdateFromGlobalVars()` is called at 100 Hz from `taskUpdateAux()` in `fc/fc_tasks.c`. This reads the current GV0 value and updates the internal motor disable flags.

## API

| Function | Description |
|----------|-------------|
| `fiuUpdateFromGlobalVars()` | Reads GV0 bitmask and updates motor disable flags. Called at 100 Hz. |
| `fiuIsMotorDisabled(uint8_t motorIndex)` | Returns `true` if the given motor should be disabled. Called by PWM driver. |

## Enabling FIU

The FIU is conditionally compiled using the `USE_FIU` preprocessor macro. To enable it, add `USE_FIU` as a compiler define in the Aurix Development Studio project settings. See the [How to build](../../../../../How_to_build.md#build-with-fault-insertion-unit-fiu) documentation for instructions.

## Files

- `fiu.h` - FIU types, constants, and function declarations
- `fiu.c` - FIU implementation

## Modified INAV files

- `drivers/pwm_output.c` - `fiuIsMotorDisabled()` check in `pwmWriteMotor()`
- `fc/fc_tasks.c` - `fiuUpdateFromGlobalVars()` call in `taskUpdateAux()`
- `flight/mixer.c` - **unmodified** (original INAV code)