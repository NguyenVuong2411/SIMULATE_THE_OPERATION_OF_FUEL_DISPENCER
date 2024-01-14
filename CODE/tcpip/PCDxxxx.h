// pcdxxxx.h
//
// Microchip C30 to CCS conversion library.
//
// Wrapper for p24Hxxxx.h, p24Fxxxx.h, p30Fxxxx.h and 
//
// __PIC24F__
// __PIC24H__
// __dsPIC30F__
// __dsPIC33F__

#ifndef __PCDXXXX_H__
#define __PCDXXXX_H__

//#define __PCD_QUIRK_1459__

//#define __PCD_QUIRKA__   //still broken

//#define __PCD_QUIRKB__   //fixed in 5.054?

//#define __PCD_QUIRK3__   //fixed in 5.054?

#ifndef __C30__
#define __C30__
#endif
#ifndef __C30_VERSION__
#define __C30_VERSION__ 0
#endif

#zero_ram

#device PASS_STRINGS=IN_RAM
#device CONST=ROM
#device PSV=16

#case
#type signed
#type short=16 int=16 long=32

#case

#define _asm #asm
#define _endasm #endasm

#define GetSystemClock()       getenv("CLOCK")
#define GetInstructionClock()  (GetSystemClock()/2)
#define GetPeripheralClock()   (GetSystemClock()/2)

#if getenv("DEVICE") == "PIC24FJ256GA110"
   #define __PIC24FJ256GA110__
   #define __PIC24F__
   #include "24FJ256GA110_registers.h"
#elif getenv("DEVICE") == "PIC24FJ128GA010"
   #define __PIC24FJ128GA010__
   #define __PIC24F__
   #include "24FJ128GA010_registers.h"
#elif getenv("DEVICE") == "PIC24EP512GU810"
   #define __PIC24EP512GU810__
   #define __PIC24E__
   #include "24EP512GU810_registers.h"  
#elif getenv("DEVICE") == "DSPIC33EP256MU806"
   #define __dsPIC33EP256MU806__
   #define __dsPIC33E__
   #include "33EP256MU806_registers.h"
#elif getenv("DEVICE") == "PIC24FJ192GB106"
   #define __PIC24FJ192GA106__
   #define __PIC24F__
   #include "24FJ192GB106_registers.h"
#elif getenv("DEVICE") == "PIC24FJ256GB106"
   #define __PIC24FJ256GA106__
   #define __PIC24F__
   #include "24FJ192GB106_registers.h"
#elif getenv("DEVICE") == "PIC24FJ256GB206"
   #define __PIC24FJ256GA206__
   #define __PIC24F__
   #include "24FJ256GB206_registers.h"
#elif getenv("DEVICE") == "DSPIC33EP512GP506"
   #define __DSPIC33EP512GP506__
   #define __dsPIC33E__
   #include "33EP512GP506_registers.h"
#elif getenv("DEVICE") == "PIC24EP512GP206"
   #define __DSPIC24EP512GP206__
   #define __PIC24E__
   #include "24EP512GP206_registers.h"
#elif getenv("DEVICE") == "PIC24EP256GP206"
   #define __DSPIC24EP256GP206__
   #define __PIC24E__
   #include "24EP256GP206_registers.h"
#else
   #error do this for your chip
#endif

/*
#if defined(TRUE)
#undef TRUE
#endif

#if defined(FALSE)
#undef FALSE
#endif
*/

#if defined(BYTE)
#undef BYTE
#endif

#if defined(BOOLEAN)
#undef BOOLEAN
#endif

#define Reset()            reset_cpu()
#define FAR
//#define ClrWdt()         WdtReset()
#define ClrWdt()         restart_wdt()
#define Nop()            delay_cycles(1)

#warning need eds support
#define __eds__

#define __prog__  rom

#define __CCS__

#inline 
unsigned int16 __builtin_tblrdh(unsigned int16 addy)
{
   unsigned int16 ret;
   WREG5 = addy;
   WREG6 = &ret;
  #asm
   TBLRDH [W5], [W6]
  #endasm
   return(ret);
}

#inline 
unsigned int16 __builtin_tblrdl(unsigned int16 addy)
{
   unsigned int16 ret;
   WREG5 = addy;
   WREG6 = &ret;
  #asm
   TBLRDL [W5], [W6]
  #endasm
   return(ret);
}

#if defined(__PCD__) && !defined(MPFS_USE_SPI_FLASH) && !defined(MPFS_USE_EEPROM)
   //#import will usually put the file outside PSV space
   
   #if (getenv("SFR_VALID:DSRPAG"))
      #define PSV_SAVE()  \
         unsigned int16 __oldPsvPage;  \
         __oldPsvPage = DSRPAG
         
      #define PSV_GOTO(x)  \
         DSRPAG = 0x200 | (x >> 15)
         
      #define PSV_RESTORE()   \
         DSRPAG = __oldPsvPage;

   #elif (getenv("SFR_VALID:PSVPAG"))
      #define PSV_SAVE()  \
         unsigned int16 __oldPsvPage;  \
         __oldPsvPage = PSVPAG
         
      #define PSV_GOTO(x)  \
         PSVPAG = (x >> 15)
         
      #define PSV_RESTORE()   \
         PSVPAG = __oldPsvPage;
         
   #else
      #error something bad happened
   #endif
#else
   #define PSV_SAVE()
   #define PSV_GOTO(x)
   #define PSV_RESTORE()
#endif

// used by MPFS2 - not sure if this is a built-in function to C30/XC16
unsigned int32 ReadProgramMemory(__ADDRESS__ a)
{
   unsigned int32 ret;
   
   PSV_SAVE();

   PSV_GOTO(a);
   
   read_program_memory(a, &ret, 4);
   
   PSV_RESTORE();
   
   return(ret);
}

//#define  strcpypgm2ram  sprintf

#endif
