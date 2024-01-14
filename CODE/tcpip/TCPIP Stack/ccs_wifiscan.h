// ccs_wifiscan.h
//
// Extra state-machine code for handling AP scanning and getting RSSI of 
// current connection.
//
// Normally, if you perform an AP scan then it will stop RSSI scanning.
// If you use EE_LOC_WIFI_SCAN_RESULTS to cache AP results to EE then RSSI
// will continue to happen.  Else, WIFIScanDiscard() will resume RSSI
// scanning.
//
// OPTIONS
// -----------------------------------------------------------------
//    CCS_WIFISCAN_NO_RSSI
//       Disable all background scanning of RSSI
//
//    CCS_WIFISCAN_DURING_ADHOC
//       Normally the background RSSI scanning will be disable in AdHoc mode.
//       If you want to turn it back on, define this. But it's probably a 
//       bad idea and might cause module lockups.
//
//    EE_LOC_WIFI_SCAN_RESULTS, EE_NUM_WIFI_SCAN_RESULTS, EE_SIZE_WIFI_SCAN_RESULT
//       If defined then scan results will be saved to the EEPROM.  
//       NUM results will be saved to LOC, where total size used is 
//       1+(NUM*SIZE).  This was implemented because on some B modules the 
//       memory would get corrupted if you start attempt to read scan results 
//       after you start a new connection.  If defined, WIFIScanGetResult() and 
//       WIFIScanGetNum() will read from EE memory instead.  When saved to
//       EE, results with blank SSIDs will be ignored.  The table will also
//       be sorted, highest RSSI first, to maximize the NUM of entries
//       saved to EE in the event your scan finds lots of results.
//
//    CCS_WIFI_SCAN_RESULTS_MALLOC
//       If defined, malloc() will be used to dynamically alocate memory
//       for scan results.  Memory will be allocated at WIFIScanStart()
//       and de-allocated at WIFIScanDiscard().  If WIFIScanIsBusy() returns
//       FALSE immediately after WIFIScanStart() then malloc() failed.
//       Using malloc() is an alternative to the saving results to the
//       EE.  Define this value to be the max number of scan results
//       to save.  The total memory allocated will be this value
//       multiplied by tWFScanResult.
//
//    CCS_WIFISCAN_DONT_AUTO_CONNECT - If defined the statemachine won't 
//       automatically call WIFIConnectStart() when scan results are found.
//       This is an alternative to the EE method provided above as a
//       workaround to faulty B units.  The intention is that after scan
//       results are found, you would manually call WIFIConnectStart() 
//       after you read them.  WIFIScanDiscard() will always call 
//       WIFIConnectStart() if this is used.
//
//    CCS_WIFISCAN_FIXED_TYPE - If defined, when performing an AP
//       scan will only scan for a certain network type.  This can be
//       defined to WF_INFRASTRUCTURE or WF_ADHOC.  If this isn't defined,
//       WF_Scan will use WF_SCAN_ALL parameter, which will return
//       all types.
//
///////////////////////////////////////////////////////////////////////////
////        (C) Copyright 1996, 2013 Custom Computer Services          ////
//// This source code may only be used by licensed users of the CCS C  ////
//// compiler.  This source code may only be distributed to other      ////
//// licensed users of the CCS C compiler.  No other use, reproduction ////
//// or distribution is permitted without written permission.          ////
//// Derivative programs created using this software in object code    ////
//// form are not restricted in any way.                               ////
///////////////////////////////////////////////////////////////////////////

#ifndef __CCS_WIFISCAN_H__
#define __CCS_WIFISCAN_H__

//#define CCS_WIFISCAN_FIXED_TYPE WF_INFRASTRUCTURE
//#define CCS_WIFISCAN_DONT_AUTO_CONNECT
//#define EE_LOC_WIFI_SCAN_RESULTS   EE_LOC_RESERVED
//#define EE_SIZE_WIFI_SCAN_RESULT   60
//#define EE_NUM_WIFI_SCAN_RESULTS  ((EE_SIZE_RESERVED-1) / EE_SIZE_WIFI_SCAN_RESULT)
//#warning !!!! DEBUG SETTINGS

//#define CCS_WIFISCAN_NO_RSSI

#ifndef WF_USE_SCAN_FUNCTIONS
#define WF_USE_SCAN_FUNCTIONS
#endif

#ifndef EE_SIZE_WIFI_SCAN_RESULT
#define EE_SIZE_WIFI_SCAN_RESULT  sizeof(tWFScanResult)
#endif

int1 WIFIRSSIIsValid(void);
unsigned int8 WIFIRSSIGet(void);

void WIFIScanEnableBackgroundRSSI(int1 enable);

void WIFIScanStart(void);

// marks WIFIScanIsBusy() as TRUE and WIFIScanIsValid() as FALSE, but
// won't actually start scan process until t TICKS has expired.  that
// way a WIFI scan can be initiated over the network (like from a webpage)
// and the WIFI will still be up long enough to send a response over
// the network (because the scan process disconnects the unit from the
// network).
void WIFIScanStartDelayed(TICK t);

int1 WIFIScanIsBusy(void);
int1 WIFIScanIsValid(void);
unsigned int8 WIFIScanGetNum(void);
void WIFIScanGetResult(unsigned int8 index, tWFScanResult *pResult);
unsigned int8 WIFIScanResultSecurityType(tWFScanResult *pResult);

// will cause WIFIScanIsValid() to return FALSE.
// if CCS_WIFISCAN_DONT_AUTO_CONNECT is used, this will call WIFIConnectStart().
// this will also resume RSSI scanning.
void WIFIScanDiscard(void);

// Init() and Task() automatically called by StackTask2.  
// WIFIScanResults() called at WF_EVENT_SCAN_RESULTS_READY.
// WIFIScanIgnoreResults() called if data buffer was cleared (DeallocateDataRxBuffer())
//DO NOT CALL THESE.
void WIFIScanInit(void);
void WIFIScanTask(void);
void WIFIScanIgnoreResults(void);
void WIFIScanResults(unsigned int8 num);

// these callbacks are required.  DO NOT CALL THESE.
void WIFIConnectStop(void);   //you cannot scan during a connection process, only when idle or connected.  this will stop the connect state machine from attempting a connection.  if in the middle of a connection, will reset the MAC with MacInit()
void WIFIConnectStart(void);  //undo a WIFIConnectStop(), called after scanning is done.

#endif
