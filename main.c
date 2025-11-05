#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "clock.h"
#include "wait.h"
#include "uart0.h"
#include "rgb_led.h"

#define ENCODER_PORT PORTB
#define ENCODER_PIN  2  // example: PB2 connected to slot sensor output

// Initialize encoder input pin
void initEncoder(void)
{
    enablePort(ENCODER_PORT);
    selectPinDigitalInput(ENCODER_PORT, ENCODER_PIN);
    enablePinPullup(ENCODER_PORT, ENCODER_PIN);  // optional, if open-collector sensor
}

// Read the current slot state (1 = light passes, 0 = blocked)
bool readEncoderSlot(void)
{
    return getPinValue(ENCODER_PORT, ENCODER_PIN);
}

// Example main: read slot transitions and print over UART
int main(void)
{
    initSystemClockTo40Mhz();
    initUart0();
    initEncoder();
    initRgb();

    bool lastState = readEncoderSlot();
    while (true)
    {
        bool currentState = readEncoderSlot();
        if (currentState != lastState)
        {
            // Slot changed: either entering or leaving beam
            putsUart0(currentState ? "Light detected\r\n" : "Blocked\r\n");
            // Blink RGB LED briefly when light is detected
            if (currentState)
            {
                setRgbColor(0, 800, 0);       // red on
                waitMicrosecond(200000);      // 200 ms
                setRgbColor(0, 0, 0);         // off
            }
            lastState = currentState;
        }
        waitMicrosecond(1000);  // debounce/sampling delay
    }
}
