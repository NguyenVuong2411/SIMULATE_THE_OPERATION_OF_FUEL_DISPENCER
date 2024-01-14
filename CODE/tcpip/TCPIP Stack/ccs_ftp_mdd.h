/*
   Original code was based off of Microchip's FTP server, but theirs was
   meant for uploading an MPFS image.
   
   This version has been modified by CCS to read/write files from an MDD
   filesystem.
   
   TCP data socket (TCP_PURPOSE_FTP_DATA) needs at least 128 TX and 128 RX 
      bytes allocated (see MAX_FTP_CMD_STRING_LEN)
   
   TCP command socket (TCP_PURPOSE_FTP_COMMAND) needs at least 128 TX and
      128 RX bytes allocated (see MAX_FTP_CMD_STRING_LEN)
    
   Supports active or passive connections

  
   LIMITATIONS
   ---------------------------------------------------------------------------
   
   This FTP server only supports 1 simultaneous connection.  Some FTP clients
   will try to use multiple connections for a data transfer.  If your FTP
   client has a configuration for simultenous connections, turn it off or
   set it to 1.  (FileZilla by default will attempt to use multiple
   connections, so for FileZilla you will have to make the change).
   
   
   CALLBACKS/CONFIG MACROS
   ---------------------------------------------------------------------------
   
   FTP_PUT_ENABLED - (optional) define this to 1 or 0 to enable/disable the
      ability for an FTP client to write to the PIC's filesystem.

   FTP_RETR_ENABLED - (optional) define this to 1 or 0 to enable/disable the
      ability for an FTP client to read from the PIC's filesystem.
      
   FTP_LIST_ENABLED - (optional) define this to 1 or 0 to enable/disable the
      ability for an FTP client to read a list of files from the PIC's 
      filesystem.
      
   FTPVerify(userString, passwdString) - (required) Return TRUE if the 
      username/password has valid authorization to use the FTP server.
   
   FTP_FORCE_DISCONNECT() - (optional) A callback provided by the user, return 
      TRUE if you want the FTP server to disconnect a connecion.
      
   FTP_MARK_ERROR() - (optiona) A callback, FTP server will call this if there
      was a problem with the filesystem during access.  This can be used to 
      log or display errors to the user.
   
   FTP_CLEAR_ERROR() - A callback, un-does a FTP_MARK_ERROR().
*/
/*********************************************************************
 *
 *              FTP Server Defs for Microchip TCP/IP Stack
 *
 *********************************************************************
 * FileName:        ftp.h
 * Dependencies:    StackTsk.h
 *                  tcp.h
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
 * Nick LaBotne         2/14/07  CCS Port
 ********************************************************************/

#ifndef FTP_H
#define FTP_H


#define FTP_USER_NAME_LEN       (10)


/*********************************************************************
 * Function:        BOOL FTPVerify(char *login, char *password)
 *
 * PreCondition:    None
 *
 * Input:           login       - Telnet User login name
 *                  password    - Telnet User password
 *
 * Output:          TRUE if login and password verfies
 *                  FALSE if login and password does not match.
 *
 * Side Effects:    None
 *
 * Overview:        Compare given login and password with internal
 *                  values and return TRUE or FALSE.
 *
 * Note:            This is a callback from Telnet Server to
 *                  user application.  User application must
 *                  implement this function in his/her source file
 *                  return correct response based on internal
 *                  login and password information.
 ********************************************************************/

BOOL FTPVerify(char *login, char *password);



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
 * Overview:        Initializes internal variables of Telnet
 *
 * Note:
 ********************************************************************/
void FTPInit(void);


/*********************************************************************
 * Function:        void FTPTask(void)
 *
 * PreCondition:    FTPInit() must already be called.
 *
 * Input:           None
 *
 * Output:          Opened Telnet connections are served.
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
BOOL FTPTask(void);


BOOL DeleteFile(void);

int1 FTPServerIdle(void);



#endif
