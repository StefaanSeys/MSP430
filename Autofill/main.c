/*
*MSP430 launchpad based 4bit lcd code..
*Assuming the clock is 1MHz..
*All delays are designed according to that...
*for eg: __delay_cycles(1000) => 1ms delay at 1MHz clock
*Otherwise use any delay routines, if available.
*Note:
In my previous codes, the initialization is not so perfect
because I deviated from the original specifications...
But those codes are at least working for me,, Any way,
now I am providing a better code which includes a
lcd_pseudo_8bit_cmd function since the LCD expects an 8 bit 
command at first, before entering to 4 bit mode...
 
-Connections:
RS -> P1.0
EN -> P1.1
D7 to D4  -> P1.7 to P1.4
R/W to GROUND
 
-Compiler used: msp430-gcc
-Command line: msp430-gcc -mmcu=msp430g2231 lcd.c -mdisable-watchdog
-Burning code: sudo mspdebug rf2500
> prog a.out
> run
 
*/
 
#include <msp430g2231.h>
#include "VLO_Library.h"
#include "LCD_Library.h" 
#include "Flash_Library.h"


#define BUTTON                    BIT3     // Button on P1.3
#define VALVE                     BIT2     // LCD Back light led on P1.2 
#define LEVEL_SWITCH              BIT6     // Floater in the water on P2.6
#define WATER_METER               BIT7     // Water meter on P2.7

#define NORMAL_MODE               0
#define LONG_BUTTON_PRESS_MODE    1
#define TURN_OFF_DISPLAY_MODE     2

#define LOW_LEVEL                 0
#define HIGH_LEVEL                1


// timer constants for ACKL @ VLO/8 and timer @ ACKL/8
#define TIMER_1_SEC                     125000 / dco_delta    
#define TIMER_1_MIN                     7500000 / dco_delta

#define LEVEL_THRESHOLD                 5//480     // nr of min's in 8 hours
#define FLASH_THRESHOLD                 1440    // nr of min's in 24 hours





 
// Global variables
CountStruct counters;             // keeps track of the three counters
int timer_mode;                   // timer mode
int dco_delta;                    // dco delta = number of 1MHz cycles in 8 VLO cycles
int level_counter;                // keeps track of the nr of min's that have passed since
                                  // the last low water detection
int flash_counter;                // keeps track of the nr of min's since the last flash write
int level_mode;                   // the level is either low or high
int day;                          // current day counter on the screen


void port_init()
{
   
    
    // Set up button (P1.3)
    P1DIR &= ~BUTTON;
    P1OUT |= BUTTON;
    P1REN |= BUTTON;
    P1IES |= BUTTON;
    P1IFG &= ~BUTTON;
    P1IE |= BUTTON;
 
    // Set up the LCD Backlight LED (P1.2)
    P1DIR |= VALVE;   // direction is OUT
    P1OUT &= ~VALVE;  // turn OFF   
    
    P2SEL = 0x00;             // don't use the PINs for the crystal
    P2DIR &= ~BIT6;    
    P2REN |= BIT6;    // use the pull up or down resistor
    P2OUT |= BIT6;    // use the pull UP resistor
    
    
    // TODO: water counter will be installed on BIT7 of P2
    // Use the interrupt for this port (see earlier versions of code)

    
}
 

 
int main()
{
  WDTCTL = WDTPW + WDTHOLD; // Stop watchdog timer
  
  /* Setup the clock */
   
  BCSCTL1 = CALBC1_1MHZ;                    // 1MHz cal value
  DCOCTL = CALDCO_1MHZ;                     // 1MHz cal value
  
  dco_delta = TI_measureVLO();              // dco delta = number of
                                            // 1MHz cycles in 8 ACLK cycles
  
  BCSCTL3 |= LFXT1S_2;                      // ACLK = VLO  (~12 MHz)
  BCSCTL1 |= DIVA_3;                        // ACLK divide by 8
  
  /* Setup the Pins on the MCU */
  lcd_init();    // For some reason, lcd_init has to happen BEFORE port_init !!  
  port_init();


  /* Initiate TimerA */ 
  
  timer_mode = NORMAL_MODE;
  CCTL0 = 0x00;				    // Disable counter interrupts
  TACTL = TASSEL_1 + MC_1 + ID_3 + TACLR;   // Timer A with ACLK/8, count UP, reset counter
  TACCR0 = TIMER_1_SEC * 3;                     // Set interval to one minute
  CCTL0 = CCIE;		                    // Enable the timer
  
  /* Read the counter values from flash */  

  counters = read_counter();
  level_mode = HIGH_LEVEL;
  day = ~0x0;
  level_counter = 0;
  
  /* to be removed in final version */
  counters.total = 1239;
  counters.day1 = 346;    
     
     while (1) {   

       char str[8];
       int a;
       
       // display count
       lcd_clear();
       LCD_LINE1;
       
       // if high water, print "HIGH"
       if (level_mode) 
         lcd_print("HIGH");
       // if we are filling, print "FILL"
       else if (LEVEL_THRESHOLD-level_counter < 1)
         lcd_print("FILL");         
       else { // else print the nr of hours:min before refill       
          // print the nr of hours before refill
          int h = (LEVEL_THRESHOLD-level_counter)/60;
          ltoa(h, str, 10);
          lcd_print(str);
          // print the nr of minutes before refill
          a = ltoa(LEVEL_THRESHOLD-level_counter - h*60, str, 10);
          if (a == 1) lcd_print(":0"); else lcd_print(":");
          lcd_print(str); 
       }
       // print the total counter
       a = ltoa(counters.total, str, 10);
       while ((12-a++) > 0)
           lcd_print(" ");
       lcd_print(str);
       
       LCD_LINE2;
       
       // print the day counter
       int dc;
       if (day) {
         lcd_print("1:");
         dc = counters.day1;
       } else {
         lcd_print("2:");
         dc = counters.day2;
       }         
       a = ltoa(dc, str, 10);
       while ((14-a++) > 0)
           lcd_print(" ");
       lcd_print(str);

       // go back to sleep
       __bis_SR_register(LPM3_bits + GIE);
     }
}

// Update the button press count, and toggle LEDs, when the button is pressed
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{ 
  P1IFG = 0;        // clear interrupt
  P1IE &= ~BUTTON;  // disable interrupts for the button to handle button bounce
  P1IES ^= BUTTON;  // reverse interrupt direction H->L, L->H
  
  if (P1IN & BUTTON) // button release
    day = ~day;
  
  /* Use watchdog timer for button debounce */
  WDTCTL = WDT_ADLY_16;   // Turn on the watchdog timer to detect long press
                          // after 16ms*8*32KHz/12KHz = 341ms
  IFG1 &= ~WDTIFG;        // Clear watchdog timer interrupt flag
  IE1 |= WDTIE;           // Enable watchdog
  

  
    
  /* Reset TimerA such that the LCD BKL will keep burning */
  CCTL0 = 0;
  TAR = 0;
  
  /* Set the timer to about 1 second for long press detection */
  timer_mode = LONG_BUTTON_PRESS_MODE;
  TACCR0 = TIMER_1_SEC;  
  CCTL0 = CCIE;
  __bic_SR_register_on_exit(LPM3_bits);   // Wake up the main loop
}

// Timer interrupt, turn off the LCD
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void) {

  
  
  if (timer_mode == LONG_BUTTON_PRESS_MODE) {    
    timer_mode = TURN_OFF_DISPLAY_MODE;
    TACCR0 = TIMER_1_SEC * 5;
    if (~P1IN & BUTTON) { // the user is still pressing the button      
      if (day) counters.day1 = 0; else counters.day2 = 0;
      __bic_SR_register_on_exit(LPM3_bits);   // Wake up the main loop
    }
  } else if (timer_mode == TURN_OFF_DISPLAY_MODE) {  
    P1OUT &= ~VALVE;  // turn OFF the VALVE
    timer_mode = NORMAL_MODE;
    TACCR0 = TIMER_1_SEC*3;    
  } else { //normal mode
    
    
    
    /* recalibrate and re-init the timer (because the calibration messes it up) */
    dco_delta = TI_measureVLO();   
    CCTL0 = 0;
    TACTL = TASSEL_1 + MC_1 + ID_3 + TACLR;   // Timer A with ACLK/8, count UP, reset counter
    TACCR0 = TIMER_1_SEC*3;                     // Set interval to one minute
    CCTL0 = CCIE;		              // Enable the timer
 
    
    
    /* check state of water level meter */
    
    // if closed = low water
    if (~P2IN & LEVEL_SWITCH) {
      level_mode = LOW_LEVEL;      
      if (++level_counter == LEVEL_THRESHOLD) {
        // Turn on the water valve
        P1OUT |= VALVE;
      } 
    // if open = high water = turn off valve and reset counter
    } else {
      level_mode = HIGH_LEVEL;
      level_counter = 0;
      // Turn off the water valve
      P1OUT &= ~VALVE;
    }
    
    
    /* check state of flash */
    
    if (++flash_counter == FLASH_THRESHOLD) {
       store_counter((char *) &counters);
       flash_counter = 0;
    }
    
    __bic_SR_register_on_exit(LPM3_bits);   // Wake up the main loop
    
  }
  
}


//#pragma vector=PORT2_VECTOR
//__interrupt void PORT2_ISR(void)
//{
// P2IE  &= ~LEVEL_SWITCH;     // turn off the interrupt
//  P2IFG = 0;          // clear interrupt
  
  
   
  /* Use watchdog timer for button debounce */
//  WDTCTL = WDT_ADLY_16;   // Turn on the watchdog timer to detect long press
                          // after 16ms*8*32KHz/12KHz = 341ms
//  IFG1 &= ~WDTIFG;        // Clear watchdog timer interrupt flag
//  IE1 |= WDTIE;           // Enable watchdog
  
  
//  counters.total++;
//  counters.day1++;
// counters.day2++;
//}


/* This function catches watchdog timer interrupts, which are
 * set to happen 681ms after the user presses the button.
 */
#pragma vector=WDT_VECTOR
__interrupt void WDT_ISR (void)
{
    IE1 &= ~WDTIE;                // Disable interrupts on the watchdog timer
    IFG1 &= ~WDTIFG;              // Clear the interrupt flag for watchdog timer
    WDTCTL = WDTPW + WDTHOLD;     // Resume holding the watchdog timer so it doesn't reset the chip
    P1IE |= BUTTON;               // Re-enable interrupts for the button  
    //P2IE  |= LEVEL_SWITCH;                // Re-enable interrupt for "vlotter"
}