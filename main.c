// IGVC Dual Encoder
// Teeth per revolution: 18
// Left: PC6 → WT1CCP0
// Right: PC7 → WT1CCP1

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "clock.h"
#include "wait.h"
#include "uart0.h"
#include "rgb_led.h"

#define LEFT_FREQ_IN_MASK   0x40   // PC6 -> WT1CCP0
#define RIGHT_FREQ_IN_MASK  0x80   // PC7 -> WT1CCP1
#define TEETH_PER_REV       18
#define SYSCLK_HZ           40000000
#define TIRE_CIRCUMFERENCE_IN 45.0f


const uint32_t TIMEOUT_TICKS = SYSCLK_HZ / 2; // 0.5s timeout (500ms)
// Left wheel data
volatile uint32_t left_ticks = 0;
volatile uint64_t left_edges = 0;
float left_rpm = 0;
float left_distance_in = 0;

// Right wheel data
volatile uint32_t right_ticks = 0;
volatile uint64_t right_edges = 0;
float right_rpm = 0;
float right_distance_in = 0;

//-----------------------------------------------------------------------------
// Hardware Initialization
//-----------------------------------------------------------------------------
void initHw()
{
    initSystemClockTo40Mhz();

    // Enable clocks
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R1; // enable Wide Timer 1
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2;     // enable Port C
    _delay_cycles(3);

    // Configure PC6 and PC7 for WT1CCP0 and WT1CCP1
    GPIO_PORTC_AFSEL_R |= LEFT_FREQ_IN_MASK | RIGHT_FREQ_IN_MASK;
    GPIO_PORTC_PCTL_R &= ~(GPIO_PCTL_PC6_M | GPIO_PCTL_PC7_M);
    GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC6_WT1CCP0 | GPIO_PCTL_PC7_WT1CCP1;
    GPIO_PORTC_DEN_R |= LEFT_FREQ_IN_MASK | RIGHT_FREQ_IN_MASK;
}

//-----------------------------------------------------------------------------
// Timer Configuration
//-----------------------------------------------------------------------------
void enableDualEncoderMode()
{
    WTIMER1_CTL_R &= ~(TIMER_CTL_TAEN | TIMER_CTL_TBEN);   // disable both channels
    WTIMER1_CFG_R = 4;                                     // 32-bit timers, A and B independent

    // --- Left wheel (Timer A) ---
    WTIMER1_TAMR_R = TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP | TIMER_TAMR_TACDIR;
    WTIMER1_CTL_R |= TIMER_CTL_TAEVENT_POS;                // capture on rising edge
    WTIMER1_IMR_R |= TIMER_IMR_CAEIM;                      // enable capture interrupt

    // --- Right wheel (Timer B) ---
    WTIMER1_TBMR_R = TIMER_TBMR_TBCMR | TIMER_TBMR_TBMR_CAP | TIMER_TBMR_TBCDIR;
    WTIMER1_CTL_R |= TIMER_CTL_TBEVENT_POS;
    WTIMER1_IMR_R |= TIMER_IMR_CBEIM;

    // Clear and start both counters
    WTIMER1_TAV_R = 0;
    WTIMER1_TBV_R = 0;
    WTIMER1_CTL_R |= (TIMER_CTL_TAEN | TIMER_CTL_TBEN);

    // Enable NVIC interrupts
    NVIC_EN3_R = (1 << (INT_WTIMER1A - 16 - 96)) | (1 << (INT_WTIMER1B - 16 - 96));
}

//-----------------------------------------------------------------------------
// Left Encoder ISR (WT1CCP0)
//-----------------------------------------------------------------------------
void WideTimer1A_Handler(void)
{
    left_ticks = WTIMER1_TAV_R;               // time between edges
    WTIMER1_TAV_R = 0;
    WTIMER1_ICR_R = TIMER_ICR_CAECINT;        // clear interrupt
    left_edges++;
}

//-----------------------------------------------------------------------------
// Right Encoder ISR (WT1CCP1)
//-----------------------------------------------------------------------------
void WideTimer1B_Handler(void)
{
    right_ticks = WTIMER1_TBV_R;              // time between edges
    WTIMER1_TBV_R = 0;
    WTIMER1_ICR_R = TIMER_ICR_CBECINT;        // clear interrupt
    right_edges++;
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
int main(void)
{
    initUart0();
    initRgb();
    initHw();
    enableDualEncoderMode();

    while (true)
    {
        // Left wheel RPM & distance
        uint32_t elapsed = WTIMER1_TAV_R; // ticks since last edge
        
        if (left_ticks == 0 || elapsed >= TIMEOUT_TICKS) {
            left_rpm = 0.0f;
        } else {
            left_rpm = (float)SYSCLK_HZ * 60.0f / (left_ticks * TEETH_PER_REV);
        }

        left_distance_in = (left_edges / (float)TEETH_PER_REV) * TIRE_CIRCUMFERENCE_IN;

        // Right wheel RPM & distance
        uint32_t elapsed_right = WTIMER1_TBV_R; // ticks since last edge

        if (right_ticks == 0 || elapsed_right >= TIMEOUT_TICKS) {
            right_rpm = 0.0f;
        } else {
            right_rpm = (float)SYSCLK_HZ * 60.0f / (right_ticks * TEETH_PER_REV);
        }   
        right_distance_in = (right_edges / (float)TEETH_PER_REV) * TIRE_CIRCUMFERENCE_IN;

        // Display
        char buf[128];
        sprintf(buf,
                "L: RPM=%.1f Dist=%.2f in | R: RPM=%.1f Dist=%.2f in\r\n",
                left_rpm, left_distance_in, right_rpm, right_distance_in);
        putsUart0(buf);

        waitMicrosecond(200000);
    }
}





// // IGVC Encoder
// // Teeth per revolution: 18
// // Input pin: PC6 WT1CCP0

// #include <stdint.h>
// #include <stdbool.h>

// #include <stdio.h>
// #include <string.h>
// #include "tm4c123gh6pm.h"
// #include "gpio.h"
// #include "clock.h"
// #include "wait.h"
// #include "uart0.h"
// #include "rgb_led.h"


// #define FREQ_IN_MASK    0x40               // PC6 frequency-> WT1CCP0
// #define TEETH_PER_REV 18
// #define SYSCLK_HZ     40000000
// #define TIRE_CIRCUMFERENCE_IN 45.0f

// uint32_t ticks = 0;
// //float period = 0; // time between teeth in seconds
// float rpm = 0;
// uint64_t edge_count = 0;
// float distance_in = 0.0f;   // total distance in inches

    


// void initHw()
// {
//     // Initialize system clock to 40 MHz
//     initSystemClockTo40Mhz();

//     // Enable clocks
//     SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
//     SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R1;
//     SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2;
//     _delay_cycles(3);

//     // Configure SIGNAL_IN for frequency and time measurements
// 	GPIO_PORTC_AFSEL_R |= FREQ_IN_MASK;              // select alternative functions for SIGNAL_IN pin
//     GPIO_PORTC_PCTL_R &= ~GPIO_PCTL_PC6_M;           // map alt fns to SIGNAL_IN
//     GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC6_WT1CCP0;
//     GPIO_PORTC_DEN_R |= FREQ_IN_MASK;                // enable bit 6 for digital input
// }

// void enableTimerMode()
// {
//     WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;                // turn-off counter before reconfiguring
//     WTIMER1_CFG_R = 4;                               // configure as 32-bit counter (A only)
//     WTIMER1_TAMR_R = TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP | TIMER_TAMR_TACDIR;
//                                                      // configure for edge time mode, count up
//     WTIMER1_CTL_R = TIMER_CTL_TAEVENT_NEG;           // measure time from negative edge to positive edge
//     WTIMER1_IMR_R = TIMER_IMR_CAEIM;                 // turn-on interrupts
//     WTIMER1_TAV_R = 0;                               // zero counter for first period
//     WTIMER1_CTL_R |= TIMER_CTL_TAEN;                 // turn-on counter
//     NVIC_EN3_R = 1 << (INT_WTIMER1A-16-96);          // turn-on interrupt 112 (WTIMER1A)
// }

// // Period timer service publishing latest time measurements every negative edge
// void wideTimer1Isr()
// {
//     ticks = WTIMER1_TAV_R;                        // read counter input
//     edge_count++;
//     setRgbColor(0, 1023, 0);
//     WTIMER1_TAV_R = 0;                           // zero counter for next edge
//     WTIMER1_ICR_R = TIMER_ICR_CAECINT;           // clear interrupt flag
// }


// // Example main: read slot transitions and print over UART
// int main(void)
// {
//     initUart0();
//     initRgb();
//     initHw();
//     enableTimerMode();



//    // bool lastState = readEncoderSlot();
//     uint16_t count = 0;

//     while (true)
//     {
//         uint32_t elapsed = WTIMER1_TAV_R; // ticks since last edge
//         const uint32_t TIMEOUT_TICKS = SYSCLK_HZ / 2; // 0.5s timeout (500ms)

//         if (ticks == 0 || elapsed >= TIMEOUT_TICKS) {
//             //period = 0.0f;
//             rpm = 0.0f;
//         } else {
//             //period = (float)ticks / SYSCLK_HZ;
//             rpm = (float)SYSCLK_HZ * 60.0f / (ticks * TEETH_PER_REV);
//         }
//         distance_in = (edge_count / (float)TEETH_PER_REV) * TIRE_CIRCUMFERENCE_IN;
//         float distance_meters = distance_in * 0.0254f;

//         char buf[48];
//         sprintf(buf, "Ticks: %lu  RPM: %.2f  Distance: %.2f meters\r\n", ticks, rpm, distance_meters);
//         putsUart0(buf);

//         waitMicrosecond(100000); // 0.1s delay
//     }
// }
