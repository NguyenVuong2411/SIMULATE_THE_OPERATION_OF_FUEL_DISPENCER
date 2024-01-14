// 'glue' file for using Microchip's TCP/IP stack inside CCS C Compiler without
// a linker.  
// Also includes some macros/defines for porting older V3 stack to this current
// stack.
// Also provides extra routines written by CCS to improve the stack.

#ifndef __CCS_STACKTSK2_H__
#define __CCS_STACKTSK2_H__

#if !defined(debug_mpfs)
 #define debug_mpfs(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q)
#else
 #define __DO_DEBUG_MPFS
#endif

#if defined(__PCH__) && !defined(__18CXX)
 #define __18CXX
#endif

#define SIZEOF_MAC_HEADER 14

// override delay.c/delay.h with CCS compatible code
#define __DELAY_H
#define Delay10us(x) delay_us((uint16_t)10*(uint16_t)x)
#define DelayMs(x)   delay_ms(x)

#define __WF_DEBUG_STRINGS_H  //don't include this file

#include "TCPIP Stack/TCPIP.h"

#if defined(MRF24WG)
 #define WF_DisplayModuleAssertInfo()
#endif

#if defined(STACK_USE_CCS_EMAIL_ALERTS)
   #include "TCPIP Stack/ccs_email_alert.h"
#endif

#if defined(STACK_USE_CCS_SCAN_TASK)
   #include "TCPIP Stack/ccs_wifiscan.h"
#endif

#if defined(STACK_USE_CCS_COAP)
   #include "TCPIP Stack/ccs_coap.h"
#endif

#if defined(STACK_USE_CCS_HTTP1_SERVER)
   #include "TCPIP Stack/ccs_HTTP.h"
#endif

#if defined(STACK_USE_CCS_HTTP2_SERVER)
   #include "TCPIP Stack/ccs_HTTP2.h"
#endif

#if defined(STACK_USE_CCS_TWITTER)
   #include "TCPIP Stack/ccs_twitter.h"
#endif

#if defined(STACK_USE_CCS_SMTP)
   #include "TCPIP Stack/ccs_SMTP.h"
#endif

#if defined(STACK_USE_CCS_TFTP_SERVER)
   #include "TCPIP Stack/ccs_tftps.h"
#endif

#if defined(STACK_USE_CCS_TFTP_CLIENT)
   #include "TCPIP Stack/ccs_tftpc.h"
#endif

#if defined(STACK_USE_CCS_SNTP_CLIENT)
   #include "TCPIP Stack/ccs_SNTP.h"
#endif

#if defined(STACK_USE_CCS_GRATUITOUS_ARP)
   #include "TCPIP Stack/ccs_gratarp.h"
#endif

#if defined(STACK_USE_CCS_HTTP_CLIENT)
   #include "TCPIP Stack/ccs_http_client.h"
#endif

#if defined(STACK_USE_CCS_FTP_SERVER)
   #include "fat/filesystem.h"
   #include "TCPIP Stack/ccs_ftp_mdd.h"
#endif

#define TickGetDiff(a, b)  (a-b)

#define MY_MAC_BYTE1                    AppConfig.MyMACAddr.v[0]
#define MY_MAC_BYTE2                    AppConfig.MyMACAddr.v[1]
#define MY_MAC_BYTE3                    AppConfig.MyMACAddr.v[2]
#define MY_MAC_BYTE4                    AppConfig.MyMACAddr.v[3]
#define MY_MAC_BYTE5                    AppConfig.MyMACAddr.v[4]
#define MY_MAC_BYTE6                    AppConfig.MyMACAddr.v[5]

#define MY_MASK_BYTE1                   AppConfig.MyMask.v[0]
#define MY_MASK_BYTE2                   AppConfig.MyMask.v[1]
#define MY_MASK_BYTE3                   AppConfig.MyMask.v[2]
#define MY_MASK_BYTE4                   AppConfig.MyMask.v[3]

#define MY_IP                           AppConfig.MyIPAddr

#define MY_IP_BYTE1                     AppConfig.MyIPAddr.v[0]
#define MY_IP_BYTE2                     AppConfig.MyIPAddr.v[1]
#define MY_IP_BYTE3                     AppConfig.MyIPAddr.v[2]
#define MY_IP_BYTE4                     AppConfig.MyIPAddr.v[3]

#define MY_GATE_BYTE1                   AppConfig.MyGateway.v[0]
#define MY_GATE_BYTE2                   AppConfig.MyGateway.v[1]
#define MY_GATE_BYTE3                   AppConfig.MyGateway.v[2]
#define MY_GATE_BYTE4                   AppConfig.MyGateway.v[3]

#if defined(STACK_USE_CCS_SNTP_CLIENT)
       #if !defined(STACK_USE_DNS)
           #define STACK_USE_DNS
       #endif
       #if !defined(STACK_USE_UDP)
           #define STACK_USE_UDP
       #endif       
#endif

#if defined(STACK_USE_CCS_HTTP2_SERVER)
   #ifndef STACK_USE_MPFS
      #define STACK_USE_MPFS
    #endif
#endif

#if defined(STACK_USE_CCS_SNTP_CLIENT)
   #if !defined(STACK_CLIENT_MODE)
       #define STACK_CLIENT_MODE
   #endif
#endif

#if STACK_USE_WIFI
   // if defined, MyWFIsConnected() won't return TRUE in AdHoc or SoftAP mode 
   // until after it receives some IP traffic.  It will then stay connected 
   // until after 5 minutes of no activity.
   // This is pretty much required because the Microchip radio modules and
   // stack don't have an event telling you the client disconnected.
   #define WIFI_ADHOC_CONNECTION_TIMER ((TICK)TICKS_PER_SECOND * 300)
      
   #if defined(WIFI_ADHOC_CONNECTION_TIMER) && (defined(WF_ADHOC) || defined(WF_SOFT_AP))
   extern int1 g_WifiAdhocIsConn;
   extern TICK g_WifiAdhocTickConn;
   #endif
   
   //extern unsigned int8 WIFI_channelList[16];
   //extern unsigned int8 WIFI_numChannelsInList;
   extern unsigned int16 g_WifiChannelListBitmap;
   static unsigned int8 WIFICreateChannelArray(unsigned int8 *p);
   
   extern unsigned int8 WIFI_region;
   
   // this is similar to MACIsLinked() and WFisConnected().  this one has some
   // filters and extra UI logic to better represent link status to the user.
   // use this for UI displays.
   // REMOVED - Use IsLinked() instead
   //int1 MyWFisConnected(void);
   
   // This routine does a few things:
   //  * Reset WIFI unit if no it goes a long time without a connection,
   //       because old modules would hang on WPA connect failure.
   //  * Ad-Hoc connection timer support (WIFI_ADHOC_CONNECTION_TIMER)
   //  * LED Traffic flickering and LED connection status
   void WIFIConnectTask(void);
   void WIFIConnectInitStates(void);
   
   #if defined(WF_FORCE_NO_PS_POLL)
      void WF_CCS_PsPollDisable(void);
   #endif
#endif

#if STACK_USE_WIFI && !defined(STACK_USE_BLUEGIGA_WF121)
   extern unsigned int8 g_connectionProfileID;
   extern int1 g_WifiConnectFail;
#endif

int1 DHCPBoundOrDisabled(void);

// A higher level version of MACIsLinked().
//  - returns FALSE if MACIsLinked() is TRUE -but- DHCP is enabled and not
//       bound.
//  - returns FALSE in WIFI if using AdHoc mode and it has been a while
//       since no traffic
//  - else, returns MACIsLinked()
int1 IsLinked(void);

//this macro called by stack when new tcp/ip traffic tx/rx.
#if STACK_USE_WIFI
   #define STACK_USE_CCS_TX_EVENT()  LinkTraffic(TRUE)
   #define STACK_USE_CCS_RX_EVENT()  LinkTraffic(FALSE)
   void LinkTraffic(int1 isTx);
   
   #define WF_SECURITY_NUM_CHOICES  (WF_SECURITY_WEP_AUTO+1)
#endif

#if defined(STACK_USE_MPFS)
   #include "TCPIP Stack/mpfs.h"
   
   extern MPFS _MpfsEofLoc;
   
   //returns number of bytes read before EOF.
   //if it returns n then no EOF.
   unsigned int16 MPFSGetBytes(unsigned int8 *pDest, unsigned int16 n);
#endif

#if defined(__PCH__)
TICK TickGetSafe(void);
#endif

BYTE GenerateRandomByteFromTimers(void);

unsigned int32 swap24(unsigned int32 v);

#endif
