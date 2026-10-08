# Fault Insertion Unit (FIU)

The FIU is a software-based fault injection module for INAV-based flight controllers. It enables controlled fault injection during flight to evaluate how the flight controller handles hardware failures, without requiring physical hardware modifications.

## Purpose

Supported fault types:

- **Motor faults**: Selective disabling of individual motors (PWM driver level)
- **Sensor bus faults**: Blocking I2C or SPI bus reads at a configurable rate (bus abstraction level)

## How it works

### Motor fault injection

The FIU operates at the PWM driver level (`drivers/pwm_output.c`). When a motor is marked as disabled, `pwmWriteMotor()` intercepts the motor command and sets the target speed to 0 instead of the calculated mixer output. The PID controller and mixer remain unmodified, preserving INAV upgrade compatibility.

### I2C bus fault injection

The FIU hooks into the `BUSTYPE_I2C` branch of `drivers/bus.c`. When active, `busRead()` and `busReadBuf()` return zero-filled data with a success status, without performing any actual I2C transfer. The affected bus is selected by the GV1 bitmask and the blocking rate is set by GV3. Both must be non-zero for a fault to occur.

### SPI bus fault injection

The FIU hooks into the `BUSTYPE_SPI` branch of `drivers/bus.c`. When active, `busRead()` and `busReadBuf()` return zero-filled data with a success status, without performing any actual SPI transfer. The affected bus is selected by the GV2 bitmask and the blocking rate is set by GV4. Both must be non-zero for a fault to occur.

### Rate-based blocking

At a given error rate (0–100%), the FIU blocks reads deterministically based on a per-bus call counter that increments at 100 Hz. At 50% rate the bus is blocked for 50 consecutive 10 ms windows (500 ms), then free for 500 ms, repeating. The pattern is reproducible — the same rate always produces the same blocking sequence.

## Configuration via Global Variables

FIU faults are controlled through INAV Global Variables, configured at runtime via **Logic Conditions** in the INAV Configurator. No recompilation is needed to change fault targets.

### GV0 — Motor disable bitmask

Bit N disables Motor N. Up to `MAX_MOTORS` motors can be disabled simultaneously.

| Bit | Motor |
|-----|-------|
| 0   | Motor 0 |
| 1   | Motor 1 |
| 2   | Motor 2 |
| 3   | Motor 3 |
| 4   | Motor 4 |
| 5   | Motor 5 |

**Logic Conditions setup (RC Channel 7 / AUX6, Switch):**

| LC | Operation    | Operand A      | Operand B  | Active  |
|----|--------------|----------------|------------|---------|
| 0  | Greater Than | RC Channel 7   | Value 1500 | Always  |
| 1  | Set GVAR     | Value 0 (GV0)  | Value 7    | LC 0    |
| 2  | Lower Than   | RC Channel 7   | Value 1500 | Always  |
| 3  | Set GVAR     | Value 0 (GV0)  | Value 0    | LC 2    |

- Switch ON (>1500 µs) → GV0 = 7 (0b000111, motors 0–2 disabled)
- Switch OFF (<1500 µs) → GV0 = 0 (no fault)

### GV1 — I2C bus select bitmask

Bit N selects I2CDEV_(N+1). Requires GV3 > 1000 to activate blocking.

| Bit | Bus      |
|-----|----------|
| 0   | I2CDEV_1 |
| 1   | I2CDEV_2 |
| 2   | I2CDEV_3 |

**Logic Conditions setup (RC Channel 6 / AUX5, Switch):**

| LC | Operation    | Operand A      | Operand B  | Active  |
|----|--------------|----------------|------------|---------|
| 4  | Greater Than | RC Channel 6   | Value 1500 | Always  |
| 5  | Set GVAR     | Value 1 (GV1)  | Value 1    | LC 4    |
| 6  | Lower Than   | RC Channel 6   | Value 1500 | Always  |
| 7  | Set GVAR     | Value 1 (GV1)  | Value 0    | LC 6    |

- Switch ON (>1500 µs) → GV1 = 1 (I2C1 selected)
- Switch OFF (<1500 µs) → GV1 = 0 (no bus selected)

### GV2 — SPI bus select bitmask

Bit N selects SPIDEV_(N+1). Requires GV4 > 1000 to activate blocking.

| Bit | Bus      |
|-----|----------|
| 0   | SPIDEV_1 |
| 1   | SPIDEV_2 |
| 2   | SPIDEV_3 |
| 3   | SPIDEV_4 |

**Logic Conditions setup (RC Channel 5 / AUX4, Switch):**

| LC | Operation    | Operand A      | Operand B  | Active  |
|----|--------------|----------------|------------|---------|
| 8  | Greater Than | RC Channel 5   | Value 1500 | Always  |
| 9  | Set GVAR     | Value 2 (GV2)  | Value 1    | LC 8    |
| 10 | Lower Than   | RC Channel 5   | Value 1500 | Always  |
| 11 | Set GVAR     | Value 2 (GV2)  | Value 0    | LC 10   |

- Switch ON (>1500 µs) → GV2 = 1 (SPI1 selected)
- Switch OFF (<1500 µs) → GV2 = 0 (no bus selected)

### GV3 — I2C error rate (RC knob)

RC knob raw value (1000–2000 µs) passed through directly. Converted internally to 0–100%.

| RC value | Error rate | Effect                        |
|----------|------------|-------------------------------|
| 1000 µs  | 0%         | No blocking (bus fully operational) |
| 1500 µs  | 50%        | 50% of reads blocked          |
| 2000 µs  | 100%       | All reads blocked             |

**Logic Conditions setup (RC Channel 8 / AUX7, Knob):**

| LC | Operation | Operand A     | Operand B    | Active |
|----|-----------|---------------|--------------|--------|
| 12 | Set GVAR  | Value 3 (GV3) | RC Channel 8 | Always |

### GV4 — SPI error rate (RC knob)

Same semantics as GV3, but controls SPI blocking rate for the bus selected by GV2.

**Logic Conditions setup (RC Channel 9 / AUX8, Knob):**

| LC | Operation | Operand A     | Operand B    | Active |
|----|-----------|---------------|--------------|--------|
| 13 | Set GVAR  | Value 4 (GV4) | RC Channel 9 | Always |

### Update frequency

`fiuUpdateFromGlobalVars()` is called at 100 Hz from `taskUpdateAux()` in `fc/fc_tasks.c`. All five GVs (GV0–GV4) are read and applied each cycle.

## API

| Function | Called from | Purpose |
|----------|-------------|---------|
| `fiuUpdateFromGlobalVars()` | `fc/fc_tasks.c` (100 Hz) | Read GV0–GV4, update all fault flags |
| `fiuIsMotorDisabled(uint8_t motorIndex)` | `drivers/pwm_output.c` | Motor fault check |
| `fiuIsI2cBusReadBlocked(I2CDevice bus)` | `drivers/bus.c` | I2C bus block check |
| `fiuIsSpiBusReadBlocked(SPIDevice bus)` | `drivers/bus.c` | SPI bus block check |
| `fiuGetState()` | `blackbox/blackbox.c` | Get state snapshot for logging |

## Enabling FIU

The FIU is conditionally compiled using the `USE_FIU` preprocessor macro. It is not defined in `target.h` and must be added as a compiler define in your build system or IDE.

## Files

- `fiu.h` — FIU types, constants, and function declarations
- `fiu.c` — FIU implementation

## Modified INAV files

| File | Modification |
|------|-------------|
| `drivers/pwm_output.c` | `fiuIsMotorDisabled()` check in `pwmWriteMotor()` |
| `drivers/bus.c` | `fiuIsSpiBusReadBlocked()` and `fiuIsI2cBusReadBlocked()` in `busRead()` and `busReadBuf()` |
| `fc/fc_tasks.c` | `fiuUpdateFromGlobalVars()` call in `taskUpdateAux()` |
| `blackbox/blackbox.c` | `fiuGetState()` read + 5 log fields (`fiuInjMotor`, `fiuInjI2c`, `fiuInjSpi`, `fiuInjI2cRate`, `fiuInjSpiRate`) |