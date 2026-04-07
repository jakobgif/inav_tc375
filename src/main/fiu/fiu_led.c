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

/*
 * FIU LED Strip Module -- software bit-bang, no DMA/timer required.
 *
 * Drives a WS2811/WS2812 strip directly via GPIO using delayNanos().
 * The strip is only updated when the FIU fault state actually changes,
 * so CPU usage is minimal.
 *
 * LED layout:
 *   0-5  Motor 0-5  RED    when motor disabled by FIU
 *   6    I2C fault  ORANGE when any I2C bus is blocked
 *   7    SPI fault  YELLOW when any SPI bus is blocked
 *   8    Baro stuck PURPLE when barometer stuck detected
 */

#include "platform.h"

#ifdef USE_FIU

#include <stdint.h>
#include <stdbool.h>

#include "fiu/fiu.h"
#include "fiu/fiu_detection.h"
#include "fiu/fiu_led.h"
#include "drivers/io.h"
#include "drivers/time.h"

// ---------------------------------------------------------------------------
// LED layout
// ---------------------------------------------------------------------------
#define LED_MOTOR_BASE   0   // LEDs 0-5: one per motor
#define LED_I2C          6
#define LED_SPI          7
#define LED_BARO_STUCK   8
#define LED_COUNT        8

// ---------------------------------------------------------------------------
// Color (WS2812 expects GRB byte order)
// ---------------------------------------------------------------------------
typedef struct { uint8_t g; uint8_t r; uint8_t b; } rgb_t;

static const rgb_t COLOR_OFF    = {  0,   0,   0};
static const rgb_t COLOR_RED    = {  0, 180,   0};
static const rgb_t COLOR_ORANGE = { 80, 180,   0};
static const rgb_t COLOR_YELLOW = {180, 180,   0};
static const rgb_t COLOR_PURPLE = {  0,   0, 180};

static rgb_t  ledBuffer[LED_COUNT];
static IO_t   dataPin;

// ---------------------------------------------------------------------------
// Bit-bang WS2811 (9 LEDs = 270 us, interrupts disabled during transfer)
// ---------------------------------------------------------------------------
static void sendByte(uint8_t byte)
{
    for (int8_t bit = 7; bit >= 0; bit--) {
        if (byte & (1 << bit)) {
            IOHi(dataPin); delayNanos(700);
            IOLo(dataPin); delayNanos(600);
        } else {
            IOHi(dataPin); delayNanos(350);
            IOLo(dataPin); delayNanos(800);
        }
    }
}

static void sendStrip(void)
{
    bool irqState = IfxCpu_disableInterrupts();

    for (uint8_t i = 0; i < LED_COUNT; i++) {
        sendByte(ledBuffer[i].g);
        sendByte(ledBuffer[i].r);
        sendByte(ledBuffer[i].b);
    }

    IfxCpu_restoreInterrupts(irqState);

    delayMicroseconds(60); // WS2811 reset pulse (>50 us)
}

// ---------------------------------------------------------------------------
// fiuLedInit -- called once from fc_init.c
// ---------------------------------------------------------------------------
void fiuLedInit(void)
{
    dataPin = IOGetByTag(IO_TAG(FIU_LED_PIN));
    IOInit(dataPin, OWNER_LED_STRIP, RESOURCE_OUTPUT, 0);
    IOConfigGPIO(dataPin, IOCFG_OUT_PP);
    IOLo(dataPin);
}

// ---------------------------------------------------------------------------
// fiuLedUpdate -- called at 100 Hz from fiuDetectionUpdate()
// Strip is only re-sent when fault state actually changes.
// ---------------------------------------------------------------------------
void fiuLedUpdate(void)
{
    // TEST: alle LEDs weiß -- zum Testen der Hardware (Level Shifter)
    // Sobald die LEDs leuchten, diesen Block durch die Fault-Mapping-Logik ersetzen.
    static bool sent = false;
    if (sent) {
        return;
    }
    sent = true;

    static const rgb_t COLOR_WHITE = {180, 180, 180};
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        ledBuffer[i] = COLOR_WHITE;
    }
    sendStrip();
}

#endif // USE_FIU