/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <platform.h>

#include "common/utils.h"

#include "drivers/io.h"

#include "drivers/bus_i2c.h"
#include "drivers/bus.h"

#include "io_impl.h"


#if !defined(UNUSED)
#define UNUSED(x) ((void)(x))
#endif

// Warning Message if Devicecount is over 3
STATIC_ASSERT(I2CDEV_COUNT<= 3, only_3_bus_on_aurix);

// Errorcounter
static volatile uint16_t i2cErrorCount = 0;

// --- BEGIN Instances for i2c busses ---
#ifdef USE_I2C_DEVICE_1
I2cBus_t I2C1 = {
    .i2cModule = &MODULE_I2C0,
};
#endif

#ifdef USE_I2C_DEVICE_2
I2cBus_t I2C2 = {
    .i2cModule = &MODULE_I2C0,
};
#endif

#ifdef USE_I2C_DEVICE_3
I2cBus_t I2C3 = {
    .i2cModule = &MODULE_I2C0,
};
#endif
// --- END Instances for i2c busses ---

// --- BEGIN Pin functions via IO TAG ---
// get SCL for i2c
static IfxI2c_Scl_InOut * getSCLPinmapFromIoTag(ioTag_t tag, Ifx_I2C * module){
    IfxI2c_Scl_InOut * pinmap = NULL_PTR;

    //get port and pin from tag
    ioRec_t *ioRec = IO_Rec(IOGetByTag(tag));
    IfxPort_Pin gpio = ioRec->gpio;

    for (int i=0; i<IFXI2C_PINMAP_NUM_MODULES; i++){
        for (int j=0; j<IFXI2C_PINMAP_SCL_INOUT_NUM_ITEMS; j++){
            //module must match
            if (IfxI2c_Scl_InOut_pinTable[i][j] != NULL){
                if( IfxI2c_Scl_InOut_pinTable[i][j]->pin.pinIndex == gpio.pinIndex && 
                    IfxI2c_Scl_InOut_pinTable[i][j]->pin.port == gpio.port && 
                    IfxI2c_Scl_InOut_pinTable[i][j]->module == module
                ){
                    pinmap = IfxI2c_Scl_InOut_pinTable[i][j];
                }
            }
        }
    }
    return pinmap;
}

// get SDA for i2c
static IfxI2c_Sda_InOut * getSDAPinmapFromIoTag(ioTag_t tag, Ifx_I2C * module){
    IfxI2c_Sda_InOut * pinmap = NULL_PTR;

    //get port and pin from tag
    ioRec_t *ioRec = IO_Rec(IOGetByTag(tag));
    IfxPort_Pin gpio = ioRec->gpio;

    for (int i=0; i<IFXI2C_PINMAP_NUM_MODULES; i++){
        for (int j=0; j<IFXI2C_PINMAP_SDA_INOUT_NUM_ITEMS; j++){
            //module must match
            if (IfxI2c_Sda_InOut_pinTable[i][j] != NULL){
                if( IfxI2c_Sda_InOut_pinTable[i][j]->pin.pinIndex == gpio.pinIndex && 
                    IfxI2c_Sda_InOut_pinTable[i][j]->pin.port == gpio.port && 
                    IfxI2c_Sda_InOut_pinTable[i][j]->module == module
                ){
                    pinmap = IfxI2c_Sda_InOut_pinTable[i][j];
                }
            }
        }
    }
    return pinmap;
}
// --- END Pin functions via IO TAG ---

// --- BEGIN i2c Hardwaremap for busses
static i2cDevice_t i2cHardwareMap[] = {
    #ifdef USE_I2C_DEVICE_1
    { .dev = &I2C1, .scl = IO_TAG(I2C1_SCL), .sda = IO_TAG(I2C1_SDA), .speed = I2C_SPEED_400KHZ },
    #else
    { .dev = NULL_PTR},
    #endif

    #ifdef USE_I2C_DEVICE_2
    { .dev = &I2C2, .scl = IO_TAG(I2C2_SCL), .sda = IO_TAG(I2C2_SDA), .speed = I2C_SPEED_400KHZ },
    #else
    { .dev = NULL_PTR},
    #endif

    #ifdef USE_I2C_DEVICE_3
    { .dev = &I2C3, .scl = IO_TAG(I2C3_SCL), .sda = IO_TAG(I2C3_SDA), .speed = I2C_SPEED_400KHZ },
    #else
    { .dev = NULL_PTR},
    #endif
};
// --- END i2c Hardwaremap for busses

// Is used in fc_init.c and speed is systemconfig
void i2cSetSpeed(uint8_t speed)
{
    for (unsigned int i = 0; i < ARRAYLEN(i2cHardwareMap); i++) {
        i2cHardwareMap[i].speed = speed;
    }
}

// Init function for the i2c
void i2cInit(I2CDevice device)
{
    if (device == I2CINVALID)
    {
        i2cErrorCount++;
        return;
    }

    i2cDevice_t *i2c = &(i2cHardwareMap[device]);

    if (!(i2c->initDone)) {
        IOInit(IOGetByTag(i2c->scl), OWNER_I2C, RESOURCE_I2C_SCL, RESOURCE_INDEX(device));
        IOInit(IOGetByTag(i2c->sda), OWNER_I2C, RESOURCE_I2C_SDA, RESOURCE_INDEX(device));

        // init the i2c config to default values
        IfxI2c_I2c_initConfig(&i2c->dev->i2cBusConfig, i2c->dev->i2cModule); 
    
        // assign pins to i2c configuration
        const IfxI2c_Pins i2cPins =
        {
            getSCLPinmapFromIoTag(i2c->scl, i2c->dev->i2cModule),   /* SCL port pin                                 */
            getSDAPinmapFromIoTag(i2c->sda, i2c->dev->i2cModule),   /* SDA port pin                                 */
            IfxPort_PadDriver_ttlSpeed1                            /* Pad driver mode                              */
        };

        i2c->dev->i2cBusConfig.pins = &i2cPins;

        // assign baudrate to i2c configuration
        switch (i2c->speed) {
            case I2C_SPEED_400KHZ:
            default:
                i2c->dev->i2cBusConfig.baudrate = 400000;
                break;

            case I2C_SPEED_800KHZ:
                i2c->dev->i2cBusConfig.baudrate = 800000;
                break;

            case I2C_SPEED_100KHZ:
                i2c->dev->i2cBusConfig.baudrate = 100000;
                break;

            case I2C_SPEED_200KHZ:
                i2c->dev->i2cBusConfig.baudrate = 200000;
                break;
        }

        // initialize i2c module
        IfxI2c_I2c_initModule(&i2c->dev->i2cBusHandle, &i2c->dev->i2cBusConfig);

        // i2c initialized
        i2c->initDone = true;
    }
}

void i2cInitDevice(busDevice_t * busDev)
{
    if(busDev == NULL_PTR)
    {
        i2cErrorCount++;
        return;
    }        

    I2CDevice device = busDev->busdev.i2c.i2cBus;
    i2cDevice_t *i2c = &(i2cHardwareMap[device]);

    // Initialize i2c device with default values, Fill structure with default values and I2C Handler
    IfxI2c_I2c_deviceConfig i2cBusDeviceConfig;
    IfxI2c_I2c_initDeviceConfig(&i2cBusDeviceConfig, &i2c->dev->i2cBusHandle); 

    // Because it is 7 bit long and bit 0 is R/W bit, the device address has to be shifted by 1
    i2cBusDeviceConfig.deviceAddress = (busDev->busdev.i2c.address) << 1;

    // Initialize the I2C device handle
    IfxI2c_I2c_initDevice(busDev->i2cBusDevice, &i2cBusDeviceConfig);

    return;
}

uint16_t i2cGetErrorCounter(void)
{
    return i2cErrorCount;
}

// i2c buffer write function
bool i2cWriteBuffer(busDevice_t * busDev, uint8_t reg, uint8_t len, const uint8_t * data, bool allowRawAccess)
{
    I2CDevice device = busDev->busdev.i2c.i2cBus;
    
    if (device == I2CINVALID)
    {
        i2cErrorCount++;
        return false;
    }
        

    i2cDevice_t *i2c = &(i2cHardwareMap[device]);

    if (!i2c->initDone)
    {
        i2cErrorCount++;
        return false;
    }
        

    IfxI2c_I2c_Status status;

    uint8_t buffer[len + 1];
    uint8_t *writeBuffer;
    uint8_t writeLength;

    if ((reg == 0xFF || len == 0) && allowRawAccess) {
        // Raw write without reg
        writeBuffer = (uint8_t *)data;
        writeLength = len;
    } else {
        // Normal Write with reg
        buffer[0] = reg;
        if (!data == NULL_PTR)
        {
            for (uint8_t i = 0; i < len; i++) {
                buffer[i + 1] = data[i];
            }
        }
        writeBuffer = buffer;
        writeLength = len + 1;
    }

    status = IfxI2c_I2c_write(busDev->i2cBusDevice, writeBuffer, writeLength);
    if(status != IfxI2c_I2c_Status_ok)
    {
        i2cErrorCount++;
        return false;
    }

    return true;
}


// i2c write function
bool i2cWrite(busDevice_t * busDev, uint8_t reg, uint8_t data, bool allowRawAccess)
{
    return i2cWriteBuffer(busDev, reg, 1, &data, allowRawAccess);
}

// i2c read function
bool i2cRead(busDevice_t * busDev, uint8_t reg, uint8_t len, uint8_t* buf, bool allowRawAccess)
{
    I2CDevice device = busDev->busdev.i2c.i2cBus;
    
    if (device == I2CINVALID || buf == NULL_PTR)
    {
        i2cErrorCount++;
        return false;
    }

    i2cDevice_t *i2c = &(i2cHardwareMap[device]);

    if (!i2c->initDone)
    {
        i2cErrorCount++;
        return false;
    }

    IfxI2c_I2c_Status status;

    if (!(reg == 0xFF || len == 0) || !allowRawAccess)
    {
        uint8_t regBuf[1] = { reg };
        status = IfxI2c_I2c_write(busDev->i2cBusDevice, regBuf, 1);
        if (status != IfxI2c_I2c_Status_ok)
        {
            i2cErrorCount++;
            return false;
        }
    }

    status = IfxI2c_I2c_read(busDev->i2cBusDevice, buf, len);
    if(status != IfxI2c_I2c_Status_ok)
    {
        i2cErrorCount++;
        return false;
    }

    return true;
}

