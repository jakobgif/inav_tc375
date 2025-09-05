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

/**
 * @file bus_spi_tc375.c
 * @author Jakob Frenzel (jakob.frenzel@hotmail.com)
 * @brief tc375 SPI implementation for inav
 * @date 2025-05-27
 */

#include <stdbool.h>
#include <stdint.h>

#include <platform.h>

#ifdef USE_SPI

#include "drivers/bus_spi.h"
#include "drivers/bus.h"
#include "drivers/io.h"
#include "drivers/io_impl.h"
#include "build/build_config.h"

//for interrupt vector
#define AURIX_CORE_ID 0

//for code optimisation
#define PSPR_FUNCTION CPU0_PSPR_FUNCTION

static IfxQspi_Sclk_Out * getSclkPinmapFromIoTag(ioTag_t tag, Ifx_QSPI * module){
    IfxQspi_Sclk_Out * pinmap = NULL_PTR;

    //get port and pin from tag
    ioRec_t *ioRec = IO_Rec(IOGetByTag(tag));
    IfxPort_Pin gpio = ioRec->gpio;

    for (int i=0; i<IFXQSPI_PINMAP_NUM_MODULES; i++){
        for (int j=0; j<IFXQSPI_PINMAP_SCLK_OUT_NUM_ITEMS; j++){
            //module must match
            if (IfxQspi_Sclk_Out_pinTable[i][j] != NULL){
                if( IfxQspi_Sclk_Out_pinTable[i][j]->pin.pinIndex == gpio.pinIndex && 
                    IfxQspi_Sclk_Out_pinTable[i][j]->pin.port == gpio.port && 
                    IfxQspi_Sclk_Out_pinTable[i][j]->module == module
                ){
                    pinmap = IfxQspi_Sclk_Out_pinTable[i][j];
                }
            }
        }
    }

    return pinmap;
}

static IfxQspi_Mtsr_Out * getMtsrPinmapFromIoTag(ioTag_t tag, Ifx_QSPI * module){
    IfxQspi_Mtsr_Out * pinmap = NULL_PTR;

    //get port and pin from tag
    ioRec_t *ioRec = IO_Rec(IOGetByTag(tag));
    IfxPort_Pin gpio = ioRec->gpio;

    for (int i=0; i<IFXQSPI_PINMAP_NUM_MODULES; i++){
        for (int j=0; j<IFXQSPI_PINMAP_MTSR_OUT_NUM_ITEMS; j++){
            //module must match
            if (IfxQspi_Mtsr_Out_pinTable[i][j] != NULL){
                if( IfxQspi_Mtsr_Out_pinTable[i][j]->pin.pinIndex == gpio.pinIndex && 
                    IfxQspi_Mtsr_Out_pinTable[i][j]->pin.port == gpio.port && 
                    IfxQspi_Mtsr_Out_pinTable[i][j]->module == module
                ){
                    pinmap = IfxQspi_Mtsr_Out_pinTable[i][j];
                }
            }
        }
    }

    return pinmap;
}

static IfxQspi_Mrst_In * getMrstPinmapFromIoTag(ioTag_t tag, Ifx_QSPI * module){
    IfxQspi_Mrst_In * pinmap = NULL_PTR;

    //get port and pin from tag
    ioRec_t *ioRec = IO_Rec(IOGetByTag(tag));
    IfxPort_Pin gpio = ioRec->gpio;

    for (int i=0; i<IFXQSPI_PINMAP_NUM_MODULES; i++){
        for (int j=0; j<IFXQSPI_PINMAP_MRST_IN_NUM_ITEMS; j++){
            //module must match
            if (IfxQspi_Mrst_In_pinTable[i][j] != NULL){
                if( IfxQspi_Mrst_In_pinTable[i][j]->pin.pinIndex == gpio.pinIndex && 
                    IfxQspi_Mrst_In_pinTable[i][j]->pin.port == gpio.port && 
                    IfxQspi_Mrst_In_pinTable[i][j]->module == module
                ){
                    pinmap = IfxQspi_Mrst_In_pinTable[i][j];
                }
            }
        }
    }

    return pinmap;
}

static IfxQspi_Slso_Out * getSlsPinmapFromIoTag(ioTag_t tag){
    IfxQspi_Slso_Out * pinmap = NULL_PTR;

    //get port and pin from tag
    ioRec_t *ioRec = IO_Rec(IOGetByTag(tag));
    IfxPort_Pin gpio = ioRec->gpio;

    for (int i=0; i<IFXQSPI_PINMAP_NUM_MODULES; i++){
        for (int j=0; j<IFXQSPI_PINMAP_NUM_SLAVESELECTS; j++){
            for (int k=0; k<IFXQSPI_PINMAP_SLSO_OUT_NUM_ITEMS; k++){
                if (IfxQspi_Slso_Out_pinTable[i][j][k] != NULL){
                    //module does not matter
                    if(IfxQspi_Slso_Out_pinTable[i][j][k]->pin.pinIndex == gpio.pinIndex && IfxQspi_Slso_Out_pinTable[i][j][k]->pin.port == gpio.port){
                        pinmap = IfxQspi_Slso_Out_pinTable[i][j][k];
                    }
                }
            }
        }
    }

    return pinmap;
}

#ifdef USE_SPI_SPEED_SLOW
static const float32_t spiSpeedMap[] = {
    500000,     // SPI_CLOCK_INITIALIZATON
    500000,    // SPI_CLOCK_SLOW
    500000,   // SPI_CLOCK_STANDARD
    500000,   // SPI_CLOCK_FAST
    500000,   // SPI_CLOCK_ULTRAFAST
};
#else
static const float32_t spiSpeedMap[] = {
    500000,     // SPI_CLOCK_INITIALIZATON
    1000000,    // SPI_CLOCK_SLOW
    5000000,    // SPI_CLOCK_STANDARD
    10000000,   // SPI_CLOCK_FAST
    20000000,   // SPI_CLOCK_ULTRAFAST
};
#endif

//static instanced for the SPI busses
#ifdef USE_SPI_DEVICE_1
SpiBus_t SPI1 = {
    .qspi = &MODULE_QSPI0,
    .rxPriority = INTPRIO_QSPI0_RX,
    .txPriority = INTPRIO_QSPI0_TX,
};
#endif
#ifdef USE_SPI_DEVICE_2
SpiBus_t SPI2 = {
    .qspi = &MODULE_QSPI1,
    .rxPriority = INTPRIO_QSPI1_RX,
    .txPriority = INTPRIO_QSPI1_TX,
};
#endif
#ifdef USE_SPI_DEVICE_3
SpiBus_t SPI3 = {
    .qspi = &MODULE_QSPI2,
    .rxPriority = INTPRIO_QSPI2_RX,
    .txPriority = INTPRIO_QSPI2_TX,
};
#endif
#ifdef USE_SPI_DEVICE_4
SpiBus_t SPI4 = {
    .qspi = &MODULE_QSPI3,
    .rxPriority = INTPRIO_QSPI3_RX,
    .txPriority = INTPRIO_QSPI3_TX,
};
#endif

/**
 * ISRs
 */
#ifdef USE_SPI_DEVICE_1
IFX_INTERRUPT(qspi0TxISR, AURIX_CORE_ID, INTPRIO_QSPI0_TX);
void PSPR_FUNCTION qspi0TxISR(void){
    IfxQspi_SpiMaster_isrTransmit(&(SPI1.spiMaster));
}

IFX_INTERRUPT(qspi0RxISR, AURIX_CORE_ID, INTPRIO_QSPI0_RX);
void PSPR_FUNCTION qspi0RxISR(void){
    IfxQspi_SpiMaster_isrReceive(&(SPI1.spiMaster));
}
#endif

#ifdef USE_SPI_DEVICE_2
IFX_INTERRUPT(qspi1TxISR, AURIX_CORE_ID, INTPRIO_QSPI1_TX);
void PSPR_FUNCTION qspi1TxISR(void){
    IfxQspi_SpiMaster_isrTransmit(&(SPI2.spiMaster));
}

IFX_INTERRUPT(qspi1RxISR, AURIX_CORE_ID, INTPRIO_QSPI1_RX);
void PSPR_FUNCTION qspi1RxISR(void){
    IfxQspi_SpiMaster_isrReceive(&(SPI2.spiMaster));
}
#endif

#ifdef USE_SPI_DEVICE_3
IFX_INTERRUPT(qspi2TxISR, AURIX_CORE_ID, INTPRIO_QSPI2_TX);
void PSPR_FUNCTION qspi2TxISR(void){
    IfxQspi_SpiMaster_isrTransmit(&(SPI3.spiMaster));
}

IFX_INTERRUPT(qspi2RxISR, AURIX_CORE_ID, INTPRIO_QSPI2_RX);
void PSPR_FUNCTION qspi2RxISR(void){
    IfxQspi_SpiMaster_isrReceive(&(SPI3.spiMaster));
}
#endif

#ifdef USE_SPI_DEVICE_4
IFX_INTERRUPT(qspi3TxISR, AURIX_CORE_ID, INTPRIO_QSPI3_TX);
void PSPR_FUNCTION qspi3TxISR(void){
    IfxQspi_SpiMaster_isrTransmit(&(SPI4.spiMaster));
}

IFX_INTERRUPT(qspi3RxISR, AURIX_CORE_ID, INTPRIO_QSPI3_RX);
void PSPR_FUNCTION qspi3RxISR(void){
    IfxQspi_SpiMaster_isrReceive(&(SPI4.spiMaster));
}
#endif

/**
 * Hardware map
 */
static spiDevice_t spiHardwareMap[] = {
#ifdef USE_SPI_DEVICE_1
    { .dev = &SPI1, .sck = IO_TAG(SPI1_SCK_PIN), .miso = IO_TAG(SPI1_MISO_PIN), .mosi = IO_TAG(SPI1_MOSI_PIN)},
#else
    { .dev = NULL },
#endif

#ifdef USE_SPI_DEVICE_2
    { .dev = &SPI2, .sck = IO_TAG(SPI2_SCK_PIN), .miso = IO_TAG(SPI2_MISO_PIN), .mosi = IO_TAG(SPI2_MOSI_PIN)},
#else
    { .dev = NULL },
#endif

#ifdef USE_SPI_DEVICE_3
    { .dev = &SPI3, .sck = IO_TAG(SPI3_PIN_SCLK), .miso = IO_TAG(SPI3_PIN_MRST), .mosi = IO_TAG(SPI3_PIN_MTSR)},
#else
    { .dev = NULL },
#endif

#ifdef USE_SPI_DEVICE_4
    { .dev = &SPI4, .sck = IO_TAG(SPI4_PIN_SCLK), .miso = IO_TAG(SPI4_PIN_MRST), .mosi = IO_TAG(SPI4_PIN_MTSR)},
#else
    { .dev = NULL },
#endif
};

/**
 * 
 */
SPIDevice spiDeviceByInstance(IfxQspi_SpiMaster_Channel *instance){
    IfxQspi_SpiMaster *handle = (IfxQspi_SpiMaster *)instance->base.driver;
#ifdef USE_SPI_DEVICE_1
    if (handle->qspi == SPI1.qspi)
        return SPIDEV_1;
#endif

#ifdef USE_SPI_DEVICE_2
    if (handle->qspi == SPI2.qspi)
        return SPIDEV_2;
#endif

#ifdef USE_SPI_DEVICE_3
    if (handle->qspi == SPI3.qspi)
        return SPIDEV_3;
#endif

#ifdef USE_SPI_DEVICE_4
    if (handle->qspi == SPI4.qspi)
        return SPIDEV_4;
#endif

    return SPIINVALID;
}

bool spiInitBus(busDevice_t * busDev, bool leadingEdge){
    SPIDevice spiBus = busDev->busdev.spi.spiBus;
    spiDevice_t *spi = &(spiHardwareMap[spiBus]);

    if (!spi->dev) {
        return false;
    }

    if (!(spi->initDone)) {
        IOInit(IOGetByTag(spi->sck),  OWNER_SPI, RESOURCE_SPI_SCK,  spiBus + 1);
        if (leadingEdge) {
            IOLo(IOGetByTag(spi->sck));
        } else {
            IOHi(IOGetByTag(spi->sck));
        }
        IOInit(IOGetByTag(spi->miso), OWNER_SPI, RESOURCE_SPI_MISO, spiBus + 1);
        IOInit(IOGetByTag(spi->mosi), OWNER_SPI, RESOURCE_SPI_MOSI, spiBus + 1);

        //just redo the entire module config each time a device is initialized
        IfxQspi_SpiMaster_Config *spiMasterConfig = &(spi->dev->spiConfig);
        IfxQspi_SpiMaster_initModuleConfig(spiMasterConfig, spi->dev->qspi);

        // set the desired mode and maximum baudrate
        spiMasterConfig->base.mode = SpiIf_Mode_master;
        spiMasterConfig->base.maximumBaudrate = spiSpeedMap[SPI_CLOCK_ULTRAFAST];

        // ISR priorities and interrupt target
        spiMasterConfig->base.txPriority = spi->dev->txPriority;
        spiMasterConfig->base.rxPriority = spi->dev->rxPriority;
        //spiMasterConfig->base.erPriority       = IFX_INTPRIO_QSPI0_ER;
        spiMasterConfig->base.isrProvider = IfxCpu_Irq_getTos(IfxCpu_getCoreIndex());

        // pin configuration
        const IfxQspi_SpiMaster_Pins pins = {
            getSclkPinmapFromIoTag(spi->sck, spi->dev->qspi), IfxPort_OutputMode_pushPull, // SCLK
            getMtsrPinmapFromIoTag(spi->mosi, spi->dev->qspi), IfxPort_OutputMode_pushPull, // MTSR
            getMrstPinmapFromIoTag(spi->miso, spi->dev->qspi), IfxPort_InputMode_pullUp,  // MRST
            IfxPort_PadDriver_cmosAutomotiveSpeed1 // pad driver mode, fastest rise time
        };
        spiMasterConfig->pins = &pins;

        // initialize module
        IfxQspi_SpiMaster_initModule(&(spi->dev->spiMaster), spiMasterConfig);

        spi->initDone = true;
    }

    //now we do the channel init
    IfxQspi_SpiMaster_ChannelConfig spiMasterChannelConfig;
    IfxQspi_SpiMaster_initChannelConfig(&spiMasterChannelConfig, &(spi->dev->spiMaster));

    // set the baudrate for this channel
    spiMasterChannelConfig.base.baudrate = spiSpeedMap[SPI_CLOCK_INITIALIZATON];

    // select pin configuration
    IfxQspi_Slso_Out * chipSelect = getSlsPinmapFromIoTag(busDev->descriptorPtr->busdev.spi.csnPin);
    if(NULL_PTR == chipSelect){
        return false;
    }
    const IfxQspi_SpiMaster_Output slsOutput = {
        chipSelect,
        IfxPort_OutputMode_pushPull,
        IfxPort_PadDriver_cmosAutomotiveSpeed1
    };
    spiMasterChannelConfig.sls.output = (IfxQspi_SpiMaster_Output)slsOutput;

    //chip select handled by inav
    //spiMasterChannelConfig.base.mode.csLeadDelay = 2;
    //spiMasterChannelConfig.base.mode.csTrailDelay = 2;

    if (leadingEdge) {
        // SPI Mode 0 (CPOL=0, CPHA=0)
        spiMasterChannelConfig.base.mode.clockPolarity = SpiIf_ClockPolarity_idleLow;
        spiMasterChannelConfig.base.mode.shiftClock = SpiIf_ShiftClock_shiftTransmitDataOnTrailingEdge;
    } else {
        // SPI Mode 3 (CPOL=1, CPHA=1)
        spiMasterChannelConfig.base.mode.clockPolarity = SpiIf_ClockPolarity_idleHigh;
        spiMasterChannelConfig.base.mode.shiftClock = SpiIf_ShiftClock_shiftTransmitDataOnLeadingEdge;
    }

    // initialize channel
    IfxQspi_SpiMaster_initChannel(busDev->spiChannel, &spiMasterChannelConfig);

    //manually turn off chip select
    spi->dev->spiMaster.qspi->SSOC.B.OEN = 0x0; // OEN bitfield controls the automatic for each CS

    //send some dummy data to prepare idle levels. CS should be disabled at this point. Therefore the dummmy data should have no effect
    uint8_t dummyData = 0xFF;
    IfxQspi_SpiMaster_exchange(busDev->spiChannel, &dummyData, NULL_PTR, 1); 
    while(spiIsBusBusy(busDev->spiChannel));  

    return true;
}

uint32_t PSPR_FUNCTION spiTimeoutUserCallback(IfxQspi_SpiMaster_Channel *instance){
    SPIDevice device = spiDeviceByInstance(instance);
    if (device == SPIINVALID) {
        return -1;
    }
    spiHardwareMap[device].errorCount++;
    return spiHardwareMap[device].errorCount;
}

uint8_t PSPR_FUNCTION spiTransferByte(IfxQspi_SpiMaster_Channel *instance, uint8_t data){
    uint16_t spiTimeout = 1000;
    uint8_t rxData = 0;
    IfxQspi_SpiMaster_exchange(instance, &data, &rxData, 1); 
    while(spiIsBusBusy(instance)){
        if ((spiTimeout--) == 0)
            return spiTimeoutUserCallback(instance);
    } 
    return rxData;
}

bool PSPR_FUNCTION spiIsBusBusy(IfxQspi_SpiMaster_Channel *instance){
    return (IfxQspi_SpiMaster_getStatus(instance) == SpiIf_Status_busy);
}

bool PSPR_FUNCTION spiTransfer(IfxQspi_SpiMaster_Channel *instance, uint8_t *rxData, const uint8_t *txData, int len){
    uint16_t spiTimeout = 1000;
    IfxQspi_SpiMaster_exchange(instance, txData, rxData, len); 
    while(spiIsBusBusy(instance)){
        if ((spiTimeout--) == 0){
            spiTimeoutUserCallback(instance);
            return false;
        }
    }
    return true;
}

void spiSetSpeed(IfxQspi_SpiMaster_Channel *instance, SPIClockSpeed_e speed){
    IfxQspi_SpiMaster_setChannelBaudrate(instance, spiSpeedMap[speed]);
}

uint16_t spiGetErrorCounter(IfxQspi_SpiMaster_Channel *instance){
    SPIDevice device = spiDeviceByInstance(instance);
    if (device == SPIINVALID) {
        return 0;
    }
    return spiHardwareMap[device].errorCount;
}

void spiResetErrorCounter(IfxQspi_SpiMaster_Channel *instance){
    SPIDevice device = spiDeviceByInstance(instance);
    if (device != SPIINVALID) {
        spiHardwareMap[device].errorCount = 0;
    }
}

#endif // USE_SPI
