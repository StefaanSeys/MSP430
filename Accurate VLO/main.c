//******************************************************************************
//  MSP430x20x2 Demo - Basic Clock, Output Buffered SMCLK, ACLK and MCLK/10
//
//  Description: This example toggles P1.0 on approximately a 1 second interval
//  The DCO is set to 8MHz to demonstrate that clock settings are preserved
//  across the function call.
//  ACLK = VLO, MCLK = SMCLK = 8MHz
//
//               MSP430F2012
//             -----------------
//         /|\|              XIN|-
//          | |                 |
//          --|RST          XOUT|-
//            |                 |
//            |       P1.4/SMCLK|-->SMCLK = 8MHz
//            |        P1.0/ACLK|-->LED 1 second interrupt
//
//  L. Westlund
//  Texas Instruments Inc.
//  March 2006
//  Built with IAR Embedded Workbench Version: 3.40A
//******************************************************************************

#include  <msp430G2231.h>
#include "VLO_Library.h"

int dco_delta;
int result;

void main(void)
{
  volatile unsigned int i;
  WDTCTL = WDTPW +WDTHOLD;                  // Stop Watchdog Timer
  P1DIR |= 0x11;                            // P1.0,1,4 outputs
  P1SEL |= 0x10;                            // P1.4 = SMCLK

  BCSCTL3 |= LFXT1S_2;                      // ACLK = VLO
  BCSCTL1 = CALBC1_1MHZ;                    // 1MHz cal value
  DCOCTL = CALDCO_1MHZ;                     // 1MHz cal value
  dco_delta = TI_measureVLO();              // dco delta = number of
                                            // 1MHz cycles in 8 ACLK cycles
  
  TACCTL0 = CCIE;                           // CCR0 interrupt enabled
  TACCR0 = (4000000 / dco_delta);           // Trigger every 1/2 second
  TACTL = TASSEL_1 + MC_1;                  // ACLK, upmode
  P1OUT = 0x01;
  _BIS_SR(LPM3_bits + GIE);                 // Enter LPM3 w/ interrupt

}

// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{
  P1OUT ^= 0x01;                            // Toggle P1.0
}
