//=============================================================================
// IGVC Dual Encoder Distance & Angle Calculator
//-----------------------------------------------------------------------------
// Target MCU: TM4C123GH6PM (Tiva C LaunchPad)
// System Clock: 40 MHz
// Author: <Your Name>
// Date: <Date>
//-----------------------------------------------------------------------------
// Purpose:
// Measure wheel rotation from two optical encoders (left & right) using
// Wide Timer Module 1 Capture Mode (WT1CCP0 and WT1CCP1). Compute each
// wheel’s RPM, distance, total robot travel distance, and heading angle.
//
//-----------------------------------------------------------------------------
// Hardware Configuration:
//
// Left Wheel Encoder Input:
// - Signal Pin: PC6 (WT1CCP0) → Wide Timer 1, Channel A
// - Direction: Input Capture (measures time between rising edges)
// - Registers:
// GPIO_PORTC_AFSEL_R: enable alternate function for PC6
// GPIO_PORTC_PCTL_R: set PC6 to function WT1CCP0 (0x07 in PCTL field)
// GPIO_PORTC_DEN_R: enable digital function on PC6
// WTIMER1_TAMR_R: configure Timer1A for edge-time capture mode
// WTIMER1_CTL_R: TAEVENT_POS → capture on positive edge
// WTIMER1_IMR_R: enable capture event interrupt (CAEIM)
// NVIC_EN3_R: enable interrupt vector 112 (WTIMER1A)
//
// Right Wheel Encoder Input:
// - Signal Pin: PC7 (WT1CCP1) → Wide Timer 1, Channel B
// - Direction: Input Capture (measures time between rising edges)
// - Registers:
// GPIO_PORTC_AFSEL_R: enable alternate function for PC7
// GPIO_PORTC_PCTL_R: set PC7 to function WT1CCP1 (0x07 in PCTL field)
// GPIO_PORTC_DEN_R: enable digital function on PC7
// WTIMER1_TBMR_R: configure Timer1B for edge-time capture mode
// WTIMER1_CTL_R: TBEVENT_POS → capture on positive edge
// WTIMER1_IMR_R: enable capture event interrupt (CBEIM)
// NVIC_EN3_R: enable interrupt vector 113 (WTIMER1B)
//
// Common Timer Configuration:
// - WTIMER1_CFG_R = 0x4 → configure as two 32-bit independent timers (A & B)
// - WTIMER1_TAV_R, WTIMER1_TBV_R → hold current count values for channels A/B
// - WTIMER1_ICR_R → used to clear capture interrupt flags
//
// Other Peripherals:
// - UART0 (PA0/PA1) used for serial output at 115200 baud
// - RGB LED module for status indication
//
//-----------------------------------------------------------------------------
// Calculation Summary:
//
// Each encoder has 18 teeth per revolution.
// Tire circumference = 45 inches
// Wheelbase = 21.89 inches (556 mm)
//
// RPM = (SYSCLK _ 60) / (ticks _ TEETH_PER_REV)
// Distance per wheel = (edge_count / TEETH_PER_REV) \* CIRCUMFERENCE
// Heading angle θ(rad) = (RightDist - LeftDist) / Wheelbase
// Forward distance = average(LeftDist, RightDist)
//
//-----------------------------------------------------------------------------
// Notes:
// - Ticks are measured in system clock cycles (25 ns @ 40 MHz).
// - WTIMER1 operates in edge-time capture up-count mode.
// - Both channels share the same NVIC group (NVIC_EN3_R).
// - Timeout detection used to zero RPM when no pulses are received.
//
//=============================================================================
