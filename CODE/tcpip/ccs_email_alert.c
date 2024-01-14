/*
   A wrapper to Microchip's SMTP (STACK_USE_CCS_EMAIL_ALERTS) library 
   for sending an e-mail based on alert flags.
   
   See ccs_email_alert.h for documentation
*/
///////////////////////////////////////////////////////////////////////////
////        (C) Copyright 1996,2014 Custom Computer Services           ////
//// This source code may only be used by licensed users of the CCS C  ////
//// compiler.  This source code may only be distributed to other      ////
//// licensed users of the CCS C compiler.  No other use, reproduction ////
//// or distribution is permitted without written permission.          ////
//// Derivative programs created using this software in object code    ////
//// form are not restricted in any way.                               ////
////                                                                   ////
//// http://www.ccsinfo.com                                            ////
///////////////////////////////////////////////////////////////////////////

#ifndef __CCS_EMAIL_ALERT_C__
#define __CCS_EMAIL_ALERT_C__

#ifndef EMAIL_ALERT_CUSTOM_BODY
#define EMAIL_ALERT_CUSTOM_BODY(first)   (TRUE)
#endif

#ifndef debug_email
#define debug_email(a,b,c,d,e,f,g,h,ui,j,k,l,m,n,o)
#endif

#if (defined(EMAIL_ALERT_BITMAP_BYTES) && (EMAIL_ALERT_BITMAP_BYTES>4))  
   typedef unsigned int32 email_alert_bitmap_t;
   #define EmailAlertSetBit(var, bit)         bit_set(var, bit)
   #define EmailAlertClearAllBits(var)        var=0
   #define EmailAlertIsBitSet(var, bit)       bit_test(var, bit)
   #define EmailAlertIsAnyBitSet(var)         (var != 0)
   #define EmailAlertCopyBits(dst, src)           dst=src
   #define EmailAlertClearBits(dst, src)           dst&=~src
#else
   typedef struct
   {
      unsigned int8 b[EMAIL_ALERT_BITMAP_BYTES];
   } email_alert_bitmap_t;
   #define EmailAlertSetBit(var, bit)         BitArraySet(&var, bit)
   #define EmailAlertClearAllBits(var)        memset(&var, 0, EMAIL_ALERT_BITMAP_BYTES)
   #define EmailAlertIsBitSet(var, bit)       BitArrayIsSet(&var, bit)
   #define EmailAlertIsAnyBitSet(var)         BitArrayNumSet(&var, EMAIL_ALERT_BITMAP_BYTES)
   #define EmailAlertCopyBits(dst, src)       memcpy(&dst, &src, EMAIL_ALERT_BITMAP_BYTES)
   #define EmailAlertClearBits(dst, src)      BitArrayInvAnd(&dst, &src, EMAIL_ALERT_BITMAP_BYTES)
#endif

typedef enum
{
   EMAIL_ALERT_SM_IDLE = 0,
   EMAIL_ALERT_SM_FLAGS = 1,
   EMAIL_ALERT_SM_CUSTOM = 2,
   EMAIL_ALERT_SM_CLOSE = 3,
   EMAIL_ALERT_SM_ABORT = 4,
   EMAIL_ALERT_SM_HOLDOFF = 5
} email_alert_sm_t;

struct
{
   email_alert_sm_t sm;
   email_alert_bitmap_t pendingSent;
   email_alert_bitmap_t pendingNew;
   unsigned int16 sentAttempts;
   unsigned int16 sentSuccess; 
   WORD lastError;
   uint8_t tries;
   
   struct
   {
      TICK t;
      TICK lastT;
      uint16_t m; //minutes
      uint16_t lastM;
      int1 inMinutes;
   } holdoff;
} g_EmailAlert;

void EmailAlertInit(void)
{
   debug_email(debug_putc, "EmailAlertInit()\r\n");
   memset(&g_EmailAlert, 0x00, sizeof(g_EmailAlert));
}

void EmailAlertSetFlag(unsigned int8 flag)
{
   debug_email(debug_putc, "EmailAlertSetFlag(%u)\r\n", flag);
      
   EmailAlertSetBit(g_EmailAlert.pendingNew, flag);
}

int1 EmailAlertIsBusy(void)
{
   return
   (
      EmailAlertIsAnyBitSet(g_EmailAlert.pendingNew) &&
      (g_EmailAlert.sm != EMAIL_ALERT_SM_HOLDOFF)
   );
}

int1 EmailAlertIsHolding(void)
{
   return
   (
      EmailAlertIsAnyBitSet(g_EmailAlert.pendingNew) &&
      (g_EmailAlert.sm == EMAIL_ALERT_SM_HOLDOFF)
   );
}

unsigned int16 EmailAlertGetAttempts(void)
{
   return(g_EmailAlert.sentAttempts);
}

unsigned int16 EmailAlertGetSuccess(void)
{
   return(g_EmailAlert.sentSuccess);
}

WORD EmailAlertGetLastError(void)
{
   return(g_EmailAlert.lastError);
}

void EmailAlertClearTimer(void)
{
   debug_email(debug_putc, "EmailAlertClearTimer()\r\n");
   
   g_EmailAlert.holdoff.lastM = g_EmailAlert.holdoff.m;
   g_EmailAlert.holdoff.lastT = TickGet() - g_EmailAlert.holdoff.t;
}

void EmailAlertSetTimerTicks(TICK ticks)
{
   debug_email(debug_putc, "EmailAlertSetTimerTicks(%lu)\r\n", ticks);
   
   g_EmailAlert.holdoff.inMinutes = FALSE;
   g_EmailAlert.holdoff.t = ticks;
}

void EmailAlertSetTimerMinutes(unsigned int16 minutes)
{
   debug_email(debug_putc, "EmailAlertSetTimerMinutes(%lu)\r\n", minutes);
   
   g_EmailAlert.holdoff.inMinutes = TRUE;
   g_EmailAlert.holdoff.m = minutes;
}

void EmailAlertTask(void)
{
   static unsigned int8 idx;
   ROM char *p;
   unsigned int16 len;
   int1 set;
   TICK t;
   static int1 first;
   
   if ((g_EmailAlert.sm == EMAIL_ALERT_SM_IDLE) && EmailAlertIsAnyBitSet(g_EmailAlert.pendingNew))
   {
      EmailAlertClearAllBits(g_EmailAlert.pendingSent);
      
      if (SMTPBeginUsage())
      {
         EmailAlertSetup();
         
         SMTPSendMail();
         
         debug_email(debug_putc, "EmailAlertTask() START\r\n");
         
         g_EmailAlert.sm++;
         idx = 0;
         
         if (g_EmailAlert.tries == 0)
         {
            g_EmailAlert.sentAttempts++;
         }
      }
   }
   
   if ((g_EmailAlert.sm > EMAIL_ALERT_SM_IDLE) && (g_EmailAlert.sm < EMAIL_ALERT_SM_CLOSE) && !SMTPIsBusy())
   {
      debug_email(debug_putc, "EmailAlertTask() ABORT %X\r\n", g_EmailAlert.sm);
      
      g_EmailAlert.sm = EMAIL_ALERT_SM_ABORT;
   }
   
   switch(g_EmailAlert.sm)
   {
      case EMAIL_ALERT_SM_IDLE:
         break;

      case EMAIL_ALERT_SM_FLAGS:
         set = EmailAlertIsBitSet(g_EmailAlert.pendingNew, idx);
         if (set)
         {
            p = g_EmailAlertStrings[idx];
            len = strlenpgm(p);
            if (SMTPIsPutReady() >= (len+2))
            {
               debug_email(debug_putc, "EmailAlertTask() Sent %u of %lu, len=%lu p=%LX\r\n", idx, (sizeof(g_EmailAlertStrings)/sizeof(ROM char*)), len, p);
               SMTPPutROMString(p);
               SMTPPutROMString((ROM char*)"\r\n");
               SMTPFlush();
               EmailAlertSetBit(g_EmailAlert.pendingSent, idx);
               set = FALSE;
            }
         }
         if (!set)
         {
            if (++idx >= (sizeof(g_EmailAlertStrings)/sizeof(ROM char*)))
            {
               g_EmailAlert.sm++;
               first = TRUE;
            }         
         }
         break;

      case EMAIL_ALERT_SM_CUSTOM:
         if (EMAIL_ALERT_CUSTOM_BODY(first))
         {
            g_EmailAlert.sm++;
            
            debug_email(debug_putc, "EmailAlertTask() FINISHING\r\n");
            
            SMTPPutDone();
         }
         first = FALSE;
         break;

      case EMAIL_ALERT_SM_ABORT:
      case EMAIL_ALERT_SM_CLOSE:
         if (!SMTPIsBusy())
         {
            g_EmailAlert.lastError = SMTPEndUsage();
            if (!g_EmailAlert.lastError)
            {
               g_EmailAlert.sentSuccess++;
               EmailAlertClearBits(g_EmailAlert.pendingNew, g_EmailAlert.pendingSent);
            }
           #if defined(__DO_DEBUG__)
            else if (++g_EmailAlert.tries >= 1)
           #else
            else if (++g_EmailAlert.tries >= 3)
           #endif
            {
               g_EmailAlert.sm = EMAIL_ALERT_SM_CLOSE;
              #if defined(EMAIL_ALERT_CLEAR_FLAGS_ON_FAIL)
               EmailAlertClearAllBits(g_EmailAlert.pendingNew);
              #endif
               debug_email(debug_putc, "EmailAlertTask() TOO_MANY\r\n");
            }
            
            if (g_EmailAlert.sm == EMAIL_ALERT_SM_CLOSE)
            {
               g_EmailAlert.tries = 0;
               g_EmailAlert.holdoff.lastT = TickGet();
               g_EmailAlert.holdoff.lastM = 0;
               g_EmailAlert.sm = EMAIL_ALERT_SM_HOLDOFF;
               debug_email(debug_putc, "EmailAlertTask() DONE EC=%LX m=%LU\r\n", g_EmailAlert.lastError, g_EmailAlert.holdoff.lastM);
            }
            else
            {
               g_EmailAlert.sm = EMAIL_ALERT_SM_IDLE;
               debug_email(debug_putc, "EmailAlertTask() RETRY EC=%LX\r\n", g_EmailAlert.lastError);
            }
         }
         break;

      case EMAIL_ALERT_SM_HOLDOFF:
         t = TickGet();
         if (g_EmailAlert.holdoff.inMinutes)
         {
            if ((t - g_EmailAlert.holdoff.lastT) >= (TICK)TICKS_PER_SECOND*60)
            {
               g_EmailAlert.holdoff.lastT += (TICK)TICKS_PER_SECOND*60;
               
               g_EmailAlert.holdoff.lastM += 1;
            }
            if (g_EmailAlert.holdoff.lastM >= g_EmailAlert.holdoff.m)
            {
               g_EmailAlert.sm = EMAIL_ALERT_SM_IDLE;
               debug_email(debug_putc, "Email Holdoff Minutes done\r\n");
            }
         }
         else
         {
            if ((t - g_EmailAlert.holdoff.lastT) >= g_EmailAlert.holdoff.t)
            {
               g_EmailAlert.sm = EMAIL_ALERT_SM_IDLE;
               debug_email(debug_putc, "Email Holdoff Tick done\r\n");
            }
         }
         break;
         
      default:
         g_EmailAlert.sm = EMAIL_ALERT_SM_IDLE;
         break;
   }
}

#endif
