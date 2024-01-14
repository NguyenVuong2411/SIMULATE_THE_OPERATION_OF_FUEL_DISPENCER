/*
   ccs_tftpc.h
   
   CCS's TFTP client.  Does a PUT to send the a file to the configured server.
   
   STACK_USE_CCS_TFTP_CLIENT needs to be defined in order for this library
   to be added to the stack.
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

#ifndef __CCS_TFTPC_H__
#define __CCS_TFTPC_H__

// returns TRUE if we are busy sending a file to a server
int1 TFTPCIsBusy(void);

// see documentation for UdpOpenEx() for 'remoteHost' and 'remoteHostType'.
// 'remotePort' is the UDP port of the server, default is 69.
// 'remoteFilename' is the filename to be stored on the server.
// returns TRUE if started successfully, 
// returns FALSE if not (probably because we are already busy with
// a previous request or no more UDP sockets available).
int1 TFTPCStart(DWORD remoteHost, BYTE remoteHostType, UDP_PORT remotePort, char* remoteFilename);

// this is a callback that must be provided in your application.
// when the TFTP Client library is ready to send a packet, this function
// will be called.  You then must UDPPut() or UDPPutArray() 512 bytes.
// If you send less than 512 bytes then this packet is considered the last 
// packet.  UDPIsPutReady() is already called before this, so UDPPut() and
// UDPPutARray() are already set to put to the correct socket.
// 'offset' is the byte offset that is being requested by the server.
// due to retries you may get 'offset' values that repeat or out of order,
// therefore this value must be looked at too see what data needs to be sent.
// this function should return the number of bytes sent to 'udpSocket'.
// Do not call UDPFlush().
unsigned int16 TFTPCPut(UDP_SOCKET udpSocket, unsigned int32 offset);

typedef enum
{
   TFTPC_ERROR_OK = 0,
   TFTPC_ERROR_NOT_STARTED = 1,  //idle, nothing started
   TFTPC_ERROR_BUSY = 2,   //still busy sending a file, or 
   TFTPC_ERROR_NO_CONNECTION = 3,   //no connection to server (bad DNS or bad ARP)
   TFTPC_ERROR_NO_ACK = 4,   //we didn't get ACKs to our message
} tftpc_error_t;

// cancel any pending transactions.
void TFTPCCancel(void);

/////////////////////////////////////////////////////////////////////////////
// DO NOT CALL THESE, THESE ARE ALREADY CALLED IN SIDE StackInit() and StackApplications().
/////////////////////////////////////////////////////////////////////////////
void TFTPCInit(void);
void TFTPCTask(void);

#endif
