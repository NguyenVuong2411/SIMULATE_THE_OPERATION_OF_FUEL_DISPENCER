// 'glue' file for using Microchip's TCP/IP stack inside CCS C Compiler without
// a linker.  
// Also includes some macros/defines for porting older V3 stack to this current
// stack.
// Also provides extra routines written by CCS to improve the stack.
//
// __DISABLE_WIFI_INTERRUPT__ was added but it never worked right; it would
// randomly miss events.

#ifndef __CCS_STACKTSK2_C__
#define __CCS_STACKTSK2_C__

APP_CONFIG AppConfig;

#if defined(__PCH__)
#if !defined(TICK_ISR_POLLING)
#int_timer0 NOCLEAR
void ISRTimer0(void)
{
   TickUpdate();
}
#endif
#endif

#ifndef WF_INT_CCS_ISR
   #if defined(__PCD__)
      #define WF_INT_CCS_ISR   INT_EXT0
   #else
      #define WF_INT_CCS_ISR   INT_EXT
   #endif
#endif

#if defined(WF_CS_TRIS) && !defined(__DISABLE_WIFI_INTERRUPT__)
   #if defined(INT_EXT) && (WF_INT_CCS_ISR==INT_EXT)
      #int_ext HIGH NOCLEAR
   #elif defined(INT_EXT0) && (WF_INT_CCS_ISR==INT_EXT0)
      #int_ext0 HIGH NOCLEAR      
   #elif defined(INT_EXT1) && (WF_INT_CCS_ISR==INT_EXT1)
      #int_ext1 HIGH NOCLEAR      
   #elif defined(INT_EXT2) && (WF_INT_CCS_ISR==INT_EXT2)
      #int_ext2 HIGH NOCLEAR
   #elif defined(INT_EXT3) && (WF_INT_CCS_ISR==INT_EXT3)
      #int_ext3 HIGH NOCLEAR
   #elif defined(INT_EXT4) && (WF_INT_CCS_ISR==INT_EXT4)
      #int_ext4 HIGH NOCLEAR
   #else
      #error do this
   #endif
   void ISREXT0(void)
   {
      #if defined(__PCD__)
         WF_INT_IF = 0;
         WF_INT_IE = 0;
         WFEintHandler();
      #else
         WFEintISR();
      #endif
   }
#endif

#if defined(WIFI_ADHOC_CONNECTION_TIMER) && (defined(WF_ADHOC) || defined(WF_SOFT_AP))
   int1 g_WifiAdhocIsConn = FALSE;
   TICK g_WifiAdhocTickConn;
   #define WIFIAdhocConnectionPing()   g_WifiAdhocIsConn=TRUE; g_WifiAdhocTickConn=TickGet()
   #define WIFIAdhocConnectionReset()  g_WifiAdhocIsConn = FALSE
#endif

#ifndef wf_debug_printf
#define wf_debug_printf(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t)
#endif

#ifndef wf_debug_putc
#define wf_debug_putc(c)
#endif

#ifndef wf_debug_array
#define wf_debug_array(p, n)
#endif

static void _WIFICreateConnectionProfile(void);

#if defined(STACK_USE_SNMPV3_SERVER)
#include <stdlibm.h>
#endif

#include "StackTsk.c"
#include "Helpers.c"
//#include "Delay.c" //not needed, we overrode this in StackTsk2.h
#include "Tick.c"
#if !defined(ENC_CS_TRIS) && !defined(WF_CS_TRIS) && !defined(ENC100_INTERFACE_MODE) && \
    (defined(__18F97J60) || defined(__18F96J65) || defined(__18F96J60) || defined(__18F87J60) || defined(__18F86J65) || defined(__18F86J60) || defined(__18F67J60) || defined(__18F66J65) || defined(__18F66J60) || \
     defined(_18F97J60) ||  defined(_18F96J65) ||  defined(_18F96J60) ||  defined(_18F87J60) ||  defined(_18F86J65) ||  defined(_18F86J60) ||  defined(_18F67J60) ||  defined(_18F66J65) ||  defined(_18F66J60))
   #include "ETH97J60.c"
#elif defined(WF_CS_TRIS)
      #include "WF_Config.c"
      #if defined(WF_USE_SCAN_FUNCTIONS)
         #include "WFScan.c"
      #endif
      #if defined(WF_USE_POWER_SAVE_FUNCTIONS)
         #include "WFPowerSave.c"
      #else
         BOOL GetAppPowerSaveMode(void) {return(TRUE);}
      #endif
      #if defined(WF_USE_TX_POWER_CONTROL_FUNCTIONS)
         #include "WFTxPower.c"
      #endif
      #include "WF_Spi.c"
      #include "WF_Eint.c"
      #include "WFConnectionProfile.c"
      #include "WFConnectionAlgorithm.c"
      #include "WFConnectionManager.c"
      #include "WFEventHandler.c"
      #include "WFInit.c"
     #if defined(STACK_USE_CCS_SCAN_TASK) && !defined(STACK_USE_BLUEGIGA_WF121)
      #include "ccs_wifiscan.c"
     #endif      
     #if defined(MRF24WG)
      #include "WFDriverCom_24G.c"
      #include "WFDriverRaw_24G.c"
      #include "WFMac_24G.c"
      #include "WFMgmtMsg_24G.c"
      #include "WFParamMsg_24G.c"     
     #else
      #include "WFDriverCom.c"
      #include "WFDriverRaw.c"
      #include "WFMac.c"
      #include "WFMgmtMsg.c"
      #include "WFParamMsg.c"
     #endif
#elif defined(ENC_CS_TRIS)
   #include "tcpip\ENC28J60.c"
#elif defined(ENC100_INTERFACE_MODE)
   #include "ENCX24J600.c"
   #define PHYREG WORD
#elif defined(__PIC32MX__) && defined(_ETH)
   // extra includes for PIC32MX with embedded ETH Controller
#elif defined(STACK_USE_BLUEGIGA_WF121)   
   #include "BlueGiga_WF121.c"
#else
   #error No Ethernet/WiFi controller defined in HardwareProfile.h.  Defines for an ENC28J60, ENC424J600/624J600, or ZeroG ZG2100 must be present.
#endif

#if !defined(STACK_EXT_MODULE_HAS_TCP_UDP)
   #include "IP.c"
   #include "ARP.c"
#endif

#if defined(STACK_USE_ICMP_SERVER) && !defined(STACK_EXT_MODULE_HAS_TCP_UDP)
   #include "ICMP.c"
#endif

#if defined(STACK_USE_UDP) && !defined(STACK_EXT_MODULE_HAS_TCP_UDP)
   #include "UDP.c"
#endif

#if defined(STACK_USE_DHCP_CLIENT) && !defined(STACK_EXT_MODULE_HAS_TCP_UDP)
   #include "DHCP.c"
#endif

#if defined(STACK_USE_DHCP_SERVER) && !defined(STACK_EXT_MODULE_HAS_TCP_UDP)
   #include "DHCPs.c"
#endif

#if defined(STACK_USE_NBNS)
   #include "NBNS.c"
#endif

#if defined(STACK_USE_DNS_SERVER) && !defined(STACK_EXT_MODULE_HAS_DNSS)
   #include "DNSs.c"
#endif

#if defined(STACK_USE_TCP) && !defined(STACK_EXT_MODULE_HAS_TCP_UDP)
   #include "TCP.c"
   
   void TCPTouch(TCP_SOCKET s)
   {
      SyncTCBStub(s);
      if(MyTCBStub.smState == TCP_ESTABLISHED)
      {
         MyTCBStub.eventTime = TickGet() + TCP_KEEP_ALIVE_TIMEOUT;
         SyncTCB();
      }
   }
#endif

#if defined(STACK_USE_DNS)  && !defined(STACK_EXT_MODULE_HAS_TCP_UDP)
   #include "_DNS.c" //ccs had to rename this driver to not conflict an s7600 driver in the default include path - a bug in the compiler
#endif

#if defined(STACK_USE_MPFS)
   #include "MPFS.c"
#endif

#if (defined(STACK_USE_HTTP2) || defined(STACK_USE_HTTP2_SERVER)) && !defined(__HTTP2_C)
    #include "HTTP2.c"
#endif

#if defined(STACK_USE_HTTP) || defined(STACK_USE_HTTP_SERVER)
    #include "HTTP.c"
#endif

#if defined(STACK_USE_CCS_HTTP1_SERVER)
   #include "ccs_HTTP.c"
#endif

#if defined(STACK_USE_CCS_HTTP2_SERVER)
   #include "ccs_HTTP2.c"
#endif

#if defined(STACK_USE_SMTP_CLIENT)
   #include "smtp.c"
#endif

#if defined(STACK_USE_CCS_SMTP)
   #include "ccs_SMTP.c"
#endif

#if defined(STACK_USE_SNTP_CLIENT)
   #include "SNTP.c"
#endif

#if defined(STACK_USE_CCS_SNTP_CLIENT)
   #include "ccs_SNTP.c"
#endif

#if defined(STACK_USE_TWITTER)
   #include "ccs_twitter.c"
#endif

#if defined(STACK_USE_TFTP_CLIENT)
   #include "TFTPc.c"
#endif

#if defined(STACK_USE_CCS_TFTP_SERVER)
   #include "ccs_tftps.c"
#endif

#if defined(STACK_USE_CCS_TFTP_CLIENT)
   #include "ccs_tftpc.c"
#endif

#if defined(STACK_USE_CCS_EMAIL_ALERTS)
   #include "ccs_email_alert.c"
#endif

#if defined(STACK_USE_ANNOUNCE) && !defined(__ANNOUNCE_C)
   #include "Announce.c"
#endif

#if defined(STACK_USE_CCS_ANNOUNCE) && !defined(__CCS_ANNOUNCE_C)
   #include "ccs_announce.c"
#endif

#if defined(STACK_USE_MPFS2)
   #include "MPFS2.c"
#endif

#if defined(STACK_USE_CCS_GRATUITOUS_ARP)
   #include "ccs_gratarp.c"
#endif

#if defined(STACK_USE_CCS_HTTP_CLIENT)
   #include "ccs_http_client.c"
#endif

#if defined(STACK_USE_CCS_FTP_SERVER)
   #include "ccs_ftp_mdd.c"
#endif

#if defined(__18CXX)
   #include "p18cxxx.c"
#endif

#if defined(STACK_USE_SNMP_SERVER)
   #include "SNMP.c"
   
   #if defined(STACK_USE_SNMPV3_SERVER)
      #include "SNMPv3.c"
      #include "SNMPv3USM.c"
      
      #if defined(__PCD__)
         #include "ccs_aes.c"
      #endif
   #endif
#endif

#if defined(STACK_USE_SSL)
   #include "SSL.c"
#endif

#if defined(STACK_USE_RSA_DECRYPT) || defined(STACK_USE_RSA_ENCRYPT)
   #include "RSA.c"
#endif

#if defined(STACK_USE_ARCFOUR)
   #include "ARCFOUR.c"
#endif

#if defined(STACK_USE_BIGINT)
   #include "BigInt.c"
   #include "BigInt_Helper.c"
#endif

#if defined(STACK_USE_RANDOM)
   #include "Random.c"
#endif

#if defined(STACK_USE_MD5) || defined(STACK_USE_SHA1)
   #include "Hashes.c"
#endif

#if defined(STACK_USE_CCS_COAP)
   #include "ccs_coap.c"
#endif

int1 DHCPBoundOrDisabled(void)
{
  #if defined(STACK_USE_DHCP_CLIENT)
   return(!DHCPIsEnabled(0) || DHCPIsBound(0));
  #else
   return(TRUE);
  #endif
}

int1 IsLinked(void)
{
  #if STACK_USE_WIFI && defined(WIFI_ADHOC_CONNECTION_TIMER) && (defined(WF_ADHOC) || defined(WF_SOFT_AP))
   if (AppConfig.networkType != WF_INFRASTRUCTURE)
   {
      return(MACIsLinked() && g_WifiAdhocIsConn);
   }
  #endif
  
  #if STACK_USE_WIFI && defined(STACK_USE_BLUEGIGA_WF121)
   if (AppConfig.networkType == WF_SOFT_AP)
   {
      return(_g_BGWF121.numClients > 0);
   }
  #endif
  
   return(MACIsLinked() && DHCPBoundOrDisabled());
}

#if defined(STACK_USE_MPFS)
MPFS _MpfsEofLoc;

unsigned int16 MPFSGetBytes(unsigned int8 *pDest, unsigned int16 n)
{
   unsigned int16 ret = 0;
   unsigned int16 i;
   unsigned int8 *p;
   int1 escaped = FALSE;
   
   p = pDest;
   
   debug_mpfs(debug_putc, "\r\nMPFSGetBytes() START %LX ", _currentHandle);
   
   PSV_SAVE();
   
   while(n)
   {
     #if defined(HTTP_INTERRUPT_TASKS)
      HTTP_INTERRUPT_TASKS();
     #endif
      
      debug_mpfs(debug_putc, "n=%LX ", n);
      
      //_MpfsEofLoc = _currentHandle;
     #if defined(MPFS_USE_SPI_FLASH)
      SPIFlashReadArray(_currentHandle, pDest, n);
     #else
      PSV_GOTO(_currentHandle);
      memcpypgm2ram(pDest, _currentHandle, n); //__ccs__ change because MPFS isn't rom pointer
     #endif

      //_currentHandle += n;  //removed when removing _MpfsEofLoc

      //debug_array("START", pDest, n);

      i = n;
      n = 0;
      
      while(i--)
      {
         _currentHandle++; //_MpfsEofLoc++;
         if (escaped || (*pDest == MPFS_DLE))
         {
            if (!escaped)
            {
               n++;
            }
            
            if (escaped || i)
            {
               if (!escaped)
               {
                  debug_mpfs(debug_putc, "! ");
                  memmove(pDest, pDest+1, i);
                  _currentHandle++;   //_MpfsEofLoc++;
                  i--;
               }
               ret++;
               escaped = FALSE;
            }
            else
            {
               escaped = TRUE;
               pDest--; //counter pDest++ below, since we need to start saving new bytes here
            }
         }
         else if(*pDest == MPFS_ETX)
         {
            _MpfsEofLoc = _currentHandle - 1;   //_MpfsEofLoc--;
            debug_mpfs(debug_putc, "EOF %LX ", _currentHandle);  //debug_mpfs2(debug_putc, "EOF %LX ", _MpfsEofLoc);
            _currentHandle = MPFS_INVALID;
            n = 0;
            break;
         }
         else
         {
            ret++;
         }
         pDest++;
      }
   }
   
   if (_currentHandle != MPFS_INVALID)
      _MpfsEofLoc = _currentHandle;
  
   //debug_array("STOP", p, ret);   

   PSV_RESTORE();
   
   return(ret);
}
#ENDIF

#if STACK_USE_WIFI
//unsigned int8 WIFI_channelList[16];
//unsigned int8 WIFI_numChannelsInList = 0;
unsigned int16 g_WifiChannelListBitmap;
unsigned int8 WIFI_region;

#if defined(WF_ADHOC)
unsigned int8 WIFI_adhocBehavior;
#endif
#endif

#if STACK_USE_WIFI && !defined(STACK_USE_BLUEGIGA_WF121)
extern int1 g_WifiConnectFail;

unsigned int8 g_connectionProfileID = 0xFF;  //used to communicate with the connection profile on the WiFi module

// this option will force the connection profile to be closed and reopened
// if we do not get a connection within this time.  might fix an errata that
// says WPA/WPA2 will lockup instead of sending an event.
#define WIFI_FORCE_RECONNECT_TICK   (TICK)(TICKS_PER_SECOND * 120)  //might be too short for WPA/WPA2 phrase

#if defined(WIFI_GET_VERSION_INFO)
extern tWFDeviceInfo WIFI_GET_VERSION_INFO;
#endif

int1 g_WIFIConnectStart;

// creates the channel array list used by the WFconnect function.
// returns the number of channels saved to p.
// p must be able to save 16 bytes.
static unsigned int8 WIFICreateChannelArray(unsigned int8 *p)
{
   unsigned int8 ret = 0;
   unsigned int16 mask = 1;
   int i;
   
   memset(p, 0, 16);
   
   for(i=0; i<16; i++)
   {
      if ((mask & g_WifiChannelListBitmap) != 0)
      {
         ret++;
         *p++ = (i+1);
      }
      
      mask *= 2;
   }
   
   return(ret);
}

void WIFIConnectInitStates(void)
{
   unsigned int8 WIFI_channelList[16];
   unsigned int8 WIFI_numChannelsInList;
   
   wf_debug_printf(wf_debug_putc, "\r\nWIFIConnectInitStates");
   
   g_connectionProfileID = 0xFF;
   
   g_WIFIConnectStart = TRUE;
   
   g_WifiConnectFail = FALSE;   
   
  #if defined(STACK_USE_CCS_SCAN_TASK)
   WIFIScanInit();
  #endif

        #if defined(WIFI_GET_VERSION_INFO)
         WF_GetDeviceInfo(&WIFI_GET_VERSION_INFO);
        #endif 

         WF_CASetScanType(MY_DEFAULT_SCAN_TYPE);   // was WF_ACTIVE_SCAN

         WIFI_numChannelsInList = WIFICreateChannelArray(WIFI_channelList);
         WF_CASetChannelList(WIFI_channelList, WIFI_numChannelsInList); //was AppConfig.channelList, AppConfig.numChannelsInList

         WF_SetRegionalDomain(WIFI_region);  //was AppConfig.region
         
         WF_CASetListRetryCount(MY_DEFAULT_LIST_RETRY_COUNT);  // was WF_RETRY_FOREVER
         
         WF_CASetEventNotificationAction(MY_DEFAULT_EVENT_NOTIFICATION_LIST); // was WF_NOTIFY_ALL_EVENTS
         
         WF_CASetBeaconTimeout(MY_DEFAULT_BEACON_TIMEOUT);

         WF_CASetBeaconTimeoutAction(WF_ATTEMPT_TO_RECONNECT);
         
         WF_CASetDeauthAction(WF_ATTEMPT_TO_RECONNECT);

       #if defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST)
           if (AppConfig.SecurityMode == WF_SECURITY_WPA_WITH_PASS_PHRASE
               || AppConfig.SecurityMode == WF_SECURITY_WPA2_WITH_PASS_PHRASE
               || AppConfig.SecurityMode == WF_SECURITY_WPA_AUTO_WITH_PASS_PHRASE) {
               WF_ConvPassphrase2Key(AppConfig.SecurityKeyLength, AppConfig.SecurityKey,
                   AppConfig.SsidLength, AppConfig.MySSID);
               AppConfig.SecurityMode--;
               AppConfig.SecurityKeyLength = 32;
           }
       #if defined (MRF24WG)
           else if (AppConfig.SecurityMode == WF_SECURITY_WPS_PUSH_BUTTON
                       || AppConfig.SecurityMode == WF_SECURITY_WPS_PIN) {
               WF_YieldPassphrase2Host();    
           }
       #endif    // defined (MRF24WG)
       #endif    // defined(DERIVE_KEY_FROM_PASSPHRASE_IN_HOST)

     #ifndef WF_FORCE_NO_PS_POLL
      #if MY_DEFAULT_PS_POLL == WF_ENABLED
           WF_PsPollEnable(TRUE);
       #if !defined(MRF24WG) 
           if (gRFModuleVer1209orLater)
               WFEnableDeferredPowerSave();
       #endif    // !defined(MRF24WG)
      #else     // MY_DEFAULT_PS_POLL != WF_ENABLED
           WF_PsPollDisable();
      #endif    // MY_DEFAULT_PS_POLL == WF_ENABLED
   
      #ifdef WF_AGGRESSIVE_PS
       #if !defined(MRF24WG)
           if (gRFModuleVer1209orLater)
               WFEnableAggressivePowerSave();
       #endif
      #endif
     #endif 
     
  //#if defined(WIFI_GET_VERSION_INFO)
   //wf_debug_printf(wf_debug_putc, " ver=%X.%X.%X", WIFI_GET_VERSION_INFO.b[0], WIFI_GET_VERSION_INFO.b[1], WIFI_GET_VERSION_INFO.b[2]);
  //#endif
   wf_debug_printf(wf_debug_putc, " REG=%u CHANS=%u ", WIFI_region, WIFI_numChannelsInList);
   wf_debug_array((unsigned int8*)WIFI_channelList, WIFI_numChannelsInList);
}

void WIFIConnectStop(void)
{
   if (!MACIsLinked() || (g_connectionProfileID != 0xFF))
   {
      wf_debug_printf(wf_debug_putc, "\r\n\n*** Reset of MAC because WIFIConnectStop()\r\n");
      MACInit();
   }
  #if defined(WF_DEBUG)
   else
   {
      wf_debug_printf(wf_debug_putc, "\r\nWIFIConnectStop");
   }
  #endif
   
   g_WIFIConnectStart = FALSE;
}

void WIFIConnectStart(void)
{
   wf_debug_printf(wf_debug_putc, "\r\nWIFIConnectStart");
   
   g_WIFIConnectStart = TRUE;
}

#if defined(APP_GENERIC_SCRATCH)
#error This should be called GENERIC_SCRATCH_BUFFER
#endif

// create connection profile at g_connectionProfileID and then send
// all of our WIFI settings to that connection profile
static void _WIFICreateConnectionProfile(void)
{
   #if !defined(GENERIC_SCRATCH_BUFFER)
      char WIFIConnectTask_Scratch[6];
   #else
      #define WIFIConnectTask_Scratch  GENERIC_SCRATCH_BUFFER
   #endif
   
   WF_CPCreate(&g_connectionProfileID);
   
   wf_debug_printf(wf_debug_putc, "\r\nCreating connect profile... ");
   
   if (g_connectionProfileID != 0xFF)
   {
      wf_debug_printf(wf_debug_putc, "%U ", g_connectionProfileID);

      wf_debug_printf(wf_debug_putc, "SSID='%s'(%u) TYPE=%u SEC=%u KEY=(%u) AdBeh=%U chans=%LX ", 
            AppConfig.MySSID,
            AppConfig.SsidLength,
            AppConfig.networkType,
            AppConfig.SecurityMode,
            AppConfig.SecurityKeyLength,
            WIFI_adhocBehavior,
            g_WifiChannelListBitmap
         );
      wf_debug_array((unsigned int8*)AppConfig.SecurityKey, AppConfig.SecurityKeyLength);
      
      g_WifiConnectFail = TRUE;

      WF_CPSetSsid(g_connectionProfileID, 
                 AppConfig.MySSID, 
                 AppConfig.SsidLength);
      
      memset(WIFIConnectTask_Scratch, 0xFF, 6);
      WF_CPSetBssid(g_connectionProfileID, WIFIConnectTask_Scratch);

      WF_CPSetNetworkType(g_connectionProfileID, AppConfig.networkType);
      
      if(AppConfig.networkType == WF_ADHOC)
      {
         WF_CPSetAdHocBehavior(g_connectionProfileID, WIFI_adhocBehavior);
      }

      WF_CPSetSecurity(g_connectionProfileID,
                     AppConfig.SecurityMode,
                     0, //AppConfig.WepKeyIndex, //AppConfig.WepKeyIndex,   /* only used if WEP enabled */
                     AppConfig.SecurityKey,
                     AppConfig.SecurityKeyLength);      

   #if (MY_DEFAULT_WIFI_SECURITY_WEP_KEYTYPE!=WF_SECURITY_WEP_SHAREDKEY)
     #if !defined(MRF24WG)
      if (gRFModuleVer1209orLater)
     #endif
      {
         // If WEP security is used, set WEP Key Type.  The default WEP Key Type is Shared Key.
         if ((AppConfig.SecurityMode == WF_SECURITY_WEP_40) || (AppConfig.SecurityMode == WF_SECURITY_WEP_104))
         {
             WF_CPSetWepKeyType(g_connectionProfileID, MY_DEFAULT_WIFI_SECURITY_WEP_KEYTYPE);
         }
      }
  #endif
   }
}

// this was based off of WF_Connect() in Microchip's demo application.
void WIFIConnectTask(void)
{   
  #if defined(WIFI_FORCE_RECONNECT_TICK)
   static TICK fTick;
  #endif

  #if defined(LED_CONNECTION_ON) || defined(LED_ACTIVITY_ON)
   static TICK l;
   // if you have both LED_CONNECTION_ON and LED_ACTIVITY_ON (2 WIFI LEDs), 
   //    then one LED is used for flickering on traffic and another is used to 
   //    show connection state.
   // if you only have LED_CONNECTION_ON (only 1 WIFI LED), then WIFI 
   //    connection status and wifi traffic flickering is shared on the same
   //    LED.
   // this routine below handles the traffic led flickering.  the LED was
   //    turned off in the TCP/IP stack, this routine turns it back on every
   //    200ms.
   if ((TickGet() - l) >= TICKS_PER_SECOND/5)
   {
      l = TickGet();
      
     #if defined(LED_ACTIVITY_ON)
      LED_ACTIVITY_OFF();
     #else
      if (IsLinked())
      {
         LED_CONNECTION_ON();
      }
      else
      {
         LED_CONNECTION_OFF();
      }
     #endif
   }
  #endif

  #if defined(STACK_USE_CCS_SCAN_TASK)
   WIFIScanTask();
  #endif

  #if defined(LED_CONNECTION_ON) && defined(LED_ACTIVITY_ON)
   // handle the connection status LED if you have 2 WIFI LEDs.
   if (IsLinked())
   {
      LED_CONNECTION_ON();
   }
   else
   {
      LED_CONNECTION_OFF();
   }
  #endif

  #if STACK_USE_WIFI && defined(WIFI_ADHOC_CONNECTION_TIMER) && (defined(WF_ADHOC) || defined(WF_SOFT_AP))
   if (AppConfig.networkType != WF_INFRASTRUCTURE)
   {
      if (
            g_WifiAdhocIsConn &&
            ((TickGet() - g_WifiAdhocTickConn) > WIFI_ADHOC_CONNECTION_TIMER)
         )
      {
         g_WifiAdhocIsConn = FALSE;
      }
   }
  #endif

  #if defined(WIFI_FORCE_RECONNECT_TICK)
   if (
         (AppConfig.networkType == WF_INFRASTRUCTURE) &&
         (g_connectionProfileID != 0xFF)
      )
   {
      if (IsLinked())
      {
         fTick = TickGet();
      }
      else if ((TickGet() - fTick) > WIFI_FORCE_RECONNECT_TICK)
      {
        #if defined(STACK_IS_CCS_EXAMPLE)
         fprintf(STREAM_UI, "\r\nLong WIFI connection timeout, reseting module");
        #endif
         MACInit();
        #if defined(WIFI_EXTRA_LINK_STATUS)
         g_LastWifiEvent = 20;
        #endif
      }
   }
  #endif

   if ((g_connectionProfileID == 0xFF) && g_WIFIConnectStart)
   {
      g_WifiConnectFail = FALSE;
      
      _WIFICreateConnectionProfile();
   }

   if(g_WifiConnectFail && g_WIFIConnectStart)
   {
      wf_debug_printf(wf_debug_putc, "\r\nStart WiFi Connect PROF=%u ", g_connectionProfileID);
     
     #if defined(WIFI_FORCE_RECONNECT_TICK)
      fTick = TickGet();
     #endif
     
     #if defined(STACK_USE_CCS_SCAN_TASK)
      WIFIScanIgnoreResults();
     #endif    

     #if defined(WF_FORCE_NO_PS_POLL)
      WF_CCS_PsPollDisable();
     #endif
   
      WF_CMConnect(g_connectionProfileID);
   }
   
   g_WifiConnectFail = 0;
}

#if defined(WF_FORCE_NO_PS_POLL)
#if defined(WF_USE_POWER_SAVE_FUNCTIONS)
   #error dont use both at the same time
#endif
typedef struct pwrModeRequestStruct
{
    UINT8 mode;
    UINT8 wake;
    UINT8 rcvDtims;
    UINT8 reserved;            /* pad byte */
} tWFPwrModeReq;

static void SendPowerModeMsg(tWFPwrModeReq *p_powerMode)
{
    UINT8 hdr[2];
    
    hdr[0] = WF_MGMT_REQUEST_TYPE;
    hdr[1] = WF_SET_POWER_MODE_SUBTYPE;

    SendMgmtMsg(hdr,
                sizeof(hdr),
               (UINT8 *)p_powerMode,
               sizeof(tWFPwrModeReq));

    /* wait for mgmt response, free buffer after it comes in (no data to read) */
   WaitForMgmtResponse(WF_SET_POWER_MODE_SUBTYPE, FREE_MGMT_BUFFER);
    
} 

void WF_CCS_PsPollDisable(void)
{
   tWFPwrModeReq   pwrModeReq;
    
    pwrModeReq.mode     = 1;  //PS_POLL_DISABLED;
    pwrModeReq.wake     = 1;
    pwrModeReq.rcvDtims = 1;
    SendPowerModeMsg(&pwrModeReq);

    //SetPowerSaveState(WF_PS_OFF); //not really needed, sets a global for their statemachine

    WFConfigureLowPowerMode(WF_LOW_POWER_MODE_OFF); 
}
#endif   //WF_FORCE_NO_PS_POLL
#endif //STACK_USE_WIFI && !defined(STACK_USE_BLUEGIGA_WF121)

#if STACK_USE_WIFI
void LinkTraffic(int1 isTx)
{
  #if defined(WIFI_ADHOC_CONNECTION_TIMER) && (defined(WF_ADHOC) || defined(WF_SOFT_AP))
   if (!isTx && (AppConfig.networkType != WF_INFRASTRUCTURE))
   {
      g_WifiAdhocIsConn = TRUE;
      g_WifiAdhocTickConn = TickGet();
   }
  #endif

   // these LEDs are desribed in WIFIConnectTask()
  #if defined(LED_ACTIVITY_ON)
   LED_ACTIVITY_ON();
  #elif defined(LED_CONNECTION_ON)
   LED_CONNECTION_OFF();
  #endif
}
#endif   //STACK_USE_WIFI

#if STACK_USE_WIFI && defined(STACK_USE_BLUEGIGA_WF121)
void WIFIConnectTask(void) {} //handled in WF121's StackTask()
#endif   //STACK_USE_WIFI && defined(STACK_USE_BLUEGIGA_WF121)

#if defined(__DO_DEBUG_MPFS)
void debug_array_mpfs(char *str, unsigned int8 *p, unsigned int16 n)
{
   unsigned char c;
   debug_mpfs(debug_putc, "\r\n%s %LX ", str, n);
   while(n--)
   {
      c = *p++;
      if ((c >= ' ') && (c <= '}'))
      {
         debug_mpfs(debug_putc, "%c", c);
      }
      else
      {
         debug_mpfs(debug_putc, "+%X", c);
      }
   }
   debug_mpfs(debug_putc, " ");
}
#else
#define debug_array_mpfs(str, p, n)
#endif

// if you call TickGet() from an ISR, this may be a lock-up condition.
// this is a morph of GetTickCopy() and TickUpdate() that works from an ISR.
// only works on PIC18.
#if defined(__PCH__)
TICK TickGetSafe(void)
{
   BYTE myTickReading[6];
   do
   {
      if(interrupt_active(INT_TIMER0))
      {
         dwInternalTicks++;
         clear_interrupt(INT_TIMER0);
      }
      myTickReading[0] = TMR0L;
      myTickReading[1] = TMR0H;
      *((DWORD*)&myTickReading[2]) = dwInternalTicks;
   } while(interrupt_active(INT_TIMER0));
   
   return *((DWORD*)&myTickReading[0]);
}
#endif

// this used to be called GenerateRandomBYTE() in an older CCS port of Helpers.c
BYTE GenerateRandomByteFromTimers(void)
{
   BYTE rand;
   
  #if getenv("TIMER0") && !defined(__PCD__)
   rand+=get_timer0();
  #endif
  
  #if getenv("TIMER1")
   rand+=get_timer1();
  #endif
  
  #if getenv("TIMER2")
   rand+=get_timer2();
  #endif
  
  #if getenv("TIMER3")
   rand+=get_timer3();
  #endif
  
  #if getenv("TIMER4")
   rand+=get_timer4();
  #endif
  
  #if getenv("TIMER5")
   rand+=get_timer5();
  #endif
  
   return(rand);
}

// this function was written by Microchip and taken from their
// CustomSNMPApp.c file.  it is supposed to return a timestamp since power-up,
// in milliseconds.
static DWORD GetTimeStampMS(void)
{

   DWORD_VAL dwvHigh, dwvLow;
    DWORD dw;
    DWORD timeStamp;
    DWORD scr;
   
   //TimeStamp
   // Get all 48 bits of the internal Tick timer
    do
      {
         dwvHigh.Val = TickGetDiv64K();
         dwvLow.Val = TickGet();
   } while(dwvHigh.w[0] != dwvLow.w[1]);
    dwvHigh.Val = dwvHigh.w[1];
    
   // Find total contribution from lower DWORD
    dw = dwvLow.Val/(DWORD)TICK_SECOND;
    timeStamp = dw*100ul;
    dw = (dwvLow.Val - dw*(DWORD)TICK_SECOND)*100ul;      // Find fractional seconds and convert to 10ms ticks
    timeStamp += (dw+((DWORD)TICK_SECOND/2ul))/(DWORD)TICK_SECOND;

   // Itteratively add in the contribution from upper WORD
   while(dwvHigh.Val >= 0x1000ul)
   {
      //timeStamp += (0x100000000000ull*100ull+(TICK_SECOND/2ull))/TICK_SECOND;
      scr = (0xC8000000+(TICK_SECOND/2ull))/TICK_SECOND;
      scr *= (DWORD)19;
      timeStamp += scr;
      dwvHigh.Val -= 0x1000;
   }   
   while(dwvHigh.Val >= 0x100ul)
   {
      //timeStamp += (0x010000000000ull*100ull+(TICK_SECOND/2ull))/TICK_SECOND;
      scr = (0xC8000000+(TICK_SECOND/2ull))/TICK_SECOND;
      scr *= (DWORD)15;
      timeStamp += scr;
      dwvHigh.Val -= 0x100;
   }   
   while(dwvHigh.Val >= 0x10ul)
   {
      //timeStamp += (0x001000000000ull*100ull+(TICK_SECOND/2ull))/TICK_SECOND;
      scr = (0xC8000000+(TICK_SECOND/2ull))/TICK_SECOND;
      scr *= (DWORD)11;
      timeStamp += scr;
      dwvHigh.Val -= 0x10;
   }   
   while(dwvHigh.Val)
   {
      //timeStamp += (0x000100000000ull*100ull+(TICK_SECOND/2ull))/TICK_SECOND;
      scr = (0xC8000000+(TICK_SECOND/2ull))/TICK_SECOND;
      scr *= (DWORD)7;
      timeStamp += scr;
      dwvHigh.Val--;
   }
    
    return timeStamp;
}

#if defined(STACK_USE_SNMP_SERVER)
BYTE SNMPValidateCommunity(BYTE * community)
{
   BYTE i;
   BYTE *ptr;
   
   /*
   If the community name is encrypted in the request from the Manager,
   agent required to decrypt it to match with the community it is
   configured for. The response from the agent should contain encrypted community 
   name using the same encryption algorithm which Manager used while
   making the request.
   */       

   // Validate that community string is a legal size
   if(strlen((char*)community) <= SNMP_COMMUNITY_MAX_LEN)
   {
      // Search to see if this is a write community.  This is done before 
      // searching read communities so that full read/write access is 
      // granted if a read and write community name happen to be the same.
      for(i = 0; i < SNMP_MAX_COMMUNITY_SUPPORT; i++)
      {
         ptr = &AppConfig.writeCommunity[i][0];
         if(ptr == NULL)
         {
            debug_snmp(debug_putc, "SNMPValidateCommunity('%s') write0 is NULL\r\n", community);
            continue;
         }
         if(*ptr == 0x00u)
         {
            debug_snmp(debug_putc, "SNMPValidateCommunity('%s') write0 is BLANK\r\n", community);
            continue;
         }
         if(strncmp((char*)community, (char*)ptr, SNMP_COMMUNITY_MAX_LEN) == 0)
         {
            debug_snmp(debug_putc, "SNMPValidateCommunity('%s') WRITE\r\n", community);
            return WRITE_COMMUNITY;
         }
      }
      
      // Did not find in write communities, search read communities
      for(i = 0; i < SNMP_MAX_COMMUNITY_SUPPORT; i++)
      {
         ptr = &AppConfig.readCommunity[i][0];
         if(ptr == NULL)
         {
            debug_snmp(debug_putc, "SNMPValidateCommunity('%s') read0 is NULL\r\n", community);
            continue;
         }
         if(*ptr == 0x00u)
         {
            debug_snmp(debug_putc, "SNMPValidateCommunity('%s') read0 is BLANK\r\n", community);
            continue;
         }
         debug_snmp(debug_putc, "SNMPValidateCommunity('%s') vs '%s'\r\n", community, &AppConfig.readCommunity[0][0]);
         if(strncmp((char*)community, (char*)ptr, SNMP_COMMUNITY_MAX_LEN) == 0)
         {
            debug_snmp(debug_putc, "SNMPValidateCommunity('%s') READ\r\n", community);
            return READ_COMMUNITY;
         }
      }
   }
   
   debug_snmp(debug_putc, "SNMPValidateCommunity('%s') INVALID (rd='%s' wr='%s')\r\n", community, &AppConfig.readCommunity[0][0], &AppConfig.writeCommunity[0][0]);
   
   // Could not find any matching community, set up to send a trap
   #if !defined(SNMP_TRAP_DISABLED)
   gOIDCorrespondingSnmpMibID = 0;
   gSpecificTrapNotification=VENDOR_TRAP_DEFAULT;
   gGenericTrapNotification=AUTH_FAILURE;
   gSendTrapFlag=TRUE;
   #endif
   
   return INVALID_COMMUNITY;  
}
#if !defined(SNMP_TRAP_DISABLED)
//SNMP CALLBACK VARIABLES
BYTE gSendTrapFlag = FALSE;
BYTE gGenericTrapNotification = ENTERPRISE_SPECIFIC;
BYTE gSpecificTrapNotification = VENDOR_TRAP_DEFAULT;
BYTE gOIDCorrespondingSnmpMibID = 0;

BYTE gSetTrapSendFlag;
#endif
#endif   //if defined(STACK_USE_SNMP_SERVER)

uint32_t swap24(uint32_t v)
{
   union
   {
      uint8_t b[4];
      uint32_t x;
   } ret;
   
   ret.b[0] = make8(v, 2);
   ret.b[1] = make8(v, 1);
   ret.b[2] = make8(v, 0);
   
   return(ret.x);
}

#endif
