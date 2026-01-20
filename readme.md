# INAV - navigation capable flight controller

# Aurix
This fork adds code to support Aurix Tricore CPUs.

| Supported Targets | TC375 Litekit |
| ----------------- | ----- |

**Want to contribute?** Just open an [issue](https://github.com/jakobgif/inav_tc375/issues) and we will get back to you.

Refer to [how_to_upgrade.md](./how_to_upgrade.md) for information regarding upgrading to the latest inav version.

Refer to [how_to_develop.md](./how_to_develop.md) for information regarding the branch names and development.

## Aurix Development Studio Setup
This entire repo can be added as submodule to an existing project. In this case the Aurix project has to be configured as follows:
- C/C++ Build -> Settings -> AURIX GCC Compiler -> Dialect -> Language standard: `none` & Other dialect flags: `-std=gnu11`
- --//-- -> Prepocessor -> define macros shown below
- --//-- -> Optimization -> `-Og` for debug or `-O2` for release
- --//-- -> Miscellaneous -> add `-include "inav_tc375\src\main\platform.h"` flag
- C/C++ General -> Paths and Symbols -> Source Location:
  - add complete inav folder to source filter
  - add `inav_tc375\lib\main\MAVLink` as source folder
  - add `inav_tc375\src\main` as source folder
  - add all CPU specific files that are not for aurix to the source filter.

## Aurix specific macros
To build for TC375 the follwing macros have to be defined in the build environment:
- `__TRICORE__`
- `TC375`
- `__TARGET__=\"TC375\"`
- `GIT_HASH` current commit hash of the wrapping project
- `GIT_TAG` latest tag of the wrapping project
- `GIT_HASH_INAV` current checked out hash of the inav submodule
- `GIT_TAG_INAV` latest tag of the submodule
- `INAV_VERSION` latest tag of inav
- `GIT_IS_DIRTY` can be defined if the local copy is dirty
- `FC_VERSION_MAJOR` version of inav
- `FC_VERSION_MINOR` version of inav
- `FC_VERSION_PATCH_LEVEL` version of inav

## Generation scripts
The inav codebase requires some files that need to be automatically generated. The following scripts need to be run from inside the aurix build folder.
- `./src/utils/aurix_generate_settings.sh` is used to generate the inav settings. **WARNING**: It could be that the path of the directory where the compiler libraries are located must be changed.
- `./src/utils/aurix_generate_version_strings.sh` is used to generate the version macros described above. This script should be run before each build.

## Linker script
The following sections must be added to the linker script:
```
CORE_ID = GLOBAL;
    SECTIONS
    {
        .busdev_registry :
        {
            PROVIDE_HIDDEN (__busdev_registry_start = .);
            KEEP (*(.busdev_registry))
            KEEP (*(SORT(.busdev_registry.*)))
            PROVIDE_HIDDEN (__busdev_registry_end = .);
        } > default_rom
        
        .pg_registry :
        {
            PROVIDE_HIDDEN (__pg_registry_start = .);
            KEEP (*(.pg_registry))
            KEEP (*(SORT(.pg_registry.*)))
            PROVIDE_HIDDEN (__pg_registry_end = .);
        } > default_rom
    
        .pg_resetdata :
        {
            PROVIDE_HIDDEN (__pg_resetdata_start = .);
            KEEP (*(.pg_resetdata))
            PROVIDE_HIDDEN (__pg_resetdata_end = .);
        } > default_rom
    }
```

## CLI
Command `aurix` can be used to show basic platform specs.

##  Target configuration
`inav_tc375\src\main\target` lists target configirations. A new target can be added by creating a new folder. A example configuration for the TC375 litekit was already created (`inav_tc375\src\main\target\FHTW_TC375_LK`). 

`targetConfiguration()` is used to set some default configuration. In case of FHTW_TC375_LK it sets the logging output port to UART4 and enables all logging.
### GPIO config
For this implementation GPIO names can be defined like this: `#define LED0 MODULE_P00_5`. Furthermore, the GPIO has to be enabled via another macro: `#define TARGET_IO_PORTMODULE_P00 0b100000`. 
### SPI config
To enable a SPI bus you need the use the macro `#define USE_SPI_DEVICE_x`. For this implementation SPI device 1 is using QSPI 0, device 2 is using QSPI 1 and so on. The SPI pins also need to be defined. Eg: 
```C
#define SPI3_PIN_SCLK MODULE_P15_8
#define SPI3_PIN_MRST MODULE_P15_7
#define SPI3_PIN_MTSR MODULE_P15_6
```
Note that all pins must be valid pinmaps that can be connected to the QPSI module. For the chip select it does not matter if the gpio pinmap uses the same QSPI module.
### I2C config
To enable the use of the I2C bus, the macro `#define USE_I2C` needs to be defined. Depending on which I2C bus is going to be used, `#define USE_I2C_DEVICE_x` with the correct bus number needs to be defined too. In total three the implementation offers 3 I2C bus interfaces. Each bus uses the Aurix I2C Module 0. The I2C pins also need to be defined. Eg:
```C
#define I2C1_SCL                MODULE_P13_1
#define I2C1_SDA                MODULE_P13_2
```
Note that all pins must be valid pinmaps that can be connected to the I2C module `MODULE_I2C0`.

### UART config
Use the macro `USE_UARTx` to enable a UART port. UART1 uses Asclin0, UART2 uses Asclin1 and so on. Afterwards use eg
```C
#define UART1_PIN_RX MODULE_P14_1
#define UART1_PIN_TX MODULE_P14_0
```
to set the UART pins. Additionally the UART count has to be set to the number of available uart ports: `#define SERIAL_PORT_COUNT`.

### Timer config
The timer is used for the pwm signal generation. These signals can be used to control motors, LEDs or sound beepers.
To activate the pwm signals on the out put the `MAX_PWM_OUTPUT_PORTS` define needs to be adjusted to acount for all on board 
pwm singal outputs.
```C
#define MAX_PWM_OUTPUT_PORTS 8
```
To control the motors, a pin needs to be defined for each motor and be assigned to an output driven by an atom module.   
```C
#define PWM_MOTOR_1_PIN MODULE_P00_6
#define PWM_MOTOR_2_PIN MODULE_P00_1
#define PWM_MOTOR_3_PIN MODULE_P00_9
```
The assignement of the motor to an atom output is processed in `timerHardware_t timerHardware[]`.
Up tp 16 motor can be assigned to the list [0 to 15].
```C
timerHardware_t timerHardware[] = {
    DEF_TIM(atomDriver[0], IfxGtm_ATOM0_5_TOUT15_P00_6_OUT, PWM_MOTOR_1_PIN, IOCFG_OUT_PP, TIM_USE_MOTOR),
    DEF_TIM(atomDriver[1], IfxGtm_ATOM1_1_TOUT10_P00_1_OUT, PWM_MOTOR_2_PIN, IOCFG_OUT_PP, TIM_USE_MOTOR),
};
```
Only one frequency can be used per atom module. Make sure to use different modules if you want to use different frequencies.


### ADC config
Define `USE_ADC` to enable the ADC. In the current implementation only 6 channels of ADC Group 0 can be used. Define the pin that shall be used by using the macro `#define ADC_CHANNEL_1_PIN 0 //aurix analog input 0`. Afterwards link the ADC channel to a feature: eg `#define VBAT_ADC_CHANNEL ADC_CHN_1` to use analog pin 0 for the battery voltage.

# F411 PSA

> INAV no longer accepts targets based on STM32 F411 MCU.

> INAV 7 was the last INAV official release available for F411 based flight controllers. INAV 8 is not officially available for F411 boards and the team has not tested either. Issues that can't be reproduced on other MCUs may not be fixed and the targets for F411 targets may eventually be completelly removed from future releases.

# ICM426xx IMUs PSA

> The filtering settings for the ICM426xx has changed to match what is used by Ardupilot and Betaflight in INAV 7.1. When upgrading from older versions you may need to recalibrate the Accelerometer and if you are not using INAV's default tune you may also want to check if the tune is still good.

# M7, M6 and older UBLOX GPS units PSA

> INAV 8.0 will mark those GPS as deprecated and INAV 9.0.0 will require UBLOX units with Protocol version 15.00 or newer. This means that you need a GPS unit based on UBLOX M8 or newer.

> If you want to check the protocol version of your unit, it is displayed in INAV's 7.0.0+ status cli command.
> INAV 8.0.0 will warn you if your GPS is too old.
> ```GPS: HW Version: Unknown Proto: 0.00 Baud: 115200 (UBLOX Proto >= 15.0 required)```


> M8, M9 and M10 GPS are the most common units in use today, are readly available and have similar capabilities.
>Mantaining and testing GPS changes across this many UBLOX versions is a challenge and takes a lot of time. Removing the support for older devices will simplify code.

![INAV](http://static.rcgroups.net/forums/attachments/6/1/0/3/7/6/a9088858-102-inav.png)

# PosHold, Navigation and RTH without compass PSA

Attention all drone pilots and enthusiasts,

Are you ready to take your flights to new heights with INAV 7.1? We've got some important information to share with you.

INAV 7.1 brings an exciting update to navigation capabilities. Now, you can soar through the skies, navigate waypoints, and even return to home without relying on a compass. Yes, you heard that right! But before you launch into the air, there's something crucial to consider.

While INAV 7.1 may not require a compass for basic navigation functions, we strongly advise you to install one for optimal flight performance. Here's why:

🛰️ Better Flight Precision: A compass provides essential data for accurate navigation, ensuring smoother and more precise flight paths.

🌐 Enhanced Reliability: With a compass onboard, your drone can maintain stability even in challenging environments, low speeds and strong wind.

🚀 Minimize Risks: Although INAV 7.1 can get you where you need to go without a compass, flying without one may result in a bumpier ride and increased risk of drift or inaccurate positioning.

Remember, safety and efficiency are paramount when operating drones. By installing a compass, you're not just enhancing your flight experience, but also prioritizing safety for yourself and those around you.

So, before you take off on your next adventure, make sure to equip your drone with a compass. It's the smart choice for smoother flights and better navigation.

Fly safe, fly smart with INAV 7.1 and a compass by your side!

# INAV Community

* [INAV Discord Server](https://discord.gg/peg2hhbYwN)
* [INAV Official on Facebook](https://www.facebook.com/groups/INAVOfficial)

## Downloads

### INAV Configurator

**Get the latest version:** **[Download INAV Configurator](https://github.com/iNavFlight/inav-configurator/releases/latest)** - Available for Windows, macOS, and Linux

The INAV Configurator is the official desktop application for configuring your INAV flight controller. Choose your platform from the Assets section on the releases page.

### INAV Firmware

**Get the latest firmware:** **[Download INAV Firmware](https://github.com/iNavFlight/inav/releases/latest)**

Download the latest INAV flight controller firmware. Flash it to your flight controller using the configurator.

## Features

* Runs on the most popular F4, AT32, F7 and H7 flight controllers
* On Screen Display (OSD) - both character and pixel style
* DJI OSD integration: all elements, system messages and warnings
* Outstanding performance out of the box
* Position Hold, Altitude Hold, Return To Home and Waypoint Missions
* Excellent support for fixed wing UAVs: airplanes, flying wings
* Blackbox flight recorder logging
* Advanced gyro filtering
* Fully configurable mixer that allows to run any hardware you want: multirotor, fixed wing, rovers, boats and other experimental devices
* Multiple sensor support: GPS, Pitot tube, sonar, lidar, temperature, ESC with BlHeli_32 telemetry
* Logic Conditions, Global Functions and Global Variables: you can program INAV with a GUI
* SmartAudio and IRC Tramp VTX support
* Telemetry: SmartPort, FPort, MAVlink, LTM, CRSF
* Multi-color RGB LED Strip support
* And many more!

For a list of features, changes and some discussion please review consult the releases [page](https://github.com/iNavFlight/inav/releases) and the documentation.

## Tools

### INAV Configurator

Official tool for INAV can be downloaded [here](https://github.com/iNavFlight/inav-configurator/releases). It can be run on Windows, MacOS and Linux machines and standalone application.

### INAV Blackbox Explorer

Tool for Blackbox logs analysis is available [here](https://github.com/iNavFlight/blackbox-log-viewer/releases)

### INAV Blackbox Tools

Command line tools (`blackbox_decode`, `blackbox_render`) for Blackbox log conversion and analysis [here](https://github.com/iNavFlight/blackbox-tools).

### Telemetry screen for EdgeTX and OpenTX

Users of EdgeTX and OpenTX radios (Taranis, Horus, Jumper, Radiomaster, Nirvana) can use INAV OpenTX Telemetry Widget screen. Software and installation instruction are available here: [https://github.com/iNavFlight/OpenTX-Telemetry-Widget](https://github.com/iNavFlight/OpenTX-Telemetry-Widget)

### OSD layout Copy, Move, or Replace helper tool

[Easy INAV OSD switcher tool](https://www.mrd-rc.com/tutorials-tools-and-testing/useful-tools/inav-osd-switcher-tool/) allows you to easily switch your OSD layouts around in INAV. Choose the from and to OSD layouts, and the method of transfering the layouts.

## Installation

See: https://github.com/iNavFlight/inav/blob/master/docs/Installation.md

## Documentation, support and learning resources
* [INAV 5 on a flying wing full tutorial](https://www.youtube.com/playlist?list=PLOUQ8o2_nCLkZlulvqsX_vRMfXd5zM7Ha)
* [INAV on a multirotor drone tutorial](https://www.youtube.com/playlist?list=PLOUQ8o2_nCLkfcKsWobDLtBNIBzwlwRC8)
* [Fixed Wing Guide](docs/INAV_Fixed_Wing_Setup_Guide.pdf)
* [Autolaunch Guide](docs/INAV_Autolaunch.pdf)
* [Modes Guide](docs/INAV_Modes.pdf)
* [Wing Tuning Masterclass](docs/INAV_Wing_Tuning_Masterclass.pdf)
* [Official documentation](https://github.com/iNavFlight/inav/tree/master/docs)
* [Official Wiki](https://github.com/iNavFlight/inav/wiki)
* [Video series by Paweł Spychalski](https://www.youtube.com/playlist?list=PLOUQ8o2_nCLloACrA6f1_daCjhqY2x0fB)
* [Target documentation](https://github.com/iNavFlight/inav/tree/master/docs/boards)

## Contributing

Contributions are welcome and encouraged.  You can contribute in many ways:

* Documentation updates and corrections.
* How-To guides - received help?  help others!
* Bug fixes.
* New features.
* Telling us your ideas and suggestions.
* Buying your hardware from this [link](https://inavflight.com/shop/u/bg/)

A good place to start is the Discord channel, Telegram channel or Facebook group. Drop in, say hi.

Github issue tracker is a good place to search for existing issues or report a new bug/feature request:

https://github.com/iNavFlight/inav/issues

https://github.com/iNavFlight/inav-configurator/issues

Before creating new issues please check to see if there is an existing one, search first otherwise you waste peoples time when they could be coding instead!

## Developers

Please refer to the development section in the [docs/development](https://github.com/iNavFlight/inav/tree/master/docs/development) folder.

Nightly builds are available for testing on the following links:

https://github.com/iNavFlight/inav-nightly/releases

https://github.com/iNavFlight/inav-configurator-nightly/releases

## INAV Releases
https://github.com/iNavFlight/inav/releases


