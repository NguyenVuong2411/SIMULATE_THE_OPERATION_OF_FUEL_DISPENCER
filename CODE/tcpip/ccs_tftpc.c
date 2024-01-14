/*
   ccs_tftpc.c
   
   CCS's TFTP client.  Does a PUT to send the a file to the configured server.
   
   See ccs_tftpc.h for documentation
*/
///////////////////////////////////////////////////////////////////////////
////        (C) Copyright 1996,2016 Custom Computer Services           ////
////                                                                   ////
//// This source code may only be used by licensed users of the CCS C  ////
//// compiler or to users who purchased the rights to use this code.   ////
//// This source code may only be distributed to other licensed users  ////
//// of the CCS C compiler or other users who purchased the rights to  ////
//// this code.  The rights to use this code can only be granted       ////
//// by CCS.  No other use, reproduction or distribution is permitted  ////
//// without written permission.  Derivative programs created using    ////
//// this software in object code form are not restricted in any way.  ////
////                                                                   ////
////                     http://www.ccsinfo.com                        ////
///////////////////////////////////////////////////////////////////////////

#ifndef __CCS_TFTPC_C__
#define __CCS_TFTPC_C__

#define _TFTPC_TIMEOUT_TICKS  (TICKS_PER_SECOND*(TICK)3)
#define _TFTPC_TX_ATTEMPTS    3  //a value of 1 means no retries

#ifndef debug_tftpc
#define debug_tftpc(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)
#endif

#ifndef TFTPC_NOTIFY_NO_SOCKETS
#define TFTPC_NOTIFY_NO_SOCKETS(remotePort)
#endif

typedef enum
{
   _TFTCP_SM_IDLE = 0,
   _TFTCP_SM_DO_TX,
   _TFTCP_SM_GET_ACK
} _tftpc_state_t;

struct
{
   UDP_SOCKET socket;
   char* remoteFilename;   
   unsigned int16 block;
   int sent;
   _tftpc_state_t state;
   TICK t;
   tftpc_error_t error;
   int1 last;
} _g_TFTPC = {INVALID_UDP_SOCKET, NULL};

// See ccs_tftpc.h for documentation
int1 TFTPCIsBusy(void)
{
   return(_g_TFTPC.state != _TFTCP_SM_IDLE);
}

// See ccs_tftpc.h for documentation
int1 TFTPCStart(DWORD remoteHost, BYTE remoteHostType, UDP_PORT remotePort, char* remoteFilename)
{
   size_t len;
   
   if (TFTPCIsBusy())
      return(FALSE);

   TFTPCInit();
   _g_TFTPC.error = TFTPC_ERROR_BUSY;
   
   len = strlen(remoteFilename);
   _g_TFTPC.remoteFilename = malloc(len + 1);
   if (_g_TFTPC.remoteFilename == NULL)
   {
      debug_tftpc(debug_putc, "TFTPCStart() MALLOC_ERROR\r\n");
      return(FALSE);
   }
   strcpy(_g_TFTPC.remoteFilename, remoteFilename);

   _g_TFTPC.socket = UDPOpenEx(remoteHost, remoteHostType, 0, remotePort);
   
   if (_g_TFTPC.socket == INVALID_UDP_SOCKET)
   {
      TFTPC_NOTIFY_NO_SOCKETS(remotePort);
      debug_tftpc(debug_putc, "TFTPCStart() NO_SOCK_ERROR\r\n");
      TFTPCInit();
      return(FALSE);
   }
   
   debug_tftpc(debug_putc, "TFTPCStart('%s') STARTED\r\n", remoteFilename);
   _g_TFTPC.state = _TFTCP_SM_DO_TX;
   _g_TFTPC.t = TickGet();
   
   return(TRUE);
}

ROM char _g_TftpcOctectStr[] = "octet\0";

// TRUE if sent
static int1 _TFTPCTx(void)
{
   unsigned int8 hdr[4];
   unsigned int16 len;
   unsigned int32 offset;

   if 
   (
      !UDPIsOpened(_g_TFTPC.socket) ||
      (UDPIsPutReady(_g_TFTPC.socket) < 516)
   )
   {
      return(FALSE);
   }
   
   _g_TFTPC.sent += 1;
   
   debug_tftpc(debug_putc, "_TFTPCTx() BLOCK=%lu try=%u\r\n", _g_TFTPC.block, _g_TFTPC.sent);
   
   if (_g_TFTPC.block == 0)
   {
      hdr[0] = 0;
      hdr[1] = 2;    //write request
      UDPPutArray(hdr, 2);
      
      len = strlen(_g_TFTPC.remoteFilename);
      UDPPutArray(_g_TFTPC.remoteFilename, len+1); //send null terminator
      
      UDPPutROMArray(_g_TftpcOctectStr, 6);  //send null terminator
   }
   else
   {
      hdr[0] = 0;
      hdr[1] = 3; //data packet
      hdr[2] = make8(_g_TFTPC.block, 1);
      hdr[3] = make8(_g_TFTPC.block, 0);
      UDPPutArray(hdr, 4);
      
      offset = (unsigned int32)_g_TFTPC.block - 1;
      offset *= (unsigned int32)512;
      len = TFTPCPut(_g_TFTPC.socket, offset);
      if (len != 512)
      {
         _g_TFTPC.last = TRUE;
      }
   }
   
   UDPFlush();
   
   return(TRUE);
}

// TRUE if we got an ack to our block
static int1 _TFTPCRx(void)
{
   unsigned int8 hdr[4];
   unsigned int16 block;
   
   if (UDPIsGetReady(_g_TFTPC.socket) < 4)
   {
      return(FALSE);
   }
   
   UDPGetArray(hdr, 4);
   UDPDiscard();
   
   block = make16(hdr[2], hdr[3]);
   
   debug_tftpc(debug_putc, "_TFTPCRx() cmd=%u block=%lu\r\n", hdr[1], block);
   
   return
   (
      (hdr[0] == 0) &&
      (hdr[1] == 4) &&
      (block == _g_TFTPC.block)
   );
}

// See ccs_tftpc.h for documentation
void TFTPCCancel(void)
{
   debug_tftpc(debug_putc, "TFTPCCancel()\r\n");
   TFTPCInit();
}

// See ccs_tftpc.h for documentation
void TFTPCInit(void)
{
   debug_tftpc(debug_putc, "TFTPCInit()\r\n");
   
   if (_g_TFTPC.socket != INVALID_UDP_SOCKET)
   {
      UDPClose(_g_TFTPC.socket);
   }
   
   if (_g_TFTPC.remoteFilename != NULL)
   {
      free(_g_TFTPC.remoteFilename);
   }
   
   memset(&_g_TFTPC, 0, sizeof(_g_TFTPC));
   
   _g_TFTPC.socket = INVALID_UDP_SOCKET;
   _g_TFTPC.error = TFTPC_ERROR_NOT_STARTED;
}

// See ccs_tftpc.h for documentation
void TFTPCTask(void)
{
   tftpc_error_t newError;
   
   if (!TFTPCIsBusy())
      return;
      
   switch(_g_TFTPC.state)
   {
      case _TFTCP_SM_DO_TX:
         if (_TFTPCTx())
         {
            _g_TFTPC.state = _TFTCP_SM_GET_ACK;
            _g_TFTPC.t = TickGet();
         }
         else if ((TickGet() - _g_TFTPC.t) >= _TFTPC_TIMEOUT_TICKS)
         {
            debug_tftpc(debug_putc, "TFTPCTask() TX_NO_UDP\r\n");
            newError = TFTPC_ERROR_NO_CONNECTION;
            TFTPCInit();
            _g_TFTPC.error = newError;
         }
         break;
         
      case _TFTCP_SM_GET_ACK:
         if (_TFTPCRx())
         {
            if 
            (
               (_g_TFTPC.block == 0) &&
               (_g_TFTPC.remoteFilename != NULL)
            )
            {
               free(_g_TFTPC.remoteFilename);
               _g_TFTPC.remoteFilename = NULL;
            }
            
            if (_g_TFTPC.last)
            {
               debug_tftpc(debug_putc, "TFTPCTask() DONE\r\n");
               newError = TFTPC_ERROR_OK;
               TFTPCInit();
               _g_TFTPC.error = newError;            
            }
            else
            {
               debug_tftpc(debug_putc, "TFTPCTask() GOT_ACK BLOCK=%lu\r\n", _g_TFTPC.block);
               _g_TFTPC.sent = 0;
               _g_TFTPC.block += 1;
               _g_TFTPC.state = _TFTCP_SM_DO_TX;
               _g_TFTPC.t = TickGet();
            }
         }
         else if ((TickGet() - _g_TFTPC.t) >= _TFTPC_TIMEOUT_TICKS)
         {
            if (_g_TFTPC.sent >= _TFTPC_TX_ATTEMPTS)
            {
               debug_tftpc(debug_putc, "TFTPCTask() RX_NO_ACK\r\n");
               newError = TFTPC_ERROR_NO_ACK;
               TFTPCInit();
               _g_TFTPC.error = newError;
            }
            else
            {
               // try again
               debug_tftpc(debug_putc, "TFTPCTask() RETRY BLOCK=%lu\r\n", _g_TFTPC.block);
               _g_TFTPC.state = _TFTCP_SM_DO_TX;
               _g_TFTPC.t = TickGet();
            }
         }
         break;
   }
}

#endif
