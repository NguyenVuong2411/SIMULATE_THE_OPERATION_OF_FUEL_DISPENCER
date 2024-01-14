// ccs_announce.h
//
// on power-up, send an announce message a few times.
//
// [optional] look for broadcast UDP message asking for the announce message.
//
// ANNOUNCE_MSG - the announce message
//
// ANNOUNCE_PORT - the UDP port to announce
//
// ANNOUNCE_RATE - period, in ticks, between power-up announcements
//
// ANNOUNCE_NUM - number of messages to send on power-up.  can be set to 0 to disable.
//
// ANNOUNCE_SCAN - if defined, the UDP socket is left open and incoming messages are
//    scanned for this string.  if this string is received, then it will retransmit
//    the announce message.  this value can't change after you all AnnounceInit
//
// ANNOUNCE_ON_SEND() - if defined, this function will be called before 
//    the announce task checks to see if there is enough TX buffer for a
//    transmit.  This can be used to populate ANNOUNCE_MSG.
//
// ANNOUNCE_CLIENT_CONNECTED() - callback, if defined, is called when
//    respoding to a client's ANNOUNCE_SCAN
//
// ANNOUNCE_CLIENT_DISCONNECTED() - callback, if defined, this is called when it is
//    done sending a response to ANNOUNCE_SCAN
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

#ifndef __CCS_ANNOUNCE_H
#define __CCS_ANNOUNCE_H

// if you are low on UDP endpoints/resources, calling this will release
// the UDP endpoint/socket being used by the announce system.  that means
// announcing and listening for broadcast UDP pings will be disabled.
// calling AnnounceInit() will re-enable.
void AnnounceStop(void);

// will be called in StackInit(), so usually this doesn't need to be called
// by the user.  But if you called AnnounceStop() to free up UDP sockets,
// calling this again will restart it.
void AnnounceInit(void);

// will be called in StackApplications(), so the user doesn't need to
// call this in the application.
void AnnounceTask(void);

#endif
