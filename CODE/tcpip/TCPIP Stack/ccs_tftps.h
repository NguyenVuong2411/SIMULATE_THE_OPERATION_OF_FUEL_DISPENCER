#ifndef TFTP_H
#define TFTP_H

/***************************************************************************
*
*                             ccs_tftps.h
*
* TFTP server for receiving files.
///
/// CONFIGURATION:
///
/// STACK_USE_CCS_TFTP_SERVER - Define this before you include stacktsk.c
///        in your application.  Defining this will cause
///        the stack to include the TFTP portion and execute the init
///        and process any TFTP tasks.
///
/// TFTP_REQUIRE_FNAME0, TFTP_REQUIRE_FNAME1, TFTP_REQUIRE_FNAME2 - 
///   OPTIONAL
///   if you only
///   want to allow certain filenames to be uploaded, define these filenames.  
///   Then also define TFTP_REQUIRE_LOCx to denote where these files go
///   in flash/EE and TFTP_REQUIRE_SIZEx to denote the max size allocated
///   for this file.
///
/// TFTP_REQUIRE_SIZE0, TFTP_REQUIRE_SIZE1, TFTP_REQUIRE_SIZE2
///   OPTIONAL
///   Similar to TFTP_REQUIRE_LOCx, but denote how much space is reserved
///   on the flash for this file.
///
/// MPFS_RESERVE_BLOCK -or-
/// TFTP_REQUIRE_LOC0, TFTP_REQUIRE_LOC1, TFTP_REQUIRE_LOC2
///   User must define these values.
///   The place where files that get uploaded are stored.
///   If you are not using forced filenames, all uploads goto 
///   MPFS_RESERVE_BLOCK.
///
/// TFTPS_WRITE_FILE_SIZE(x, n) -
///   OPTIONAL
///   When done writing a file, this is called to denote the size of
///   the file.  Usually this isn't needed, but it will if TFTP_READ_ENABLED
///   is 1 and you want to download the file again.  x is the filename
///   requested (TFTP_REQUIRE_LOC0, TFTP_REQUIRE_LOC1, etc), or 0 if
///   none of those are defined, n is the size of the file.
///
/// TFTPS_STARTED_FILE(x) - 
///   OPTIONAL
///   Macro x called when a new upload is started.
///   if TFTP_REQUIRE_FNAME0 or TFTP_REQUIRE_FNAME1 is defined, then x
///   will be either 0 or 1.  If they are not defined, x will always be 0.
///
/// TFTPS_FINISHED_FILE(x) - 
///   OPTIONAL
///   Similar to TFTPS_STARTED_FILE(x), but called when file
///   is finished.
///
/// TFTPS_CLIENT_CONNECTED() -
///   OPTIONAL
///   Callback is called to notify the application if a client has connected
///   to the TFTP server.
///
/// TFTPS_CLIENT_DISCONNECTED() -
///   OPTIONAL
///   Callback is called to notify the application if a client has disconnected
///   from the TFTP server.
///
/// TFTP_PORT - 
///   OPTIONAL
///   The TCP/IP port the TFTP server will listen to for TFTP
///   connections.  Port 69 is almost exclusively used for TFTP traffic
*
* VERSION HISTORY
* ---------------------------------------------------------
*  April 28th, 2015 - Total re-write.  This was done to accomodate WF121 by
*     closing the listen socket once a packet is heard and re-opening a
*     client socket with node info from server socket.  Also totally cleaned
*     the code.   Read support has been removed, you can only write to the
*     flash.
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
#ifndef TFTP_PORT
#define TFTP_PORT 69
#endif

// if you have limited UDP endpoints/sockets, calling this will close
// the UDP socket being used for the UDP server.  This will stop TFTP from
// working until you call TFTPSInit() to restart the server.
void TFTPSStop(void);

// this is called by StackInit(), so usually the user doesn't need to call
// this.  BUT if you stpped the TFTP Server with TFTPSStop(), then you
// can re-enable it with this.
void TFTPSInit(void);

// this is called by StackApplications(), so the user doesn't need to call this.
void TFTPSTask(void);

#endif   //once
