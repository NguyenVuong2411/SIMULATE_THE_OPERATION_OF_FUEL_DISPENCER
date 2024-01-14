/*
   A wrapper to Microchip's SMTP (STACK_USE_SMTP_CLIENT) library 
   for sending an e-mail based on alert flags.
   
   To use this, define STACK_USE_CCS_EMAIL_ALERTS
   
   User can also define EMAIL_ALERT_CUSTOM_BODY(first) macro, which can be used
   to send a custom body after all the flag strings have been sent.  This
   macro should return TRUE when the body is done being sent, FALSE if
   there is more data.  This macro should first call SMTPIsPutReady() to
   see how much data is available in the TX buffer (if any), then call
   a put function (SMTPPut(), SMTPPutArray(), SMTPPutString(), 
   SMTPPutROMArray() or SMTPPutROMString()), and when done putting
   that data call SMTPFlush().  The first paramtere passed to the
   EMAIL_ALERT_CUSTOM_BODY() macro will be TRUE when it's the first time
   being called for this e-mail, FALSE on subsequent calls.
   
   EMAIL_ALERT_CLEAR_FLAGS_ON_FAIL - If this is defined, if SMTP failed
      to send an e-mail after 3 retries then it will clear any pending
      flags.  If it is not defined, the flags will remain set and it will
      start a new set of 3 retries after the holdoff period.  While the
      flags are set during the holdoff the EmailAlertIsBusy() will still
      return TRUE.
*/
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

#ifndef __CCS_EMAIL_ALERT_H__
#define __CCS_EMAIL_ALERT_H__

// intialize e-mail and alert system.  call before anything else.
// CCS already added this to StackInit(), so you don't 
// need to call this.
void EmailAlertInit(void);

// set a flag (0 to 31).  once a flag is sent, an e-mail will be sent.
// email will contain one string per flag set, where strings come from
// the user defined g_EmailAlertStrings[];
// the email won't start until EmailAlertTask() is called (currently
// in StackTask()), that means you can have several flags set
// in one e-mail.
// if you set a flag and we were currently busy sending other flags
// (see EmailAlertIsBusy()) then this flag will be immediately sent
// once the e-mail is done.
//
// ex:
//    ROM char* g_EmailAlertStrings[] = {
//          (ROM char*)"Alert flag 0",
//          (ROM char*)"Alert flag 1"
//       };
void EmailAlertSetFlag(unsigned int8 flag);

// normally there is a hold-off timer to slow down e-mails.
// calling this will clear that timer so it will immediately send
// any flags that are set (or next flags set will be immediately sent).
void EmailAlertClearTimer(void);

// set the holdoff timer between e-mails.
void EmailAlertSetTimerTicks(TICK ticks);
void EmailAlertSetTimerMinutes(unsigned int16 minutes);

// see EmailAlertSetFlag()
//extern ROM char* g_EmailAlertStrings[];

// returns TRUE if the system is attempting to send a message.
int1 EmailAlertIsBusy(void);

// returns TRUE if the system has queued flags to send but can't because
// it's in the holdoff period.
int1 EmailAlertIsHolding(void);

// get the number of times e-mails were attempted.
unsigned int16 EmailAlertGetAttempts(void);

// get the number of times e-mails were successful.
// this won't update until after EmailAlertIsBusy() goes back to FALSE.
unsigned int16 EmailAlertGetSuccess(void);

// get the error code of the last attempt.
// will be 0 if the e-mail was success.
// this won't update until after EmailAlertIsBusy() goes back to FALSE.
WORD EmailAlertGetLastError(void);

// a callback the user must provide.
// in this function, the user must call the relevant EmailAlertSetXXXX() 
// needed to setup an e-mail.
// this will be called during EmailAlertTask() after the user called
// EmailAlertSetFlag();
void EmailAlertSetup(void);

// these functions can be used to configure the e-mail client.
// any strings saved in RAM have to be globally or statically alocated.
// do not call if EmailAlertIsBusy() is TRUE
#define EmailAlertSetPort(x)              SMTPClient.ServerPort=x
#define EmailAlertSetHost(pStr)           SMTPClient.Server.szRAM=pStr; SMTPClient.ROMPointers.Server=FALSE
#define EmailAlertSetHostROM(pStr)        SMTPClient.Server.szROM=pStr; SMTPClient.ROMPointers.Server=TRUE
#define EmailAlertSetUsername(pStr)       SMTPClient.Username.szRAM=pStr; SMTPClient.ROMPointers.Username=FALSE
#define EmailAlertSetUsernameROM(pStr)    SMTPClient.Username.szROM=pStr; SMTPClient.ROMPointers.Username=TRUE
#define EmailAlertSetPassword(pStr)       SMTPClient.Password.szRAM=pStr; SMTPClient.ROMPointers.Password=FALSE
#define EmailAlertSetPasswordROM(pStr)    SMTPClient.Password.szROM=pStr; SMTPClient.ROMPointers.Password=TRUE
#define EmailAlertSetTo(pStr)             SMTPClient.To.szRAM=pStr; SMTPClient.ROMPointers.To=FALSE
#define EmailAlertSetToROM(pStr)          SMTPClient.To.szROM=pStr; SMTPClient.ROMPointers.To=TRUE
#define EmailAlertSetCC(pStr)             SMTPClient.CC.szRAM=pStr; SMTPClient.ROMPointers.CC=FALSE
#define EmailAlertSetCCROM(pStr)          SMTPClient.CC.szROM=pStr; SMTPClient.ROMPointers.CC=TRUE
#define EmailAlertSetBCC(pStr)            SMTPClient.BCC.szRAM=pStr; SMTPClient.ROMPointers.BCC=FALSE
#define EmailAlertSetBCCROM(pStr)         SMTPClient.BCC.szROM=pStr; SMTPClient.ROMPointers.BCC=TRUE
#define EmailAlertSetFrom(pStr)           SMTPClient.From.szRAM=pStr; SMTPClient.ROMPointers.From=FALSE
#define EmailAlertSetFromROM(pStr)        SMTPClient.From.szROM=pStr; SMTPClient.ROMPointers.From=TRUE
#define EmailAlertSetSubject(pStr)        SMTPClient.Subject.szRAM=pStr; SMTPClient.ROMPointers.Subject=FALSE
#define EmailAlertSetSubjectROM(pStr)     SMTPClient.Subject.szROM=pStr; SMTPClient.ROMPointers.Subject=TRUE
#define EmailAlertSetOtherHeaders(pStr)   SMTPClient.OtherHeaders.szRAM=pStr; SMTPClient.ROMPointers.OtherHeaders=FALSE
#define EmailAlertSetOtherHeadersROM(pStr)  SMTPClient.OtherHeaders.szROM=pStr; SMTPClient.ROMPointers.OtherHeaders=TRUE
#if defined(STACK_USE_SSL_CLIENT)
#define EmailAlertSetUseSLL(x)            SMTPClient.UseSSL=x
#endif

// check to see if there are any flags set, and if so then send
// the message.
// CCS already added this to StackApplications(), so you don't 
// need to call this.
void EmailAlertTask(void);

#endif
