//////////////////////////////////////////////////////////////////////////////
///                                                                        ///
///                            CCS_SNTP.H                                  ///
///                                                                        ///
/// This is a modified version of the Microchip SNTP. It has been modified ///
/// to use the time drivers to keep time rather than Tick. All CCS time    ///
/// drivers function with this (uncomment the driver you want to use)      ///
///                                                                        ///
/// Connects to an NTP server (NTP_SERVER) and gets the UTC time from the  ///
/// server in seconds (Epoch time). The NTP library automatically sets the ///
/// timebase time to this new time, so future calls to time() get the      ///
/// proper time.  NTP client automatically updates time at                 ///
/// NTP_QUERY_INTERVAL rate.                                               ///
///                                                                        ///
/// You can fully disable NTP by setting NTP_SERVER to a null string.      ///
///                                                                        ///
/// If NTP_SERVER is a constant string, then you also need to define       ///
/// NTP_SERVER_IS_MACRO.  For example:                                     ///
///      #define NTP_SERVER "pool.ntp.org"                                 ///
///      #define NTP_SERVER_IS_MACRO                                       ///
///                                                                        ///
/// If NTP_QUERY_INTERVAL is 0 then it will not automatically poll         ///
/// NTP server, instead user must manually initiate NTP with NTPNow().     ///
///                                                                        ///
/// Since NTP client will save time to the RTC in UTC, several local time  ///
/// functions have been added to pull back time to local timezone.         ///
/// Time-zone offset can be defined with NTP_TIMEZONE_OFFSET.              ///
///                                                                        ///
//////////////////////////////////////////////////////////////////////////////
///                                                                     ///
/// VERSION HISTORY                                                     ///
///                                                                     ///
/// Sep 26 2014                                                         ///
///   Updated for V5 stack (removed DNS and ARP since this is handled   ///
///      by UDPOpenEx()).                                               ///
///   User can override NTP_QUERY_INTERVAL.  If NTP_QUERY_INTERVAL is   ///
///      0 then it won't automatically query NTP at interval.           ///
///   Fixed NTPBusy().                                                  ///
///   Added NTPUsed() and NTPNow().                                     ///
///   Added NTP_TIMEZONE_OFFSET.  If undefined, will use 0, which means ///
///      time() will return UTC.                                        ///
///   Added TIME_T_USES_2010 support.                                   ///
///   Removed NTPGetUTCSeconds() due to confusion with                  ///
///      NTP_TIMEZONE_OFFSET.  Just use time() instead, which will      ///
///      return time at your local timezone.                            ///
///   NTPInit() no longer calls TimeInit().  TimeInit() must be called  ///
///      in your application.                                           ///
///                                                                     ///
///////////////////////////////////////////////////////////////////////////
////        (C) Copyright 1996,2006 Custom Computer Services           ////
//// This source code may only be used by licensed users of the CCS C  ////
//// compiler.  This source code may only be distributed to other      ////
//// licensed users of the CCS C compiler.  No other use, reproduction ////
//// or distribution is permitted without written permission.          ////
//// Derivative programs created using this software in object code    ////
//// form are not restricted in any way.                               ////
///////////////////////////////////////////////////////////////////////////

#ifndef __CCS_SNTP_H__
#define __CCS_SNPT_H__

#include <time.h>

// normally you don't need to call this, since it's called in StackInit().
// but you can call this later if you changed any NTP settings.
void NTPInit(void);

// don't call this - it's already called in StackApplications()
void NTPTask(void);

//time stored is valid.  not valid if NTPUsed() is FALSE or NTPBusy() is TRUE.
int1 NTPOk(void);

//NTP has been used.
int1 NTPUsed(void);

// returns TRUE if the NTP client is in the middle of a transaction with a server
int1 NTPBusy(void);

// start a transaction now, regardless of time interval
void NTPNow(void);

// when using NTP, the client will write the UTC time to the RTC and calls
// to time() will then read the UTC time.  to read local time, use time_local().
time_t time_local(time_t *p);
void SetTimeSecLocal(time_t sTime);
void SetTimeLocal(struct_tm * nTime);
void GetTimeLocal(struct_tm *pRetTm);

#endif
