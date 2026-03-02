# Fault Insertion Unit (FIU)

The FIU is a software-based fault injection module for INAV-based flight controllers. It enables controlled fault injection during flight to evaluate how the flight controller handles hardware failures, without requiring physical hardware modifications.

## Purpose

Supported fault types:

- **Motor faults**: Selective disabling of individual motors (PWM driver level)
- **Sensor bus faults**: Blocking I2C or SPI bus reads for all devices on the respective bus (bus abstraction level)

## How it works

### Motor fault injection

The FIU operates at the PWM driver level (`drivers/pwm_output.c`). When a motor is marked as disabled, the `pwmWriteMotor()` function intercepts the motor command and sets the target speed to 0 instead of the calculated mixer output. The PID controller and mixer remain unmodified, preserving INAV upgrade compatibility.

### I2C bus fault injection

The FIU hooks into the `BUSTYPE_I2C` branch of `drivers/bus.c`. When active, `busRead()` and `busReadBuf()` return zero-filled data with a success status, without performing any actual I2C transfer. The affected bus is selected by the GV1 bitmask (Bit N blocks I2CDEV_(N+1)). This simulates a bus failure for all devices on the blocked bus.

### SPI bus fault injection

The FIU hooks into the `BUSTYPE_SPI` branch of `drivers/bus.c`. When active, `busRead()` and `busReadBuf()` return zero-filled data with a success status, without performing any actual SPI transfer. The affected bus is selected by the GV2 bitmask (Bit N blocks SPIDEV_(N+1)). This simulates a bus failure for all devices on the blocked bus.

## Configuration via Global Variables

FIU faults are controlled through INAV Global Variables, configured at runtime via **Logic Conditions** in the INAV Configurator. No recompilation is needed to change fault targets.

### GV0 - Motor disable bitmask

Each bit corresponds to one motor. A maximum of `MAX_MOTORS` motors can be disabled.

| Bit | Motor |
|-----|-------|
| 0   | Motor 0 |
| 1   | Motor 1 |
| 2   | Motor 2 |
| 3   | Motor 3 |
| ... | ...     |
| MAX_MOTORS-1 | Motor MAX_MOTORS-1 |

**Examples** (6-motor configuration, MAX_MOTORS = 6, valid bits 0–5):
- `GV0 = 7` (0b000111) - disable motors 0, 1, 2
- `GV0 = 63` (0b111111) - disable all 6 motors

**Logic Conditions setup (example: RC Channel 7 controls motor fault):**

| LC | Operation    | Operand A      | Operand B   | Active  |
|----|--------------|----------------|-------------|---------|
| 0  | Greater Than | RC Channel 7   | Value 1500  | Always  |
| 1  | Set GVAR     | Value 0 (GV0)  | Value 63    | LC 0    |
| 2  | Lower Than   | RC Channel 7   | Value 1500  | Always  |
| 3  | Set GVAR     | Value 0 (GV0)  | Value 0     | LC 2    |

- LC 0 detects switch ON (>1500 µs) → LC 1 sets GV0 = 63 (all motors disabled)
- LC 2 detects switch OFF (<1500 µs) → LC 3 sets GV0 = 0 (no fault)

### GV1 - I2C bus block

| Value | Effect |
|-------|--------|
| 0     | No fault (I2C normal) |
| 1     | Block all I2C bus reads (`FIU_I2C_BUS_BLOCK`) |

**Logic Conditions setup (example: RC Channel 6 controls I2C fault):**

| LC | Operation    | Operand A      | Operand B   | Active  |
|----|--------------|----------------|-------------|---------|
| 4  | Greater Than | RC Channel 6   | Value 1500  | Always  |
| 5  | Set GVAR     | Value 1 (GV1)  | Value 1     | LC 4    |
| 6  | Lower Than   | RC Channel 6   | Value 1500  | Always  |
| 7  | Set GVAR     | Value 1 (GV1)  | Value 0     | LC 6    |

- LC 4 detects switch ON (>1500 µs) → LC 5 sets GV1 = 1 (I2C blocked)
- LC 6 detects switch OFF (<1500 µs) → LC 7 sets GV1 = 0 (no fault)

### GV2 - SPI bus block

| Value | Effect |
|-------|--------|
| 0     | No fault (SPI normal) |
| 1     | Block all SPI bus reads (`FIU_SPI_BUS_BLOCK`) |

**Logic Conditions setup (example: RC Channel 5 controls SPI fault):**

| LC | Operation    | Operand A      | Operand B   | Active  |
|----|--------------|----------------|-------------|---------|
| 8  | Greater Than | RC Channel 5   | Value 1500  | Always  |
| 9  | Set GVAR     | Value 2 (GV2)  | Value 1     | LC 8    |
| 10 | Lower Than   | RC Channel 5   | Value 1500  | Always  |
| 11 | Set GVAR     | Value 2 (GV2)  | Value 0     | LC 10   |

- LC 8 detects switch ON (>1500 µs) → LC 9 sets GV2 = 1 (SPI blocked)
- LC 10 detects switch OFF (<1500 µs) → LC 11 sets GV2 = 0 (no fault)

### Update frequency

`fiuUpdateFromGlobalVars()` is called at 100 Hz from `taskUpdateAux()` in `fc/fc_tasks.c`.

## API

| Function | Description |
|----------|-------------|
| `fiuUpdateFromGlobalVars()` | Reads GV0/GV1/GV2, updates all fault flags. Called at 100 Hz. |
| `fiuIsMotorDisabled(uint8_t motorIndex)` | Returns `true` if the given motor should be disabled. Called by PWM driver. |
| `fiuIsI2cBusReadBlocked(void)` | Returns `true` if all I2C bus reads should be blocked. Called by `bus.c`. |
| `fiuIsSpiBusReadBlocked(void)` | Returns `true` if all SPI bus reads should be blocked. Called by `bus.c`. |

## Enabling FIU

The FIU is conditionally compiled using the `USE_FIU` preprocessor macro. To enable it, add `USE_FIU` as a compiler define in your build system or IDE project settings.

## Files

- `fiu.h` - FIU types, constants, and function declarations
- `fiu.c` - FIU implementation

## Modified INAV files

- `drivers/pwm_output.c` - `fiuIsMotorDisabled()` check in `pwmWriteMotor()`
- `drivers/bus.c` - `fiuIsI2cBusReadBlocked()` check in I2C branch and `fiuIsSpiBusReadBlocked()` check in SPI branch of `busRead()` and `busReadBuf()`
- `fc/fc_tasks.c` - `fiuUpdateFromGlobalVars()` call in `taskUpdateAux()`