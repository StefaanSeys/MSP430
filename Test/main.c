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
pseudo_8bit_cmd function since the LCD expects an 8 bit 
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
 
#define RS(X)     P1OUT = ((P1OUT & ~(BIT0)) | (X))
#define EN(X)   P1OUT = ((P1OUT & ~(BIT1)) | (X<<1))
#define LCD_STROBE do{EN(1);EN(0);}while(0)
#define databits P1OUT  // P1.7 - D7, ....., P1.4 - D4
#define LINE1 cmd(0x80)
#define LINE2 cmd(0xc0)
#define     BUTTON                BIT3     // Button on P1.3
#define     BKL                   BIT2     // LCD Back light led on P1.2 

#define info_seg_B          0x1080
 
// How many times have we pushed the button?
long  buttonPresses = 0;


void port_init()
{
    P1OUT = 0 ;
    P1DIR = 0xff;
}
 
void data(unsigned char c)
{
    RS(1);
    __delay_cycles(40);  //40 us delay
    databits = (databits & 0x0f) | (c & 0xf0);
    LCD_STROBE;
    databits = (databits & 0x0f) | (c << 4) ;
    LCD_STROBE;
}
 
void cmd(unsigned char c)
{
    RS(0);
    __delay_cycles(40); //40 us delay
    databits = (databits & 0x0f) | (c & 0xf0);
    LCD_STROBE;
    databits = (databits & 0x0f) | (c << 4) ;
    LCD_STROBE;
}
 
void pseudo_8bit_cmd(unsigned char c)
{
    RS(0);
    __delay_cycles(15000); //15 ms delay
    databits = (c & 0xf0);
    LCD_STROBE;
}
void clear(void)
{
    cmd(0x01);
    __delay_cycles(3000); //3 ms delay
}
 
void lcd_init()
{
    pseudo_8bit_cmd(0x30); //this command is like 8 bit mode command
    pseudo_8bit_cmd(0x30); //lcd expect 8bit mode commands at first
    pseudo_8bit_cmd(0x30); //for more details, check any 16x2 lcd spec
    pseudo_8bit_cmd(0x20);
    cmd(0x28);             //4 bit mode command started, set two line
    cmd(0x0c);             // Make cursorinvisible
    clear();               // Clear screen
    cmd(0x6);              // Set entry Mode(auto increment of cursor)
}
 
void string(char *p)
{
    while(*p) data(*p++);
}

char *ltoa(long num, char *str, int radix) {
  char sign = 0;		
  char temp[33];  //an int can only be 32 bits long		
                  //at radix 2 (binary) the string 		
                  //is at most 16 + 1 null long.
		
  int temp_loc = 0;		
  int digit;		
  int str_loc = 0;
		
		
  //save sign for radix 10 conversion		
  if (radix == 10 && num < 0) {		
      sign = 1;		
      num = -num;		
  }
		
  
		
  //construct a backward string of the number.		
  do {		
      digit = (unsigned long)num % radix;		
      if (digit < 10) 		
          temp[temp_loc++] = digit + '0';		
      else		
          temp[temp_loc++] = digit - 10 + 'A';	
      num = ((unsigned long)num) / radix;	
  } while ((unsigned long)num > 0);
		
		
  //now add the sign for radix 10		
  if (radix == 10 && sign) {		
      temp[temp_loc] = '-';		
  } else {		
      temp_loc--;		
  }
		
		
		
  //now reverse the string.		
  while ( temp_loc >=0 ) {// while there are still chars		
      str[str_loc++] = temp[temp_loc--];		
  }
		
  str[str_loc] = 0; // add null termination.		
  return str;
}


// Write counter value to flash info segment B
void store_counter () {
  char *flash_ptr;                          // Flash pointer
  flash_ptr = (char *)info_seg_B;           // Initialize Flash pointer
  
  char* count = (char *) &buttonPresses;     // Initialize pointer to counter
  
  // Erase the flash segment
  FCTL1 = FWKEY + ERASE;                    // Set Erase bit
  FCTL3 = FWKEY;                            // Clear Lock bit
  *flash_ptr = 0;                           // Dummy write to erase Flash segment
  while(FCTL3 & BUSY);                      // Wait until erase is complete
  
  // Write value to flash
  FCTL1 = FWKEY + WRT;                      // Set WRT bit for write operation  
  
  int i = 0;    
  for (i=0; i<4; i++) {
    *flash_ptr = *count;
    flash_ptr++;
    count++;
    while((FCTL3 & BUSY));                    // Wait until write completes
  } 
  
  FCTL1 = FWKEY;                            // Clear WRT bit
  FCTL3 = FWKEY + LOCK;                     // Set LOCK bit  
}

// Read the counter value
void read_counter() {  
  buttonPresses = *((long *)info_seg_B);  
}



 
int main()
{
  WDTCTL = WDTPW + WDTHOLD; // Stop watchdog timer
  
  BCSCTL3 |= LFXT1S1; // Use the VLO for the ACLK (~12 kHz)
  BCSCTL1 |= DIVA_3;  // ACLK divide by 8
    
    port_init();
    lcd_init();

    // Set up button (P1.3)
    P1DIR &= ~BUTTON;
    P1OUT |= BUTTON;
    P1REN |= BUTTON;
    P1IES |= BUTTON;
    P1IFG &= ~BUTTON;
    P1IE |= BUTTON;
    
    // Read the counter value from flash    
    read_counter();
    
    // Set up the BKL (P1.2)
    P1DIR |= BKL;   // direction is OUT
    P1OUT &= ~BKL;  // turn OFF
    
    TACCR0 = 12000;				// Count limit (will give about 8 seconds of light)
    CCTL0 = 0x00;				// Disable counter interrupts
    TACTL = TASSEL_1 + MC_1 + ID_3 + TACLR;		// Timer A with ACLK/8, count UP, reset counter

    
    // __enable_interrupt();
     
     LINE1;
     string("Druk op knop");
     
     __bis_SR_register(LPM3_bits + GIE);
     
     clear();
     
     while (1) {    
        // display count
        LINE1;
        string("Vandaag");
        string(": ");
        LINE2;
        string("Totaal: ");
        char times[8];
        ltoa(buttonPresses, times, 10);
        string(times);
        // store count in flash
        store_counter();
        // go back to sleep
        __bis_SR_register(LPM3_bits + GIE);
     }
}

// Update the button press count, and toggle LEDs, when the button is pressed
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{ 
  P1OUT |= BKL;  // turn ON
  buttonPresses++;
  P1IFG = 0; // clear interrupt
  CCTL0 = CCIE;				// Enable counter interrupts, bit 4=1
  __bic_SR_register_on_exit(LPM0_bits + GIE);   // Wake up the main loop
}

// Timer interrupt, turn off the LCD
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void) {			
  P1OUT &= ~BKL;  // turn OFF the BKL
  CCTL0 = 0x00;					// Disable counter interrupts
  //__bic_SR_register_on_exit(LPM4_bits + GIE); // Go to sleep mode with clocks disabled (LPM4)         
}

