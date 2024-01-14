/*********************************************************************
 *
 *  Dynamic Host Configuration Protocol (DHCP) Server
 *  Module for Microchip TCP/IP Stack
 *    -Provides automatic IP address, subnet mask, gateway address, 
 *     DNS server address, and other configuration parameters on DHCP 
 *     enabled networks.
 *    -Reference: RFC 2131, 2132
 *
 *********************************************************************
 * FileName:        DHCPs.c
 * Dependencies:    UDP, ARP, Tick
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32
 * Compiler:        Microchip C32 v1.05 or higher
 *               Microchip C30 v3.12 or higher
 *               Microchip C18 v3.30 or higher
 *               HI-TECH PICC-18 PRO 9.63PL2 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * Copyright (C) 2002-2009 Microchip Technology Inc.  All rights
 * reserved.
 *
 * Microchip licenses to you the right to use, modify, copy, and
 * distribute:
 * (i)  the Software when embedded on a Microchip microcontroller or
 *      digital signal controller product ("Device") which is
 *      integrated into Licensee's product; or
 * (ii) ONLY the Software driver source files ENC28J60.c, ENC28J60.h,
 *      ENCX24J600.c and ENCX24J600.h ported to a non-Microchip device
 *      used in conjunction with a Microchip ethernet controller for
 *      the sole purpose of interfacing with the ethernet controller.
 *
 * You should refer to the license agreement accompanying this
 * Software for additional information regarding your rights and
 * obligations.
 *
 * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * MICROCHIP BE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
 *
 *
 * Author               Date       Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Howard Schlunder     02/28/07   Original
 * Darren Rook/CCS      12/19/12 Renamed MyDHCPsSocket to MyDHCPsSocket.
 * Microchip            06/15/13 ???? lots of changes
 * Darren Rook/CCS      03/23/15 DHCPServerInit() now uses assigns IP pool
 *                                  based upon our IP address in APP_CONFIG,
 *                                  instead of the default IP address which
 *                                  may not match what we actually have
 *                                  configured from EE.
 *                               Removed 'const' from some function parameters,
 *                                  it should work in CCS but for some
 *                                  reason it's not.
 ********************************************************************/
#define __DHCPS_C

#include "TCPIPConfig.h"

#if defined(STACK_USE_DHCP_SERVER)

#ifndef debug_dhcps
#define debug_dhcps(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z)
#endif

#ifndef DHCPS_NOTIFY_NO_SOCKETS
#define DHCPS_NOTIFY_NO_SOCKETS(port)
#endif

#include "TCPIP Stack/TCPIP.h"

// Duration of our DHCP Lease in seconds.  This is extrememly short so 
// the client won't use our IP for long if we inadvertantly 
// provide a lease on a network that has a more authoratative DHCP server.
#ifndef DHCP_LEASE_DURATION
#define DHCP_LEASE_DURATION            60*60ul         // 1Hr
#endif
/// Ignore: #define DHCP_MAX_LEASES                         2      // Not implemented
#ifndef MAX_DHCP_CLIENTS_NUMBER
#define MAX_DHCP_CLIENTS_NUMBER                         1
#endif
typedef struct
{
   MAC_ADDR ClientMAC;
   IP_ADDR    Client_Addr;
   BOOL       isUsed;
   UINT32    Client_Lease_Time;
}DHCP_IP_POOL;
DHCP_IP_POOL DhcpIpPool[MAX_DHCP_CLIENTS_NUMBER];

static unsigned int8  _g_DhcpsNextIp;
static unsigned int8 _DhcpsGetNextIpByte(void)
{
   _g_DhcpsNextIp += 1;
   
   while
   (
      (_g_DhcpsNextIp == 0) || 
      (_g_DhcpsNextIp == 1) || 
      (_g_DhcpsNextIp == 255) ||
      (_g_DhcpsNextIp == AppConfig.MyIPAddr.v[3])
   )
   {
      _g_DhcpsNextIp += 1;
   }
   
   return(_g_DhcpsNextIp);
}

// DHCP Control Block.  Lease IP address is derived from index into DCB array.
typedef struct
{
   DWORD       LeaseExpires;   // Expiration time for this lease
   MAC_ADDR   ClientMAC;      // Client's MAC address.  Multicase bit is used to determine if a lease is given out or not
   enum 
   {
      LEASE_UNUSED = 0,
      LEASE_REQUESTED,
      LEASE_GRANTED
   } smLease;               // Status of this lease
} DHCP_CONTROL_BLOCK;

static UDP_SOCKET         MyDHCPsSocket = INVALID_UDP_SOCKET;      // Socket used by DHCP Server
//static IP_ADDR            DHCPNextLease;   // IP Address to provide for next lease
BOOL                   bDHCPServerEnabled = TRUE;   // Whether or not the DHCP server is enabled

static void DHCPReplyToDiscovery(BOOTP_HEADER *Header);
static void DHCPReplyToRequest(BOOTP_HEADER *Header, BOOL bAccept, BOOL bRenew);

static void DHCPServerInit(void);

static BOOL isIpAddrInPool(IP_ADDR ipaddr) ;
static IP_ADDR GetIPAddrFromIndex_DhcpPool(UINT8 index);
static UINT8 preAssign_ToDHCPClient_FromPool(BOOTP_HEADER *Header);
static UINT8 postAssign_ToDHCPClient_FromPool(MAC_ADDR *macAddr, IP_ADDR *ipv4Addr);
static void renew_dhcps_Pool(void);
//static BOOL Compare_MAC_addr(const MAC_ADDR *macAddr1, const MAC_ADDR *macAddr2);
//static UINT8 getIndexByMacaddr_DhcpPool(const MAC_ADDR *MacAddr);
//static BOOL isMacAddr_Effective(const MAC_ADDR *macAddr);
static BOOL Compare_MAC_addr(MAC_ADDR *macAddr1, MAC_ADDR *macAddr2);
static UINT8 getIndexByMacaddr_DhcpPool(MAC_ADDR *MacAddr);
static BOOL isMacAddr_Effective(MAC_ADDR *macAddr);

static enum
{
   DHCPS_DISABLE,
   DHCPS_OPEN_SOCKET,
   DHCPS_LISTEN
} smDHCPServer = DHCPS_OPEN_SOCKET;


/*****************************************************************************
  Function:
   void DHCPServerTask(void)

  Summary:
   Performs periodic DHCP server tasks.

  Description:
   This function performs any periodic tasks requied by the DHCP server 
   module, such as processing DHCP requests and distributing IP addresses.

  Precondition:
   None

  Parameters:
   None

  Returns:
     None
  ***************************************************************************/
void DHCPServerTask(void)
{
   BYTE             i;
   BYTE            Option, Len;
   BOOTP_HEADER      BOOTPHeader;
   DWORD            dw;
   BOOL            bAccept, bRenew;


   // Init IP pool
   static BOOL flag_init = FALSE;
   if(flag_init == FALSE)
   {
      flag_init = TRUE;
      DHCPServerInit();
   }
   
#if defined(STACK_USE_DHCP_CLIENT)
   // Make sure we don't clobber anyone else's DHCP server
   if(DHCPIsServerDetected(0))
      return;
#endif

   if(!bDHCPServerEnabled)
      return;

   renew_dhcps_Pool();
   switch(smDHCPServer)
   {
      case DHCPS_DISABLE:
         break;
      case DHCPS_OPEN_SOCKET:
         // Obtain a UDP socket to listen/transmit on
         //MyDHCPsSocket = UDPOpen(DHCP_SERVER_PORT, NULL, DHCP_CLIENT_PORT);
         MyDHCPsSocket = UDPOpenEx(0,UDP_OPEN_SERVER,DHCP_SERVER_PORT, DHCP_CLIENT_PORT);
         if(MyDHCPsSocket == INVALID_UDP_SOCKET)
         {
            DHCPS_NOTIFY_NO_SOCKETS(DHCP_CLIENT_PORT);
            break;
         }


         // Decide which address to lease out
         // Note that this needs to be changed if we are to 
         // support more than one lease
         /*
         DHCPNextLease.Val = (AppConfig.MyIPAddr.Val & AppConfig.MyMask.Val) + 0x02000000;
         if(DHCPNextLease.v[3] == 255u)
            DHCPNextLease.v[3] += 0x03;
         if(DHCPNextLease.v[3] == 0u)
            DHCPNextLease.v[3] += 0x02;
            */

         smDHCPServer++;

      case DHCPS_LISTEN:
         // Check to see if a valid DHCP packet has arrived
         if(UDPIsGetReady(MyDHCPsSocket) < 241u)
            break;

         // Retrieve the BOOTP header
         UDPGetArray((BYTE*)&BOOTPHeader, sizeof(BOOTPHeader));
      
      #if defined(__PCD_QUIRK2__)
         #warning PCD QUIRK
         if(TRUE == isIpAddrInPool((unsigned int32)BOOTPHeader.ClientIP)){bRenew= TRUE; bAccept = TRUE;}
      #else
         if(TRUE == isIpAddrInPool(BOOTPHeader.ClientIP)){bRenew= TRUE; bAccept = TRUE;}
      #endif
         else if(BOOTPHeader.ClientIP.Val == 0x00000000u) {bRenew = FALSE; bAccept = TRUE;}
         else                                             
         {
            bRenew = FALSE; 
            bAccept = FALSE;
            debug_dhcps(debug_putc, "DHCPS bAccept=FALSE DHCPS_LISTEN\r\n");
         }
         //bAccept = (BOOTPHeader.ClientIP.Val == DHCPNextLease.Val) || (BOOTPHeader.ClientIP.Val == 0x00000000u);

         // Validate first three fields
         if(BOOTPHeader.MessageType != 1u)
            break;
         if(BOOTPHeader.HardwareType != 1u)
            break;
         if(BOOTPHeader.HardwareLen != 6u)
            break;

         // Throw away 10 unused bytes of hardware address,
         // server host name, and boot file name -- unsupported/not needed.
         for(i = 0; i < 64+128+(16-sizeof(MAC_ADDR)); i++)
            UDPGet(&Option);

         // Obtain Magic Cookie and verify
         UDPGetArray((BYTE*)&dw, sizeof(DWORD));
         if(dw != 0x63538263ul)
            break;

         // Obtain options
         for(;;)
         {
            // Get option type
            if(!UDPGet(&Option))
               break;
            if(Option == DHCP_END_OPTION)
               break;

            // Get option length
            UDPGet(&Len);
   
            // Process option
            switch(Option)
            {
               case DHCP_MESSAGE_TYPE:
                  UDPGet(&i);
                  switch(i)
                  {
                     case DHCP_DISCOVER_MESSAGE:
                        DHCPReplyToDiscovery(&BOOTPHeader);
                        break;

                     case DHCP_REQUEST_MESSAGE:
                        DHCPReplyToRequest(&BOOTPHeader, bAccept, bRenew);
                        break;

                     // Need to handle these if supporting more than one DHCP lease
                     case DHCP_RELEASE_MESSAGE:
                     case DHCP_DECLINE_MESSAGE:
                        break;
                  }
                  break;

               case DHCP_PARAM_REQUEST_IP_ADDRESS:
                  if(Len == 4u)
                  {
                     IP_ADDR tmp_ip;
                     // Get the requested IP address and see if it is the one we have on offer.
                     UDPGetArray((BYTE*)&dw, 4);
                     Len -= 4;
                     tmp_ip.Val = dw;
                     //bAccept = (dw == DHCPNextLease.Val);
                     if(TRUE == isIpAddrInPool(tmp_ip)){bRenew= TRUE; bAccept = TRUE;}
                     else if(tmp_ip.Val == 0x00000000u) {bRenew = FALSE; bAccept = TRUE;}
                     else 
                     {
                        bRenew = FALSE; 
                        bAccept = FALSE;
                        debug_dhcps(debug_putc, "DHCP bAccept=FALSE DHCP_PARAM_REQUEST_IP_ADDRESS2\r\n");
                     }
   
                  }
                  break;

               case DHCP_END_OPTION:
                  UDPDiscard();
                  return;
            }

            // Remove any unprocessed bytes that we don't care about
            while(Len--)
            {
               UDPGet(&i);
            }
         }         

         UDPDiscard();
         break;
   }
}


/*****************************************************************************
  Function:
   static void DHCPReplyToDiscovery(BOOTP_HEADER *Header)

  Summary:
   Replies to a DHCP Discover message.

  Description:
   This function replies to a DHCP Discover message by sending out a 
   DHCP Offer message.

  Precondition:
   None

  Parameters:
   Header - the BootP header this is in response to.

  Returns:
     None
  ***************************************************************************/
static void DHCPReplyToDiscovery(BOOTP_HEADER *Header)
{
   BYTE i;
   INT8         IndexOfPool;
   IP_ADDR       ipAddr;
   // Set the correct socket to active and ensure that 
   // enough space is available to generate the DHCP response
   if(UDPIsPutReady(MyDHCPsSocket) < 300u)
      return;

   // find in pool
   IndexOfPool = preAssign_ToDHCPClient_FromPool(Header);
   if( -1 == IndexOfPool) return;
   
   // Begin putting the BOOTP Header and DHCP options
   UDPPut(BOOT_REPLY);         // Message Type: 2 (BOOTP Reply)
   // Reply with the same Hardware Type, Hardware Address Length, Hops, and Transaction ID fields
   UDPPutArray((BYTE*)&(Header->HardwareType), 7);
   UDPPut(0x00);            // Seconds Elapsed: 0 (Not used)
   UDPPut(0x00);            // Seconds Elapsed: 0 (Not used)
   UDPPutArray((BYTE*)&(Header->BootpFlags), sizeof(Header->BootpFlags));
   UDPPut(0x00);            // Your (client) IP Address: 0.0.0.0 (none yet assigned)
   UDPPut(0x00);            // Your (client) IP Address: 0.0.0.0 (none yet assigned)
   UDPPut(0x00);            // Your (client) IP Address: 0.0.0.0 (none yet assigned)
   UDPPut(0x00);            // Your (client) IP Address: 0.0.0.0 (none yet assigned)
   //UDPPutArray((BYTE*)&DHCPNextLease, sizeof(IP_ADDR));   // Lease IP address to give out
   ipAddr = GetIPAddrFromIndex_DhcpPool(IndexOfPool);
   UDPPutArray((UINT8*)&ipAddr, sizeof(IP_ADDR));   // Lease IP address to give out
   UDPPut(0x00);            // Next Server IP Address: 0.0.0.0 (not used)
   UDPPut(0x00);            // Next Server IP Address: 0.0.0.0 (not used)
   UDPPut(0x00);            // Next Server IP Address: 0.0.0.0 (not used)
   UDPPut(0x00);            // Next Server IP Address: 0.0.0.0 (not used)
   UDPPut(0x00);            // Relay Agent IP Address: 0.0.0.0 (not used)
   UDPPut(0x00);            // Relay Agent IP Address: 0.0.0.0 (not used)
   UDPPut(0x00);            // Relay Agent IP Address: 0.0.0.0 (not used)
   UDPPut(0x00);            // Relay Agent IP Address: 0.0.0.0 (not used)
   UDPPutArray((BYTE*)&(Header->ClientMAC), sizeof(MAC_ADDR));   // Client MAC address: Same as given by client
   for(i = 0; i < 64+128+(16-sizeof(MAC_ADDR)); i++)   // Remaining 10 bytes of client hardware address, server host name: Null string (not used)
      UDPPut(0x00);                           // Boot filename: Null string (not used)
   UDPPut(0x63);            // Magic Cookie: 0x63538263
   UDPPut(0x82);            // Magic Cookie: 0x63538263
   UDPPut(0x53);            // Magic Cookie: 0x63538263
   UDPPut(0x63);            // Magic Cookie: 0x63538263
   
   // Options: DHCP Offer
   UDPPut(DHCP_MESSAGE_TYPE);   
   UDPPut(1);
   UDPPut(DHCP_OFFER_MESSAGE);

   // Option: Subnet Mask
   UDPPut(DHCP_SUBNET_MASK);
   UDPPut(sizeof(IP_ADDR));
   UDPPutArray((BYTE*)&AppConfig.MyMask, sizeof(IP_ADDR));

   // Option: Lease duration
   UDPPut(DHCP_IP_LEASE_TIME);
   UDPPut(4);
   UDPPut((DHCP_LEASE_DURATION>>24) & 0xFF);
   UDPPut((DHCP_LEASE_DURATION>>16) & 0xFF);
   UDPPut((DHCP_LEASE_DURATION>>8) & 0xFF);
   UDPPut((DHCP_LEASE_DURATION) & 0xFF);

   // Option: Server identifier
   UDPPut(DHCP_SERVER_IDENTIFIER);   
   UDPPut(sizeof(IP_ADDR));
   UDPPutArray((BYTE*)&AppConfig.MyIPAddr, sizeof(IP_ADDR));

   // Option: Router/Gateway address
   UDPPut(DHCP_ROUTER);      
   UDPPut(sizeof(IP_ADDR));
   UDPPutArray((BYTE*)&AppConfig.MyIPAddr, sizeof(IP_ADDR));

   // Option: DNS server address
   UDPPut(DHCP_DNS);
   UDPPut(sizeof(IP_ADDR));
   UDPPutArray((BYTE*)&AppConfig.MyIPAddr, sizeof(IP_ADDR));

   // No more options, mark ending
   UDPPut(DHCP_END_OPTION);

   // Add zero padding to ensure compatibility with old BOOTP relays that discard small packets (<300 UDP octets)
   while(UDPTxCount < 300u)
      UDPPut(0); 

   // Force remote destination address to be the broadcast address, regardless 
   // of what the node's source IP address was (to ensure we don't try to 
   // unicast to 0.0.0.0).
   memset((void*)&UDPSocketInfo[MyDHCPsSocket].remote.remoteNode, 0xFF, sizeof(NODE_INFO));

   // Transmit the packet
   UDPFlush();
}


/*****************************************************************************
  Function:
   static void DHCPReplyToRequest(BOOTP_HEADER *Header, BOOL bAccept)

  Summary:
   Replies to a DHCP Request message.

  Description:
   This function replies to a DHCP Request message by sending out a 
   DHCP Acknowledge message.

  Precondition:
   None

  Parameters:
   Header - the BootP header this is in response to.
   bAccept - whether or not we've accepted this request

  Returns:
     None
  
  Internal:
   Needs to support more than one simultaneous lease in the future.
  ***************************************************************************/
static void DHCPReplyToRequest(BOOTP_HEADER *Header, BOOL bAccept, BOOL bRenew)
{
   BYTE i;
   INT8 indexOfPool = 255;
   IP_ADDR       ipAddr;
   // Set the correct socket to active and ensure that 
   // enough space is available to generate the DHCP response
   if(UDPIsPutReady(MyDHCPsSocket) < 300u)
      return;

   // Search through all remaining options and look for the Requested IP address field
   // Obtain options
   
   
   while(UDPIsGetReady(MyDHCPsSocket))
   {
      BYTE Option, Len;
      DWORD dw;
      MAC_ADDR tmp_MacAddr;
      
      // Get option type
      if(!UDPGet(&Option))
         break;
      if(Option == DHCP_END_OPTION)
         break;

      // Get option length
      UDPGet(&Len);
      if(bRenew)
      {
         if((Option == DHCP_PARAM_REQUEST_CLIENT_ID) && (Len == 7u))
         {
            // Get the requested IP address and see if it is the one we have on offer.   If not, we should send back a NAK, but since there could be some other DHCP server offering this address, we'll just silently ignore this request.
            UDPGet(&i);
            UDPGetArray((UINT8*)&tmp_MacAddr, 6);
            Len -= 7;
            indexOfPool = getIndexByMacaddr_DhcpPool(&tmp_MacAddr);//(&tmp_MacAddr,(IPV4_ADDR*)&Header->);
            if(-1 != indexOfPool)
            {
               if(GetIPAddrFromIndex_DhcpPool(indexOfPool).Val ==   Header->ClientIP.Val)
                  postAssign_ToDHCPClient_FromPool(&tmp_MacAddr, &(Header->ClientIP));
               else
               {
                  debug_dhcps(debug_putc, "DHCPS bAccept=FALSE DHCP_PARAM_REQUEST_CLIENT_ID0\r\n");
                  bAccept = FALSE;
               }
            }
            else
            {
               debug_dhcps(debug_putc, "DHCPS bAccept=FALSE DHCP_PARAM_REQUEST_CLIENT_ID1\r\n");
               bAccept = FALSE;
            }
            
            break;
         }
      }
      else
      {
      //
         if((Option == DHCP_PARAM_REQUEST_IP_ADDRESS) && (Len == 4u))
         {
            // Get the requested IP address and see if it is the one we have on offer.  If not, we should send back a NAK, but since there could be some other DHCP server offering this address, we'll just silently ignore this request.
            UDPGetArray((UINT8*)&dw, 4);
            Len -= 4;
            indexOfPool = postAssign_ToDHCPClient_FromPool(&(Header->ClientMAC),(IP_ADDR*)&dw);
            if( -1 == indexOfPool)
            {
               bAccept = FALSE;
               debug_dhcps(debug_putc, "DHCPS bAccept=FALSE DHCP_PARAM_REQUEST_IP_ADDRESS1\r\n");
            }
            break;
         }
      }

      

      // Remove the unprocessed bytes that we don't care about
      while(Len--)
      {
         UDPGet(&i);
      }
   }         

   // Begin putting the BOOTP Header and DHCP options
   UDPPut(BOOT_REPLY);         // Message Type: 2 (BOOTP Reply)
   // Reply with the same Hardware Type, Hardware Address Length, Hops, and Transaction ID fields
   UDPPutArray((BYTE*)&(Header->HardwareType), 7);
   UDPPut(0x00);            // Seconds Elapsed: 0 (Not used)
   UDPPut(0x00);            // Seconds Elapsed: 0 (Not used)
   UDPPutArray((BYTE*)&(Header->BootpFlags), sizeof(Header->BootpFlags));
   UDPPutArray((BYTE*)&(Header->ClientIP), sizeof(IP_ADDR));// Your (client) IP Address:
   //UDPPutArray((BYTE*)&DHCPNextLease, sizeof(IP_ADDR));   // Lease IP address to give out
   if(bAccept)      ipAddr = GetIPAddrFromIndex_DhcpPool(indexOfPool);
   else          ipAddr.Val=0u;
   UDPPutArray((UINT8*)&ipAddr, sizeof(IP_ADDR));   // Lease IP address to give out
   UDPPut(0x00);            // Next Server IP Address: 0.0.0.0 (not used)
   UDPPut(0x00);            // Next Server IP Address: 0.0.0.0 (not used)
   UDPPut(0x00);            // Next Server IP Address: 0.0.0.0 (not used)
   UDPPut(0x00);            // Next Server IP Address: 0.0.0.0 (not used)
   UDPPut(0x00);            // Relay Agent IP Address: 0.0.0.0 (not used)
   UDPPut(0x00);            // Relay Agent IP Address: 0.0.0.0 (not used)
   UDPPut(0x00);            // Relay Agent IP Address: 0.0.0.0 (not used)
   UDPPut(0x00);            // Relay Agent IP Address: 0.0.0.0 (not used)
   UDPPutArray((BYTE*)&(Header->ClientMAC), sizeof(MAC_ADDR));   // Client MAC address: Same as given by client
   for(i = 0; i < 64+128+(16-sizeof(MAC_ADDR)); i++)   // Remaining 10 bytes of client hardware address, server host name: Null string (not used)
      UDPPut(0x00);                           // Boot filename: Null string (not used)
   UDPPut(0x63);            // Magic Cookie: 0x63538263
   UDPPut(0x82);            // Magic Cookie: 0x63538263
   UDPPut(0x53);            // Magic Cookie: 0x63538263
   UDPPut(0x63);            // Magic Cookie: 0x63538263
   
   // Options: DHCP lease ACKnowledge
   if(bAccept)
   {
      UDPPut(DHCP_OPTION_ACK_MESSAGE);   
      UDPPut(1);
      UDPPut(DHCP_ACK_MESSAGE);
   }
   else   // Send a NACK
   {
      UDPPut(DHCP_OPTION_ACK_MESSAGE);   
      UDPPut(1);
      UDPPut(DHCP_NAK_MESSAGE);
   }

   // Option: Lease duration
   UDPPut(DHCP_IP_LEASE_TIME);
   UDPPut(4);
   UDPPut((DHCP_LEASE_DURATION>>24) & 0xFF);
   UDPPut((DHCP_LEASE_DURATION>>16) & 0xFF);
   UDPPut((DHCP_LEASE_DURATION>>8) & 0xFF);
   UDPPut((DHCP_LEASE_DURATION) & 0xFF);

   // Option: Server identifier
   UDPPut(DHCP_SERVER_IDENTIFIER);   
   UDPPut(sizeof(IP_ADDR));
   UDPPutArray((BYTE*)&AppConfig.MyIPAddr, sizeof(IP_ADDR));

   // Option: Subnet Mask
   UDPPut(DHCP_SUBNET_MASK);
   UDPPut(sizeof(IP_ADDR));
   UDPPutArray((BYTE*)&AppConfig.MyMask, sizeof(IP_ADDR));

   // Option: Router/Gateway address
   UDPPut(DHCP_ROUTER);      
   UDPPut(sizeof(IP_ADDR));
   UDPPutArray((BYTE*)&AppConfig.MyIPAddr, sizeof(IP_ADDR));

   // Option: DNS server address
   UDPPut(DHCP_DNS);
   UDPPut(sizeof(IP_ADDR));
   UDPPutArray((BYTE*)&AppConfig.MyIPAddr, sizeof(IP_ADDR));

   // No more options, mark ending
   UDPPut(DHCP_END_OPTION);

   // Add zero padding to ensure compatibility with old BOOTP relays that discard small packets (<300 UDP octets)
   while(UDPTxCount < 300u)
      UDPPut(0); 

   // Force remote destination address to be the broadcast address, regardless 
   // of what the node's source IP address was (to ensure we don't try to 
   // unicast to 0.0.0.0).
   memset((void*)&UDPSocketInfo[MyDHCPsSocket].remote.remoteNode, 0xFF, sizeof(NODE_INFO));

   // Transmit the packet
   UDPFlush();
}

// new darren rook/ccs
static void DHCPServerInit(void)
{
   int i;
   //init ip pool
   _g_DhcpsNextIp = AppConfig.MyIPAddr.v[3];
   
   for(i = 0;i < MAX_DHCP_CLIENTS_NUMBER; i++)
   {
      DhcpIpPool[i].isUsed = FALSE;
      DhcpIpPool[i].Client_Lease_Time = 0; //   1 hour

      DhcpIpPool[i].Client_Addr.v[0] = AppConfig.MyIPAddr.v[0];
      DhcpIpPool[i].Client_Addr.v[1] = AppConfig.MyIPAddr.v[1];
      DhcpIpPool[i].Client_Addr.v[2] = AppConfig.MyIPAddr.v[2];
      DhcpIpPool[i].Client_Addr.v[3] = _DhcpsGetNextIpByte();
      
      DhcpIpPool[i].ClientMAC.v[0]=0;
      DhcpIpPool[i].ClientMAC.v[1]=0;
      DhcpIpPool[i].ClientMAC.v[2]=0;
      DhcpIpPool[i].ClientMAC.v[3]=0;
      DhcpIpPool[i].ClientMAC.v[4]=0;
      DhcpIpPool[i].ClientMAC.v[5]=0;
   }
}

static void renew_dhcps_Pool(void)
{
   static UINT32 dhcp_timer=0;
   UINT32 current_timer = TickGet();
   int i;
   if((current_timer - dhcp_timer)<1*TICK_SECOND)
   {
      return;
   }
   dhcp_timer = current_timer;
   for(i=0;i<MAX_DHCP_CLIENTS_NUMBER;i++)
   {
      if(DhcpIpPool[i].isUsed == FALSE) continue;

      if(DhcpIpPool[i].Client_Lease_Time != 0) DhcpIpPool[i].Client_Lease_Time --;

      if(DhcpIpPool[i].Client_Lease_Time == 0)
      {
         DhcpIpPool[i].isUsed = FALSE;
         
         DhcpIpPool[i].ClientMAC.v[0]=00;
         DhcpIpPool[i].ClientMAC.v[1]=00;
         DhcpIpPool[i].ClientMAC.v[2]=00;
         DhcpIpPool[i].ClientMAC.v[3]=00;
         DhcpIpPool[i].ClientMAC.v[4]=00;
         DhcpIpPool[i].ClientMAC.v[5]=00;
         
         DhcpIpPool[i].Client_Addr.v[3] = _DhcpsGetNextIpByte();
      }
   }
}

//static UINT8 getIndexByMacaddr_DhcpPool(const MAC_ADDR *MacAddr)
static UINT8 getIndexByMacaddr_DhcpPool(MAC_ADDR *MacAddr)
{
   int i;
   if(FALSE == isMacAddr_Effective(MacAddr)) return -1;
   for(i=0;i<MAX_DHCP_CLIENTS_NUMBER;i++)
   {
      if(TRUE == Compare_MAC_addr(&DhcpIpPool[i].ClientMAC, MacAddr)) return i;
   }
   return -1;
}
static BOOL isIpAddrInPool(IP_ADDR ipaddr)
{
   int i;
   for(i=0;i<MAX_DHCP_CLIENTS_NUMBER;i++)
   {
      if(DhcpIpPool[i].Client_Addr.Val == ipaddr.Val)
      {
            return TRUE;
      }
   }
   return FALSE;
}

static IP_ADDR GetIPAddrFromIndex_DhcpPool(UINT8 index)
{
   IP_ADDR tmpIpAddr;
   tmpIpAddr.Val=0u;
   if(index > MAX_DHCP_CLIENTS_NUMBER) return tmpIpAddr;
   return DhcpIpPool[index].Client_Addr;
}
//static BOOL Compare_MAC_addr(const MAC_ADDR *macAddr1, const MAC_ADDR *macAddr2)
static BOOL Compare_MAC_addr(MAC_ADDR *macAddr1, MAC_ADDR *macAddr2)
{
   int i;
   for(i=0;i<6;i++)
   {
      if(macAddr1->v[i] != macAddr2->v[i]) return FALSE;
   }
   return TRUE;
}
//static BOOL isMacAddr_Effective(const MAC_ADDR *macAddr)
static BOOL isMacAddr_Effective(MAC_ADDR *macAddr)
{
   int i;
   for(i=0;i<6;i++)
   {
      if(macAddr->v[i] != 0) return TRUE;
   }
   return FALSE;
}
static UINT8 preAssign_ToDHCPClient_FromPool(BOOTP_HEADER *Header)
{
   int i;
   // if MAC==00:00:00:00:00:00, then return -1
   if(FALSE == isMacAddr_Effective(&(Header->ClientMAC)))
   {
      debug_dhcps(debug_putc, "DHCPS preAssign !effective\r\n");
      return -1;
   }
   // Find in Pool, look for the same MAC addr
   for(i=0;i<MAX_DHCP_CLIENTS_NUMBER;i++)
   {
      if(TRUE == Compare_MAC_addr(&DhcpIpPool[i].ClientMAC, &Header->ClientMAC))
      {
         //if(true == DhcpIpPool[i].isUsed) return -1;
         //DhcpIpPool[i].isUsed = true;
         debug_dhcps(debug_putc, "DHCPS Pool PreAssign is %lu %X:%X:%X:%X:%X:%X\r\n", i, Header->ClientMAC.v[0], Header->ClientMAC.v[1], Header->ClientMAC.v[2], Header->ClientMAC.v[3], Header->ClientMAC.v[4], Header->ClientMAC.v[5]);
         return i;
      }
   }
   // Find in pool, look for a empty MAC addr
   for(i=0;i<MAX_DHCP_CLIENTS_NUMBER;i++)
   {
      if(FALSE == isMacAddr_Effective(&DhcpIpPool[i].ClientMAC))
      {  // this is empty MAC in pool
         int j;
        //#if defined(__PCD__)
        #if 0
         #warning QUIRK?
         memcpy(&DhcpIpPool[i].ClientMAC, &Header->ClientMAC, 6);
        #else
         #warning QUIRK?
         for(j=0;j<6;j++)  
         {
            DhcpIpPool[i].ClientMAC.v[j] = Header->ClientMAC.v[j];
         }
        #endif
         ////debug_dhcps(debug_putc, "DHCPS Pool PreAssign add_empty0 %u %X:%X:%X:%X:%X:%X\r\n", i, Header->ClientMAC.v[0], Header->ClientMAC.v[1], Header->ClientMAC.v[2], Header->ClientMAC.v[3], Header->ClientMAC.v[4], Header->ClientMAC.v[5]);
         debug_dhcps(debug_putc, "DHCPS Pool PreAssign add_empty0 %lu %X:%X:%X:%X:%X:%X\r\n", i, DhcpIpPool[i].ClientMAC.v[0], DhcpIpPool[i].ClientMAC.v[1], DhcpIpPool[i].ClientMAC.v[2], DhcpIpPool[i].ClientMAC.v[3], DhcpIpPool[i].ClientMAC.v[4], DhcpIpPool[i].ClientMAC.v[5]);
         //DhcpIpPool[i].isUsed = true;
         return i;
      }
   }
   #if 1
   // Find in pool, look for a unsued item
   for(i=0;i<MAX_DHCP_CLIENTS_NUMBER;i++)
   {
      if(FALSE == DhcpIpPool[i].isUsed)
      {  // this is unused MAC in pool
         int j;
         for(j=0;j<6;j++)  DhcpIpPool[i].ClientMAC.v[j] = Header->ClientMAC.v[j];
         debug_dhcps(debug_putc, "DHCPS Pool PreAssign add_empty1 %lu %X:%X:%X:%X:%X:%X\r\n", i, Header->ClientMAC.v[0], Header->ClientMAC.v[1], Header->ClientMAC.v[2], Header->ClientMAC.v[3], Header->ClientMAC.v[4], Header->ClientMAC.v[5]);
         DhcpIpPool[i].isUsed = TRUE;
         return i;
      }
   }
   #endif
   return -1;
   
}
static UINT8 postAssign_ToDHCPClient_FromPool(MAC_ADDR *macAddr, IP_ADDR *ipv4Addr)
{
   int i;
   debug_dhcps(debug_putc, "DHCPS Pool PostAssign check %X:%X:%X:%X:%X:%X %u.%u.%u.%u\r\n", macAddr->v[0], macAddr->v[1], macAddr->v[2], macAddr->v[3], macAddr->v[4], macAddr->v[5], ipv4Addr->v[0], ipv4Addr->v[1], ipv4Addr->v[2], ipv4Addr->v[3]);
   for(i=0;i<MAX_DHCP_CLIENTS_NUMBER;i++)
   {
      debug_dhcps(debug_putc, "---DHCPS Pool %lu check %X:%X:%X:%X:%X:%X %u.%u.%u.%u\r\n", i, DhcpIpPool[i].ClientMAC.v[0], DhcpIpPool[i].ClientMAC.v[1], DhcpIpPool[i].ClientMAC.v[2], DhcpIpPool[i].ClientMAC.v[3], DhcpIpPool[i].ClientMAC.v[4], DhcpIpPool[i].ClientMAC.v[5], DhcpIpPool[i].Client_Addr.v[0], DhcpIpPool[i].Client_Addr.v[1], DhcpIpPool[i].Client_Addr.v[2], DhcpIpPool[i].Client_Addr.v[3]);
      
      if(ipv4Addr->Val == DhcpIpPool[i].Client_Addr.Val)
      {
         if(TRUE == Compare_MAC_addr(macAddr,&DhcpIpPool[i].ClientMAC))
         {
            DhcpIpPool[i].isUsed = TRUE;
            DhcpIpPool[i].Client_Lease_Time = DHCP_LEASE_DURATION;
            debug_dhcps(debug_putc, "DHCPS Pool PostAssign matches %lu\r\n", i);
            return i;
         }
         else 
         {
            debug_dhcps(debug_putc, "DHCPS Pool PostAssign !IP\r\n");
            return -1;
         }
      }
   }
   debug_dhcps(debug_putc, "DHCPS Pool PostAssign !FOUND\r\n");
   return -1;
}
void DHCPServer_Disable(void)
{
   smDHCPServer = DHCPS_DISABLE;

   if(MyDHCPsSocket != INVALID_UDP_SOCKET)
   {
      UDPClose(MyDHCPsSocket);
      MyDHCPsSocket = INVALID_UDP_SOCKET;
   }
}

void DHCPServer_Enable(void)
{
   if(smDHCPServer == DHCPS_DISABLE)
   {
      smDHCPServer = DHCPS_OPEN_SOCKET;
      
      DHCPServerInit();
   }
}
#endif
