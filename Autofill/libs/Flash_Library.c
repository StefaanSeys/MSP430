#include "Flash_Library.h"
#include <msp430.h>



// Write counter value to flash info segment B
void store_counter(char* ptr) {
  char *flash_ptr;                          // Flash pointer
  flash_ptr = (char *)info_seg_B;           // Initialize Flash pointer
  

  //char* count = (char *) &counter;     // Initialize pointer to counter
  
  // Erase the flash segment
  FCTL1 = FWKEY + ERASE;                    // Set Erase bit
  FCTL3 = FWKEY;                            // Clear Lock bit
  *flash_ptr = 0;                           // Dummy write to erase Flash segment
  while(FCTL3 & BUSY);                      // Wait until erase is complete
  
  // Write value to flash
  FCTL1 = FWKEY + WRT;                      // Set WRT bit for write operation  
  
  int i = 0;    
  for (i=0; i<12; i++) {
    *flash_ptr = *ptr;
    flash_ptr++;
    ptr++;
    while((FCTL3 & BUSY));                    // Wait until write completes
  } 
  
  FCTL1 = FWKEY;                            // Clear WRT bit
  FCTL3 = FWKEY + LOCK;                     // Set LOCK bit  
}



// Read the counter value
CountStruct read_counter() {  
  return *((CountStruct *)info_seg_B);  
}