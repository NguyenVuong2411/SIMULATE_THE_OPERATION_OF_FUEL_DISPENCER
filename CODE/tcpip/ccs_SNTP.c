//////////////////////////////////////////////////////////////////////////////
///                                                                        ///
/// ccs_SNTP.c - CCS Simple Network Time Protocol                          ///
///                                                                        ///
/// See ccs_SNTP.h for documentation.                                      ///
///                                                                        ///
///////////////////////////////////////////////////////////////////////////
///                                                                     ///
/// VERSION HISTORY                                                     ///
///                                                                     ///
/// Sep 26 2014                                                         ///
///   Many changes, see ccs_SNTP.h.                                     ///
///                                                                     ///
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

#ifndef __CCS_SNTP_C__
#define __CCS_SNTP_C__

#ifndef debug_ntp
#define debug_ntp(a,b,c,d,e,f,g,h,i,j)
#endif

#include "TCPIPConfig.h"
#include "TCPIP Stack/TCPIP.h"

#include <time.h>

#ifndef SNTP_CALLBACK_GOT_TIME
#define SNTP_CALLBACK_GOT_TIME(t)
#endif

#ifndef SNTP_NOTIFY_NO_SOCKETS
#define SNTP_NOTIFY_NO_SOCKETS(port)
#endif

#if !defined(__TIME_C__)
#if 0   //set to 1 if you have an external DS1305 real time clock.
   #include <ds1305.c>
#elif 0 //set to 1 if you have a PIC with internal real time clock.
   #include <rtcperipheral.c>
#else
   // if you don't use one of the above real time clock devices, the
   // example will use a PIC timer.  this is the least accurate
   // but is compatabile with every PIC.
   
   // map rtcticks.c library to the already existing Tick system
   #define GET_RTC_TICKS   TickGet
   #define CLOCKS_PER_SECOND  TICKS_PER_SECOND
   
   #include <rtcticks.c>
#endif
#endif

#if defined(TIME_T_USES_2010)
#define NTP_EPOCH           (86400ul * (365ul * 110ul + 27ul)) // Reference Epoch (default: 01-Jan-2010 00:00:00)
#else
#define NTP_EPOCH           (86400ul * (365ul * 70ul + 17ul)) // Reference Epoch (default: 01-Jan-1970 00:00:00)
#endif

#ifndef NTP_QUERY_INTERVAL
#define NTP_QUERY_INTERVAL  (10) // wait time before resynchronizing the date/time (minutes)
#endif

#ifndef NTP_TIMEZONE_OFFSET
#define NTP_TIMEZONE_OFFSET   (0)
#endif

#define NTP_UDP_INTERVAL    (30)          // (seconds)
#define NTP_REPLY_TIMEOUT   (20)           // wait time before assuming the query has failed (seconds)
#define NTP_SERVER_PORT     (123ul)       // Port for contacting NTP servers
#define NTP_LOCAL_PORT      (7609ul)      // Local UDP Port


// These are normally available network time servers.
// Use the server for your region for best results
// *** NOTE ***
// *** If DNSResolve of the server name causes the program to
// *** crash, use the IP address of the pool instead. The IP
// *** of the server may change, so verify using the ping command.
#ifndef NTP_SERVER
#define NTP_SERVER   "pool.ntp.org"    // global server
#define NTP_SERVER_IS_MACRO
#endif

struct
{
   UDP_SOCKET socket;
   time_t lastUpdate;
   int1 ok;
   int1 used;
   TICK t;
   unsigned int16 m;

   enum
   {
      NTP_START = 0,
      NTP_CONNECTING,
      NTP_UDP_PUT,
      NTP_UDP_GET,
      NTP_WAIT,
      NTP_DISABLED
   } state; 
} g_Ntp = {INVALID_UDP_SOCKET};

int1 NTPUsed(void)
{
   return(g_Ntp.used);
}

int1 NTPOk(void)
{
   return(g_Ntp.ok);
}

// Defines the structure of an NTP packet
typedef struct
{
   struct
   {
      BYTE mode           : 3;   // NTP mode
      BYTE versionNumber  : 3;   // SNTP version number
      BYTE leapIndicator  : 2;   // Leap second indicator
   } flags;                      // Flags for the packet

   BYTE stratum;                 // Stratum level of local clock
   CHAR poll;                    // Poll interval
   CHAR precision;               // Precision (seconds to nearest power of 2)
   DWORD root_delay;             // Root delay between local machine and server
   DWORD root_dispersion;        // Root dispersion (maximum error)
   DWORD ref_identifier;         // Reference clock identifier
   DWORD ref_ts_secs;            // Reference timestamp (in seconds)
   DWORD ref_ts_fraq;            // Reference timestamp (fractions)
   DWORD orig_ts_secs;           // Origination timestamp (in seconds)
   DWORD orig_ts_fraq;           // Origination timestamp (fractions)
   DWORD recv_ts_secs;           // Time at which request arrived at sender (seconds)
   DWORD recv_ts_fraq;           // Time at which request arrived at sender (fractions)
   DWORD tx_ts_secs;             // Time at which request left sender (seconds)
   DWORD tx_ts_fraq;             // Time at which request left sender (fractions)
} NTP_PACKET;

void NTPTask(void)
{
   NTP_PACKET         pkt;
   WORD               w;
   static TICK exp;
   time_t s;
   
   if (g_Ntp.socket != INVALID_UDP_SOCKET)
   {
      if ((TickGet() - g_Ntp.t) >= exp)
      {
         debug_ntp(debug_putc, "NTPTask() TIMEOUT %X\r\n", g_Ntp.state);
         g_Ntp.ok = FALSE;
         g_Ntp.state = NTP_WAIT;
         g_Ntp.t = TickGet();
         UDPClose(g_Ntp.socket);
         g_Ntp.socket = INVALID_UDP_SOCKET;
      }
   }
   
   switch(g_Ntp.state)
   {
      case NTP_START:
         if (MACIsLinked() && DHCPBoundOrDisabled())
         {
            g_Ntp.m = 0;
           #if defined(NTP_SERVER_IS_MACRO)
            g_Ntp.socket = UDPOpenEx((rom char*)NTP_SERVER, UDP_OPEN_ROM_HOST, NTP_LOCAL_PORT, NTP_SERVER_PORT);
           #else
            g_Ntp.socket = UDPOpenEx(NTP_SERVER, UDP_OPEN_RAM_HOST, NTP_LOCAL_PORT, NTP_SERVER_PORT);
           #endif
            if(g_Ntp.socket == INVALID_UDP_SOCKET)
            {
               SNTP_NOTIFY_NO_SOCKETS(NTP_LOCAL_PORT);
               break;
            }
            debug_ntp(debug_putc, "NTPTask() Start\r\n");            
            g_Ntp.t = TickGet();
            g_Ntp.state = NTP_CONNECTING;
            exp = (TICK)(NTP_UDP_INTERVAL * TICKS_PER_SECOND);
         }
         break;

      case NTP_CONNECTING:
           if (UDPIsOpened(g_Ntp.socket))
           {
               debug_ntp(debug_putc, "NTPTask() UDP_CON\r\n");
               g_Ntp.state = NTP_UDP_PUT;
           }
           break;
           
      case NTP_UDP_PUT:
         // Make certain the socket can be written to
         if (UDPIsPutReady(g_Ntp.socket) >= sizeof(pkt))
         {
            debug_ntp(debug_putc, "NTPTask() UDP_TX\r\n");
            // Transmit a time request packet
            memset(&pkt, 0, sizeof(pkt));
            pkt.flags.versionNumber = 3;   // NTP Version 3
            pkt.flags.mode = 3;            // NTP Client
            //pkt.orig_ts_secs = swapl(NTP_EPOCH);
            UDPPutArray((BYTE*) &pkt, sizeof(pkt));   
            UDPFlush();   
            g_Ntp.t = TickGet();
            g_Ntp.state = NTP_UDP_GET;
            exp = (TICK)(NTP_REPLY_TIMEOUT * TICKS_PER_SECOND);
         }
         break;

      case NTP_UDP_GET:
         // Look for a response time packet
         if (UDPIsGetReady(g_Ntp.socket) >= sizeof(pkt))
         {
            // Get the response time packet
            w = UDPGetArray((BYTE*) &pkt, sizeof(pkt));
            UDPClose(g_Ntp.socket);
            g_Ntp.socket = INVALID_UDP_SOCKET;
            
            // Validate packet size
            if(w != sizeof(pkt)) 
            {
               debug_ntp(debug_putc, "NTPTask() INVALID_RX\r\n");
               
               g_Ntp.ok = FALSE;
            }
            else
            {
               g_Ntp.ok = TRUE;

               // Set out local time to match the returned time
               s = swapl(pkt.tx_ts_secs) - NTP_EPOCH;

               // Do rounding.  If the partial seconds is > 0.5 then add 1 to the seconds count.
               if(((BYTE*)&pkt.tx_ts_fraq)[0] & 0x80)
                  s++;
               SNTP_CALLBACK_GOT_TIME(s);
               SetTimeSec(s);      
            }
            debug_ntp(debug_putc, "NTPTask() DONE\r\n");
            g_Ntp.state = NTP_WAIT;
            g_Ntp.t = TickGet();
         }
         break;

      case NTP_WAIT:
         // Requery the NTP server after a specified NTP_QUERY_INTERVAL time (ex: 10 minutes) has elapsed.
         if (NTP_QUERY_INTERVAL != 0)
         {
            if ((TickGet() - g_Ntp.t) >= (TICKS_PER_SECOND*(TICK)60))
            {
               g_Ntp.t = TickGet();
               g_Ntp.m++;
               
               if (++g_Ntp.m >= NTP_QUERY_INTERVAL)
               {
                  debug_ntp(debug_putc, "NTPTask() interval expired\r\n");
                  g_Ntp.state = NTP_START;
               }
            }
         }
         break;
         
      case NTP_DISABLED:
         break;
   }
}

time_t time_local(time_t *p)
{
   time_t t;
   t = time(NULL);
   t += NTP_TIMEZONE_OFFSET;
   if (p)
      *p = t;
   return(t);
}

void GetTimeLocal(struct_tm *pRetTm)
{
   time_t t;
   struct_tm *p;
   
   t = time_local(NULL);
   
   p = localtime(&t);
   
   memcpy(pRetTm, p, sizeof(struct_tm));
}

void SetTimeSecLocal(time_t sTime)
{
   sTime -= NTP_TIMEZONE_OFFSET;
   SetTimeSec(sTime);
}

void SetTimeLocal(struct_tm * nTime)
{
   time_t t;
   
   t = mktime(nTime);
   
   SetTimeSecLocal(t);
}

void NTPNow(void)
{
   if (!NTPBusy() && (g_Ntp.state != NTP_DISABLED))
   {
      g_Ntp.used = TRUE;
      g_Ntp.state = NTP_START;
   }
}

int1 NTPBusy(void)
{
   return((g_Ntp.state != NTP_WAIT) && (g_Ntp.state != NTP_DISABLED));
}

int1 NTPDisabled(void)
{
   return(g_Ntp.state == NTP_DISABLED);
}

void NTPInit(void)
{
   debug_ntp(debug_putc, "NTPInit()\r\n");
   
   if (g_Ntp.socket != INVALID_UDP_SOCKET)
   {
      UDPClose(INVALID_UDP_SOCKET);
   }
   
   memset(&g_Ntp, 0x00, sizeof(g_Ntp));
   
   g_Ntp.socket = INVALID_UDP_SOCKET;
   g_Ntp.t = TickGet();
   
   if (strlen(NTP_SERVER) == 0)
   {
      g_Ntp.state = NTP_DISABLED;
   }
   else if (NTP_QUERY_INTERVAL != 0)
   {
      g_Ntp.used = TRUE;
      g_Ntp.state = NTP_START;
   }
   else
   {
      g_Ntp.state = NTP_WAIT;
   }
}

#endif //__CCS_SNTP_C__
