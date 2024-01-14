// ccs_wifiscan.c
//
// Extra state-machine code for handling AP scanning and getting RSSI of 
// current connection.
///////////////////////////////////////////////////////////////////////////
////        (C) Copyright 1996, 2013 Custom Computer Services          ////
//// This source code may only be used by licensed users of the CCS C  ////
//// compiler.  This source code may only be distributed to other      ////
//// licensed users of the CCS C compiler.  No other use, reproduction ////
//// or distribution is permitted without written permission.          ////
//// Derivative programs created using this software in object code    ////
//// form are not restricted in any way.                               ////
///////////////////////////////////////////////////////////////////////////

#ifndef __CCS_WIFISCAN_C__
#define __CCS_WIFISCAN_C__

#ifndef debug_scan_printf
   #define debug_scan_printf(a,b,c,d,e,f,g,h,i,j,k,l)
#endif

#ifndef STACK_USE_CCS_SCAN_TASK_RSSI_PERIOD_SUCCESS
#define STACK_USE_CCS_SCAN_TASK_RSSI_PERIOD_SUCCESS  ((TICK)60 * TICKS_PER_SECOND)
#endif

#ifndef STACK_USE_CCS_SCAN_TASK_RSSI_PERIOD_FAIL
#define STACK_USE_CCS_SCAN_TASK_RSSI_PERIOD_FAIL  ((TICK)5 * TICKS_PER_SECOND)
#endif

#if defined(CCS_WIFISCAN_EE_LOC)
   #error should be called EE_LOC_WIFI_SCAN_RESULTS
#endif

#if defined(CCS_WIFISCAN_EE_NUM)
   #error should be called EE_NUM_WIFI_SCAN_RESULTS
#endif

#if defined(CCS_WIFISCAN_EE_SIZE)
   #error should be called EE_SIZE_WIFI_SCAN_RESULT
#endif

tWFScanResult g_WifiScanScrResult;

UINT8 g_ScanConnectionProfile;

#if defined(EE_LOC_WIFI_SCAN_RESULTS) && defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
#error cant use both
#endif

#if defined(EE_LOC_WIFI_SCAN_RESULTS)
static void _WifiScanResultsToEE(uint8_t num);
#elif defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
static void _WifiScanResultsToMalloc(void);
#endif

static void _WIFIScanResults(unsigned int8 num, int1 valid);

enum
{
   WIFI_SCAN_STATE_IDLE = 0,
   WIFI_SCAN_DOING_AP,
   WIFI_SCAN_DOING_RSSI,
  #if defined(EE_LOC_WIFI_SCAN_RESULTS)
   WIFI_SCAN_SAVING_AP_TO_EE,
  #elif defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
   WIFI_SCAN_SAVING_AP_TO_MALLOC,
  #endif
   WIFI_SCAN_FINISH_AP,
   WIFI_SCAN_READ_RSSI
} g_WifiScanState;

//int1 g_WifiScanNetworkTypeNeedsFixing = FALSE;

#if defined(EE_LOC_WIFI_SCAN_RESULTS)
   unsigned int8 g_WifiScanToEE;
   #define g_WifiScanRssiDisable FALSE
#elif defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
   struct
   {
      tWFScanResult *p;   
      unsigned int8 numTotal; //the number of scan results in the module that needs to be read
      unsigned int8 numFiltered; //the number of filtered results saved to 'p'
   } _g_WifiScanToMalloc = {NULL, 0, 0xFF};
   
   #define g_WifiScanRssiDisable FALSE
#else
   unsigned int8 g_WifiScanApNum;   //0xFF is invalid
   int1 g_WifiScanRssiDisable;
#endif

UINT8 g_WifiScanRssiValue; //0 is lowest value.  0xff is invalid
TICK g_WifiScanRssiTick, g_WifiScanRssiDuration;

int1 g_WifiScanApWhenReady;
int1 g_WifiScanApDelayedDo;
TICK g_WifiScanApDelayedStart, g_WifiScanApDelayedDuration;

static void _WIFIScanDiscard(void)
{
   debug_scan_printf(debug_putc, "\r\n_WIFIScanDiscard() ");
  #if defined(EE_LOC_WIFI_SCAN_RESULTS)
   unsigned int8 num = 0xFF;
   EEWriteBytes(EE_LOC_WIFI_SCAN_RESULTS, &num, 1);
  #elif defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
   _g_WifiScanToMalloc.numFiltered = 0xFF;
  #else
   g_WifiScanApNum = 0xFF;
  #endif
}

void WIFIScanDiscard(void)
{
   debug_scan_printf(debug_putc, "\r\nWIFIScanDiscard() ");
   
   _WIFIScanDiscard();

  #if defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
   if (_g_WifiScanToMalloc.p != NULL)
   {
      free(_g_WifiScanToMalloc.p);
      _g_WifiScanToMalloc.p = NULL;
   }
  #endif
  
  #if !defined(EE_LOC_WIFI_SCAN_RESULTS) && !defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
   g_WifiScanRssiDisable = FALSE;
  #endif
  
  #if defined(CCS_WIFISCAN_DONT_AUTO_CONNECT)
   WIFIConnectStart();
  #endif
}

static void _WIFIAPScanStart(void)
{
   UINT16 ret16 = WF_SUCCESS;
   UINT8 cp;
  #if defined(CCS_WIFISCAN_FIXED_TYPE)
   #if !defined(GENERIC_SCRATCH_BUFFER)
      char WIFIConnectTask_Scratch[6];
   #else
      #define WIFIConnectTask_Scratch  GENERIC_SCRATCH_BUFFER
   #endif
  #endif

   
   debug_scan_printf(debug_putc, "\r\n_WIFIAPScanStart() ");

   WIFIConnectStop(); 
   
  #if defined(WF_SOFT_AP)
   if (AppConfig.networkType == WF_SOFT_AP)
   {
      debug_scan_printf(debug_putc, "\r\n_WIFIAPScanStart() putting radio into INF mode");
      AppConfig.networkType = WF_INFRASTRUCTURE;   //temporarily put radio into inf mode
      _WIFICreateConnectionProfile();
      AppConfig.networkType = WF_SOFT_AP;
      if (g_connectionProfileID != 0xFF)
      {
        #if defined(WF_FORCE_NO_PS_POLL)
         WF_CCS_PsPollDisable();
        #endif
      
         WF_CMConnect(g_connectionProfileID);
         
         WIFIConnectStop();  //delete the profile
      }
      //g_WifiScanNetworkTypeNeedsFixing = TRUE;
   }
  #endif
   
   g_WifiScanApWhenReady = FALSE;
   g_WifiScanApDelayedDo = FALSE;
   
   g_WifiScanState = WIFI_SCAN_DOING_AP;
  
  #if defined(EE_LOC_WIFI_SCAN_RESULTS)
   _WifiScanResultsToEE(0xFF);
  #elif defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
   _g_WifiScanToMalloc.numFiltered = 0xFF;
  #else
   g_WifiScanRssiDisable = TRUE;
   g_WifiScanApNum = 0xFF;   
  #endif

   WF_CASetChannelList(NULL, 0);
     
  #if !defined(CCS_WIFISCAN_FIXED_TYPE)
   debug_scan_printf(debug_putc, "\r\nNo scan profile (WF_SCAN_ALL)");
   cp = WF_SCAN_ALL;
  #else
   if (g_ScanConnectionProfile != 0xFF)
   {
      debug_scan_printf(debug_putc, "\r\nDeleting scan profile %U", g_ScanConnectionProfile);
      WF_CPDelete(g_ScanConnectionProfile);
      g_ScanConnectionProfile = 0xFF;
   }
   
   debug_scan_printf(debug_putc, "\r\nCreating scan profile");
   
   WF_CPCreate(&g_ScanConnectionProfile);
   
   if (g_ScanConnectionProfile == 0xFF)
   {
      debug_scan_printf(debug_putc, "\r\nERR Scan Profile %LX\r\n", g_ScanConnectionProfile);
      ret16 = -1;
   }
   else
   {
      cp = g_ScanConnectionProfile;

      /*WF_CPSetSsid(g_ScanConnectionProfile, 
                 AppConfig.MySSID, 
                 AppConfig.SsidLength);*/
      
      memset(WIFIConnectTask_Scratch, 0xFF, 6);
      WF_CPSetBssid(g_ScanConnectionProfile, WIFIConnectTask_Scratch);

      WF_CPSetNetworkType(g_ScanConnectionProfile, CCS_WIFISCAN_FIXED_TYPE);
      
      //WF_CASetScanType(MY_DEFAULT_SCAN_TYPE);   // was WF_ACTIVE_SCAN

      //WF_CASetChannelList(WIFI_channelList, WIFI_numChannelsInList); //was AppConfig.channelList, AppConfig.numChannelsInList

      //WF_SetRegionalDomain(WIFI_region);  //was AppConfig.region   
   }
  #endif
   
   if (ret16 == WF_SUCCESS)
   {
      debug_scan_printf(debug_putc, "\r\nStarting scan profile %U", cp);
      ret16 = WF_Scan(cp);
   }
   
   if (ret16 != WF_SUCCESS)  //error
   {
      debug_scan_printf(debug_putc, "\r\nERR WF_Scan %U %LX\r\n", cp, ret16);
      _WIFIScanResults(0, FALSE);
   }
}

int1 WIFIRSSIIsValid(void)
{
   return((g_WifiScanRssiValue != 0xFF) && !g_WifiScanRssiDisable);
}

unsigned int8 WIFIRSSIGet(void)
{
   return(g_WifiScanRssiValue);
}

void WIFIScanStart(void)
{
   debug_scan_printf(debug_putc, "\r\nWIFIScanStart() ");
   
  #if defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
   _WIFIScanDiscard();
   _g_WifiScanToMalloc.p = malloc((size_t)CCS_WIFI_SCAN_RESULTS_MALLOC * sizeof(tWFScanResult));
   debug_scan_printf(debug_putc, "p=%LX ", _g_WifiScanToMalloc.p);
   if (_g_WifiScanToMalloc.p == NULL)
      return;
  #endif
  
   g_WifiScanApWhenReady = TRUE;
   g_WifiScanApDelayedDo = FALSE;
}

void WIFIScanStartDelayed(TICK t)
{
   debug_scan_printf(debug_putc, "\r\nWIFIScanStartDelayed() ");
  #if defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
   _WIFIScanDiscard();
   _g_WifiScanToMalloc.p = malloc((size_t)CCS_WIFI_SCAN_RESULTS_MALLOC * sizeof(tWFScanResult));
   debug_scan_printf(debug_putc, "p=%LX ", _g_WifiScanToMalloc.p);
   if (_g_WifiScanToMalloc.p == NULL)
      return;
  #endif
  
   g_WifiScanApWhenReady = TRUE;
   g_WifiScanApDelayedDo = TRUE;
   g_WifiScanApDelayedStart = TickGet();
   g_WifiScanApDelayedDuration = t;
}

int1 WIFIScanIsBusy(void)
{
   return
   (  
      g_WifiScanApWhenReady
      || (g_WifiScanState == WIFI_SCAN_DOING_AP)
      || (g_WifiScanState == WIFI_SCAN_FINISH_AP)
     #if defined(EE_LOC_WIFI_SCAN_RESULTS)
      || (g_WifiScanState == WIFI_SCAN_SAVING_AP_TO_EE)
     #elif defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
      || (g_WifiScanState == WIFI_SCAN_SAVING_AP_TO_MALLOC)
     #endif
   );
}

int1 WIFIScanIsValid(void)
{
  #if defined(EE_LOC_WIFI_SCAN_RESULTS)
   unsigned int8 ret;
   EEReadBytes(&ret, EE_LOC_WIFI_SCAN_RESULTS, 1);
   return(ret != 0xFF);
  #elif defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
   return(_g_WifiScanToMalloc.numFiltered != 0xFF);
  #else
   return(g_WifiScanApNum != 0xFF);
  #endif
}

unsigned int8 WIFIScanGetNum(void)
{
  #if defined(EE_LOC_WIFI_SCAN_RESULTS)
   unsigned int8 ret;
   EEReadBytes(&ret, EE_LOC_WIFI_SCAN_RESULTS, 1);
   return(ret);
  #elif defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
   return(_g_WifiScanToMalloc.numFiltered);
  #else
   return(g_WifiScanApNum);
  #endif
}

UINT8 _WIFIScanNormalizeRSSIValue(UINT8 rssi)
{
   //rssi is the value from tWFScanResult.
   //it doesn't have units.  also the range is different between the B and
   //G module.  this will normalize it to a set range for all modules where
   //0 is worst.
   
  #if defined(MRF24WG)
   #define __RSSI_MIN   43
   #define __RSSI_MAX   128
  #else
   #define __RSSI_MIN   106
   #define __RSSI_MAX   200
  #endif
  
  #if 1
   if (rssi < __RSSI_MIN)
      rssi = __RSSI_MIN;
   
   rssi -= __RSSI_MIN;
  #endif
   
   return(rssi);
}

unsigned int8 WIFIScanResultSecurityType(tWFScanResult *pResult)
{
   unsigned int8 ret, apConfig;
   
   apConfig = pResult->apConfig;
   
   ret = WF_SECURITY_OPEN;
   
   if (bit_test(apConfig, 4))
   {
      if (bit_test(apConfig, 6))
      {
         ret = WF_SECURITY_WPA_WITH_PASS_PHRASE;
      }
      else if (bit_test(apConfig, 7))
      {
         ret = WF_SECURITY_WPA2_WITH_PASS_PHRASE;
      }
      else
      {
         ret = WF_SECURITY_WEP_AUTO;
      }         
   }
   
   return(ret);
}

#if defined(EE_LOC_WIFI_SCAN_RESULTS)
#if (EE_SIZE_WIFI_SCAN_RESULT < sizeof(tWFScanResult))
#error not allocating enough space in EEPROM for individual scan result
#endif
#endif

static void _WIFIScanNormalizeValues(void)
{  
   g_WifiScanScrResult.ssid[WF_MAX_SSID_LENGTH-1] = 0;   //WF_MAX_SSID_LENGTH and ssidLen
   if (g_WifiScanScrResult.ssidLen < WF_MAX_SSID_LENGTH)
   {
      g_WifiScanScrResult.ssid[g_WifiScanScrResult.ssidLen] = 0;
   }
   
   g_WifiScanScrResult.rssi = _WIFIScanNormalizeRSSIValue(g_WifiScanScrResult.rssi);
}

void WIFIScanGetResult(unsigned int8 index, tWFScanResult *pResult)
{
  #if defined(EE_LOC_WIFI_SCAN_RESULTS)
   EEPROM_ADDRESS loc;
  #elif defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
   unsigned int8 *pResults;
  #endif
   
   memset(pResult, 0x00, sizeof(tWFScanResult));
   
   if (WIFIScanIsValid() && (index < WIFIScanGetNum()))
   {  
     #if defined(EE_LOC_WIFI_SCAN_RESULTS)
      loc = EE_LOC_WIFI_SCAN_RESULTS + 1;
      loc += (EEPROM_ADDRESS)index * EE_SIZE_WIFI_SCAN_RESULT;
      EEReadBytes(pResult, loc, sizeof(tWFScanResult));
     #elif defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
      pResults = _g_WifiScanToMalloc.p;
      pResults += (size_t)sizeof(tWFScanResult) * index;
      memcpy(pResult, pResults, sizeof(tWFScanResult));
      debug_scan_printf(debug_putc, "\r\nWIFIScanGetResult(%U,%X) p=%X ", index, pResult, pResults);
     #else
      WF_ScanGetResult(index, &g_WifiScanScrResult);
      
      _WIFIScanNormalizeValues();
      
      memcpy(pResult, &g_WifiScanScrResult, sizeof(g_WifiScanScrResult));
     #endif
   }
}
  
void WIFIScanInit(void)
{
   g_ScanConnectionProfile = 0xFF;
      
   g_WifiScanApWhenReady = FALSE;
   
   g_WifiScanApDelayedDo = FALSE;
   
   g_WifiScanState = WIFI_SCAN_STATE_IDLE;
     
   _WIFIScanDiscard();
   
  #if !defined(EE_LOC_WIFI_SCAN_RESULTS) && !defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
   g_WifiScanRssiDisable = FALSE;
  #endif   

   g_WifiScanRssiValue = 0xFF;
   
   WIFIScanEnableBackgroundRSSI(TRUE);
}

#if defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
typedef struct
{
   unsigned int8 index;
   unsigned int8 rssi;
} _WifiScanResultsToMalloc_sort_table_t;

static void _WifiScanResultsToMalloc(void)
{
   _WifiScanResultsToMalloc_sort_table_t sortTable[CCS_WIFI_SCAN_RESULTS_MALLOC];
   unsigned int8 i, j, n;
   unsigned int8 numSortTable;
   tWFScanResult *p;

   debug_scan_printf(debug_putc, "\r\n_WifiScanResultsToMalloc %u ", _g_WifiScanToMalloc.numTotal);

   if (_g_WifiScanToMalloc.numTotal == 0)
   {
      _g_WifiScanToMalloc.numFiltered = 0;
      return;
   }
   
   numSortTable = 0;
   memset(sortTable, 0, sizeof(sortTable));
   
   for (i=0; i<_g_WifiScanToMalloc.numTotal; i++)
   {
      WF_ScanGetResult(i, &g_WifiScanScrResult);
      _WIFIScanNormalizeValues();
     
      debug_scan_printf(debug_putc, "\r\nSCAN found '%s' rssi %u at index %u ", g_WifiScanScrResult.ssid, g_WifiScanScrResult.rssi, i);
      
      for (j=0; j<numSortTable; j++)
      {
         if (sortTable[j].rssi < g_WifiScanScrResult.rssi)
            break;   //j is location weaker rssi
      }
      debug_scan_printf(debug_putc, "put at %u ", j);
      if (j >= numSortTable)
      {  //we are the weakest rssi, or no entries in the table.  store to end of the table if room.
         if (numSortTable < CCS_WIFI_SCAN_RESULTS_MALLOC)
         {
            debug_scan_printf(debug_putc, "(END) ");
            
            sortTable[numSortTable].index = i;
            sortTable[numSortTable].rssi = g_WifiScanScrResult.rssi;
            numSortTable++;
         }
      }
      else
      {
         //push everything back 1
         n = numSortTable - j;
         if (numSortTable >= CCS_WIFI_SCAN_RESULTS_MALLOC)
            n--;
         
         debug_scan_printf(debug_putc, "move %u ", n);
         memmove(&sortTable[j+1], &sortTable[j], (size_t)n * sizeof(_WifiScanResultsToMalloc_sort_table_t));
         
         //save new entry at sorted position
         sortTable[j].index = i;
         sortTable[j].rssi = g_WifiScanScrResult.rssi;
         
         if (++numSortTable > CCS_WIFI_SCAN_RESULTS_MALLOC)
            numSortTable = CCS_WIFI_SCAN_RESULTS_MALLOC;
      }
      debug_scan_printf(debug_putc, "total is %u ", numSortTable);
   }
   
   p = _g_WifiScanToMalloc.p;
   for (i=0; i<numSortTable; i++)
   {
      WF_ScanGetResult(sortTable[i].index, &g_WifiScanScrResult);
      _WIFIScanNormalizeValues();
      debug_scan_printf(debug_putc, "\r\nWF_SCAN Saving '%s' (%u) to %u 0x%LX ", g_WifiScanScrResult.ssid, sortTable[i].index, i, p);
      memcpy(p, &g_WifiScanScrResult, sizeof(tWFScanResult));
      p++;
   }
   
   _g_WifiScanToMalloc.numFiltered = numSortTable;
}
#endif   //if defined(CCS_WIFI_SCAN_RESULTS_MALLOC)

#if defined(EE_LOC_WIFI_SCAN_RESULTS)
typedef struct
{
   UINT8 index;
   UINT8 rssi;
} rssi_qsort_t;


signed int8 _WifiRssiQsortCompare(void* a, void* b)
{
   UINT8 rssiA, rssiB;
   
   rssiA = ((rssi_qsort_t*)a)->rssi;
   rssiB = ((rssi_qsort_t*)b)->rssi;
   
   /*  //this is for sorting low to high
   if (rssiA < rssiB)   return -1;
   if (rssiA > rssiB)   return 1;
   return 0;   //rssiA == rssiB
   */
   
   // this is for sorting high to low
   if (rssiA < rssiB)   return 1;
   if (rssiA > rssiB)   return -1;
   return 0;   //rssiA == rssiB
}

void _WifiRssiQsortFunc(char * qdata, unsigned int qitems, unsigned int qsize) 
{
   unsigned int m,j,i,l;
   int1 done;
   unsigned int8 t[16];

   m = qitems/2;
   while( m > 0 ) {
     for(j=0; j<(qitems-m); ++j) {
        i = j;
        do
        {
           done=1;
           l = i+m;
           if(_WifiRssiQsortCompare(qdata+i*qsize, qdata+l*qsize) > 0 ) {
              memcpy(t, qdata+i*qsize, qsize);
              memcpy(qdata+i*qsize, qdata+l*qsize, qsize);
              memcpy(qdata+l*qsize, t, qsize);
              if(m <= i)
                i -= m;
                done = 0;
           }
        } while(!done);
     }
     m = m/2;
   }
}

#ifndef WIFI_SCAN_SORT_TABLE_SIZE
#define WIFI_SCAN_SORT_TABLE_SIZE   64
#endif

// pass 0xFF for num to clear/invalidate values in memory
// pass 0 for num for valid results of none found
// else num represents valid results of num number.
static void _WifiScanResultsToEE(uint8_t num)
{
   EEPROM_ADDRESS loc;
   UINT8 numSaved = 0;
   UINT8 listIndex;
   rssi_qsort_t sortTable[WIFI_SCAN_SORT_TABLE_SIZE];

   debug_scan_printf(debug_putc, "\r\n_WifiScanResultsToEE %u ", num);

   if (num == 0xFF)
   {
      EEWriteBytes(EE_LOC_WIFI_SCAN_RESULTS, &num, 1);
   }
   else
   {   
      loc = EE_LOC_WIFI_SCAN_RESULTS + 1;
      
      listIndex = 0;
      
      memset(sortTable, 0x00, sizeof(sortTable));
      
      while ((listIndex < num) && (listIndex < WIFI_SCAN_SORT_TABLE_SIZE))
      {
         //debug_scan_printf(debug_putc, "\r\nWF_ScanGetResult %u ... ", listIndex);
         
         WF_ScanGetResult(listIndex, &g_WifiScanScrResult);
         
         //debug_scan_printf(debug_putc, "DONE! ");
         
         sortTable[listIndex].index = listIndex;
         sortTable[listIndex].rssi = g_WifiScanScrResult.rssi;
         
         listIndex++;
      }
      
      _WifiRssiQsortFunc(sortTable, listIndex, sizeof(rssi_qsort_t));
      
      listIndex = 0;
      
      while ((listIndex < num) && (numSaved < EE_NUM_WIFI_SCAN_RESULTS))
      {
         //debug_scan_printf(debug_putc, "\r\nWF_ScanGetResult SORTED %u ... ", listIndex);
         
         WF_ScanGetResult(sortTable[listIndex].index, &g_WifiScanScrResult);
         
         //debug_scan_printf(debug_putc, "DONE! ");

         if (g_WifiScanScrResult.ssidLen)
         {
            _WIFIScanNormalizeValues();   

            //debug_scan_printf(debug_putc, "\r\nSaving %u (%u) '%s' ", listIndex, sortTable[listIndex].index, g_WifiScanScrResult.ssid);
           
            EEWriteBytes(loc, &g_WifiScanScrResult, sizeof(tWFScanResult));
            
            loc += EE_SIZE_WIFI_SCAN_RESULT;
                        
            numSaved++;
         }
         
         listIndex++;
      }
      
      debug_scan_printf(debug_putc, "\r\nDone, saved %u ", numSaved);
      
      EEWriteBytes(EE_LOC_WIFI_SCAN_RESULTS, &numSaved, 1);
   }
}
#endif

static int1 WIFIScanMACIsReady(void)
{
   int1 ret;
  #if defined(STACK_USE_TCP)
   unsigned int8 tcpSock = 0;
  #endif
   
   ret = MACIsTxReady();
   
  #if defined(STACK_USE_TCP)
   while(ret && (tcpSock < TCP_CONFIGURATION))
   {
      if (TCPIsConnected(tcpSock) && !TCPIsPutReady(tcpSock))
      {
         ret = FALSE;
      }
      tcpSock++;
   }
  #endif
   
   return(ret);
}

static void _WIFIScanResultHandleRSSI(void)
{
   WF_ScanGetResult(0, &g_WifiScanScrResult);
   _WIFIScanNormalizeValues();
   g_WifiScanRssiValue = g_WifiScanScrResult.rssi;
   
   g_WifiScanState = WIFI_SCAN_STATE_IDLE;
   
   g_WifiScanRssiTick = TickGet();
   g_WifiScanRssiDuration = STACK_USE_CCS_SCAN_TASK_RSSI_PERIOD_SUCCESS;
}

#if defined(CCS_WIFISCAN_NO_RSSI)
   #define g_WIFIScanEnableBackgroundRSSI FALSE
#else
   int1 g_WIFIScanEnableBackgroundRSSI;
#endif

void WIFIScanEnableBackgroundRSSI(int1 enable)
{
  #if !defined(CCS_WIFISCAN_NO_RSSI)
   g_WIFIScanEnableBackgroundRSSI = enable;
   
   debug_scan_printf(debug_putc, "\r\nWIFIScanEnableBackgroundRSSI %u dis=%u cpid=%X ", 
         enable,
         g_WifiScanRssiDisable,
         g_connectionProfileID,
     );   
  #endif
}

void WIFIScanTask(void)
{
   unsigned int8 WIFI_channelList[16];
   unsigned int8 WIFI_numChannelsInList;
   
  #if 0
   static UINT8 debug = 0xFF;
   
   if (g_WifiScanState != debug)
   {
      debug_scan_printf(debug_putc, "\r\nWIFIScanTask %x -> %x ", debug, g_WifiScanState);
      debug = g_WifiScanState;
   }
  #endif
  
  #if defined(EE_LOC_WIFI_SCAN_RESULTS)
   if (g_WifiScanState == WIFI_SCAN_SAVING_AP_TO_EE)
   {
      _WifiScanResultsToEE(g_WifiScanToEE);
   
      g_WifiScanState = WIFI_SCAN_FINISH_AP;
   }
  #elif defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
   if (g_WifiScanState == WIFI_SCAN_SAVING_AP_TO_MALLOC)
   {
      _WifiScanResultsToMalloc();
   
      g_WifiScanState = WIFI_SCAN_FINISH_AP;
   }
  #endif

   if (g_WifiScanState == WIFI_SCAN_FINISH_AP)
   {
      debug_scan_printf(debug_putc, "\r\nWIFI_SCAN_FINISH_AP going to IDLE ");
      
      WIFI_numChannelsInList = WIFICreateChannelArray(WIFI_channelList);
      WF_CASetChannelList(WIFI_channelList, WIFI_numChannelsInList);
      
      /* don't need to do this because WIFIConnectStart() resends it?
     #if defined(WF_SOFT_AP)
      if (g_WifiScanNetworkTypeNeedsFixing)
      {
         debug_scan_printf(debug_putc, "\r\n_WIFIAPScanStart() putting radio into user mode");
         WF_CPSetNetworkType(g_connectionProfileID, AppConfig.networkType);
         g_WifiScanNetworkTypeNeedsFixing = FALSE;
      }
     #endif
      */
     #if !defined(CCS_WIFISCAN_DONT_AUTO_CONNECT)
      WIFIConnectStart();
     #endif
      g_WifiScanState = WIFI_SCAN_STATE_IDLE;
   }

   if (g_WifiScanState == WIFI_SCAN_READ_RSSI)
   {
      _WIFIScanResultHandleRSSI();
   }

   if (g_WifiScanState != WIFI_SCAN_STATE_IDLE)
   {
      return;
   }
   
   if (g_WifiScanApWhenReady && WIFIScanMACIsReady())
   {
      if 
      (
         !g_WifiScanApDelayedDo ||
         (
            g_WifiScanApDelayedDo &&
            ((TickGet() - g_WifiScanApDelayedStart) >= g_WifiScanApDelayedDuration)
         )
      )
      {
         _WIFIAPScanStart();
      }
   }
  #if !defined(CCS_WIFISCAN_NO_RSSI)
   else if
   (
      g_WIFIScanEnableBackgroundRSSI &&
      !g_WifiScanRssiDisable &&
      (g_connectionProfileID != 0xFF) && 
     #if !defined(CCS_WIFISCAN_DURING_ADHOC)
      (AppConfig.networkType != WF_ADHOC) &&
     #endif
     #if defined(WF_SOFT_AP)
      (AppConfig.networkType != WF_SOFT_AP) &&
     #endif
      ((TickGet() - g_WifiScanRssiTick) >= g_WifiScanRssiDuration)
   )
   {
      if (MACIsLinked() && WIFIScanMACIsReady())
      {
         if (WF_Scan(g_connectionProfileID) == WF_SUCCESS)
         {
            g_WifiScanState = WIFI_SCAN_DOING_RSSI;
            g_WifiScanRssiDuration = STACK_USE_CCS_SCAN_TASK_RSSI_PERIOD_SUCCESS;
            debug_scan_printf(debug_putc, "\r\nPeriodic scan start. ");
         }
         else
         {
            g_WifiScanRssiValue = 0xFF;
            g_WifiScanRssiDuration = STACK_USE_CCS_SCAN_TASK_RSSI_PERIOD_FAIL;
            debug_scan_printf(debug_putc, "\r\nPeriodic scan fail. ");
         }
      }
      else
      {
         debug_scan_printf(debug_putc, "\r\nPeriodic scan NOT READY %X %X ", MACIsLinked(), WIFIScanMACIsReady());
      }
      g_WifiScanRssiTick = TickGet();
   }
   else if
   (
      (g_connectionProfileID == 0xFF) ||
      !MACIsLinked()
   )
   {
      //cause an immediate scan once we connect
      g_WifiScanRssiTick = TickGet();
      g_WifiScanRssiDuration = 0;
   }
  #endif
}

void WIFIScanIgnoreResults(void)
{
   // abort any pending results and ignore any incoming results.
   g_WifiScanState = WIFI_SCAN_STATE_IDLE;
}

static void _WIFIScanResults(unsigned int8 num, int1 valid)
{  
   if (g_WifiScanState == WIFI_SCAN_DOING_RSSI)
   {
      if ((num == 0) || !valid)
      {
         if (!valid)
         {
            g_WifiScanRssiValue = 0xFF;
         }
         
         g_WifiScanState = WIFI_SCAN_STATE_IDLE;
         g_WifiScanRssiDuration = STACK_USE_CCS_SCAN_TASK_RSSI_PERIOD_FAIL;
      }
      else
      {
         g_WifiScanState = WIFI_SCAN_READ_RSSI;
      }
   }
   else if (g_WifiScanState == WIFI_SCAN_DOING_AP)
   {
      if (!valid)
      {
         num = 0xFF;
      }
      
#if defined(EE_LOC_WIFI_SCAN_RESULTS)
      g_WifiScanState = WIFI_SCAN_SAVING_AP_TO_EE;
      g_WifiScanToEE = num;
#elif defined(CCS_WIFI_SCAN_RESULTS_MALLOC)
      g_WifiScanState = WIFI_SCAN_SAVING_AP_TO_MALLOC;
      _g_WifiScanToMalloc.numTotal = num;
#else
      //g_WifiScanState = WIFI_SCAN_STATE_IDLE;
      g_WifiScanState = WIFI_SCAN_FINISH_AP; //new 09/10/2015
      g_WifiScanApNum = num;
#endif
   }
}

void WIFIScanResults(unsigned int8 num)
{
   _WIFIScanResults(num, TRUE);
}

#endif
