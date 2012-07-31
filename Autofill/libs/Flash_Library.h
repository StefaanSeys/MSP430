#ifndef Flash_Library
#define Flash_Library

typedef struct {
   long  total;
   long  day1;
   long  day2;
} CountStruct;

#define info_seg_B          0x1080


void store_counter (char *);
CountStruct read_counter();


#endif