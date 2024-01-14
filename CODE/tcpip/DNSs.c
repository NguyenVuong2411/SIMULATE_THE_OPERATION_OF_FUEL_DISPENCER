/*********************************************************************
 *
 *  Domain Name System (DNS) Server dummy
 *  Module for Microchip TCP/IP Stack
 *    -Acts as a DNS server, but gives out the local IP address for all 
 *    queries to force web browsers to access the board.
 *    -Reference: RFC 1034 and RFC 1035
 *
 *********************************************************************
 * FileName:        DNSs.c
 * Dependencies:    UDP
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32
 * Compiler:        Microchip C32 v1.05 or higher
 *               Microchip C30 v3.12 or higher
 *               Microchip C18 v3.30 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * Copyright (C) 2002-2010 Microchip Technology Inc.  All rights
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
 * Author               Date      Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Howard Schlunder     01/18/2010   Original
 *
 * Darren Rook/CCS      10/22/2014   Repeat the question in the response.
 *                                     DO NOT USE DNSs.C FROM THEIR LATEST
 *                                     STACKS AS IT IS BROKEN.
 * Darren Rook/CCS      07/30/215    DNSServerTask() changed to support
 *                                     DNSS_ONLY_URL and to not deadlock
 *                                     the PICMCU while waiting for UDPIsPutReady()
 *                                     to be TRUE.  Max hostname len during
 *                                     request cannot be larger than
 *                                     DNSS_MAX_QUESTION_LEN (this value can be
 *                                     changed to change RAM allocation needed).
 *                                     Only answer A (IPv4) type record requests,
 *                                     that means ignore other record requests 
 *                                     like AAAA (IPv6).  DNSS_ONLY_ENA can be
 *                                     used to dynamically enable the use of
 *                                     DNSS_ONLY_URL.
 *                                     If DNSS_ONLY_URL returns NULL or is an 
 *                                     empty string, then it accepts all URLs. 
 ********************************************************************/
#define DNSS_C

#include "TCPIPConfig.h"

#if defined(STACK_USE_DNS_SERVER)

#include "TCPIP Stack/TCPIP.h"

#ifndef DNSS_NOTIFY_NO_SOCKETS
#define DNSS_NOTIFY_NO_SOCKETS(port)
#endif

// Default port for the DNS server to listen on
#ifndef DNSS_PORT
#define DNSS_PORT      53u
#endif

#ifndef debug_dnss
#define debug_dnss(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t)
#endif

#ifndef DNSS_MAX_QUESTION_LEN
#define DNSS_MAX_QUESTION_LEN 128
#endif

#if defined(DNSS_ONLY_URL) && !defined(DNSS_ONLY_ENA)
   #define DNSS_ONLY_ENA   1
#endif

static void DNSCopyRXNameToTX(void);
static size_t DNSCopyRXQuestionToRAM(unsigned int8 *question, size_t max);
static void DNSCopyRAMQuestionToTX(unsigned int8 *question, size_t len);
static int1 DNSQuestionMatchesHostname(unsigned int8 *question);
static unsigned int16 DNSQuestionToType(unsigned int8 *question, size_t len);

/*********************************************************************
 * Function:        void DNSServerTask(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Sends dummy responses that point to ourself for DNS requests
 *
 * Note:            None
 ********************************************************************/
 void DNSServerTask(void)
{
   static UDP_SOCKET   MySocket = INVALID_UDP_SOCKET;
   static struct   __PACKED
   {
      WORD wTransactionID;
      WORD wFlags;
      WORD wQuestions;
      WORD wAnswerRRs;
      WORD wAuthorityRRs;
      WORD wAdditionalRRs;
   } DNSHeader;
   static unsigned int8 question[DNSS_MAX_QUESTION_LEN];
   static size_t questionLen;
   static int1 ok;
   static int1 state;   //0 for init/rx, 1 for tx

   if (!state)
   {
      // Create a socket to listen on if this is the first time calling this function
      if(MySocket == INVALID_UDP_SOCKET)
      {
         MySocket = UDPOpen(DNSS_PORT, NULL, INVALID_UDP_SOCKET);
         
         if(MySocket == INVALID_UDP_SOCKET)
         {
            DNSS_NOTIFY_NO_SOCKETS(DNSS_PORT);
         }
         
         //MySocket = UDPOpenEx(0,UDP_OPEN_SERVER,DNSS_PORT,0);
         debug_dnss(debug_putc, "DNSS STARTED, SOCK=%X\r\n", MySocket);
         return;
      }
   
      if(UDPIsGetReady(MySocket))
      {
         debug_dnss(debug_putc, "DNSS incoming...\r\n");
      }
   
      // See if a DNS query packet has arrived
      if(UDPIsGetReady(MySocket) < sizeof(DNSHeader))
         return;

      debug_dnss(debug_putc, "DNSS incoming...\r\n");
  
      // Read DNS header
      UDPGetArray((BYTE*)&DNSHeader, sizeof(DNSHeader));
   
      // Ignore this packet if it isn't a query
      if((DNSHeader.wFlags & 0x8000) == 0x8000u)
         return;
   
      // Ignore this packet if there are no questions in it
      if(DNSHeader.wQuestions == 0u)
         return;

      questionLen = DNSCopyRXQuestionToRAM(question, sizeof(question));

      debug_dnss(debug_putc, "DNSS got question\r\n");
       
      state = 1;
   }
   
   if (state)
   {
      // Block until we can transmit a DNS response packet
      if(!UDPIsPutReady(MySocket))
         return;
         
      debug_dnss(debug_putc, "DNSS start TX of reply\r\n");
   
      // Write DNS response packet
      UDPPutArray((BYTE*)&DNSHeader.wTransactionID, 2);   // 2 byte Transaction ID
      if(DNSHeader.wFlags & 0x0100)
         UDPPut(0x81);   // Message is a response with recursion desired
      else
         UDPPut(0x80);   // Message is a response without recursion desired flag set
      ok = (DNSQuestionToType(question, questionLen) == 1);
      debug_dnss(debug_putc, "DNSQuestionToType() == 0x%LX\r\n", DNSQuestionToType(question, questionLen));
     #if defined(DNSS_ONLY_URL)
      if (DNSS_ONLY_ENA)
      {
         ok &= DNSQuestionMatchesHostname(question);
         debug_dnss(debug_putc, "DNSQuestionMatchesHostname() ok=%u\r\n", ok);
      }
     #endif
      if (ok)
      {
         UDPPut(0x80);   // Recursion available
      }
      else
      {
         UDPPut(0x83);   // Recursion available, no such name
      }
      UDPPut(0x00);   // 0x0000 Questions
      UDPPut(0x01);
      if (ok)
      {
         UDPPut(0x00);   // 0x0001 Answers RRs
         UDPPut(0x01);
      }
      else
      {
         UDPPut(0x00);   // 0x0000 Answers RRs
         UDPPut(0x00);   
      }
      UDPPut(0x00);   // 0x0000 Authority RRs
      UDPPut(0x00);
      UDPPut(0x00);   // 0x0000 Additional RRs
      UDPPut(0x00);
     // question
      DNSCopyRAMQuestionToTX(question, questionLen);   // Copy hostname of first question over to TX packet
     // answer
      if (ok)
      {
         DNSCopyRAMQuestionToTX(question, questionLen);   // Copy hostname of first question over to TX packet
         /* this is now done in DNSCopyRXNameToTX()
         UDPPut(0x00);   // Type A Host address
         UDPPut(0x01);
         UDPPut(0x00);   // Class INternet
         UDPPut(0x01);
         */
         UDPPut(0x00);   // Time to Live 10 seconds
         UDPPut(0x00);
         UDPPut(0x00);
         UDPPut(0x0A);
         UDPPut(0x00);   // Data Length 4 bytes
         UDPPut(0x04);
         UDPPutArray((BYTE*)&AppConfig.MyIPAddr.Val, 4);   // Our IP address
      }
      
      UDPFlush();
      state = 0;
   }
}



/*****************************************************************************
  Function:
   static void DNSCopyRXNameToTX(void)

  Summary:
   Copies a DNS hostname, possibly including name compression, from the RX 
   packet to the TX packet (without name compression in TX case).
   
  Description:
   None

  Precondition:
   RX pointer is set to currently point to the DNS name to copy

  Parameters:
   None

  Returns:
     None
  ***************************************************************************/
static void DNSCopyRXNameToTX(void)
{
   WORD w;
   BYTE i;
   BYTE len;

   while(1)
   {
      // Get first byte which will tell us if this is a 16-bit pointer or the 
      // length of the first of a series of labels
      if(!UDPGet(&i))
         return;
      
      // Check if this is a pointer, if so, get the reminaing 8 bits and seek to the pointer value
      if((i & 0xC0u) == 0xC0u)
      {
         ((BYTE*)&w)[1] = i & 0x3F;
         UDPGet((BYTE*)&w);
        #if defined(STACK_USE_BLUEGIGA_WF121)
         UDPSetRxBuffer(w);
        #else
         IPSetRxBuffer(sizeof(UDP_HEADER) + w);
        #endif
         continue;
      }

      // Write the length byte
      len = i;
      UDPPut(len);
      
      // Exit if we've reached a zero length label
      if(len == 0u)
      {
         //send TYPE and CLASS
         while (len < 4)
         {
            UDPGet(&i);
            UDPPut(i);
            len++;
         }
         
         return;
      }
      
      // Copy all of the bytes in this label   
      while(len--)
      {
         UDPGet(&i);
         UDPPut(i);
      }
   }
}

static size_t DNSCopyRXQuestionToRAM(unsigned int8 *question, size_t max)
{
   size_t ret = 0;
   WORD w;
   BYTE i;
   BYTE len;
   #define DNSCopyRXNameToRAM_PUSH(c)     *question = c; question++; ret++
   
   max -= 5;   //save 5 bytes for 0x00, TYPE and CLASS

   while(1)
   {
      // Get first byte which will tell us if this is a 16-bit pointer or the 
      // length of the first of a series of labels
      if(!UDPGet(&i))
         return(0);
      
      // Check if this is a pointer, if so, get the reminaing 8 bits and seek to the pointer value
      if((i & 0xC0u) == 0xC0u)
      {
         ((BYTE*)&w)[1] = i & 0x3F;
         UDPGet((BYTE*)&w);
        #if defined(STACK_USE_BLUEGIGA_WF121)
         UDPSetRxBuffer(w);
        #else
         IPSetRxBuffer(sizeof(UDP_HEADER) + w);
        #endif
         continue;
      }

      // Write the length byte
      len = i;
      
      // Exit if we've reached a zero length label
      if(len == 0u)
      {
         DNSCopyRXNameToRAM_PUSH(0);
         
         //send TYPE and CLASS
         while (len < 4)
         {
            UDPGet(&i);
            DNSCopyRXNameToRAM_PUSH(i); //UDPPut(i);
            len++;
         }
         
         return(ret);   //done, success
      }
      else
      {
         if ((len + 1) > (max - ret))
         {
            //no more space left, stop pushing onto ram
            max = ret;
         }
         else
         {
            DNSCopyRXNameToRAM_PUSH(len);
            while(len--)
            {
               UDPGet(&i);
               DNSCopyRXNameToRAM_PUSH(i);
            }
         }
      }
   }
   
   return(ret);   //probably shouldn't get here
}

static void DNSCopyRAMQuestionToTX(unsigned int8 *question, size_t len)
{
   while(len--)
   {
      UDPPut(*question);
      question++;
   }
}

// question format is the same as the question in the DNS query, which
//    includes length fields instead of '.' and 5 bytes at the end
//    which is the TYPE and CLASS
// DNSS_ONLY_URL hostname is in a format of "hostname.com"
// returns TRUE if 'question' matches DNSS_ONLY_URL
#if defined(DNSS_ONLY_URL)
static int1 DNSQuestionMatchesHostname(unsigned int8 *question)
{
   char hostname[DNSS_MAX_QUESTION_LEN], c;
   size_t idx = 0;
   unsigned int8 len;
   char *onlyUrl = DNSS_ONLY_URL;
   
   #define _DNSQuestionMatchesHostname_PULL()   *question++
   #define _DNSQuestionMatchesHostname_PUSH(x)  hostname[idx++]=x

   if ((onlyUrl == NULL) || (strlen(DNSS_ONLY_URL) == 0))
      return(TRUE);
   
   // first, turn question into a hostname
   // we assume that question won't be longer than DNSS_MAX_QUESTION_LEN
   for(;;)
   {
      len = _DNSQuestionMatchesHostname_PULL();
      
      if (!len)
         break;
      
      if (idx != 0)
      {
         _DNSQuestionMatchesHostname_PUSH('.');
      }
      
      while(len--)
      {
         c = _DNSQuestionMatchesHostname_PULL();
         _DNSQuestionMatchesHostname_PUSH(c);
      }
   }
   
   _DNSQuestionMatchesHostname_PUSH(0);
   
   debug_dnss(debug_putc, "DNSQuestionMatchesHostname() '%s' vs '%s'\r\n", hostname, onlyUrl);
   
   return(stricmp(hostname, onlyUrl) == 0);
}
#endif

static unsigned int16 DNSQuestionToType(unsigned int8 *question, size_t len)
{
   union
   {
      unsigned int8 b[2];
      unsigned int16 w;
   } ret;
   
   ret.b[0] = question[len-3];
   ret.b[1] = question[len-4];

   return(ret.w);
}

#endif //#if defined(STACK_USE_DNS_SERVER)
