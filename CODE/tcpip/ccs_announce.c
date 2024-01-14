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

#ifndef __CCS_ANNOUNCE_C
#define __CCS_ANNOUNCE_C

#ifndef debug_announce
#define debug_announce(a,b,c,d,e,f,g,h,i,j,k,l,m,n)
#endif

#include "TCPIPConfig.h"

#if defined(STACK_USE_CCS_ANNOUNCE)

#include "TCPIP Stack/TCPIP.h"

// The announce port
#ifndef ANNOUNCE_PORT
#define ANNOUNCE_PORT   6789
#endif

#ifndef ANNOUNCE_RATE
#define ANNOUNCE_RATE   (TICKS_PER_SECOND*3)
#endif

#ifndef ANNOUNCE_NUM
#define ANNOUNCE_NUM 3
#endif

#ifndef ANNOUNCE_MSG
#define ANNOUNCE_MSG "PIC TCP/IP Announce"
#endif

#ifndef ANNOUNCE_ON_SEND
#define ANNOUNCE_ON_SEND()
#endif

#ifndef ANNOUNCE_CLIENT_DISCONNECTED
#define ANNOUNCE_CLIENT_DISCONNECTED()
#endif

#ifndef ANNOUNCE_CLIENT_CONNECTED
#define ANNOUNCE_CLIENT_CONNECTED()
#endif

struct
{
   UDP_SOCKET socket;
   unsigned int8 num;
   TICK t;
   NODE_INFO node;
   int1 nodeValid;
   int1 isListen;
   int1 stop;
} g_Announce = {INVALID_UDP_SOCKET, ANNOUNCE_NUM};

void AnnounceStop(void)
{
   debug_announce(debug_putc, "AnnounceStop()\r\n");
   
   AnnounceInit();
   
   g_Announce.stop = TRUE;
}

void AnnounceInit(void)
{
   unsigned int8 oldNum;
   
   debug_announce(debug_putc, "AnnounceInit()\r\n");
   
   if (g_Announce.socket != INVALID_UDP_SOCKET)
   {
      UDPClose(g_Announce.socket);
   }
   
   oldNum = g_Announce.num;
  
   memset(&g_Announce, 0x00, sizeof(g_Announce));
   
   g_Announce.t = TickGet();
   g_Announce.socket = INVALID_UDP_SOCKET;
   g_Announce.num = oldNum;
   
   if (g_Announce.num == 0)
   {
      g_Announce.isListen = TRUE;
   }
}

static int1 _AnnounceSend(void)
{
   unsigned int8 len;
   int1 ret = FALSE;
      
   ANNOUNCE_ON_SEND();
   
   len = strlen(ANNOUNCE_MSG);
   
   if (len <= UDPIsPutReady(g_Announce.socket))
   {
      ret = TRUE;

      UDPPutArray(ANNOUNCE_MSG, len);

      UDPFlush();
   }
     
   return(ret);
}

void AnnounceTask(void)
{  
  #if defined(ANNOUNCE_SCAN)
   char scr[80];
   static unsigned int8 len;
  #endif
   
   if (g_Announce.stop)
      return;
      
   if (!DHCPBoundOrDisabled())
   {
      g_Announce.nodeValid = FALSE;
      
      if (g_Announce.socket != INVALID_UDP_SOCKET)
      {
         debug_announce(debug_putc, "AnnounceTask() CLOSE_BECAUSE_NOT_BOUND\r\n");
         UDPClose(g_Announce.socket);
         g_Announce.socket = INVALID_UDP_SOCKET;
      }
   }
  
   if (!g_Announce.nodeValid && DHCPBoundOrDisabled())
   {
      // broadcast MAC address
      memset(&g_Announce.node.MACAddr, 0xFF, 6);
      
      // Set the IP subnet's broadcast address (broadcast UDP address)
      g_Announce.node.IPAddr.Val = (AppConfig.MyIPAddr.Val & AppConfig.MyMask.Val) |
                   ~AppConfig.MyMask.Val;

     #if defined(ANNOUNCE_SCAN)
      len = strlen(ANNOUNCE_SCAN);
      if (len >= sizeof(scr))
      {
         len = sizeof(scr) - 1;
      }
     #endif
                   
      g_Announce.nodeValid = TRUE;
      
      debug_announce(debug_putc, "AnnounceTask() CREATED_NODE\r\n");
   }
   

   if (g_Announce.nodeValid && (g_Announce.socket == INVALID_UDP_SOCKET))
   {
      
      if (g_Announce.isListen)
      {
         debug_announce(debug_putc, "AnnounceTask() LISTEN_SOCKET\r\n");
         g_Announce.socket = UDPOpenEx(NULL, UDP_OPEN_SERVER, ANNOUNCE_PORT, ANNOUNCE_PORT);

      }
      else
      {
         debug_announce(debug_putc, "AnnounceTask() OPEN_SOCKET\r\n");
         g_Announce.socket = UDPOpen(ANNOUNCE_PORT, &g_Announce.node, ANNOUNCE_PORT);
      }
   }  

  #if defined(ANNOUNCE_SCAN)
   if
   (
      g_Announce.isListen &&
      (g_Announce.socket != INVALID_UDP_SOCKET) &&
      (UDPIsGetReady(g_Announce.socket) >= len)
   )
   {     
      UDPGetArray(scr, len);
      scr[len] = 0;
      
      if (strcmp(scr, ANNOUNCE_SCAN) == 0)
      {
         ANNOUNCE_CLIENT_CONNECTED();
         g_Announce.num = 1;
         g_Announce.t = TickGet() - ANNOUNCE_RATE;
      }
   }
  #endif
   
   if 
   (
      (g_Announce.socket != INVALID_UDP_SOCKET) &&
      MACIsLinked() &&
      DHCPBoundOrDisabled() &&
      g_Announce.num && 
      ((TickGet() - g_Announce.t) >= (TICK)ANNOUNCE_RATE) &&
      _AnnounceSend()
   )
   {
      if (g_Announce.isListen)
      {
         ANNOUNCE_CLIENT_DISCONNECTED();
      }
      
      if ((--g_Announce.num == 0) && !g_Announce.isListen)
      {
         UDPClose(g_Announce.socket);
         g_Announce.socket = INVALID_UDP_SOCKET;
         g_Announce.isListen = TRUE;
      }
      
      g_Announce.t = TickGet();
   }
}

#endif //#if defined(STACK_USE_ANNOUNCE)
#endif   //#ifndef __CCS_ANNOUNCE_C
