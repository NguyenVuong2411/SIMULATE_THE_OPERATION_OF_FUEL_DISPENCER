/*
   See ccs_ftp_mdd.h for more documentation.
*/
/*********************************************************************
 *
 *             FTP ServerModule for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        FTP.c
 * Dependencies:    StackTsk.h
 *                  TCP.h
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F
 * Complier:        Microchip C18 v3.02 or higher
 *               Microchip C30 v2.01 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * This software is owned by Microchip Technology Inc. ("Microchip") 
 * and is supplied to you for use exclusively as described in the 
 * associated software agreement.  This software is protected by 
 * software and other intellectual property laws.  Any use in 
 * violation of the software license may subject the user to criminal 
 * sanctions as well as civil liability.  Copyright 2006 Microchip
 * Technology Inc.  All rights reserved.
 *
 * This software is provided "AS IS."  MICROCHIP DISCLAIMS ALL 
 * WARRANTIES, EXPRESS, IMPLIED, STATUTORY OR OTHERWISE, NOT LIMITED 
 * TO MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND 
 * INFRINGEMENT.  Microchip shall in no event be liable for special, 
 * incidental, or consequential damages.
 *
 *
 * Author               Date    Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Nilesh Rajbharti     4/23/01  Original        (Rev 1.0)
 * Nilesh Rajbharti     11/13/02 Fixed FTPServer()
 * Howard Schlunder     07/10/06 Added hash printing to FTP client
 * Howard Schlunder     07/20/06 Added FTP_RESP_DATA_NO_SOCKET error message
 * Nick LaBonte         02/14/07 CCS Port with FAT support
 * Nick LaBonte         02/22/07 MPFS support re-implemented; cleanup
  ********************************************************************/
#define THIS_IS_FTP

#include <string.h>
#include <stdlib.h>
//#include "tcpip/ftp.h"

#ifndef debug_ftp
#define debug_ftp(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)
#else
#define __DEBUG_FTP_ENABLED__
#endif

#ifndef debug_fat
#define debug_fat(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o)
#endif

#ifndef FTP_MAX_USERNAME_LEN
#define FTP_MAX_USERNAME_LEN  32
#endif

/*
#ifndef FTP_FORCE_DISCONNECT
#define FTP_FORCE_DISCONNECT()   (FALSE)
#endif
*/

#ifndef FTP_MARK_ERROR
#define FTP_MARK_ERROR()
#endif

#ifndef FTP_CLEAR_ERROR
#define FTP_CLEAR_ERROR()
#endif

#ifndef FTP_MARK_USING_FILESYSTEM
#define FTP_MARK_USING_FILESYSTEM()
#endif

#ifndef FTP_FREE_USING_FILESYSTEM
#define FTP_FREE_USING_FILESYSTEM()
#endif

//#if defined(STACK_USE_MPFS)
#if 0
   #define FILE   MPFS
   #define fatclose(x)     MPFSPutEnd(); MPFSClose();
   #define fatputc(x,y)    MPFSPut(x)
#endif

//#if defined(STACK_USE_MPFS)
#if 0
   #define FTP_DELETE_ENABLED 0
#else
   #define FTP_DELETE_ENABLED 1
#endif

#ifndef FTP_PUT_ENABLED
#define FTP_PUT_ENABLED    0
#endif

#ifndef FTP_RETR_ENABLED
#define FTP_RETR_ENABLED   1
#endif

#ifndef FTP_LIST_ENABLED
#define FTP_LIST_ENABLED   1
#endif

#if !defined(STACK_USE_HTTP2) //ccs http2 server
   int1 FTPWriteMMC = 0;
#endif

#define FTP_COMMAND_PORT                (21)
#define FTP_DATA_PORT                   (20)
#define FTP_TIMEOUT                     ((TICK)180 * TICK_SECOND)
#define MAX_FTP_ARGS                    (7)
#define MAX_FTP_CMD_STRING_LEN          (100)

typedef enum _SM_FTP
{
    SM_FTP_NOT_CONNECTED,
    SM_FTP_CONNECTED,
    SM_FTP_USER_NAME,
    SM_FTP_USER_PASS,
    SM_FTP_RESPOND,
    SM_FTP_CONNECTED_RESPOND_WAIT
} SM_FTP;

typedef enum _SM_FTP_CMD
{
    SM_FTP_CMD_IDLE,
    SM_FTP_CMD_IDLE2,
    SM_FTP_CMD_WAIT,
    SM_FTP_CMD_SEND_RESPONSE,
    SM_FTP_CMD_RECEIVE,
    SM_FTP_CMD_RECEIVE_WAIT,
    SM_FTP_CMD_RECEIVE_DONE,
    SM_FTP_CMD_WAIT_FOR_DISCONNECT
} SM_FTP_CMD;

typedef enum _FTP_COMMAND
{
    FTP_CMD_USER,
    FTP_CMD_PASS,
    FTP_CMD_QUIT,
    FTP_CMD_QUIT2,    
#if FTP_PUT_ENABLED
    FTP_CMD_STOR,
#endif
    FTP_CMD_PORT,
    FTP_CMD_ABORT,
#if FTP_DELETE_ENABLED
    FTP_CMD_DELE,
#endif
#if FTP_RETR_ENABLED
    FTP_CMD_RETR,
#endif
#if FTP_LIST_ENABLED
    FTP_CMD_LIST,
    FTP_CMD_PWD,
    FTP_CMD_TYPE,
#endif
    FTP_CMD_PASV,
    FTP_CMD_SYST,
    FTP_CMD_SIZE,
    FTP_CMD_CWD,
    FTP_CMD_UNKNOWN,
    FTP_CMD_NONE,
} FTP_COMMAND;

/*
char  FTP_CMD_STR_USER[] = "USER",
      FTP_CMD_STR_PASS[] = "PASS",
      FTP_CMD_STR_QUIT[] = "QUIT",
#if FTP_PUT_ENABLED
      FTP_CMD_STR_STOR[] = "STOR",
#endif
      FTP_CMD_STR_PORT[] = "PORT",
      FTP_CMD_STR_ABOR[] = "ABOR";
#if FTP_DELETE_ENABLED
char  FTP_CMD_STR_DELE[] = "DELE";
#endif
#if FTP_RETR_ENABLED
char  FTP_CMD_STR_RETR[] = "RETR";
#endif
*/

#define FTP_CMD_STR_USER "USER"
#define FTP_CMD_STR_PASS "PASS"
#define FTP_CMD_STR_QUIT "QUIT"
#if FTP_PUT_ENABLED
  #define FTP_CMD_STR_STOR "STOR"
#endif
#define FTP_CMD_STR_PORT "PORT"
#define FTP_CMD_STR_ABOR "ABOR"
#if FTP_DELETE_ENABLED
 #define FTP_CMD_STR_DELE "DELE"
#endif
#if FTP_RETR_ENABLED
 #define FTP_CMD_STR_RETR "RETR"
#endif
#if FTP_LIST_ENABLED
 #define FTP_CMD_STR_LIST  "LIST"
 #define FTP_CMD_STR_PWD   "PWD"
 #define FTP_CMD_STR_TYPE   "TYPE"
#endif


typedef enum _FTP_RESPONSE
{
   FTP_RESP_BANNER = 0,
   FTP_RESP_USER_OK,
   FTP_RESP_PASS_OK,
   FTP_RESP_QUIT_OK,
   FTP_RESP_STOR_OK,
   FTP_RESP_UNKNOWN,
   FTP_RESP_LOGIN,
   FTP_RESP_DATA_OPEN,
   FTP_RESP_DATA_READY,
   FTP_RESP_BAD_FILE,
   FTP_RESP_DATA_CLOSE,
   FTP_RESP_DATA_NO_SOCKET,
   FTP_RESP_OK,
   FTP_RESP_TEST_RUNNING,
  #if FTP_LIST_ENABLED
   FTP_RESP_SYSTEM_ERROR,
   FTP_RESP_PWD,
  #endif

   // added by CCS
   FTP_RESP_SYST,

   FTP_RESP_NONE,
   
   // these were added by CCS
   FTP_RESP_SIZE,
   FTP_RESP_PASV
                                       // This must always be the last
                                       // There is no corresponding string.
} FTP_RESPONSE;

#if FTP_LIST_ENABLED
 #define FTP_RESPONSES_COUNT  17
#else
 #define FTP_RESPONSES_COUNT  15
#endif

ROM char FTP_RESPONSES[FTP_RESPONSES_COUNT][40]=
{
   "220 Ready\r\n",
   "331 Password required\r\n",
   "230 Logged in\r\n",
   "221 Bye\r\n",
   "500 \r\n",
   "502 Not Implemented\r\n",
   "530 Login Required\r\n",
   "150 Transferring data...\r\n",
   "125 Done\r\n",
   "550 Error: Can't Open File\r\n",
   "\r\n226 Transfer Complete\r\n",
   "425 Can't Open Connection\r\n",
   "200 OK\r\n",
   "550 System Busy, Test Running\r\n",
  #if FTP_LIST_ENABLED
   "550 System Error\r\n",
   "257 \"/\" is the current directory\r\n",
  #endif
   "215 UNIX Type: L8\r\n"
};

/*
const char FTP_RESP_BANNER_STR[]="220 Ready\r\n",
      FTP_RESP_USER_OK_STR[]="331 Password required\r\n",
      FTP_RESP_PASS_OK_STR[]="230 Logged in\r\n",
      FTP_RESP_QUIT_OK_STR[]="221 Bye\r\n",
      FTP_RESP_STOR_OK_STR[]="500 \r\n",
      FTP_RESP_UNKNOWN_STR[]="502 Not Implemented\r\n",
      FTP_RESP_LOGIN_STR[]="530 Login Required\r\n",
      FTP_RESP_DATA_OPEN_STR[]="150 Transferring data...\r\n",
      FTP_RESP_DATA_READY_STR[]="125 Done\r\n",
#if FTP_DELETE_ENABLED
      FTP_RESP_BAD_FILE_STR[]="550 Error: Can't Open File\r\n",
#endif
      FTP_RESP_DATA_CLOSE_STR[]="\r\n226 Transfer Complete\r\n",
      FTP_RESP_DATA_NO_SOCKET_STR[]="425 Can't Open Connection\r\n",
      FTP_RESP_OK_STR[]="200 OK\r\n";
*/

union
{
    struct
    {
        unsigned char bUserSupplied : 1;
        unsigned char bLoggedIn: 1;
    } Bits;
    BYTE Val;
} FTPFlags;


TCP_SOCKET       FTPSocket = INVALID_SOCKET;      // Main ftp command socket.
TCP_SOCKET       FTPDataSocket = INVALID_SOCKET;  // ftp data socket.
WORD_VAL         FTPDataPort;    // ftp data port number as supplied by client

SM_FTP           smFTP;          // ftp server FSM state
SM_FTP_CMD       smFTPCommand;   // ftp command FSM state

FTP_COMMAND      FTPCommand;
FTP_RESPONSE     FTPResponse;

char             FTPUser[FTP_MAX_USERNAME_LEN+1];
char             FTPString[MAX_FTP_CMD_STRING_LEN+2];
BYTE             FTPStringLen;
char             *FTP_argv[MAX_FTP_ARGS];    // Parameters for a ftp command
BYTE             FTP_argc;       // Total number of params for a ftp command
TICK             lastActivity;   // Timeout keeper.
int32            FTPaddy;

struct
{
   int1 enabled;  //set TRUE when PASV command is heard, FALSE if PORT command is heard;
   int1 inUse; //set to TRUE when the passive socket is in use with a pervious GET, LIST, or PUT command.
} g_FtpPasv;

/*
#if STACK_HW_VON_LOGGER
int1 g_FTPFATIsBlocked = FALSE;

void FTPFATSetBlock(int1 setToBlock)
{
   g_FTPFATIsBlocked = setToBlock;
   FATSetBlock(setToBlock);
}

int1 FTPFATIsBlocked(void)
{
   return(g_FTPFATIsBlocked);
}
#endif
*/

FSFILE           *fstream;
//#elif defined(STACK_USE_MPFS)

// Private helper functions.
void ParseFTPString(void);
FTP_COMMAND ParseFTPCommand(char *cmd);
void ParseFTPString(void);
BOOL ExecuteFTPCommand(FTP_COMMAND cmd);
BOOL PutFile(void);
#if FTP_LIST_ENABLED
BOOL ListDir(void);
#endif
static BOOL GetFile(void);
BOOL Quit(void);

void _FTPCloseDataSocket(void)
{
  if(FTPDataSocket != INVALID_SOCKET)
  {
     TCPClose(FTPDataSocket);
     TCPDisconnect(FTPDataSocket);
     FTPDataSocket = INVALID_SOCKET;
  }
  g_FtpPasv.inUse = FALSE;
}

void FTPPutChar(char c)
{
   TCPPut(FTPSocket, c);
}


/*********************************************************************
 * Function:        void FTPInit(void)
 *
 * PreCondition:    TCP module is already initialized.
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Initializes internal variables of FTP
 *
 * Note:
 ********************************************************************/
void FTPInit(void)
{
    FTPSocket       = TCPOpen(0, TCP_OPEN_SERVER, FTP_COMMAND_PORT, TCP_PURPOSE_FTP_COMMAND);
    smFTP           = SM_FTP_NOT_CONNECTED;
    FTPStringLen    = 0;
    FTPFlags.Val    = 0;
    FTPDataPort.Val = FTP_DATA_PORT;
    FTPString[0]    = 0;
    FTPDataSocket   = INVALID_SOCKET;
    debug_ftp(debug_putc, "FTP INIT\r\n");
}


/*********************************************************************
 * Function:        void FTPTask(void)
 *
 * PreCondition:    FTPInit() must already be called.
 *
 * Input:           None
 *
 * Output:          Opened FTP connections are served.
 *
 * Side Effects:    None
 *
 * Overview:
 *
 * Note:            This function acts as a task (similar to one in
 *                  RTOS).  This function performs its task in
 *                  co-operative manner.  Main application must call
 *                  this function repeatdly to ensure all open
 *                  or new connections are served on time.
 ********************************************************************/
BOOL FTPTask(void)
{
    BYTE v;
    TICK currentTick;
    static TICK lastTick;
    //static unsigned int8 cmdLen;
    //char pMsg[30];
    unsigned int8 i;
   #if defined(__DEBUG_FTP_ENABLED__)
    static unsigned int8 debug;
    static unsigned int8 debugCmd = -1;
   #endif
   int1 lbContinue = TRUE;
   
   while(lbContinue)
   {
      lbContinue = FALSE;
   
    if ( !TCPIsConnected(FTPSocket) )
    {
        if (smFTP != SM_FTP_NOT_CONNECTED)
        {
            debug_ftp(debug_putc, "FTP DISCONNECT\r\n");
            if (
                  (
                    #if FTP_RETR_ENABLED
                     (FTPCommand == FTP_CMD_RETR) || 
                    #endif
                    #if FTP_PUT_ENABLED
                     (FTPCommand == FTP_CMD_STOR)
                    #else
                     0
                    #endif
                  ) &&
                  (FTPDataSocket != INVALID_SOCKET)
               )
            {
               FSfclose(fstream);
               FTP_FREE_USING_FILESYSTEM();
            }
        }
        FTPStringLen    = 0;
        FTPCommand      = FTP_CMD_NONE;
        smFTP           = SM_FTP_NOT_CONNECTED;
        FTPFlags.Val    = 0;
        smFTPCommand    = SM_FTP_CMD_IDLE;
        FTPFlags.Bits.bLoggedIn = FALSE;
        FTPFlags.Bits.bUserSupplied = FALSE;
        _FTPCloseDataSocket();
        
        return TRUE;
    }

    if (TCPIsGetReady(FTPSocket))
    {
        lastActivity    = TickGet();

        while( TCPGet(FTPSocket, &v ) )
        {
            FTPString[FTPStringLen++]   = v;
            if ( FTPStringLen == MAX_FTP_CMD_STRING_LEN )
                FTPStringLen            = 0;
        }
        TCPDiscard(FTPSocket);


        if ( v == '\n' )
        {
            FTPString[FTPStringLen]     = '\0';
            FTPStringLen                = 0;
            ParseFTPString();
            FTPCommand                  = ParseFTPCommand(FTP_argv[0]);
            debug_ftp(debug_putc, "FTP COMMAND '%s' %U\r\n", FTP_argv[0], FTPCommand);
        }
    }
    else if ( smFTP != SM_FTP_NOT_CONNECTED )
    {
        currentTick = TickGet();
        currentTick = TickGetDiff(currentTick, lastActivity);
        if ( currentTick >= FTP_TIMEOUT )
        {
            lastActivity                = TickGet();
            smFTPCommand = SM_FTP_CMD_IDLE;
            FTPCommand                  = FTP_CMD_QUIT;
            smFTP                       = SM_FTP_CONNECTED;
            debug_ftp(debug_putc, "FTP TIMEOUT\r\n");
        }
       #if defined(FTP_FORCE_DISCONNECT)
        else if (FTP_FORCE_DISCONNECT())
        {
            lastActivity                = TickGet();
            smFTPCommand = SM_FTP_CMD_IDLE;
            FTPCommand                  = FTP_CMD_QUIT2;
            smFTP                       = SM_FTP_CONNECTED;
        }
       #endif
    }

    switch(smFTP)
    {
    case SM_FTP_NOT_CONNECTED:
        FTPResponse = FTP_RESP_BANNER;
        lastActivity = TickGet();
        smFTP = SM_FTP_RESPOND;
        break;
        /* No break - Continue... */ //darren doesn't agree with this statement

    case SM_FTP_RESPOND:
SM_FTP_RESPOND_Label:
        if(!TCPIsPutReady(FTPSocket))
      {
         return TRUE;
      }
      else
      {
         i=0;
         if ((FTPResponse == FTP_RESP_PASV) && (FTPDataSocket != INVALID_SOCKET) && !g_FtpPasv.inUse)
         {
            _FTPCloseDataSocket();
         }
         if ((FTPResponse == FTP_RESP_PASV) && (FTPDataSocket != INVALID_SOCKET))
         {
            FTPResponse = FTP_RESP_TEST_RUNNING;
         }
         else if ((FTPResponse == FTP_RESP_PASV) && (FTPDataSocket == INVALID_SOCKET))
         {
            g_FtpPasv.inUse = FALSE;
            FTPDataSocket = TCPOpen(NULL, TCP_OPEN_SERVER, NextPort, TCP_PURPOSE_FTP_DATA);
            if (FTPDataSocket == INVALID_SOCKET)
            {
               FTPResponse = FTP_RESP_DATA_NO_SOCKET;
            }
            else
            {               
               g_FtpPasv.enabled = TRUE;
            }
         }
         if (FTPResponse == FTP_RESP_PASV)
         {
            sprintf(FTPString, "227 Entering passive mode (%u,%u,%u,%u,%u,%u).\r\n",
                  MY_IP_BYTE1,
                  MY_IP_BYTE2,
                  MY_IP_BYTE3,
                  MY_IP_BYTE4,
                  make8(NextPort, 1), 
                  make8(NextPort, 0)
               );
            printf(FTPPutChar, "%s", FTPString);
            NextPort++;
            debug_ftp(debug_putc, "FTP RESPONSE_PASV %s", FTPString);
         }
         else if (FTPResponse == FTP_RESP_SIZE)
         {
            FSFILE *f;
            char *p;
            p = FTP_argv[1];
            if (*p == '/') p++;
            FTP_MARK_USING_FILESYSTEM();
            f = FSfopen(p, FS_READ);
            if (f)
            {
              #if 1
               sprintf(FTPString, "213 %lu\r\n", f->size);
              #else
               FSfseek(f, 0, SEEK_END);
               sprintf(FTPString, "213 %lu\r\n", FSftell(f));
              #endif
               FSfclose(f);
            }
            else
            {
               sprintf(FTPString, "550 No such file or directory\r\n");
            }
            FTP_FREE_USING_FILESYSTEM();
            printf(FTPPutChar, "%s", FTPString);
            debug_ftp(debug_putc, "FTP RESPONSE_SIZE %s", FTPString);
         }
         else
         {
            if (FTPResponse >= FTP_RESP_NONE)
               FTPResponse = FTP_RESP_UNKNOWN;
            debug_ftp(debug_putc, "FTP RESPONSE %U: ", i);
            //cmdLen = 0;
            
            //while ((v=FTP_RESPONSES[FTPResponse][i++])!=0)   //ccs removed
            for(;;)   //ccs added
            {
               v = FTP_RESPONSES[FTPResponse][i++];  //ccs added this and next line
               if (!v)  break;

               //cmdLen++;
               FTPPutChar(v);
               debug_ftp(debug_putc, "%c", v);
            }
         }
         lastTick = TickGet();
         TCPFlush(FTPSocket);
         debug_ftp(debug_putc, "\r\n");
         //FTPResponse = FTP_RESP_NONE;
         smFTP = SM_FTP_CONNECTED_RESPOND_WAIT;
      }
        // No break - this will speed up little bit

    case SM_FTP_CONNECTED_RESPOND_WAIT:
      /* old revere seq hack
      currentTick = TickGet();
      if (TCPGotACK(FTPSocket))
      {
         lbContinue = TRUE;
         smFTP = SM_FTP_CONNECTED;
         FTPResponse = FTP_RESP_NONE;
      }
      else if (TickGetDiff(currentTick, lastTick) >= (TCP_START_TIMEOUT_VAL * 2))
      {
         TCPReverseSEQ(FTPSocket, cmdLen);
         smFTP = SM_FTP_RESPOND;
         lbContinue = TRUE;
      }
      */
      /* new stack that handles retries better */
      smFTP = SM_FTP_CONNECTED;
      FTPResponse = FTP_RESP_NONE;
      lbContinue = TRUE;
      break;
      
    case SM_FTP_CONNECTED:
        TCPTouch(FTPSocket);
        if ( FTPCommand != FTP_CMD_NONE )
        {
          #if defined(__DEBUG_FTP_ENABLED__)
            if (FTPCommand != debugCmd)
            {
               debugCmd = FTPCommand;
               debug_ftp(debug_putc, "FTP EXECUTE CMD %U\r\n", FTPCommand);
            }
          #endif
            if ( ExecuteFTPCommand(FTPCommand) )
            {
                debug_ftp(debug_putc, "FTP EXECUTE_FINISH ");
                if ( FTPResponse != FTP_RESP_NONE )
                {
                    smFTP = SM_FTP_RESPOND;
                    debug_ftp(debug_putc, "RESPOND ");
                }
                else if ( FTPCommand == FTP_CMD_QUIT )
                {
                  debug_ftp(debug_putc, "QUIT ");
                    smFTP = SM_FTP_NOT_CONNECTED;
                }

                debug_ftp(debug_putc, "\r\n");
                FTPCommand = FTP_CMD_NONE;
                smFTPCommand = SM_FTP_CMD_IDLE;
            }
            else if ( FTPResponse != FTP_RESP_NONE )
            {
                debug_ftp(debug_putc, "FTP EXECUTE_RESPOND %U\r\n", FTPResponse);
                smFTP = SM_FTP_RESPOND;
                goto SM_FTP_RESPOND_Label;
            }
        }
        break;
    }

   }
   
   #if defined(__DEBUG_FTP_ENABLED__)
    if (debug != smFTP)
    {
      debug_ftp(debug_putc, "FTPTask %U->%U\r\n", debug, smFTP);
      debug = smFTP;
    }
   #endif

    return TRUE;
}

static BOOL ExecuteFTPCommand(FTP_COMMAND cmd)
{
   unsigned int8 ret = TRUE;
   
   //FSMountCheck();
   
    switch(cmd)
    {
    case FTP_CMD_USER:
        FTPFlags.Bits.bUserSupplied = TRUE;
        FTPFlags.Bits.bLoggedIn = FALSE;
        FTPResponse = FTP_RESP_USER_OK;
        strncpy(FTPUser, FTP_argv[1], sizeof(FTPUser));
        break;

    case FTP_CMD_PASS:
        if ( !FTPFlags.Bits.bUserSupplied )
            FTPResponse = FTP_RESP_LOGIN;
        else
        {
            if ( FTPVerify(FTPUser, FTP_argv[1]) )
            {
                FTPFlags.Bits.bLoggedIn = TRUE;
                FTPResponse = FTP_RESP_PASS_OK;
            }
            else
                FTPResponse = FTP_RESP_LOGIN;
        }
        break;

    case FTP_CMD_QUIT:
    case FTP_CMD_QUIT2:    
        ret = Quit();
        break;

    case FTP_CMD_PORT:
        g_FtpPasv.enabled = FALSE;
        FTPDataPort.v[1] = (BYTE)atoi(FTP_argv[5]);
        FTPDataPort.v[0] = (BYTE)atoi(FTP_argv[6]);
        FTPResponse = FTP_RESP_OK;
        break;

#if FTP_LIST_ENABLED
    case FTP_CMD_LIST:
         ret = ListDir();
         break;
    
    case FTP_CMD_PWD:
      FTPResponse = FTP_RESP_PWD;
      break;
    
    case FTP_CMD_PASV:
      FTPResponse = FTP_RESP_PASV;
      break;

    case FTP_CMD_SYST:
      FTPResponse = FTP_RESP_SYST;
      break;
      
    case FTP_CMD_SIZE:
      FTPResponse = FTP_RESP_SIZE;
      break;

    case FTP_CMD_CWD:
      FTPResponse = FTP_RESP_BAD_FILE;
      break;
      
    case FTP_CMD_TYPE:
      FTPResponse = FTP_RESP_OK;
      break;
#endif

#if FTP_PUT_ENABLED
    case FTP_CMD_STOR:
        ret = PutFile();
        break;
#endif

#if FTP_RETR_ENABLED
    case FTP_CMD_RETR:
        ret = GetFile();
        break;
#endif

#if FTP_DELETE_ENABLED
    case FTP_CMD_DELE:
        ret = DeleteFile();
        break;
#endif

    case FTP_CMD_ABORT:
        FTPResponse = FTP_RESP_OK;
        _FTPCloseDataSocket();
        break;

    default:
        FTPResponse = FTP_RESP_UNKNOWN;
        break;
    }
    
    return(ret);
}

static BOOL Quit(void)
{
   static TICK tickFtpQuit;
    switch(smFTPCommand)
    {
    case SM_FTP_CMD_IDLE:
#if FTP_PUT_ENABLED || FTP_RETR_ENABLED || FTP_LIST_ENABLED
        if ( smFTPCommand == SM_FTP_CMD_RECEIVE ){
           #if (FTP_PUT_ENABLED || FTP_RETR_ENABLED)
            FSfclose(fstream);
            FTP_FREE_USING_FILESYSTEM();
           #endif
            FTPWriteMMC=0;
        }
#endif

        if ( FTPDataSocket != INVALID_SOCKET )
        {
#if FTP_PUT_ENABLED || FTP_RETR_ENABLED
            FSfclose(fstream);
            FTP_FREE_USING_FILESYSTEM();
#endif
            FTPWriteMMC=0;
            _FTPCloseDataSocket();
            //FTPDataSocket = INVALID_SOCKET;
            smFTPCommand = SM_FTP_CMD_WAIT;
        }
        else
            goto Quit_Done;
        break;

    case SM_FTP_CMD_WAIT:
        if ( !TCPIsConnected(FTPDataSocket) )
        {
Quit_Done:
            FTPDataSocket = INVALID_SOCKET;
            if (FTPCommand == FTP_CMD_QUIT)
               FTPResponse = FTP_RESP_QUIT_OK;
            else
               FTPResponse = FTP_RESP_TEST_RUNNING;
            smFTPCommand = SM_FTP_CMD_WAIT_FOR_DISCONNECT;
            tickFtpQuit = TickGet();
        }
        break;

    case SM_FTP_CMD_WAIT_FOR_DISCONNECT:
        if (
               (TCPGetTxFIFOFull(FTPSocket) == 0) ||
               ((TickGet() - tickFtpQuit) >= ((8)*TICKS_PER_SECOND))
           )
        {
            if ( TCPIsConnected(FTPSocket) )
            {
               if (TCPGetTxFIFOFull(FTPSocket) != 0)
               {
                  //double call to forcefull close this
                  TCPDisconnect(FTPSocket);
               }
                TCPDisconnect(FTPSocket);
                FTPSocket = INVALID_SOCKET;
            }
        }
        break;

    }
    return FALSE;
}

#if 0
char g_FTPLastFilename[40];
#endif

#define  FAT_PUT_TIMEOUTS_MAX 3

static BOOL GetFile(void)  //PIC->PC
{
   int1 ret=FALSE;
   int ec;
   //char buffer[1500];
   //char buffer[150];
  #if defined(__DEBUG_FTP_ENABLED__)
   static unsigned int8 debug;
  #endif
   static unsigned int32 idx = 0;
   //static int8 putTimeouts;
   static unsigned int8 delays;
   static unsigned int8 done;
   static unsigned int16 txCount;
   static TICK lastTick;
   TICK currTick;
   int1 lbContinue = TRUE;
   
   while (lbContinue)
   {
   lbContinue = FALSE;
   
   if (TCPIsConnected(FTPDataSocket))
      TCPDiscard(FTPDataSocket);
   
   switch(smFTPCommand)
   {
      case SM_FTP_CMD_IDLE:
         if ( !FTPFlags.Bits.bLoggedIn )
         {
            FTPResponse     = FTP_RESP_LOGIN;
            ret = TRUE;
         }
         else
         {
            char *p;
            p = FTP_argv[1];
            if (*p == '/') p++;
            FTP_MARK_USING_FILESYSTEM();
            fstream = FSfopen(p, FS_READ);  //old g_FTPLastFilename
            if (!fstream)
            {
               FTP_FREE_USING_FILESYSTEM();
               FTPResponse = FTP_RESP_BAD_FILE;
               FTP_MARK_ERROR();
               ret = TRUE;
            }
            else
            {
               FTP_CLEAR_ERROR();
               lbContinue = TRUE;
               smFTPCommand = SM_FTP_CMD_IDLE2;
               delays = 0;
               idx = 0;
               //putTimeouts = 0;
            }
         }
         break;

      case SM_FTP_CMD_IDLE2:
               //fatclose(&fstream);
               //FTPResponse     = FTP_RESP_DATA_OPEN;
               if (!g_FtpPasv.enabled)
               {
                  FTPDataSocket   = TCPOpen((PTR_BASE)&TCPGetRemoteInfo(FTPSocket)->remote, TCP_OPEN_NODE_INFO, FTPDataPort.Val, TCP_PURPOSE_FTP_DATA);
               }
               else
               {
                  g_FtpPasv.inUse = TRUE;
               }
                  
               lastTick = TickGet();
   
               // Make sure that a valid socket was available and returned
               // If not, return with an error
               if(FTPDataSocket != INVALID_SOCKET)
               {
                  lbContinue = TRUE;
                  smFTPCommand = SM_FTP_CMD_WAIT;
               }
               else
               {
                  FSfclose(fstream);
                  FTP_FREE_USING_FILESYSTEM();
                  FTPResponse = FTP_RESP_DATA_NO_SOCKET;
                  ret = TRUE;
               }
        break;

      case SM_FTP_CMD_WAIT:
         currTick = TickGet();
         TCPTouch(FTPSocket);
         if (TCPIsConnected(FTPDataSocket))
         {
            smFTPCommand = SM_FTP_CMD_SEND_RESPONSE;
            FTPResponse     = FTP_RESP_DATA_OPEN;
            lbContinue = TRUE;
            delays = 0;
         }
         else if (TickGetDiff(currTick, lastTick) >= (TICKS_PER_SECOND * 10))
         {
            _FTPCloseDataSocket();
            if (delays < 3)
            {
               delays++;
               lbContinue = TRUE;
               smFTPCommand = SM_FTP_CMD_IDLE2;
            }
            else
            {
               FSfclose(fstream);
               FTP_FREE_USING_FILESYSTEM();
               FTPResponse = FTP_RESP_DATA_NO_SOCKET;
               ret = TRUE;               
            }
         }
         break;

      case SM_FTP_CMD_SEND_RESPONSE:
         TCPTouch(FTPDataSocket);
         //if (TCPIsPutReady(FTPSocket))    //wait until the response is sent before sending the file on the data port
         if (TCPGetTxFIFOFull(FTPSocket) == 0)  //wait until the response is sent before sending the file on the data port
         {
            done = FALSE;
            smFTPCommand = SM_FTP_CMD_RECEIVE;
            lbContinue = TRUE;
         }
         break;

      case SM_FTP_CMD_RECEIVE:
         if (TCPIsPutReady(FTPDataSocket))
         {
            unsigned int32 len;
            
            txCount = 0;
            
            /*
            if (delays)
            {
               delays--;
               break;
            }
            
            delays = MAX_SOCKETS + MAX_UDP_SOCKETS + 1;
            */
            
            lastActivity    = TickGet();
            
            len = TCPIsPutReady(FTPDataSocket);
           
           #if 1
            ec = 0;
           #else
            ec = FSfseek(fstream, idx, SEEK_SET);
           #endif

            len = (len < sizeof(FTPString)) ? len : sizeof(FTPString);  // get minimum
           
            if (!ec)
            {
               txCount = FSfread(FTPString, sizeof(char), len, fstream);
               if ((FSerror() != CE_GOOD) && (FSerror() != CE_EOF))
               {
                  ec = -1;
               }
               debug_ftp(debug_putc, "FTP READ FILE LEN %LX (ASK %LX, IDX %LX %X)\r\n", txCount, len, idx, ec);
            }
            
            if (ec)
            {
                  debug_ftp(debug_putc, "FTP GET FILE MAX TIMEOUTS\r\n");
                  FTP_MARK_ERROR();
                  FSfclose(fstream);
                  FTP_FREE_USING_FILESYSTEM();
                  _FTPCloseDataSocket();
                  FTPResponse = FTP_RESP_BAD_FILE;
                  ret = TRUE;
                  break;            
            }
            
            if (txCount)
            {
               idx += txCount;
               TCPPutArray(FTPDataSocket, FTPString, txCount);
               
               lbContinue = TRUE;
               
               smFTPCommand = SM_FTP_CMD_RECEIVE_WAIT;
               
               lastTick = TickGet();
            }
            else
            {
               lbContinue = TRUE;
               smFTPCommand = SM_FTP_CMD_RECEIVE_DONE;
               TCPFlush(FTPDataSocket);
            }            
                                          
            debug_ftp(debug_putc, "FTP GET FILE SENDING %LU BYTES\r\n", txCount);
            
            if (txCount != len)
            {
               done = TRUE;
              #if defined(__DEBUG_FTP_ENABLED__)
               debug_ftp(debug_putc, "FTP EOF\r\n");
              #endif        
               TCPFlush(FTPDataSocket);              
            }
         }
         break;
      
      case SM_FTP_CMD_RECEIVE_WAIT:
         /* manual retry hack for v3 stack
         currTick = TickGet();
         if (TCPGotACK(FTPDataSocket))
         {
            lbContinue = TRUE;
            smFTPCommand = SM_FTP_CMD_RECEIVE_DONE;
           #if defined(__DEBUG_FTP_ENABLED__)
            debug_ftp(debug_putc, "FTP GET ACK\r\n");
           #endif
         }
         else if (TickGetDiff(currTick, lastTick) >= (TCP_START_TIMEOUT_VAL*2))
         {
            TCPReverseSEQ(FTPDataSocket, txCount); //abort current transmission, and undo SEQ count
            idx -= txCount;
            smFTPCommand = SM_FTP_CMD_RECEIVE;
            lbContinue = TRUE;
           #if defined(__DEBUG_FTP_ENABLED__)
            debug_ftp(debug_putc, "FTP NO ACK, RETRY\r\n");
           #endif
         }
         */
         /* v5 stack has better retry algorithm */
         lbContinue = TRUE;
         smFTPCommand = SM_FTP_CMD_RECEIVE_DONE;
         break;
         
      case SM_FTP_CMD_RECEIVE_DONE:
            if (done)
            {
               //EOF, transfer is done
               FSfclose(fstream);
               FTP_FREE_USING_FILESYSTEM();
               smFTPCommand = SM_FTP_CMD_WAIT_FOR_DISCONNECT;
               lastTick = TickGet();
               lbContinue = TRUE;
               debug_ftp(debug_putc, "FTP GET FILE DONE SENDING\r\n");              
            }
            else
            {
              #if defined(__DEBUG_FTP_ENABLED__)
               debug_ftp(debug_putc, "FTP SEND NEXT PART OF FILE\r\n");
              #endif            
               lbContinue = TRUE;
               smFTPCommand = SM_FTP_CMD_RECEIVE;
            }
         break;
      
      case SM_FTP_CMD_WAIT_FOR_DISCONNECT:
         if 
         (
            (TCPGetTxFIFOFull(FTPDataSocket) == 0) ||
            (TickGetDiff(TickGet(), lastTick) >= (TICKS_PER_SECOND * 10))
         )
         {
            _FTPCloseDataSocket();
            FTPResponse     = FTP_RESP_DATA_CLOSE;
            ret = TRUE;
         }
         break;
   }
   
   }
   
  #if defined(__DEBUG_FTP_ENABLED__)
   if ((smFTPCommand != debug) || ret)
   {
      debug_ftp(debug_putc, "FTP GET FILE %U->%U RET=%U\r\n", debug, smFTPCommand, ret);
      debug = smFTPCommand;
   }
  #endif
   
   return(ret);
}

#if FTP_LIST_ENABLED

// DOS FAT date/time format
/*

               24                16                 8                 0
+-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+
|Y|Y|Y|Y|Y|Y|Y|M| |M|M|M|D|D|D|D|D| |h|h|h|h|h|m|m|m| |m|m|m|s|s|s|s|s|
+-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+
 \___________/\________/\_________/ \________/\____________/\_________/
    year        month       day      hour       minute        second

The year is stored as an offset from 1980. 
Seconds are stored in two-second increments. 
(So if the "second" value is 15, it actually represents 30 seconds.)
*/

#include <time.h>

static void FTPPrettyDate(unsigned int32 timestamp, char *ptr)
{
   struct_tm tm;
   
   memset(&tm, 0x00, sizeof(tm));
   
   // first, convert fat timestamp to a standard time.h struct_tm
   tm.tm_sec = (timestamp & 0x1F) * 2;
   tm.tm_min = ((timestamp >> 5) & 0x3f);
   tm.tm_hour = ((timestamp >> 11) & 0x1f);
   tm.tm_mday = ((timestamp >> 16) & 0x1f);
   tm.tm_mon = ((timestamp >> 21) & 0xf);
   tm.tm_year = (timestamp >> 25);
   tm.tm_year += 80; //FAT is years since 1980.  struct_tm is years since 1900
   
   if (tm.tm_mday == 0)
      tm.tm_mday = 1;
   
   switch(tm.tm_mon)
   {
      default:
      case 0:
      case 1:  sprintf(ptr, "Jan");   break;
      case 2:  sprintf(ptr, "Feb");   break;
      case 3:  sprintf(ptr, "Mar");   break;
      case 4:  sprintf(ptr, "Apr");   break;
      case 5:  sprintf(ptr, "May");   break;
      case 6:  sprintf(ptr, "Jun");   break;
      case 7:  sprintf(ptr, "Jul");   break;
      case 8:  sprintf(ptr, "Aug");   break;
      case 9:  sprintf(ptr, "Sep");   break;
      case 10:  sprintf(ptr, "Oct");   break;
      case 11:  sprintf(ptr, "Nov");   break;
      case 12:  sprintf(ptr, "Dec");   break;
   }
   
   ptr += 3;
   
   sprintf(ptr, " %U %LU",
         tm.tm_mday,
         tm.tm_year + (unsigned int16)1900
      );
}

static BOOL ListDir(void)  //PIC->PC
{
   //int8 ec;
   int ec;
   int1 ret=FALSE;
   //char buffer[1500];
   //char buffer[150];
   //static int8 debug;
   static unsigned int8 fileIdx, lastIncIdx;
   unsigned int8 i;
   //static int16 txCount; //only needs to be static for v3 stack code, which is commented out
   unsigned int16 txCount;
   static int1 doClose;
   TICK currTick;
   static TICK lastTick;
   int1 needFlush;
   
   if (TCPIsConnected(FTPDataSocket))
      TCPDiscard(FTPDataSocket);
   
   switch(smFTPCommand)
   {
      case SM_FTP_CMD_IDLE:
         if ( !FTPFlags.Bits.bLoggedIn )
         {
            FTPResponse     = FTP_RESP_LOGIN;
            ret = TRUE;
         }
         else
         {
            fileIdx = 0;

               //FTPResponse     = FTP_RESP_DATA_OPEN;
               
               if (!g_FtpPasv.enabled)
               {
                  FTPDataSocket = TCPOpen((PTR_BASE)&TCPGetRemoteInfo(FTPSocket)->remote, TCP_OPEN_NODE_INFO, FTPDataPort.Val, TCP_PURPOSE_FTP_DATA);
               }
               else
               {
                  g_FtpPasv.inUse = TRUE;
               }
   
               lastTick = TickGet();
               lastIncIdx = 0;
   
               // Make sure that a valid socket was available and returned
               // If not, return with an error
               if(FTPDataSocket != INVALID_SOCKET)
                  smFTPCommand = SM_FTP_CMD_WAIT;
               else
               {
                  FTPResponse = FTP_RESP_DATA_NO_SOCKET;
                  ret = TRUE;
               }
        }
        break;

      case SM_FTP_CMD_WAIT:
         currTick = TickGet();
         if (TCPIsConnected(FTPDataSocket))
         {
               smFTPCommand = SM_FTP_CMD_SEND_RESPONSE;
               FTPResponse     = FTP_RESP_DATA_OPEN;
         }
         else if (TickGetDiff(currTick, lastTick) >= (TICKS_PER_SECOND * 10))
         {
            _FTPCloseDataSocket();
            if (lastIncIdx > 3)
            {
               FTPResponse = FTP_RESP_DATA_NO_SOCKET;
               ret = TRUE;
            }
            else
            {
               smFTPCommand = SM_FTP_CMD_IDLE;
            }
         }
         break;

      case SM_FTP_CMD_SEND_RESPONSE:
         TCPTouch(FTPDataSocket);
         doClose = FALSE;
         if (TCPGetTxFIFOFull(FTPSocket) == 0) //wait until the response is sent before sending the file on the data port
            smFTPCommand = SM_FTP_CMD_RECEIVE;
         break;

      case SM_FTP_CMD_RECEIVE:
         if (TCPIsPutReady(FTPDataSocket) > sizeof(FTPString))
         {
            char *ptr;

            lastIncIdx = 0;
            
            lastActivity = TickGet();
           
            i = 0;

            needFlush = FALSE;

            //while (TCPIsPutReady(FTPDataSocket) >= sizeof(g_FtpBuf))
            for(;;)
            {
               SearchRec s;
               //USB_WIZ_TIME t;
               //int1 pm=FALSE;
               
               ptr = FTPString;
               
               // skip files already read
               for (; i <= fileIdx; i++)
               {
                  if (i == 0)
                  {
                     ec = FindFirst("*.*", ATTR_MASK, &s);
                  }
                  else
                  {
                     ec = FindNext(&s);
                  }
                  if (ec)
                  {
                     break;
                  }
               }
               fileIdx++;

               if (ec && (FSerror() != CE_FILE_NOT_FOUND))
               {
                  _FTPCloseDataSocket();
                  FTP_MARK_ERROR();
                  FTPResponse = FTP_RESP_BAD_FILE;
                  ret = TRUE;
                  break;
               }
               else
               {
                  FTP_CLEAR_ERROR();
               }

               if (ec)
               {
                  doClose = TRUE;
                  break;
               }

               //unix style
               //drwxr-xr-x 1 user01 ftp 512 Jan 29 23:32 prog
               if ((s.attributes & ATTR_DIRECTORY) == ATTR_DIRECTORY)
                  *ptr++ = 'd';
               else
                  *ptr++ = '-';

               if ((s.attributes & ATTR_READ_ONLY) == ATTR_READ_ONLY)
                  sprintf(ptr, "r-xr-xr-x  ");
               else
                  sprintf(ptr, "rwxrwxrwx  ");
               ptr += 11;
              
               sprintf(ptr, "1 user user ");
               ptr += 12;
               
               sprintf(ptr, "%LU ", s.filesize);
               ptr += strlen(ptr);
               
               FTPPrettyDate(s.timestamp, ptr);
               ptr += strlen(ptr);
               
               *ptr = ' ';
               ptr++;
               strcpy(ptr, s.filename);
               ptr += strlen(ptr);
               
               sprintf(ptr, "\r\n");
               ptr += strlen(ptr);
              
               txCount = ptr - FTPString;
               
               lastIncIdx++;

debug_ftp(debug_putc, "FTP LIST '%s' len=%lu\r\n", FTPString, txCount);

               TCPPutArray(FTPDataSocket, FTPString, txCount);
               
               needFlush = TRUE;
               
               lastTick = TickGet();
               
               break;   //like this?
            }
            
            if (needFlush)
            {
               TCPFlush(FTPDataSocket);
            }
            
            if (doClose)
            {
               smFTPCommand = SM_FTP_CMD_RECEIVE_DONE;
            }
         }
         break;

      case SM_FTP_CMD_RECEIVE_WAIT:
         /* old v3 stack with bad retry algorithm
         currTick = TickGet();
         if (TCPGotACK(FTPDataSocket))
         {
            smFTPCommand = SM_FTP_CMD_RECEIVE_DONE;
         }
         else if (TickGetDiff(currTick, lastTick) >= (TICKS_PER_SECOND*5))
         {
            TCPReverseSEQ(FTPDataSocket, txCount); //abort current transmission, and undo SEQ count
            fileIdx -= lastIncIdx;
            smFTPCommand = SM_FTP_CMD_RECEIVE;
         }
         */
         /* v5 stack with better retries */
         smFTPCommand = SM_FTP_CMD_RECEIVE_DONE;
         break;      
         
      case SM_FTP_CMD_RECEIVE_DONE:
         if (doClose)
         {
            //EOF, transfer is done
            smFTPCommand = SM_FTP_CMD_WAIT_FOR_DISCONNECT;
         }
         else
            smFTPCommand = SM_FTP_CMD_RECEIVE;
         break;

      case SM_FTP_CMD_WAIT_FOR_DISCONNECT:        
         txCount = TCPGetTxFIFOFull(FTPDataSocket); 
         if 
         (
            (txCount == 0) ||
            (TickGetDiff(TickGet(),lastTick) > (TICKS_PER_SECOND*8))
         )
         {
            _FTPCloseDataSocket();
            FTPResponse     = FTP_RESP_DATA_CLOSE;
            ret = TRUE;
         }
         break;
   }
   
   /*
   if ((smFTPCommand != debug) || ret)
   {
      //debug_ftp(debug_putc, "FTP LIST DIR %U->%U RET=%U\r\n", debug, smFTPCommand, ret);
      printf(_RS232_putc, "FTP LIST DIR %U->%U RET=%U\r\n", debug, smFTPCommand, ret);
      debug = smFTPCommand;
   }
   */
   
   return(ret);
}
#endif

static BOOL PutFile(void)
{
    BYTE v;
    //char buf[FTP_BUFFER_SIZE];
    unsigned int8 i;
//FTP_argv[1] holds filename
//char filename[20];
//sprintf(filename, "/%s", FTP_argv[1]);

    switch(smFTPCommand)
    {
      case SM_FTP_CMD_IDLE:
        if ( !FTPFlags.Bits.bLoggedIn )
        {
            FTPResponse     = FTP_RESP_LOGIN;
            return TRUE;
        }
        else
        {
            FTPResponse     = FTP_RESP_DATA_OPEN;
            if (!g_FtpPasv.enabled)
            {
               FTPDataSocket = TCPOpen((PTR_BASE)&TCPGetRemoteInfo(FTPSocket)->remote, TCP_OPEN_NODE_INFO, FTPDataPort.Val, TCP_PURPOSE_FTP_DATA);
            }
            else
            {
               g_FtpPasv.inUse = TRUE;
            }

         // Make sure that a valid socket was available and returned
         // If not, return with an error
         if(FTPDataSocket != INVALID_SOCKET)
         {
               smFTPCommand = SM_FTP_CMD_WAIT;
         }
         else
         {
               FTPResponse = FTP_RESP_DATA_NO_SOCKET;
               return TRUE;
         }
        }
        break;

      case SM_FTP_CMD_WAIT:
         if ( TCPIsConnected(FTPDataSocket) )
        {
         lastActivity    = TickGet();
#if FTP_PUT_ENABLED
         fstream = FSfopen(FTP_argv[1], FS_WRITE);
         #error handle LoggingMarkFileBusy(); for this
#endif
         FTPWriteMMC=1;
         smFTPCommand = SM_FTP_CMD_RECEIVE;
        }
        break;

    case SM_FTP_CMD_RECEIVE:
        if ( TCPIsGetReady(FTPDataSocket) )
        {
            // Reload timeout timer.
            lastActivity    = TickGet();
            i=0;
            while( TCPGet(FTPDataSocket, &v) )
            {
               FTPString[i]=v;
               ++i;
               if(i == sizeof(FTPString))
               {
                  FSfwrite(FTPString, sizeof(char), sizeof(FTPString), fstream);
                  FTPaddy+=sizeof(FTPString);
                  i=0;
               }
            }
            if(i)
            {
               FSfwrite(FTPString, sizeof(char), i, fstream);
               FTPaddy+=i;
            }               
            TCPDiscard(FTPDataSocket);
            FTPWriteMMC=1;
        }
        else if ( !TCPIsConnected(FTPDataSocket) )
        {
#if FTP_PUT_ENABLED
            FSfclose(fstream);
            FTP_FREE_USING_FILESYSTEM();
#endif
            FTPWriteMMC=0;
            _FTPCloseDataSocket();
            FTPResponse     = FTP_RESP_DATA_CLOSE;
            return TRUE;
        }
    }
    return FALSE;
}

#if FTP_DELETE_ENABLED

//////FUNCTION/////
BOOL DeleteFile(void)
{
//FTP_argv[1] holds filename
//char filename[20];
//sprintf(filename, "/%s", FTP_argv[1]);
    if(smFTPCommand==SM_FTP_CMD_IDLE)
    {
        if ( !FTPFlags.Bits.bLoggedIn )
        {
            FTPResponse     = FTP_RESP_LOGIN;
            return TRUE;
        }
        else
        {
            FTP_MARK_USING_FILESYSTEM();
            fstream = FSfopen(FTP_argv[1], FS_READ);
            if(fstream == NULL)
            {
               FTP_FREE_USING_FILESYSTEM();
               FTP_MARK_ERROR();
               FTPResponse     = FTP_RESP_BAD_FILE;
               return TRUE;
            }else
            {
               FTP_CLEAR_ERROR();
               FSfclose(fstream);
               FTP_FREE_USING_FILESYSTEM();
               //if (read_eeprom8(EE_LOC_AUTO_DELETE))
               {
                  FSremove(FTP_argv[1]);
               }
               FTPWriteMMC=1;
               FTPResponse     = FTP_RESP_OK;
               return TRUE;
            }
        }
    }
    return FALSE;
}

#endif
//*****FUNCTION*******//
static FTP_COMMAND ParseFTPCommand(char *cmd)
{
    FTP_COMMAND returnval=0;
/*    0:FTP_CMD_STR_USER[] = "USER",
      1:FTP_CMD_STR_PASS[] = "PASS",
      2:FTP_CMD_STR_QUIT[] = "QUIT",
      3:FTP_CMD_STR_STOR[] = "STOR",
      4:FTP_CMD_STR_PORT[] = "PORT",
      5:FTP_CMD_STR_ABOR[] = "ABOR";

    for ( i = 0; i < (FTP_COMMAND)FTP_COMMAND_TABLE_SIZE; i++ )
    {
      if ( !memcmppgm2ram((void*)cmd, (ROM void*)FTPCommandString[i], 4) )
      if(strcmp(cmd, FTPCommandString[i])==0)
         return i;
    }
*/
   debug_fat(debug_putc, "ParseFTPCommand('%s')\r\n", cmd);
   
   if(strcmp(cmd, FTP_CMD_STR_USER)==0)
      returnval=FTP_CMD_USER;
   else if(strcmp(cmd, FTP_CMD_STR_PASS)==0)
      returnval=FTP_CMD_PASS;
   else if(strcmp(cmd, FTP_CMD_STR_QUIT)==0)
      returnval=FTP_CMD_QUIT;
#if FTP_PUT_ENABLED
   else if(strcmp(cmd, FTP_CMD_STR_STOR)==0)
      returnval=FTP_CMD_STOR;
#endif
   else if(strcmp(cmd, FTP_CMD_STR_PORT)==0)
      returnval=FTP_CMD_PORT;
   else if(strcmp(cmd, FTP_CMD_STR_ABOR)==0)
      returnval=FTP_CMD_ABORT;
#if FTP_DELETE_ENABLED
   else if(strcmp(cmd, FTP_CMD_STR_DELE)==0)
      returnval=FTP_CMD_DELE;
#endif
#if FTP_RETR_ENABLED
   else if(strcmp(cmd, FTP_CMD_STR_RETR)==0)
      returnval=FTP_CMD_RETR;
#endif
#if FTP_LIST_ENABLED
   else if (strcmp(cmd, FTP_CMD_STR_LIST)==0)
      returnval=FTP_CMD_LIST;
   else if (strcmp(cmd, FTP_CMD_STR_PWD)==0)
      returnval=FTP_CMD_PWD;
   else if (strcmp(cmd, FTP_CMD_STR_TYPE)==0)
      returnval=FTP_CMD_TYPE;
#endif
   else if (strcmp(cmd, "PASV") == 0)
      returnval=FTP_CMD_PASV;
   else if (strcmp(cmd, "SYST") == 0)
      returnval=FTP_CMD_SYST;      
   else if (strcmp(cmd, "SIZE") == 0)
      returnval=FTP_CMD_SIZE;      
   else if (strcmp(cmd, "CWD") == 0)
      returnval=FTP_CMD_CWD;      
   else returnval=FTP_CMD_UNKNOWN;
  
    return returnval;
}


static void ParseFTPString(void)
{
    BYTE *p;
    BYTE v;
    enum { SM_FTP_PARSE_PARAM, SM_FTP_PARSE_SPACE } smParseFTP;

    smParseFTP  = SM_FTP_PARSE_PARAM;
    p           = (BYTE*)&FTPString[0];

    // Skip white blanks
    while( *p == ' ' )
        p++;

    FTP_argv[0]  = (char*)p;
    FTP_argc     = 1;

    //while( (v = *p) ) //ccs removed
    for(;;)
    {
        v = *p;   //ccs added this and next line
        if (!v)   break;

        switch(smParseFTP)
        {
        case SM_FTP_PARSE_PARAM:
            if ( v == ' ' || v == ',' )
            {
                *p = '\0';
                smParseFTP = SM_FTP_PARSE_SPACE;
            }
            else if ( v == '\r' || v == '\n' )
                *p = '\0';
            break;

        case SM_FTP_PARSE_SPACE:
            if ( v != ' ' )
            {
                FTP_argv[FTP_argc++] = (char*)p;
                smParseFTP = SM_FTP_PARSE_PARAM;
            }
            break;
        }
        p++;
    }
}

int1 FTPServerIdle(void)
{
   return(smFTP == SM_FTP_NOT_CONNECTED);
}
