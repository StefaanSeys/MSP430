#include "LCD_Library.h"
#include <msp430.h>

void lcd_data(unsigned char c)
{
    LCD_RS(1);
    __delay_cycles(40);  //40 us delay
    LCD_DATABITS = (LCD_DATABITS & 0x0f) | (c & 0xf0);
    LCD_STROBE;
    LCD_DATABITS = (LCD_DATABITS & 0x0f) | (c << 4) ;
    LCD_STROBE;
}
 
void lcd_cmd(unsigned char c)
{
    LCD_RS(0);
    __delay_cycles(40); //40 us delay
    LCD_DATABITS = (LCD_DATABITS & 0x0f) | (c & 0xf0);
    LCD_STROBE;
    LCD_DATABITS = (LCD_DATABITS & 0x0f) | (c << 4) ;
    LCD_STROBE;
}
 
void lcd_pseudo_8bit_cmd(unsigned char c)
{
    LCD_RS(0);
    __delay_cycles(15000); //15 ms delay
    LCD_DATABITS = (c & 0xf0);
    LCD_STROBE;
}
void lcd_clear(void)
{
    lcd_cmd(0x01);
    __delay_cycles(3000); //3 ms delay
}
 
void lcd_init()
{
    lcd_pseudo_8bit_cmd(0x30); //this command is like 8 bit mode command
    lcd_pseudo_8bit_cmd(0x30); //lcd expect 8bit mode commands at first
    lcd_pseudo_8bit_cmd(0x30); //for more details, check any 16x2 lcd spec
    lcd_pseudo_8bit_cmd(0x20);
    lcd_cmd(0x28);             //4 bit mode command started, set two line
    lcd_cmd(0x0c);             // Make cursorinvisible
    lcd_clear();               // Clear screen
    lcd_cmd(0x6);              // Set entry Mode(auto increment of cursor)
}
 
void lcd_print(char *p)
{
    while(*p) lcd_data(*p++);
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
