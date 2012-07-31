#ifndef LCD_Library
#define LCD_Library

#define LCD_RS(X)       P1OUT = ((P1OUT & ~(BIT0)) | (X))
#define LCD_EN(X)       P1OUT = ((P1OUT & ~(BIT1)) | (X<<1))
#define LCD_STROBE      do{LCD_EN(1);LCD_EN(0);}while(0)
#define LCD_DATABITS    P1OUT  // P1.7 - D7, ....., P1.4 - D4
#define LCD_LINE1       lcd_cmd(0x80)
#define LCD_LINE2       lcd_cmd(0xc0)


void lcd_data(unsigned char c);
void lcd_cmd(unsigned char c);
void lcd_pseudo_8bit_cmd(unsigned char c);
void lcd_clear(void);
void lcd_init();
void lcd_print(char *p);
char *ltoa(long num, char *str, int radix);


#endif