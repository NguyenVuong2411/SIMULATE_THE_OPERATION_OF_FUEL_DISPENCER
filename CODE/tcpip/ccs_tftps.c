#ifndef THIS_IS_TFTP_C
#define THIS_IS_TFTP_C
/***************************************************************************
*
*                             ccs_tftps.c
*
* TFTP server for receiving files.  See ccs_tftps.h for documentation.
*
****************************************************************************/
///////////////////////////////////////////////////////////////////////////
////        (C) Copyright 1996, 2015 Custom Computer Services          ////
//// This source code may only be used by licensed users of the CCS C  ////
//// compiler.  This source code may only be distributed to other      ////
//// licensed users of the CCS C compiler.  No other use, reproduction ////
//// or distribution is permitted without written permission.          ////
//// Derivative programs created using this software in object code    ////
//// form are not restricted in any way.                               ////
///////////////////////////////////////////////////////////////////////////

#ifndef debug_tftp
 #define debug_tftp(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z)
#else
 #define __DO_DEBUG_TFTP
#endif

#ifndef TFTPS_NOTIFY_NO_SOCKETS
#define TFTPS_NOTIFY_NO_SOCKETS(port)
#endif

#ifndef TFTP_WRITE_ENABLED
#define TFTP_WRITE_ENABLED 1
#endif

#if TFTP_READ_ENABLED
#error this feature removed
#endif

#ifndef TFTPS_FINISHED_FILE
#define TFTPS_FINISHED_FILE(x)
#endif

#ifndef TFTPS_STARTED_FILE
#define TFTPS_STARTED_FILE(x)
#endif

#ifndef TFTPS_CLIENT_CONNECTED
#define TFTPS_CLIENT_CONNECTED()
#endif

#ifndef TFTPS_CLIENT_DISCONNECTED
#define TFTPS_CLIENT_DISCONNECTED()
#endif

#ifndef TFTPS_WRITE_FILE_SIZE
   #define TFTPS_WRITE_FILE_SIZE(x,n)
#endif

#if defined(STACK_USE_MPFS)
   #define MPFS_CLEAR_BLOCK() mpfsFlags.bits.bNotAvailable = 0
   #define MPFS_SET_BLOCK()   mpfsFlags.bits.bNotAvailable = 1
#elif defined(STACK_USE_MPFS2)
   extern BOOL isMPFSLocked;
   #define MPFS_CLEAR_BLOCK() MPFSInit()
   #define MPFS_SET_BLOCK()   isMPFSLocked = 1
#else
   #define MPFS_CLEAR_BLOCK()
   #define MPFS_SET_BLOCK()
#endif

#if defined(GENERIC_SCRATCH_BUFFER)
   #define TFTPS_SCRATCH_BUFFER   GENERIC_SCRATCH_BUFFER
   
   #define TFTP_BUFFER_SIZE   sizeof(TFTPS_SCRATCH_BUFFER)
   
   #if (TFTP_BUFFER_SIZE > 512)
      #undef TFTP_BUFFER_SIZE
      #define TFTP_BUFFER_SIZE      512
   #endif
#else
   #ifndef TFTP_BUFFER_SIZE
      #define TFTP_BUFFER_SIZE   120
   #elif (TFTP_BUFFER_SIZE > 512)
      #undef TFTP_BUFFER_SIZE
      #define TFTP_BUFFER_SIZE      512
   #endif

   char TftpsScratchBuffer[TFTP_BUFFER_SIZE];
   
   #define TFTPS_SCRATCH_BUFFER TftpsScratchBuffer
#endif


typedef enum
{
   _TFTPS_OPCODE_WRITE_REQUEST = 2,
   _TFTPS_OPCODE_DATA = 3,
   _TFTPS_OPCODE_ACK = 4,
   _TFTPS_OPCODE_ERROR = 5
} _tftps_opcode_t;

typedef enum
{
   _TFTPS_SM_STOPPED = 0,
   _TFTPS_SM_INIT,
   _TFTPS_SM_LISTENING,
   _TFTPS_SM_PUT_ERROR_BAD_FILENAME,
   _TFTPS_SM_PUT_ACK,
   _TFTPS_SM_GET_CMD,
   _TFTPS_SM_PUT_ERROR_BAD_CMD
} _tftps_state_t;

struct
{
   UDP_SOCKET socket;
   _tftps_state_t state;
   TICK tick;
   unsigned int16 block;
   unsigned int32 addy; //where data is being saved on the flash
   unsigned int8 filename; // _TFTPGetFilename() populates this after reading write request, -1 if invalid
} _g_TFTPS = {INVALID_UDP_SOCKET, _TFTPS_SM_STOPPED};

static void _TFTPSReset(void)
{
   debug_tftp(debug_putc, "_TFTPSReset()\r\n");
   
   if (_g_TFTPS.socket != INVALID_UDP_SOCKET)
   {
      UDPClose(_g_TFTPS.socket);
   }
   
   if (_g_TFTPS.state >= _TFTPS_SM_PUT_ACK)
   {
      MPFS_CLEAR_BLOCK();
   }
   
   if (_g_TFTPS.state > _TFTPS_SM_LISTENING)
   {
      TFTPS_CLIENT_DISCONNECTED();
   }
   
   memset(&_g_TFTPS, 0, sizeof(_g_TFTPS));
   
   _g_TFTPS.socket = INVALID_UDP_SOCKET;
   
   _g_TFTPS.state = _TFTPS_SM_INIT;
}

void TFTPSStop(void)
{
   debug_tftp(debug_putc, "TFTPSStop()\r\n");
   
   _TFTPSReset();
   
   _g_TFTPS.state = _TFTPS_SM_STOPPED;
}


#if defined(TFTP_REQUIRE_FNAME1) && !defined(TFTP_REQUIRE_FNAME0)
   #error your code was written for the old tftp server, fix your definitions
#endif

static void _TFTPSGetFilename(void)
{
#if defined(TFTP_REQUIRE_FNAME0)
   char c;
   unsigned int8 j=0;
   
   _g_TFTPS.filename = -1;
   
   if (!UDPGet(&c))
      return;
   
   if(c == '/')
   {
      if (!UDPGet(&c))
         return;
   }
      
   j=0;
   
   while ((c!=0) && (c!='/'))
   {
      TFTPS_SCRATCH_BUFFER[j++] = c;
      if (!UDPGet(&c))
      {
         return;
      }
   }
   
   TFTPS_SCRATCH_BUFFER[j] = 0;
   
  #if defined(TFTP_REQUIRE_FNAME0)
   if (stricmp(TFTPS_SCRATCH_BUFFER, TFTP_REQUIRE_FNAME0) == 0)
   {
      _g_TFTPS.filename = 0;
   }
  #endif
  #if defined(TFTP_REQUIRE_FNAME1)
   if (stricmp(TFTPS_SCRATCH_BUFFER, TFTP_REQUIRE_FNAME1) == 0)
   {
      _g_TFTPS.filename = 1;
   }
  #endif
  #if defined(TFTP_REQUIRE_FNAME2)
   if (stricmp(TFTPS_SCRATCH_BUFFER, TFTP_REQUIRE_FNAME2) == 0)
   {
      _g_TFTPS.filename = 2;
   }
  #endif
#else
   _g_TFTPS.filename = 0;
#endif
}

unsigned int32 _TFTPSFileAddy(void)
{
   unsigned int32 ret;
   
  #if defined(TFTP_REQUIRE_FNAME0)
   if (_g_TFTPS.filename == 0)
      ret = TFTP_REQUIRE_LOC0;
  #endif
   
  #if defined(TFTP_REQUIRE_FNAME1)
   if (_g_TFTPS.filename == 1)
      ret = TFTP_REQUIRE_LOC1;
  #endif
   
  #if defined(TFTP_REQUIRE_FNAME2)
   if (_g_TFTPS.filename == 2)
      ret = TFTP_REQUIRE_LOC2;
  #endif 
      
  #if !defined(TFTP_REQUIRE_FNAME0)
   ret = MPFS_RESERVE_BLOCK;
  #endif
            
   return(ret);
}

unsigned int32 _TFTPSFileMaxSize(void)
{
   unsigned int32 ret;
   
   #if defined(TFTP_REQUIRE_FNAME0)
      #if defined(TFTP_REQUIRE_FNAME0)
         if (_g_TFTPS.filename == 0)
            ret = TFTP_REQUIRE_SIZE0;
      #endif
      #if defined(TFTP_REQUIRE_FNAME1)
         if (_g_TFTPS.filename == 1)
            ret = TFTP_REQUIRE_SIZE1;
      #endif
      #if defined(TFTP_REQUIRE_FNAME2)
         if (_g_TFTPS.filename == 2)
            ret = TFTP_REQUIRE_SIZE2;
      #endif 
   #else
      ret = MPFS_RESERVE_SIZE;
   #endif
   
   return(ret);
}

void TFTPSInit(void)
{
   debug_tftp(debug_putc, "TFTPSInit()\r\n");

   _TFTPSReset();
}

void TFTPSTask(void)
{
   union
   {
      unsigned int8 b[4];
      unsigned int16 w[2];
      unsigned int32 dw;
      
      /*struct
      {
         IP_ADDR ip;
         UDP_PORT port;
      };*/
   } scr;
   
   if 
   (
      (_g_TFTPS.state > _TFTPS_SM_LISTENING) &&
      ((TickGet() - _g_TFTPS.tick) > ((TICK)TICKS_PER_SECOND*5))
   )
   {
      debug_tftp(debug_putc, "TFTPS socket timeout\r\n");
      _TFTPSReset();
   }
   
  #if defined(__DO_DEBUG_TFTP)
   static unsigned int8 old = -1;
   if (_g_TFTPS.state != old)
   {
      debug_tftp(debug_putc, "%LX TFTPSTask %X->%X\r\n", TickGet(), old, _g_TFTPS.state);
      old = _g_TFTPS.state;
   }
  #endif
  
   switch(_g_TFTPS.state)
   {
      default:
      case _TFTPS_SM_STOPPED:
         break;
         
      case _TFTPS_SM_INIT:
         _g_TFTPS.socket = UDPOpen(TFTP_PORT, NULL, 0);
         if (_g_TFTPS.socket != INVALID_UDP_SOCKET)
         {
            debug_tftp(debug_putc, "TFTPS socket opened, now listening\r\n");
            
            _g_TFTPS.state = _TFTPS_SM_LISTENING;
            
            _g_TFTPS.filename = -1;
         }
         else
         {
            TFTPS_NOTIFY_NO_SOCKETS(TFTP_PORT);
         }
         break;
         
      case _TFTPS_SM_LISTENING:
      case _TFTPS_SM_GET_CMD:
         if (UDPIsGetReady(_g_TFTPS.socket) > 2)
         {
            UDPGet(&scr.b[0]);
            UDPGet(&scr.b[0]);
            
            if (scr.b[0] == _TFTPS_OPCODE_WRITE_REQUEST)
            {
               _TFTPSGetFilename();
               
               TFTPS_CLIENT_CONNECTED();
               
               UDPDiscard();
               
               if (_g_TFTPS.filename == -1)
               {
                  debug_tftp(debug_putc, "TFTPS invalid filename\r\n");
                  
                  _g_TFTPS.state = _TFTPS_SM_PUT_ERROR_BAD_FILENAME;
               }
               else
               {
                  MPFS_SET_BLOCK();
                  _g_TFTPS.addy = _TFTPSFileAddy();
                  _g_TFTPS.state = _TFTPS_SM_PUT_ACK;
                  _g_TFTPS.tick = TickGet();
                  //_g_TFTPS.block = 0;   //this done by _TFTPReset()
                  
                  debug_tftp(debug_putc, "TFTPS write file%X addy%LX max%LX t%LX\r\n", _g_TFTPS.filename, _g_TFTPS.addy, _TFTPSFileMaxSize(), TickGet());
               }
            }
            else if 
            (
               (scr.b[0] == _TFTPS_OPCODE_DATA) &&
               (UDPIsGetReady(_g_TFTPS.socket) >= 2) &&
               (_g_TFTPS.filename != -1)
            )
            {
               UDPGet(&scr.b[1]); //big endian
               UDPGet(&scr.b[0]); //scr.w[0] now holds incoming block number
               
               if (scr.w[0] == _g_TFTPS.block)
               {
                  //repeat block, just ack it
                  debug_tftp(debug_putc, "%LX TFTPS acking repeat block %LX\r\n", TickGet(), scr.w[0]);
                  _g_TFTPS.state = _TFTPS_SM_PUT_ACK;
               }
               else if (scr.w[0] == (_g_TFTPS.block + (unsigned int16)1))
               {
                  _g_TFTPS.block = scr.w[0];
                  debug_tftp(debug_putc, "%LX TFTPS got block %LX\r\n", TickGet(), scr.w[0]);
                  scr.w[0] = 0;
                  scr.w[1] = 0;
                  while(UDPIsGetReady(_g_TFTPS.socket) > 0)
                  {
                     UDPGet(&TFTPS_SCRATCH_BUFFER[scr.w[0]]);
                     scr.w[0] += 1;
                     scr.w[1] += 1;
                     
                     if ((scr.w[0] >= TFTP_BUFFER_SIZE) || (UDPIsGetReady(_g_TFTPS.socket) == 0))
                     {
                        if ((_g_TFTPS.addy + scr.w[0]) <= (_TFTPSFileMaxSize() + _TFTPSFileAddy()))
                        {
                           debug_tftp(debug_putc, "%LX TFTPS write block%LX addy%LX len%LX\r\n", TickGet(), _g_TFTPS.block, _g_TFTPS.addy, scr.w[0]);
                           SPIFlashWriteBytes(_g_TFTPS.addy, TFTPS_SCRATCH_BUFFER, scr.w[0]);
                        }
                        
                        _g_TFTPS.addy += scr.w[0];
                        scr.w[0] = 0;
                     }
                  }
                  if (scr.w[1] != 512)
                  {
                     debug_tftp(debug_putc, "%LX TFTPS done writing len=0x%LX\r\n", TickGet(), _g_TFTPS.addy - _TFTPSFileAddy());
                     
                     TFTPS_WRITE_FILE_SIZE(_g_TFTPS.filename, _g_TFTPS.addy - _TFTPSFileAddy());
                     
                     TFTPS_FINISHED_FILE(_g_TFTPS.filename);
                  }
                  
                  _g_TFTPS.state = _TFTPS_SM_PUT_ACK;
               }
               else
               {
                  _g_TFTPS.state = _TFTPS_SM_PUT_ERROR_BAD_CMD;
                  debug_tftp(debug_putc, "TFTPS block sequence error %X\r\n", scr.b[0]);
               }
            }
            else
            {
               _g_TFTPS.state = _TFTPS_SM_PUT_ERROR_BAD_CMD;
               
               debug_tftp(debug_putc, "%LX TFTPS unknown cmd%X fn%X rdy%LX\r\n", TickGet(), scr.b[0], _g_TFTPS.filename, UDPIsGetReady(_g_TFTPS.socket));
               
               UDPDiscard();
            }            
         }
         break;
      
      case _TFTPS_SM_PUT_ACK:
         if (UDPIsPutReady(_g_TFTPS.socket) >= 4)
         {
            UDPPut(0);
            UDPPut(_TFTPS_OPCODE_ACK);
            UDPPut(make8(_g_TFTPS.block,1));
            UDPPut(make8(_g_TFTPS.block,0));
            UDPFlush();
            debug_tftp(debug_putc, "%LX TFTPS Write ACK %LX\r\n", TickGet(), _g_TFTPS.block);

            _g_TFTPS.state = _TFTPS_SM_GET_CMD;
            _g_TFTPS.tick = TickGet();
         }
         break;

      case _TFTPS_SM_PUT_ERROR_BAD_CMD:
      case _TFTPS_SM_PUT_ERROR_BAD_FILENAME:
         if(UDPIsPutReady(_g_TFTPS.socket) >= 30)
         {
            UDPPut(0);
            UDPPut(_TFTPS_OPCODE_ERROR);
            UDPPut(0);
            
            if (_g_TFTPS.state == _TFTPS_SM_PUT_ERROR_BAD_FILENAME)
            {
               UDPPut(0);
               printf(UDPPut, "Generic Error");
            }
            else
            {
               UDPPut(4);
               printf(UDPPut, "Bad Command");
            }
            
            UDPPut(0);
            UDPFlush();
            
            debug_tftp(debug_putc, "TFTPS SENT ERROR %X\r\n", _g_TFTPS.state);
            
            _TFTPSReset();
         }
         break;        
   }
}

#endif   //once
