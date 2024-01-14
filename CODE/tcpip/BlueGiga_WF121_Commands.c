// BlueGiga_WF121_Commands.c
//
// Statemachine and routines for sending/receiving API commands to the Bluegiga 
// WF121 unit.

#ifndef __BLUEGIGA_WF121_CMDS_C__
#define __BLUEGIGA_WF121_CMDS_C__

#ifndef debug_wf121_cmds
   #define debug_wf121_cmds(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)
   #define debug_wf121_cmds_array(p, a)
#else
   #define __do_debug_wf121_cmds
   #define debug_wf121_cmds_array(p, a)   debug_array(p, a)
#endif

#ifndef debug_wf121_packet
   #define debug_wf121_packet(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)
   #define debug_wf121_packet_array(p, a)
#else
   #define __do_debug_wf121_packet
   #define debug_wf121_packet_array(p, a) debug_array(p, a)
#endif

#ifndef debug_wf121_packet2
   #define debug_wf121_packet2(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)
   #define debug_wf121_packet2_array(p, a)
#else
   #define __do_debug_wf121_packet2
   #define debug_wf121_packet2_array(p, a) debug_array(p, a)
#endif
   
#ifndef debug_scan_printf
   #define debug_scan_printf(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)
#endif

// the saleae logic analzyer's SPI mode isn't happy unless it sees pulses
// on the chip select line.
#if defined(BLUEGIGA_WF121_USES_SPI)
#define __BGWF121_ANALYZER_FRIENDLY //normally not defined
#endif

#ifndef _BGWF121_SHOW_EXCEPTION
#define _BGWF121_SHOW_EXCEPTION(addr, type)
#endif

//#define __BGWF121_POLL_SPI (TICKS_PER_SECOND/10)

#if defined(BLUEGIGA_WF121_USES_SPI)
   #warning !! this should be defined at a higher level and not in the driver
   #define __BGWF121_API_RX_ISR  (INT_EXT)
#endif

#if defined(BLUEGIGA_WF121_USES_UART)
   #define _BGWF121_UartFlowIsGo()     !input(PIN_BGWF121_RTS)
   #define _BGWF121_UartFlowIsStop()   input(PIN_BGWF121_RTS)
   #define _BGWF121_UartFlowSetGo()    output_low(PIN_BGWF121_CTS)
   #define _BGWF121_UartFlowSetStop()  output_high(PIN_BGWF121_CTS)
   
   #warning !! this should be defined at a higher level and not in the driver
   #define __BGWF121_API_RX_ISR  INT_RDA2
#endif

typedef enum
{
   _BGWF121_STATE_INIT = 0,
   _BGWF121_STATE_SEND_RESET,
   _BGWF121_STATE_RESET_WAIT,
   _BGWF121_STATE_GET_BOOT,
   _BGWF121_STATE_SYNC_WAIT,
   _BGWF121_STATE_WIFI_OP_MODE_WAIT,
   _BGWF121_STATE_WIFI_ON_WAIT1,
   _BGWF121_STATE_WIFI_ON_WAIT2,
   _BGWF121_STATE_SET_POWER_START,
   _BGWF121_STATE_SET_POWER_WAIT,
  #if defined(STACK_USE_CCS_SCAN_TASK)
   _BGWF121_STATE_SCAN_UNSORTED_START,
   _BGWF121_STATE_SCAN_UNSORTED_WAIT,
   _BGWF121_STATE_SCAN_SORTED_START,
   _BGWF121_STATE_SCAN_SORTED_WAIT,
  #endif
   _BGWF121_STATE_PS_MAGIC_READ_START,
   _BGWF121_STATE_PS_MAGIC_READ_WAIT,
   _BGWF121_STATE_TCPIP_CONFIG_START,
   _BGWF121_STATE_TCPIP_CONFIG_WAIT,
   _BGWF121_STATE_SERVERS_DISABLE_START,
   _BGWF121_STATE_SERVERS_DISABLE_WAIT,
   _BGWF121_STATE_TCPIP_SET_MAC_START,
   _BGWF121_STATE_TCPIP_SET_MAC_WAIT,   
   _BGWF121_STATE_DHCP_HOSTNAME_START,
   _BGWF121_STATE_DHCP_HOSTNAME_WAIT,
  #if defined(DNSS_ONLY_URL)
   _BGWF121_STATE_DNSS_ONLY_URL_ENA_START,
   _BGWF121_STATE_DNSS_ONLY_URL_ENA_WAIT,
   _BGWF121_STATE_DNSS_ONLY_URL_VAL_START,
   _BGWF121_STATE_DNSS_ONLY_URL_VAL_WAIT,   
  #endif     
   _BGWF121_STATE_DHCPS_IP_START,
   _BGWF121_STATE_DHCPS_IP_WAIT,
  #if defined(STACK_USE_DNS)
   _BGWF121_STATE_DNS_IP_START,
   _BGWF121_STATE_DNS_IP_WAIT,
  #endif
   _BGWF121_STATE_SERVERS_CONFIG_START,
   _BGWF121_STATE_SERVERS_CONFIG_WAIT,
  #if 0
   _BGWF121_STATE_DEBUG1_START,
   _BGWF121_STATE_DEBUG1_WAIT,
   _BGWF121_STATE_DEBUG2_START,
   _BGWF121_STATE_DEBUG2_WAIT,
   _BGWF121_STATE_DEBUG3_START,
   _BGWF121_STATE_DEBUG3_WAIT,   
  #endif
   _BGWF121_STATE_PASSWORD_START,
   _BGWF121_STATE_PASSWORD_WAIT,
   _BGWF121_STATE_PS_MAGIC_WRITE_START,
   _BGWF121_STATE_PS_MAGIC_WRITE_WAIT,
   _BGWF121_STATE_CONNECT_START,
   _BGWF121_STATE_CONNECT_WAIT,
   _BGWF121_STATE_IDLE,
   _BGWF121_STATE_CLOSE_UNINITIATED_ENDPOINT_START,
   _BGWF121_STATE_CLOSE_UNINITIATED_ENDPOINT_WAIT,
   _BGWF121_STATE_RECONNECT_START,
   _BGWF121_STATE_RECONNECT_WAIT
} _bgwf121_stacktask_sm_t;

#define __BGWF121_PS_MAGIC_LOC   0x8000
#define __BGWF121_PS_MAGIC_VAL   0x56

typedef enum
{
   // how to map an event to _g_BGWF121.eventBitmap
   _BGWF121_EVENT_BIT_BOOT = 0,
   _BGWF121_EVENT_BIT_WIFI_POWER = 1,
   _BGWF121_EVENT_BIT_WIFI_CONNECT = 2,
   _BGWF121_EVENT_BIT_MAC_ADDR = 3,
   _BGWF121_EVENT_BIT_TCPIP_CONFIG = 4,
   _BGWF121_EVENT_BIT_TCPIP_DNS = 5,
   _BGWF121_EVENT_BIT_WIFI_INTERFACE = 6,
   _BGWF121_EVENT_BIT_SCANNED = 7,
   _BGWF121_EVENT_BIT_SCAN_SORT = 8,
   _BGWF121_EVENT_BIT_AP_STARTED = 9,
   _BGWF121_EVENT_BIT_PS_SAVED = 10
} _bgwf121_event_map_t;
#define _BGWF121_EXPECTED_BITMAP_POST_SYNC   (0b0111111)

// this is set with 'Wifi Is On--sme--WIFI' or 'Wifi Is Off--sme--WIFI' event
typedef enum
{
   _BGWF121_WIFI_POWER_UNKNOWN = 0,
   _BGWF121_WIFI_POWER_OFF = 1,
   _BGWF121_WIFI_POWER_ON = 2,
   _BGWF121_WIFI_POWER_ERROR = 3
} _bgwf121_wifi_on_t;

#define __BGAPI_MAX_TX_SIZE__ (WF121_TCPUDP_SCRATCH_RAM_TX+6)   //the send endpoint command.  when buffering incoming spi reads we need to be at least as big as the largest TX message

// this structure gets cleared everytime the WF121 gets reset
struct
{
   _bgwf121_stacktask_sm_t state;
   
  #if defined(BLUEGIGA_WF121_USES_SPI)
   unsigned int8 rxBufferData[__BGAPI_MAX_TX_SIZE__];
   unsigned int16 rxBufferIn;
   unsigned int16 rxBufferOut;
   unsigned int16 rxBufferNum;
   unsigned int8 doRead;
  #endif

   unsigned int8 hdrRxIdx;
   unsigned int16 eventBitmap;    //see _bgwf121_event_map_t
   unsigned int32 syncEndpointsBitmap;
   unsigned int32 expectedResponse;
   unsigned int32 uninitiatedEndpoint; //endpoints that need closing
   unsigned int32 endpointsEnabledBitmap; //bitmap of which endpoints sent us _BGAPI_EVENT_ENDPOINT_STATUS with a 'type' field was less than 33
   unsigned int32 endpointsActiveBitmap; //bitmap of which endpoints sent us _BGAPI_EVENT_ENDPOINT_STATUS with an 'active' field that wasn't 0
   unsigned int32 endpointsClosedBitmap;  //bitmap of endpoints that we got an _BGAPI_EVENT_ENDPOINT_STATUS with a 'type' field of 0 (closed)
  #if defined(STACK_USE_UDP) || defined(STACK_USE_TCP)
   TCPUDP_SOCKET_INFO *pWorkingSocket;
  #endif
   _bgwf121_wifi_on_t wifiOn;
   TICK tick;
   unsigned int8 numClients;
   int1 isLinked;   
   int1 waitingForExpectedResponse;
   int1 errorOnExpectedResponse;
   int1 dhcpBound;
   int1 magicValid;
  #if !defined(__WF121_NO_VERSION)
   int1 verValid;
  #endif
  
  #if defined(STACK_USE_CCS_SCAN_TASK)
   int1 wifiScanDo;
   int1 wifiScanValid;
   int1 wifiScanDelayed;
   TICK wifiScanDelayDuration;
   TICK wifiScanDelayStart;
   unsigned int8 wifiScanNum;
  #endif
  
  #if !defined(__WF121_NO_VERSION)
   bgwf121_version_t verInfo;
  #endif
} _g_BGWF121 = {0};

int1 _g_BGWF121_dhcpEnabled;

void _BGWF121_ResetState(void)
{
   debug_wf121_cmds(debug_putc, "_BGWF121_ResetState()\r\n");
   
   _g_BGWF121.state = _BGWF121_STATE_INIT;
}

#if !defined(__WF121_NO_VERSION)
bgwf121_version_t * BGWF121GetVersionInfo(void)
{
   if (!_g_BGWF121.verValid)
      return(0);

   return(&_g_BGWF121.verInfo);
}
#endif

#define _BGWF121_SPI_XFER(x)  spi_xfer(STREAM_SPI_BGWF121, x)

#define _BGAPI_COMMAND_SYSTEM_HELLO       (0x02010008)   //he should respond with the same exact packet
#define _BGAPI_COMMAND_SYSTEM_SYNC        (0x00010008) //will respond with several events
#define _BGAPI_COMMAND_SYSTEM_POWER_SAVE  (0x03010008)
#define _BGAPI_COMMAND_CONFIG_SET_MAC     (0x01020008)
#define _BGAPI_COMMAND_WIFI_ON            (0x00030008)
#define _BGAPI_COMMAND_WIFI_START_SCAN    (0x03030008)
#define _BGAPI_COMMAND_WIFI_PASSWORD      (0x05030008)
#define _BGAPI_COMMAND_WIFI_CONNECT_SSID  (0x07030008)
#define _BGAPI_COMMAND_WIFI_OP_MODE       (0x0A030008)
#define _BGAPI_COMMAND_WIFI_AP_START      (0x0B030008)
#define _BGAPI_COMMAND_WIFI_SCAN_SORT     (0x0D030008)
#define _BGAPI_COMMAND_WIFI_AP_PWD        (0x0F030008)
#define _BGAPI_COMMAND_TCPIP_TCP_LISTEN   (0x00040008)
#define _BGAPI_COMMAND_TCPIP_TCP_CONNECT  (0x01040008)
#define _BGAPI_COMMAND_TCPIP_UDP_LISTEN   (0x02040008)
#define _BGAPI_COMMAND_TCPIP_UDP_CONNECT  (0x03040008)
#define _BGAPI_COMMAND_TCPIP_CONFIG       (0x04040008)
#define _BGAPI_COMMAND_TCPIP_DNS_CONFIG   (0x05040008)
#define _BGAPI_COMMAND_TCPIP_DNS_RESOLVE  (0x06040008)
#define _BGAPI_COMMAND_TCPIP_UDP_BIND     (0x07040008)
#define _BGAPI_COMMAND_TCPIP_DHCP_HOSTNAME   (0x08040008)
#define _BGAPI_COMMAND_ENDPOINT_SEND      (0x00050008)
#define _BGAPI_COMMAND_ENDPOINT_CLOSE     (0x04050008)
#define _BGAPI_COMMAND_PS_SAVE            (0x03070008)
#define _BGAPI_COMMAND_PS_LOAD            (0x04070008)
#define _BGAPI_COMMAND_HTTPS_ENABLE       (0x00090008)

#define _BGAPI_EVENT_SYSTEM_BOOT          (0x00010088)
#define _BGAPI_EVENT_SYSTEM_EXCEPTION     (0x02010088)
#define _BGAPI_EVENT_SYSTEM_POWER_SAVE    (0x03010088)
#define _BGAPI_EVENT_CONFIG_MAC_ADDRESS   (0x00020088)
#define _BGAPI_EVENT_WIFI_ON              (0x00030088)
#define _BGAPI_EVENT_WIFI_OFF             (0x01030088)
#define _BGAPI_EVENT_WIFI_SCANNED         (0x04030088)
#define _BGAPI_EVENT_WIFI_CONNECTED       (0x05030088)
#define _BGAPI_EVENT_WIFI_DISCONNECTED    (0x06030088)
#define _BGAPI_EVENT_WIFI_INTERFACE       (0x07030088)
#define _BGAPI_EVENT_WIFI_CONNECT_FAILED  (0x08030088)
#define _BGAPI_EVENT_WIFI_AP_STARTED      (0x0A030088)
#define _BGAPI_EVENT_WIFI_AP_STOPPED      (0x0B030088)
#define _BGAPI_EVENT_WIFI_AP_FAILED       (0x0C030088)
#define _BGAPI_EVENT_WIFI_AP_CONNECT      (0x0D030088)
#define _BGAPI_EVENT_WIFI_AP_DISCONNECT   (0x0E030088)
#define _BGAPI_EVENT_WIFI_SCAN_RESULT     (0x0f030088)
#define _BGAPI_EVENT_WIFI_SCAN_SORT       (0x10030088)
#define _BGAPI_EVENT_TCPIP_CONFIG         (0x00040088)
#define _BGAPI_EVENT_TCPIP_DNS_SERVER     (0x01040088)
#define _BGAPI_EVENT_TCPIP_ENDPOINT_STATUS (0x02040088)
#define _BGAPI_EVENT_TCPIP_DNS_RESULT     (0x03040088)
#define _BGAPI_EVENT_TCPIP_UDP_DATA       (0x04040088)
#define _BGAPI_EVENT_ENDPOINT_SYNTAX_ERROR (0x00050088)
#define _BGAPI_EVENT_ENDPOINT_DATA        (0x01050088)
#define _BGAPI_EVENT_ENDPOINT_STATUS      (0x02050088)
#define _BGAPI_EVENT_ENDPOINT_CLOSING     (0x03050088)

#if !defined(BLUEGIGA_WF121_USES_UART) && !defined(BLUEGIGA_WF121_USES_SPI)
#define Define one of these
#endif

#if defined(BLUEGIGA_WF121_USES_SPI)
#define _BGWF121_Kbhit() (_g_BGWF121.doRead > 0)
#define _BGWF121_IsPutReady()  (TRUE)

static void _BGWF121_PutByte(unsigned int8 v)
{
   v = _BGWF121_SPI_XFER(v);
   
   _g_BGWF121.rxBufferData[_g_BGWF121.rxBufferIn] = v;

   if (++_g_BGWF121.rxBufferIn >= __BGAPI_MAX_TX_SIZE__)
   {
      _g_BGWF121.rxBufferIn = 0;
   }
      
   if (_g_BGWF121.rxBufferNum < __BGAPI_MAX_TX_SIZE__)
      _g_BGWF121.rxBufferNum += 1;
   else
   {
      //overflow
      debug_wf121_cmds(debug_putc, "!! BGWF121 SPI RX OVERFLOW !!\r\n");
     #if 0
      #warning !! TEMP CRASHING
      for(;;) {}
     #endif
      if (++_g_BGWF121.rxBufferOut >= __BGAPI_MAX_TX_SIZE__)
         _g_BGWF121.rxBufferOut = 0;
   }
}

static unsigned int8 _BGWF121_GetByte(void)
{
   unsigned int8 v;
   
   if (_g_BGWF121.rxBufferNum)
   {
      v = _g_BGWF121.rxBufferData[_g_BGWF121.rxBufferOut];
      
      _g_BGWF121.rxBufferNum--;
      
      if (++_g_BGWF121.rxBufferOut >= __BGAPI_MAX_TX_SIZE__)
      {
         _g_BGWF121.rxBufferOut = 0;
      }
   }
   else
   {
      v = _BGWF121_SPI_XFER(0);
   }
   
   return(v);
}
#endif

#if defined(BLUEGIGA_WF121_USES_UART)
#define _BGWF121_UARTRX_SIZE  275
#if (_BGWF121_UARTRX_SIZE>=0x100)
   typedef unsigned int16 _bgwf121_uartrx_idx_t;
#else
   typedef unsigned int8 _bgwf121_uartrx_idx_t;
#endif

struct
{
   _bgwf121_uartrx_idx_t len;
   _bgwf121_uartrx_idx_t in;
   _bgwf121_uartrx_idx_t out;
   char b[_BGWF121_UARTRX_SIZE];
} _g_Bgwf121_uartrx = {0,0,0};

#if (__BGWF121_API_RX_ISR==INT_RDA)
#int_rda
#elif (__BGWF121_API_RX_ISR==INT_RDA2)
#int_rda2
#else
#error do this
#endif
void _BGWF121_UartRxIsr(void)
{
   do
   {
      _g_Bgwf121_uartrx.b[_g_Bgwf121_uartrx.in] = fgetc(STREAM_UART_BGWF121);
      
      if (++_g_Bgwf121_uartrx.in >= _BGWF121_UARTRX_SIZE)
      {
         _g_Bgwf121_uartrx.in = 0;
      }
      
      if (++_g_Bgwf121_uartrx.len > _BGWF121_UARTRX_SIZE)
      {
         // overflow
         _g_Bgwf121_uartrx.len = _BGWF121_UARTRX_SIZE;
         
         if (++_g_Bgwf121_uartrx.out >= _BGWF121_UARTRX_SIZE)
         {
            _g_Bgwf121_uartrx.out = 0;
         }
      }
      
      if (_g_Bgwf121_uartrx.len > 200)
      {
         _BGWF121_UartFlowSetStop();
      }      
   } while(kbhit(STREAM_UART_BGWF121));
}

int1 _BGWF121_Kbhit(void)
{
   int1 ret;
   
   disable_interrupts(__BGWF121_API_RX_ISR);
   
   ret = (_g_Bgwf121_uartrx.len > 0);
  
   enable_interrupts(__BGWF121_API_RX_ISR);
   
   return(ret);
}

#define _BGWF121_IsPutReady()  _BGWF121_UartFlowIsGo()

static void _BGWF121_PutByte(unsigned int8 v)
{
   while (!_BGWF121_UartFlowIsGo()) 
   {
      ImportantTasks();
   }
   fputc(v, STREAM_UART_BGWF121);
}

static unsigned int8 _BGWF121_GetByte(void)
{
   char c;
   
   while (!_BGWF121_Kbhit()) {}
   
   disable_interrupts(__BGWF121_API_RX_ISR);

   c = _g_Bgwf121_uartrx.b[_g_Bgwf121_uartrx.out];

   if (++_g_Bgwf121_uartrx.out >= _BGWF121_UARTRX_SIZE)
   {
      _g_Bgwf121_uartrx.out = 0;
   }
   
   if (--_g_Bgwf121_uartrx.len < 100)
   {
      _BGWF121_UartFlowSetGo();
   }   
  
   enable_interrupts(__BGWF121_API_RX_ISR);

   return(c);
}
#endif


void _BGWF121_GetBytes(unsigned int8 *p, unsigned int16 n)
{
   unsigned int8 c;
   while(n--)
   {
      c = _BGWF121_GetByte();
      debug_wf121_packet2(debug_putc, "%02X ", c);
      if (p)
      {
         *p = c;
         p++;
      }
   }
}

// get 11bit length from 32bit header field
static unsigned int16 _BGWF121_HdrLen(unsigned int32 hdr)
{
   unsigned int16 len;
   len = (hdr >> 8) & 0xFF;
   len += (hdr & 0x7) << 8;
   return(len);
}

#ifndef DEF_MICROCHIP_DEFAULT_MAC
#define DEF_MICROCHIP_DEFAULT_MAC {0x00u, 0x04u, 0xA3u, 0x00u, 0x00u, 0x00u}
#endif

#ifndef SETTINGS_TCP_IS_CHANGED
#define SETTINGS_TCP_IS_CHANGED()   (FALSE)
#endif

#ifndef SETTINGS_TCP_CLEAR_CHANGED
#define SETTINGS_TCP_CLEAR_CHANGED()
#endif

static void _WIFIScanSetResult(unsigned int8 spiBytes);

static void _BGWF121_HandleIncomingPacket(unsigned int32 hdr)
{
   unsigned int16 len;
   unsigned int8 v, i;
   union
   {
      unsigned int8 b[4];
      unsigned int16 w[2];
      unsigned int32 dw;
      IP_ADDR ip[3];
   } scr;

   len = _BGWF121_HdrLen(hdr);
   
   hdr &= 0xFFFF00F8;   //mask of len bits
   
   if (hdr == _BGAPI_EVENT_SYSTEM_BOOT)
   {
      debug_wf121_cmds(debug_putc, "BGWF121 EVENT BOOT\r\n");
      
      if (_g_BGWF121.state == _BGWF121_STATE_GET_BOOT)
      {
         bit_set(_g_BGWF121.eventBitmap, _BGWF121_EVENT_BIT_BOOT);
       
        #if !defined(__WF121_NO_VERSION)
         if (len >= 14)
         {
            _BGWF121_GetBytes(&_g_BGWF121.verInfo, 14);
            len -= 14;
            _g_BGWF121.verValid = TRUE;
         }
        #endif
      }
      else
      {
         _BGWF121_ResetState();
      }
   }
   else if 
   (
      (hdr == _BGAPI_EVENT_WIFI_ON) || 
      (hdr == _BGAPI_EVENT_WIFI_OFF)
   )
   {
      debug_wf121_cmds(debug_putc, "BGWF121 WIFI ON/OFF ");
      
      if (len >= 2)
      {
         _BGWF121_GetBytes(&scr.w[0], 2);
         len -= 2;
         
         debug_wf121_cmds(debug_putc, "ec%LX ", scr.w[0]);
      }
      else
      {
         scr.w[0] = -1;
      }
      
      if (scr.w[0] != 0)
      {
         debug_wf121_cmds(debug_putc, "ERROR\r\n");
         _g_BGWF121.wifiOn = _BGWF121_WIFI_POWER_ERROR;
         _BGWF121_ResetState();
      }
      else if (hdr == _BGAPI_EVENT_WIFI_ON)
      {
         debug_wf121_cmds(debug_putc, "ON\r\n");
         _g_BGWF121.wifiOn = _BGWF121_WIFI_POWER_ON;
         bit_set(_g_BGWF121.eventBitmap, _BGWF121_EVENT_BIT_WIFI_POWER);
      }
      else
      {
         debug_wf121_cmds(debug_putc, "OFF\r\n");
         _g_BGWF121.wifiOn = _BGWF121_WIFI_POWER_OFF;
         bit_set(_g_BGWF121.eventBitmap, _BGWF121_EVENT_BIT_WIFI_POWER);
      }
   }
   else if (hdr == _BGAPI_EVENT_WIFI_CONNECTED) //Connected--sme--WIFI
   {
      if (len >= 2)
      {
         _BGWF121_GetBytes(&scr.w[0], 2);
         len -= 2;
         
         if (scr.b[1] == 0) //wifi
         {            
            bit_set(_g_BGWF121.eventBitmap, _BGWF121_EVENT_BIT_WIFI_CONNECT);
            
            //#warning !!! TEMPORARY DEBUG NEXT LINE
            //_g_BGWF121.isLinked = (scr.b[0] == 0);
            
            debug_wf121_cmds(debug_putc, "BGWF121 LINKED TO AP %u (%X)\r\n", _g_BGWF121.isLinked, scr.b[0]);
         }
      }
   }
   else if (hdr == _BGAPI_EVENT_WIFI_INTERFACE) //Interface Status--sme--WIFI
   {
      debug_wf121_cmds(debug_putc, "BGWF121 INTERFACE STATUS EVENT ");
      
      if (len >= 2)
      {
         _BGWF121_GetBytes(&scr, 2);
         len -= 2;
         
         bit_set(_g_BGWF121.eventBitmap, _BGWF121_EVENT_BIT_WIFI_INTERFACE);
      
         if (scr.b[0] == 0)   //WIFI
         {
            _g_BGWF121.isLinked = (scr.b[1] != 0);
         }
         
         debug_wf121_cmds(debug_putc, "link%X ", scr.b[1]);
      }
      
      debug_wf121_cmds(debug_putc, "\r\n");
   }
   else if (hdr == _BGAPI_EVENT_CONFIG_MAC_ADDRESS)
   {
      unsigned int8 mac[] = DEF_MICROCHIP_DEFAULT_MAC;
      if (len >= 7)
      {
         _BGWF121_GetBytes(&scr.b[0], 1);
         len -= 1;
         
         if (scr.b[0] == 0) //wifi
         {
            bit_set(_g_BGWF121.eventBitmap, _BGWF121_EVENT_BIT_MAC_ADDR);
            
            if (memcmp(mac, &AppConfig.MyMACAddr, 6) == 0)  //only overwrite AppConfig.MyMACAddr if it's the default value
            {
               _BGWF121_GetBytes(&AppConfig.MyMACAddr, 6);
               len -= 6;
            
               debug_wf121_cmds(debug_putc, "BGWF121 EVENT MAC ADDRESS ");
               debug_wf121_cmds_array(&AppConfig.MyMACAddr, 6);
               debug_wf121_cmds(debug_putc, "\r\n");
            }
         }
      }
   }
   else if (hdr == _BGAPI_EVENT_TCPIP_CONFIG)
   {
      if (len >= 13)
      {
         bit_set(_g_BGWF121.eventBitmap, _BGWF121_EVENT_BIT_TCPIP_CONFIG);

         _BGWF121_GetBytes(&scr.ip[0], 4);
         _BGWF121_GetBytes(&scr.ip[1], 4);
         _BGWF121_GetBytes(&scr.ip[2], 4);
         _BGWF121_GetBytes(&v, 1);
         len -= 13;
         
         debug_wf121_cmds(debug_putc, "BGWF121 EVENT TCPIP CONFIG ip%U.%U.%U.%U dhcp%X en%U ",
               scr.ip[0].v[0],
               scr.ip[0].v[1],
               scr.ip[0].v[2],
               scr.ip[0].v[3],
               v,
               _g_BGWF121_dhcpEnabled
            );
       
        #if defined(STACK_USE_DHCP_CLIENT)
         if (_g_BGWF121_dhcpEnabled)
         {
            if (v == 1) //dhcp being used
            {
               debug_wf121_cmds(debug_putc, "USING ");
               memcpy(&AppConfig.MyIPAddr, &scr.ip[0], 4);
               memcpy(&AppConfig.MyMask, &scr.ip[1], 4);
               memcpy(&AppConfig.MyGateway, &scr.ip[2], 4);
               
               unsigned int8 scrip[4];
               memset(scrip, 0x00, sizeof(scrip));
               if (memcmp(&AppConfig.MyIPAddr, scrip, 4) == 0)
               {
                  //ip is 0.0.0.0, we are not bound
                  _g_BGWF121.dhcpBound = 0;
               }
               else
               {
                  //ip is non-zero, we are bound
                  _g_BGWF121.dhcpBound = 1;
               }
            }
         }
        #endif
         debug_wf121_cmds(debug_putc, "\r\n");
      }
   }
   else if (hdr == _BGAPI_EVENT_TCPIP_DNS_SERVER)
   {
      if (len >= 5)
      {
         bit_set(_g_BGWF121.eventBitmap, _BGWF121_EVENT_BIT_TCPIP_DNS);

         _BGWF121_GetBytes(&v, 1);
         _BGWF121_GetBytes(&scr.ip[0], 4);
         len -= 5;
        
         debug_wf121_cmds(debug_putc, "BGWF121 EVENT TCPIP DNS ip%U.%U.%U.%U p%X en%X",
               scr.ip[0].v[0],
               scr.ip[0].v[1],
               scr.ip[0].v[2],
               scr.ip[0].v[3],
               v,
               _g_BGWF121_dhcpEnabled
            );

        #if defined(STACK_USE_DHCP_CLIENT)
         if (_g_BGWF121_dhcpEnabled)
         {
            debug_wf121_cmds(debug_putc, "USING ");
            if (v == 0)
            {
               memcpy(&AppConfig.PrimaryDNSServer, &scr.ip[0], 4);
            }
            else
            {
               memcpy(&AppConfig.SecondaryDNSServer, &scr.ip[0], 4);
            }
         }
        #endif
         debug_wf121_cmds(debug_putc, "\r\n");
      }
   }
   else if (hdr == _BGAPI_EVENT_ENDPOINT_STATUS)
   {
      if (len >= 8)
      {
         _BGWF121_GetBytes(&v, 1);     //endpoint
         _BGWF121_GetBytes(&scr, 4);   //type
         _BGWF121_GetBytes(NULL, 2);   //streaming and destination
         _BGWF121_GetBytes(&i, 1);  //active
         len -= 8;
         
         if (v < 32)
         {
            bit_set(_g_BGWF121.syncEndpointsBitmap, v);
         
            debug_wf121_cmds(debug_putc, "BGWF121 EVENT ENDPOINT STATUS ep%X ", v);

            if (scr.dw == 0)
            {
               bit_clear(_g_BGWF121.endpointsEnabledBitmap, v);
               bit_set(_g_BGWF121.endpointsClosedBitmap, v);
            }
            else
            {
               bit_set(_g_BGWF121.endpointsEnabledBitmap, v);
            }
            
            if (i != 0)
            {
               bit_set(_g_BGWF121.endpointsActiveBitmap, v);
            }
            else
            {
               bit_clear(_g_BGWF121.endpointsActiveBitmap, v);
            }
           
           #if defined(STACK_USE_UDP) || defined(STACK_USE_TCP)
            i = _BgapiEndpointToSocket(v, FALSE);
            if (i != -1)
            {
            }
            else if (_BgapiTempEndpointToSocket(v) == -1)
            {
               if 
               (
                  (scr.dw != 0x0) &&
                  (scr.dw != 0x1) &&
                  (scr.dw != 0x2) &&
                  (scr.dw != 0x40) &&
                  (scr.dw != 0x100) &&
                  (scr.dw != 0x200) &&
                  (scr.dw != 0x400)
               )
               {
                  debug_wf121_cmds(debug_putc, "UNASKED! ");
                  bit_set(_g_BGWF121.uninitiatedEndpoint, v);
               }
            }
           #endif
         }
         
         debug_wf121_cmds(debug_putc, "s%X type%LX\r\n", i, scr.dw); 
      }
   }
    #if defined(STACK_USE_UDP) || defined(STACK_USE_TCP)
   else if (hdr == _BGAPI_EVENT_ENDPOINT_CLOSING)
   {
      if (len >= 3)
      {
         _BGWF121_GetBytes(&scr, 3);   //scr.w[0] is reason/success, scr.b[2] is endpoint
         len -= 3;
         
         debug_wf121_cmds(debug_putc, "BWF121 ENDPOINT CLOSING EVENT ec%LX ep%X ", scr.w[0], scr.b[2]);
         
         i = _BgapiEndpointToSocket(scr.b[2], FALSE);
         if (i != -1)
         {
            _g_TcpUdpSocketInfo[i].flags.doClose = TRUE;
            debug_wf121_cmds(debug_putc, "do(s=%X, sm=%X) ", i, _g_TcpUdpSocketInfo[i].sm);
         }
         i = _BgapiTempEndpointToSocket(scr.b[2]);
         if (i != -1)
         {
            _g_TcpUdpSocketInfo[i].flags.doClose = TRUE;
            debug_wf121_cmds(debug_putc, "doTemp(s=%X, sm=%X) ", i, _g_TcpUdpSocketInfo[i].sm);
         }         
         debug_wf121_cmds(debug_putc, "\r\n");
      }
   }
  #endif
   else if (hdr == _BGAPI_EVENT_SYSTEM_EXCEPTION)
   {
      if (len >= 5)
      {
         _BGWF121_GetBytes(&scr.dw, 4);
         _BGWF121_GetBytes(&v, 1);
         len -= 5;
      }
            
      debug_wf121_cmds(debug_putc, "BGWF121 EXCEPTION addr%LX type%X\r\n", scr.dw, v);
      
      _BGWF121_SHOW_EXCEPTION(scr.dw, v);
      
      _BGWF121_ResetState();
   }
   else if 
   (
      (hdr == _BGAPI_EVENT_WIFI_CONNECT_FAILED) || 
      (hdr == _BGAPI_EVENT_WIFI_DISCONNECTED)
   )
   {
      if (len >= 3)
      {
         _BGWF121_GetBytes(&scr.w[0], 2);
         _BGWF121_GetBytes(&v, 1);
         len -= 3;
      
         if (v == 0)
         {
            _g_BGWF121.state = _BGWF121_STATE_RECONNECT_START;
            
            debug_wf121_cmds(debug_putc, "BGWF121 CONNECT FAILED/DIS r%X\r\n", scr.w[0]);
         }
      }
   }
   else if (hdr == _BGAPI_COMMAND_WIFI_CONNECT_SSID)
   {
      debug_wf121_cmds(debug_putc, "BGWF121 GOT CONNECT RESPONSE ");
      if 
      (
         _g_BGWF121.waitingForExpectedResponse && 
         (len >= 3) &&
         (_BGAPI_COMMAND_WIFI_CONNECT_SSID == _g_BGWF121.expectedResponse)
      )
      {
         _BGWF121_GetBytes(&scr.w[0], 2);
         _BGWF121_GetBytes(&v, 1);
         len -= 3;
         if ((v == 0) && (scr.w[0] == 0))
         {
            _g_BGWF121.waitingForExpectedResponse = FALSE;
            debug_wf121_cmds(debug_putc, "OK ");
         }
      }
     #if defined(__do_debug_wf121_cmds)
      else
      {
         debug_wf121_cmds(debug_putc, "NOT_OK ");
      }
     #endif
      debug_wf121_cmds(debug_putc, "\r\n");
   }
   else if 
   (
      (hdr == _BGAPI_COMMAND_TCPIP_CONFIG) ||
      (hdr == _BGAPI_COMMAND_TCPIP_DNS_RESOLVE) ||
      (hdr == _BGAPI_COMMAND_WIFI_ON) ||
      (hdr == _BGAPI_COMMAND_TCPIP_UDP_BIND) ||
      (hdr == _BGAPI_COMMAND_WIFI_OP_MODE) ||
      (hdr == _BGAPI_COMMAND_HTTPS_ENABLE) ||
      (hdr == _BGAPI_COMMAND_PS_SAVE) ||
      (hdr == _BGAPI_COMMAND_TCPIP_DNS_CONFIG)
     #if defined(STACK_USE_CCS_SCAN_TASK)
      || (hdr == _BGAPI_COMMAND_WIFI_START_SCAN)
      || (hdr == _BGAPI_COMMAND_WIFI_SCAN_SORT)
     #endif
   )
   {
     #if defined(__do_debug_wf121_cmds)
      if (hdr == _BGAPI_COMMAND_TCPIP_CONFIG)
      {
         debug_wf121_cmds(debug_putc, "BGWF121 GOT TCPIP CONFIG RESPONE ");
      }
      else if (hdr == _BGAPI_COMMAND_TCPIP_DNS_RESOLVE)
      {
         debug_wf121_cmds(debug_putc, "BGWF121 GOT DNS RESOLVE RESPONE ");
      }
      else if (hdr == _BGAPI_COMMAND_TCPIP_UDP_BIND)
      {
         debug_wf121_cmds(debug_putc, "BGWF121 UDP BIND RESPONSE ");
      }
      #if defined(STACK_USE_CCS_SCAN_TASK)
      else if (hdr == _BGAPI_COMMAND_WIFI_START_SCAN)
      {
         debug_wf121_cmds(debug_putc, "BGWF121 GOT START SCAN RESPONE ");
      }
      else if (hdr == _BGAPI_COMMAND_WIFI_SCAN_SORT)
      {
         debug_wf121_cmds(debug_putc, "BGWF121 UDP SCAN SORT RESPONSE ");
      }      
      #endif
      else if (hdr == _BGAPI_COMMAND_WIFI_OP_MODE)
      {
         debug_wf121_cmds(debug_putc, "BGWF121 WIFI OP MODE RESPONSE ");
      }
      else if (hdr == _BGAPI_COMMAND_HTTPS_ENABLE)
      {
         debug_wf121_cmds(debug_putc, "BGWF121 GOT HTTPS ENABLE RESPONSE ");
      }
      else if (hdr == _BGAPI_COMMAND_PS_SAVE)
      {
         debug_wf121_cmds(debug_putc, "BGWF121 GOT PS SAVE RESPONSE ");
      }
      else if (hdr == _BGAPI_COMMAND_TCPIP_DNS_CONFIG)
      {
         debug_wf121_cmds(debug_putc, "BGWF121 GOT DNS CONFIG RESPONSE ");
      }
      else  // _BGAPI_COMMAND_WIFI_ON
      {
         debug_wf121_cmds(debug_putc, "BGWF121 GOT POWER ON RESPONSE ");
      }
     #endif
      if 
      (
         _g_BGWF121.waitingForExpectedResponse && 
         (hdr == _g_BGWF121.expectedResponse)
      )
      {
         if (len >= 2)
         {
            _BGWF121_GetBytes(&scr.w[0], 2);
            len -= 2;
         }
         else
         {
            scr.w[0] = -1;
         }
         
         if (scr.w[0] == 0)
         {
            _g_BGWF121.waitingForExpectedResponse = FALSE;
            debug_wf121_cmds(debug_putc, "OK ");
         }
         else
         {
            _g_BGWF121.errorOnExpectedResponse = TRUE;
            debug_wf121_cmds(debug_putc, "error%LX ", scr.w[0]);
         }
      }
     #if defined(__do_debug_wf121_cmds)
      else
      {
         debug_wf121_cmds(debug_putc, "NOT_OK ");
      }
     #endif
      debug_wf121_cmds(debug_putc, "\r\n");
   }
  #if defined(STACK_USE_UDP) || defined(STACK_USE_TCP)
   else if (hdr == _BGAPI_COMMAND_ENDPOINT_CLOSE)
   {
      debug_wf121_cmds(debug_putc, "BGWF121 ENDPOINT CLOSE RESPONSE hdr%LX exp%LX w%U p%LX ", hdr, _g_BGWF121.expectedResponse, _g_BGWF121.waitingForExpectedResponse, _g_BGWF121.pWorkingSocket);
      if 
      (
         _g_BGWF121.waitingForExpectedResponse && 
         (len >= 3) &&
         (hdr == _g_BGWF121.expectedResponse)
      )
      {
         _BGWF121_GetBytes(&scr, 3);
         len -= 3;
         
         if (scr.w[0] == 0)
         {
            _g_BGWF121.waitingForExpectedResponse = FALSE;         
            debug_wf121_cmds(debug_putc, "OK ep%X ", scr.b[2]);
         }
        #if defined(__do_debug_wf121_cmds)
         else
         {
            debug_wf121_cmds(debug_putc, "ERROR ec%LX ", scr.w[0]);
         }
        #endif
      }
      
      debug_wf121_cmds(debug_putc, "\r\n");
   }
   else if
   (
      (hdr == _BGAPI_COMMAND_TCPIP_UDP_CONNECT) ||
      (hdr == _BGAPI_COMMAND_TCPIP_UDP_LISTEN) ||
      (hdr == _BGAPI_COMMAND_TCPIP_TCP_LISTEN) ||
      (hdr == _BGAPI_COMMAND_TCPIP_TCP_CONNECT)
   )
   {
     #if defined(__do_debug_wf121_cmds)
      if (hdr == _BGAPI_COMMAND_TCPIP_UDP_CONNECT)
      {
         debug_wf121_cmds(debug_putc, "BGWF121 UDP CONNECT RESPONSE ");
      }
      else if (hdr == _BGAPI_COMMAND_TCPIP_UDP_LISTEN)
      {
         debug_wf121_cmds(debug_putc, "BGWF121 UDP LISTEN RESPONSE ");
      }
      else if (hdr == _BGAPI_COMMAND_TCPIP_TCP_CONNECT)
      {
         debug_wf121_cmds(debug_putc, "BGWF121 TCP CONNECT RESPONSE ");
      }
      else if (hdr == _BGAPI_COMMAND_TCPIP_TCP_LISTEN)
      {
         debug_wf121_cmds(debug_putc, "BGWF121 TCP LISTEN RESPONSE ");
      }    
     #endif
      if 
      (
         _g_BGWF121.waitingForExpectedResponse && 
         (len >= 3) &&
         (hdr == _g_BGWF121.expectedResponse) &&
         (_g_BGWF121.pWorkingSocket != NULL)
      )
      {
         _BGWF121_GetBytes(&scr, 3);
         len -= 3;
         _g_BGWF121.waitingForExpectedResponse = FALSE;
         _g_BGWF121.errorOnExpectedResponse = (scr.w[0] != 0);
         if (!_g_BGWF121.errorOnExpectedResponse)
         {
            if (_g_BGWF121.pWorkingSocket->flags.makingTempEndpoint)
            {
               debug_wf121_cmds(debug_putc, "TEMP ");
               _g_BGWF121.pWorkingSocket->flags.makingTempEndpoint = 0;
               _g_BGWF121.pWorkingSocket->tempEndpoint = scr.b[2];            
            }
            else
            {
               _g_BGWF121.pWorkingSocket->endpoint = scr.b[2];
            }
               
            debug_wf121_cmds(debug_putc, "OK ep%X ", scr.b[2]);
         }
        #if defined(__do_debug_wf121_cmds)
         else
         {
            debug_wf121_cmds(debug_putc, "ERROR ec%LX ", scr.w[0]);
         }
        #endif
      }
      
      debug_wf121_cmds(debug_putc, "\r\n");
   }
  #endif
   else if
   (
      (hdr == _BGAPI_COMMAND_SYSTEM_POWER_SAVE) ||
      (hdr == _BGAPI_COMMAND_TCPIP_DHCP_HOSTNAME) ||
      (hdr == _BGAPI_COMMAND_WIFI_PASSWORD)
   )
   {
     #if defined(__do_debug_wf121_cmds)
      if (hdr == _BGAPI_COMMAND_SYSTEM_POWER_SAVE)
      {
         debug_wf121_cmds(debug_putc, "BGWF121 GOT POWER SAVE RESPONSE ");
      }
      else if (hdr == _BGAPI_COMMAND_WIFI_PASSWORD)
      {
         debug_wf121_cmds(debug_putc, "BGWF121 GOT PASSWORD RESPONSE ");      
      }
      else if (hdr == _BGAPI_COMMAND_TCPIP_DHCP_HOSTNAME)
      {
         debug_wf121_cmds(debug_putc, "BGWF121 GOT DHCP HOSTNAME RESPONSE ");
      }
     #endif
     
      if 
      (
         _g_BGWF121.waitingForExpectedResponse && 
         (len >= 1) &&
         (hdr == _g_BGWF121.expectedResponse)
      )
      {
         _BGWF121_GetBytes(&scr.b[0], 1);
         len -= 1;
         if (scr.b[0] == 0)
         {
            _g_BGWF121.waitingForExpectedResponse = FALSE;
            debug_wf121_cmds(debug_putc, "OK ");
         }
        #if defined(__do_debug_wf121_cmds)
         else
         {
            debug_wf121_cmds(debug_putc, "ec%X ", scr.b[0]);
         }
        #endif
      }
     #if defined(__do_debug_wf121_cmds)
      else
      {
         debug_wf121_cmds(debug_putc, "NOT_OK ");
      }
     #endif
      debug_wf121_cmds(debug_putc, "\r\n");   
   }
  #if defined(STACK_USE_UDP) || defined(STACK_USE_TCP)
   else if (hdr == _BGAPI_EVENT_TCPIP_DNS_RESULT)
   {
      debug_wf121_cmds(debug_putc, "BGWF121 EVENT DNS RESULT ");
      if 
      (
         (_g_BGWF121.pWorkingSocket != NULL) &&
         (_g_BGWF121.pWorkingSocket->flags.dnsResolving)
      )
      {
         if (len >= 6)
         {
            _BGWF121_GetBytes(&scr.w[0], 2);
            len -= 2;
            if (scr.w[0] == 0)
            {
               _BGWF121_GetBytes(&_g_BGWF121.pWorkingSocket->remote.remoteNode.IPAddr, 4);
               len -= 4;
               _g_BGWF121.pWorkingSocket->flags.dnsResolving = FALSE;

               debug_wf121_cmds(debug_putc, "ip%U.%U.%U.%U ",
                     _g_BGWF121.pWorkingSocket->remote.remoteNode.IPAddr.v[0],
                     _g_BGWF121.pWorkingSocket->remote.remoteNode.IPAddr.v[1],
                     _g_BGWF121.pWorkingSocket->remote.remoteNode.IPAddr.v[2],
                     _g_BGWF121.pWorkingSocket->remote.remoteNode.IPAddr.v[3]
                  );               
            }
           #if defined(__do_debug_wf121_cmds)
            else
            {
               debug_wf121_cmds(debug_putc, "NOT_OK_ec%X ", scr.w[0]);
            }
           #endif            
         }
        #if defined(__do_debug_wf121_cmds)
         else
         {
            debug_wf121_cmds(debug_putc, "NOT_OK2 ");
         }
        #endif         
      }
      debug_wf121_cmds(debug_putc, "\r\n");
   }
   else if (hdr == _BGAPI_COMMAND_PS_LOAD)
   {
      debug_wf121_cmds(debug_putc, "BGWF121 RESPONSE PS_LOAD ");
      
      if (len >= 4)
      {
         _BGWF121_GetBytes(&scr, 3);   //scr.w[0] holds error code, scr.b[2] holds length of data
         _BGWF121_GetBytes(&i, 1);  //holds first byte of data
         len -= 4;
         
         debug_wf121_cmds(debug_putc, "EC=%LX LEN=%X V=%X ", scr.w[0], scr.b[2], i);
         
        #if defined(__do_debug_wf121_cmds)
         while(len)
         {
            _BGWF121_GetBytes(&v, 1);
            len--;
            debug_wf121_cmds(debug_putc, "%X ", v);
         }
        #endif
        
         if 
         (
            (_g_BGWF121.state == _BGWF121_STATE_PS_MAGIC_READ_WAIT) &&
            (scr.w[0] == 0) && 
            (scr.b[2] == 1) && 
            (i == __BGWF121_PS_MAGIC_VAL)
         )
         {
            if (SETTINGS_TCP_IS_CHANGED())
            {
               debug_wf121_cmds(debug_putc, "PS_MAGIC_VALID,BUT_LOCAL_CHANGES ");
            }
            else
            {
               debug_wf121_cmds(debug_putc, "MAGIC_VALID");
               _g_BGWF121.magicValid = TRUE;
            }
         }
      }
      if (hdr == _g_BGWF121.expectedResponse)
      {
         _g_BGWF121.waitingForExpectedResponse = FALSE;
      }

      debug_wf121_cmds(debug_putc, "\r\n");
   }
   else if (hdr == _BGAPI_EVENT_TCPIP_ENDPOINT_STATUS)
   {
      debug_wf121_cmds(debug_putc, "BGWF121 EVENT TCPIP EP STATUS ");
      
      if (len >= 13)
      {
         _BGWF121_GetBytes(&scr, 1);   //scr.b[0] holds endpoint
         _BGWF121_GetBytes(NULL, 4);   //skip local ip
         _BGWF121_GetBytes(&scr.w[1], 2); //scr.w[1] holds local port
         len -= 7;
         
         i = _BgapiEndpointToSocket(scr.b[0], FALSE);
         
         debug_wf121_cmds(debug_putc, "ep%X s%X lp%LX ", scr.b[0], i, scr.w[1]);
         
         if (i == -1)
         {
            // see if this local port matches one of our TCP servers
            for(v=0; v<MAX_UDP_TCP_SOCKETS; v++)
            {
               if 
               (
                  _g_TcpUdpSocketInfo[v].flags.isServer &&
                  !_g_TcpUdpSocketInfo[v].flags.isUDP &&
                  !_g_TcpUdpSocketInfo[v].flags.clientConnected &&
                  (_g_TcpUdpSocketInfo[v].localPort == scr.w[1])
               )
               {
                  break;
               }
            }
            if (v < MAX_UDP_TCP_SOCKETS)
            {
               debug_wf121_cmds(debug_putc, "- newep%X to server%X(s%X) ", scr.b[0], _g_TcpUdpSocketInfo[v].endpoint, v);
              #if defined(__BGWF121_TCP_SERVER_WONT_SIMULTANEOUS__)
               if (_g_TcpUdpSocketInfo[v].tempEndpoint != -1)
               {
                  bit_set(_g_BGWF121.uninitiatedEndpoint, _g_TcpUdpSocketInfo[v].tempEndpoint);
               }
               _g_TcpUdpSocketInfo[v].tempEndpoint = _g_TcpUdpSocketInfo[v].endpoint;
              #else
               bit_set(_g_BGWF121.uninitiatedEndpoint, _g_TcpUdpSocketInfo[v].endpoint);
              #endif
               _g_TcpUdpSocketInfo[v].endpoint = scr.b[0];
               _g_TcpUdpSocketInfo[v].flags.clientConnected = TRUE;
               _BGWF121_GetBytes(&_g_TcpUdpSocketInfo[v].remote.remoteNode.IPAddr, 4);
               _BGWF121_GetBytes(&_g_TcpUdpSocketInfo[v].remotePort, 2);
               len -= 6;
            }
         }
      }
      
      debug_wf121_cmds(debug_putc, "\r\n");
   }
   else if (hdr == _BGAPI_EVENT_ENDPOINT_DATA)
   {
      if (len >= 2)
      {
         _BGWF121_GetBytes(&scr, 2);
         len -= 2;
         scr.b[2] = _BgapiEndpointToSocket(scr.b[0], TRUE);
         
         debug_wf121_cmds(debug_putc, "BGWF121 EVENT EP DATA t%LX ep%X s%X n%X ", TickGet(), scr.b[0], scr.b[2], scr.b[1]);
         
         if ((scr.b[1] != 0) && (scr.b[2] != -1))
         {
            #if defined(STACK_USE_CCS_RX_EVENT)
            STACK_USE_CCS_RX_EVENT();
            #endif
            _g_BgapiTcpUdpScratchRx.socket = scr.b[2];
            _g_BgapiTcpUdpScratchRx.num = scr.b[1];
            _g_BgapiTcpUdpScratchRx.idx = 0;
            _BGWF121_GetBytes(_g_BgapiTcpUdpScratchRx.b, scr.b[1]);
            len -= scr.b[1];
         }
         
         debug_wf121_cmds(debug_putc, "t%LX\r\n", TickGet());
      }
   }
   else if (hdr == _BGAPI_EVENT_TCPIP_UDP_DATA)
   {
      if (len >= 9)
      {
         _BGWF121_GetBytes(&scr, 1);
         len -= 1;
         v = _BgapiEndpointToSocket(scr.b[0], TRUE);
         
         debug_wf121_cmds(debug_putc, "BGWF121 EVENT UDP DATA t%LX ep%X s%X ", TickGet(), scr.b[0], v);
         
         if (v != -1)
         {
            _BGWF121_GetBytes(&_g_TcpUdpSocketInfo[v].remote.remoteNode.IPAddr, 4);
            _BGWF121_GetBytes(&_g_TcpUdpSocketInfo[v].remotePort, 2);
            _BGWF121_GetBytes(&scr, 2);
            len -= 8;
            
            debug_wf121_cmds(debug_putc, "n%LX ", scr.w[0]);
            
            if (scr.w[0] > WF121_TCPUDP_SCRATCH_RAM_RX)
               scr.w[0] = WF121_TCPUDP_SCRATCH_RAM_RX;
            if (scr.w[0] != 0)
            {
               _g_BgapiTcpUdpScratchRx.socket = v;
               _g_BgapiTcpUdpScratchRx.num = scr.w[0];
               _g_BgapiTcpUdpScratchRx.idx = 0;
               _BGWF121_GetBytes(_g_BgapiTcpUdpScratchRx.b, scr.w[0]);
               len -= scr.w[0];
            }
         }
         
         debug_wf121_cmds(debug_putc, "t%LX\r\n", TickGet());
      }
   }
  #endif
  #if defined(STACK_USE_CCS_SCAN_TASK)
   else if (hdr == _BGAPI_EVENT_WIFI_SCANNED)
   {
      scr.b[0] = -1;
      
      if (len >= 1)
      {
         _BGWF121_GetBytes(&scr, 1);   //scr.b[0] is result code, should be 0x00 for OK
         len -= 1;
      }
      
      debug_wf121_cmds(debug_putc, "BGWF121 EVENT WIFI SCANNED ec%X\r\n", scr.b[0]);
      
      if (scr.b[0] != 0)
      {
         _g_BGWF121.errorOnExpectedResponse = TRUE;
      }
      
      bit_set(_g_BGWF121.eventBitmap, _BGWF121_EVENT_BIT_SCANNED);
   }  
   else if (hdr == _BGAPI_EVENT_WIFI_SCAN_SORT)
   {
      debug_wf121_cmds(debug_putc, "BGWF121 EVENT SCAN SORTED\r\n");
      
      bit_set(_g_BGWF121.eventBitmap, _BGWF121_EVENT_BIT_SCAN_SORT);
   }
   else if (hdr == _BGAPI_EVENT_WIFI_SCAN_RESULT)
   {
      debug_wf121_cmds(debug_putc, "BGWF121 GOT EVENT SCAN RESULT "); 
      if (len >= 12)
      {
         debug_wf121_cmds(debug_putc, "\r\n");
         _WIFIScanSetResult(len);
         len = 0;
      }
      else
      {
         debug_wf121_cmds(debug_putc, "ERROR\r\n");
      }
   }
  #endif
   else if (hdr == _BGAPI_EVENT_WIFI_AP_STARTED)
   {
      scr.b[0] = -1;
      
      debug_wf121_cmds(debug_putc, "BGWF121 EVENT AP STARTED ");
      
      if (len >= 1)
      {
         _BGWF121_GetBytes(&scr.b[0], 1);
         len -= 1;         
      }
      
      if (scr.b[0] == 0)
      {
         bit_set(_g_BGWF121.eventBitmap, _BGWF121_EVENT_BIT_AP_STARTED);
         debug_wf121_cmds(debug_putc, "OK ");
      }
      
      debug_wf121_cmds(debug_putc, "\r\n");
   }
   else if
   (
      (hdr == _BGAPI_EVENT_WIFI_AP_STOPPED) ||
      (hdr == _BGAPI_EVENT_WIFI_AP_FAILED)
   )
   {
      debug_wf121_cmds(debug_putc, "BGWF121 EVENT AP STOP/FAIL hdr%LX\r\n", hdr);
      _BGWF121_ResetState();
   }
   else if (hdr == _BGAPI_EVENT_WIFI_AP_CONNECT)
   {
      _g_BGWF121.numClients += 1;
      debug_wf121_cmds(debug_putc, "BGWF121 EVENT AP CLIENT JOINED num%X\r\n", _g_BGWF121.numClients);
   }
   else if (hdr == _BGAPI_EVENT_WIFI_AP_DISCONNECT)
   {
      if (_g_BGWF121.numClients != 0)
         _g_BGWF121.numClients -= 1;
      debug_wf121_cmds(debug_putc, "BGWF121 EVENT AP CLIENT DROPPED num%X\r\n", _g_BGWF121.numClients);
   }
   else if (_g_BGWF121.waitingForExpectedResponse && (hdr == _g_BGWF121.expectedResponse))
   {
      debug_wf121_cmds(debug_putc, "BGWF121 GOT RESPONSE %LX\r\n", hdr);
      _g_BGWF121.waitingForExpectedResponse = 0;
   }
  #if defined(__do_debug_wf121_cmds)
   else if (hdr == _BGAPI_EVENT_ENDPOINT_SYNTAX_ERROR)
   {
      debug_wf121_cmds(debug_putc, "BGWF121 EVENT ENDPOINT SYNTAX ERROR - ");
     #if !defined(__do_debug_wf121_packet2)
      while(len--)
      {
         debug_wf121_cmds(debug_putc, "%02X ", _BGWF121_GetByte());
      }
     #endif
      debug_wf121_cmds(debug_putc, "\r\n");
   }
   else if (hdr == _BGAPI_EVENT_SYSTEM_POWER_SAVE)
   {
      if (len >= 1)
      {
         _BGWF121_GetBytes(&scr.b[0], 1);
         len -= 1;
      }
      else
      {
         scr.b[0] = 0xFF;
      }
      
      debug_wf121_cmds(debug_putc, "BGWF121 EVENT POWER SAVING STATE %X\r\n", scr.b[0]);
   }
   else
   {
      debug_wf121_cmds(debug_putc, "BGWF121 GOT UNHANDLED CMD hdr%LX\r\n", hdr);
   }
  #endif
 
   // if there are any data bytes left, remove them
   if (len)
   {
      _BGWF121_GetBytes(NULL, len);
   }
}

// returns TRUE if it received and processed a command
#if defined(BLUEGIGA_WF121_USES_SPI)
static int1 _BGWF121_RxHeader(void)
{
   static int1 holdoffDo;
   static TICK holdoffTick;
   static union
   {
      int8 b[4];
      int32 dw;
   } hdr;
   unsigned int8 max = 16;
   unsigned int8 v;
   
   if (holdoffDo)
   {
      if ((TickGet() - holdoffTick) >= TICKS_PER_SECOND/500)
      {
         holdoffDo = FALSE;
      }
      else
      {
         return(FALSE);
      }
   }
   
   while (_g_BGWF121.doRead || _g_BGWF121.rxBufferNum)
   {
      v = _BGWF121_GetByte();
      
      //debug_printf(debug_putc, "-%X-", v);
      
      if
      (
         (_g_BGWF121.hdrRxIdx) ||
         ((v != 0) && (v != 0xFF))
      )
      {
         hdr.b[_g_BGWF121.hdrRxIdx++] = v;
   
         if (_g_BGWF121.hdrRxIdx >= 4)
         {
            _g_BGWF121.hdrRxIdx = 0;
  
            if (_g_BGWF121.doRead)
               _g_BGWF121.doRead -= 1;
           
           #if defined(__do_debug_wf121_packet2)
            debug_wf121_packet2(debug_putc, "\r\n");
           #endif
            debug_wf121_packet(debug_putc, "BGWF121_RX t%LX h", TickGet());
            debug_wf121_packet_array(&hdr, 4);
           #if defined(__do_debug_wf121_packet2)
            debug_wf121_packet2(debug_putc, "- ");
           #else
            debug_wf121_packet(debug_putc, "\r\n");
           #endif
            
            v = hdr.b[0] & 0xF8;
            
            if ((v == (unsigned int8)0x8) || (v == (unsigned int8)0x88))
            {
               _BGWF121_HandleIncomingPacket(hdr.dw);
               return(TRUE);
            }
         }
      }
      else if (_g_BGWF121.rxBufferNum==0)
      {
         if (--max == 0)
         {
            // hold off from reading again, otherwise reading too fast causes
            // the WF121 to crash.
            holdoffDo = TRUE;
            holdoffTick = TickGet();
            return(FALSE);
         }
      }
   }
   
   return(FALSE);
}
#endif

#if defined(BLUEGIGA_WF121_USES_UART)
static int1 _BGWF121_RxHeader(void)
{
   static union
   {
      int8 b[4];
      int32 dw;
   } hdr;
   unsigned int8 v,x;
   static TICK t;

   if (_BGWF121_Kbhit())
   {
      while (_BGWF121_Kbhit())
      {
         v = _BGWF121_GetByte();
         
         if (_g_BGWF121.hdrRxIdx == 0)
         {
            t = TickGet();
         }
         
         hdr.b[_g_BGWF121.hdrRxIdx++] = v;

         if (_g_BGWF121.hdrRxIdx == 1)
         {
            x = v & 0xF8;
            
            if ((x != 0x8) && (x != 0x88))
            {
               _g_BGWF121.hdrRxIdx = 0;
            }
         }

         if (_g_BGWF121.hdrRxIdx >= 4)
         {
            _g_BGWF121.hdrRxIdx = 0;
           
           #if defined(__do_debug_wf121_packet2)
            debug_wf121_packet2(debug_putc, "\r\n");
           #endif
            debug_wf121_packet(debug_putc, "BGWF121_RX t%LX h", TickGet());
            debug_wf121_packet_array(&hdr, 4);
           #if defined(__do_debug_wf121_packet2)
            debug_wf121_packet2(debug_putc, "- ");
           #else
            debug_wf121_packet(debug_putc, "\r\n");
           #endif
            
            //v = hdr.b[0] & 0xF8;
            
            //if ((v == (unsigned int8)0x8) || (v == (unsigned int8)0x88))
            {
               _BGWF121_HandleIncomingPacket(hdr.dw);
               
               return(TRUE);
            }
         }
      }
   }
   else if (_g_BGWF121.hdrRxIdx != 0)
   {
      if ((TickGet() - t) >= TICKS_PER_SECOND/8)
      {
         _g_BGWF121.hdrRxIdx = 0;
      }
   }
   
   return(FALSE);
}
#endif

static void _BGWF121_BgapiTxBytes(unsigned int16 n, unsigned int8 *p)
{
   if ((p == NULL) || (n == 0))
      return;

   debug_wf121_packet2_array(p, n);

   while(n--)
   {
      _BGWF121_PutByte(*p++);
   }
}
#if !getenv("PSV")
static void _BGWF121_BgapiTxBytesROM(unsigned int16 n, rom unsigned int8 *p)
{
   unsigned int8 c;
   
   if ((p == NULL) || (n == 0))
      return;

   while(n--)
   {
      c = *p++;
      
      debug_wf121_packet2(debug_putc, "%02X ", c);
      
      _BGWF121_PutByte(c);
   }
}
#endif

static void _BGWF121_BgapiTxPacket_Prepare(unsigned int32 hdr)
{
   if (hdr == _BGAPI_COMMAND_PS_SAVE)
   {
      bit_clear(_g_BGWF121.eventBitmap, _BGWF121_EVENT_BIT_PS_SAVED);
   }
   
   _g_BGWF121.expectedResponse = hdr;
}

/**
   Transmit a BGAPI packet.
   
   'hdr' is the 32bit header, NOT INCLUDING length.  The length will be 
   appended to this before Tx.  You can use _BGAPI_MAKE_HDR() macro to 
   populate this value.
   
   'payloadLen' is the length of 'pPayload' and can be 0-2047.
   
   if 'pPayload' is NULL, it won't transmit anything.  'pPayload' can be
   NULL and 'payloadLen' can be non-zero at the same time.
*/
static void _BGWF121_BgapiTxPacket2(unsigned int32 hdr, 
   unsigned int16 payloadLen1, unsigned int8 *pPayload1, 
   unsigned int16 payloadLen2, unsigned int8 *pPayload2)
{
   unsigned int16 payloadLen;
   
   _BGWF121_BgapiTxPacket_Prepare(hdr);
   
   payloadLen = payloadLen1 + payloadLen2;
   
   hdr |= ((payloadLen >> 8) & 0x7);
   hdr |= ((payloadLen << 8) & (unsigned int16)0xFF00);

  #if defined(__do_debug_wf121_packet2)
   debug_wf121_packet2(debug_putc, "\r\n");
  #endif
   debug_wf121_packet(debug_putc, "BGWF121_TX t%LX h", TickGet());
  #if !defined(__do_debug_wf121_packet2)
   debug_wf121_packet_array(&hdr, 4);
  #endif
  
  #if defined(__BGWF121_ANALYZER_FRIENDLY)
   output_high(PIN_BGWF121_CS);
   delay_us(50);
   output_low(PIN_BGWF121_CS);
   delay_us(50);
  #endif
  
   _BGWF121_BgapiTxBytes(4, &hdr);
   
   debug_wf121_packet2(debug_putc, "- ");
   
   _BGWF121_BgapiTxBytes(payloadLen1, pPayload1);
   
   _BGWF121_BgapiTxBytes(payloadLen2, pPayload2);
   
   //_BGWF121_SpiTx(0);   //send a 0, so the MOSI is idling at low
   
  #if !defined(__do_debug_wf121_packet2)
   debug_wf121_packet(debug_putc, "\r\n");
  #endif
   
   _g_BGWF121.waitingForExpectedResponse = TRUE;
   _g_BGWF121.errorOnExpectedResponse = FALSE;
   _g_BGWF121.tick = TickGet();   
   
  #if defined(__BGWF121_ANALYZER_FRIENDLY)
   output_high(PIN_BGWF121_CS);
   delay_us(50);
   output_low(PIN_BGWF121_CS);
   delay_us(50);
  #endif
}
static void _BGWF121_BgapiTxPacket(unsigned int32 hdr, 
   unsigned int16 payloadLen, unsigned int8 *pPayload)
{
   _BGWF121_BgapiTxPacket2(hdr, payloadLen, pPayload, 0, NULL);
}
static void _BGWF121_BgapiTxPacketROM2(unsigned int32 hdr, rom char *string)
{
   // payload format is: x y
   //    x is one byte, and is strlen of 'string'
   //    y is 'string'
  #if getenv("PSV")
   unsigned int8 len;
   len = strlen(string);
   _BGWF121_BgapiTxPacket2(hdr, 1, &len, len, string);
  #else
   unsigned int8 len;
   
   len = strlenpgm(string) + 1;
   
   _BGWF121_BgapiTxPacket_Prepare(hdr);
   
   hdr |= (((unsigned int16)len << 8) & (unsigned int16)0xFF00);

   debug_wf121_packet(debug_putc, "BGWF121_TX h ");
  #if !defined(__do_debug_wf121_packet2)
   debug_wf121_packet_array(&hdr, 4);
  #endif
  
  #if defined(__BGWF121_ANALYZER_FRIENDLY)
   output_high(PIN_BGWF121_CS);
   delay_us(50);
   output_low(PIN_BGWF121_CS);
   delay_us(50);
  #endif  

   _BGWF121_BgapiTxBytes(4, &hdr);
   
   debug_wf121_packet2(debug_putc, "- ");
   
   len -= 1;
   _BGWF121_BgapiTxBytes(1, &len);
   
   _BGWF121_BgapiTxBytesROM(len, string);
   
   //_BGWF121_SpiTx(0);   //send a 0, so the MOSI is idling at low
   
   debug_wf121_packet(debug_putc, "\r\n");
   
   _g_BGWF121.waitingForExpectedResponse = TRUE;
   _g_BGWF121.errorOnExpectedResponse = FALSE;
   _g_BGWF121.tick = TickGet();
   
  #if defined(__BGWF121_ANALYZER_FRIENDLY)
   output_high(PIN_BGWF121_CS);
   delay_us(50);
   output_low(PIN_BGWF121_CS);
   delay_us(50);
  #endif   
  #endif
}

void _BGWF121_BgapiTxTcpipConfig(void)
{
   struct __PACKED
   {
      IP_ADDR ip;
      IP_ADDR mask;
      IP_ADDR gw;
      unsigned int8 useDHCP;  //0=static, 1=use dhcp
   } payload;
   
   _g_BGWF121_dhcpEnabled = AppConfig.Flags.bIsDHCPEnabled;
   
   memcpy(&payload.ip, &AppConfig.MyIPAddr, 4);
   memcpy(&payload.mask, &AppConfig.MyMask, 4);
   memcpy(&payload.gw, &AppConfig.MyGateway, 4);
  
  #if defined(STACK_USE_DHCP_CLIENT)
   if (_g_BGWF121_dhcpEnabled)
      payload.useDHCP = 1;
   else
  #endif
      payload.useDHCP = 0;

   debug_wf121_cmds(debug_putc, "BGWF121 TX TCPIP CONFIG ip%U.%U.%U.%U dhcp%X\r\n",
         payload.ip.v[0],
         payload.ip.v[1],
         payload.ip.v[2],
         payload.ip.v[3],
         payload.useDHCP
      );
  
   _BGWF121_BgapiTxPacket(
         _BGAPI_COMMAND_TCPIP_CONFIG, 
         sizeof(payload), 
         &payload
      );
   
   _g_BGWF121.state = _BGWF121_STATE_TCPIP_CONFIG_WAIT;
}

void MACInit(void)
{
   #if defined(BLUEGIGA_WF121_USES_SPI)
   spi_init(STREAM_SPI_BGWF121);
   
   output_low(PIN_BGWF121_CS);
   output_drive(PIN_BGWF121_CS);
   
   ext_int_edge(0, L_TO_H);
   #warning tie the above to the proper int_ext
   #endif
   
   output_high(PIN_BGWF121_RESET);
   output_drive(PIN_BGWF121_RESET);
  
   _BGWF121_ResetState();
  
  #if defined(STACK_USE_UDP) || defined(STACK_USE_TCP)
   _TcpUdpSocketsInit();
  #endif
}

int1 MACIsLinked(void)
{
   return(_g_BGWF121.isLinked);
}

#if defined(BLUEGIGA_WF121_USES_SPI)
#if (__BGWF121_API_RX_ISR==INT_EXT)
   #int_ext
#else
   #error do
#endif
static void _Bgwf121_Isr(void)
{
   _g_BGWF121.doRead++;
}
#endif

#ifndef BGWF121_ON_RESET
   #define BGWF121_ON_RESET()
#endif

#ifndef BGWF121_AFTER_RESET
   #define BGWF121_AFTER_RESET()
#endif

#ifndef debug_TxIsEmpty
#define debug_TxIsEmpty() (TRUE)
#endif

void StackTask(void)
{
   union
   {
      unsigned int8 b[6];
      unsigned int16 w[3];
      unsigned int32 dw;
   } scr;
   IP_ADDR ip;   
   static unsigned int8 i8;
  #if defined(__BGWF121_POLL_SPI)
   static TICK t;
  #endif
   
  #if defined(__do_debug_wf121_cmds)
   static _bgwf121_stacktask_sm_t debug;
   if (_g_BGWF121.state != debug)
   {
      debug_wf121_cmds(debug_putc, "BGWF121_StackTask() %X->%X\r\n", debug, _g_BGWF121.state);
      debug = _g_BGWF121.state;
   }
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

   if (_g_BGWF121.state >= _BGWF121_STATE_GET_BOOT)
   {   
      if (_BGWF121_RxHeader())
      {
         // give other tasks time to do something before we keep receiving
         // new messages.  also give other tasks time to pull the
         // TCP or UDP data before another message overwrites it.
         return;
      }
   
      if (_BGWF121_Kbhit() || (_g_BGWF121.hdrRxIdx != 0))
      {
         // don't send it a new command if it's busy sending us something.
         return;
      }
   }
   
   if (!debug_TxIsEmpty())
   {
      // DEBUGGING
      // wait until debug stream's TX buffer is empty before we send a command
      return;
   }
 
  #if defined(STACK_USE_CCS_SCAN_TASK)
   if 
   (
      _g_BGWF121.wifiScanDelayed && 
      ((TickGet() - _g_BGWF121.wifiScanDelayStart) >= _g_BGWF121.wifiScanDelayDuration)
   )
   {
      WIFIScanStart();
   }
  #endif
 
   /*
   if 
   (
      (
         _g_BGWF121.doRead ||
         _g_BGWF121.rxBufferNum ||
         _g_BGWF121.hdrRxIdx
        #if defined(__BGWF121_POLL_SPI)
         || ((TickGet() - t) >= (TICKS_PER_SECOND/10))
        #endif
      ) &&
      (_g_BGWF121.state >= _BGWF121_STATE_GET_BOOT)
   )
   {
     #if defined(__BGWF121_POLL_SPI)
      t = TickGet();
     #endif
     
      _BGWF121_RxHeader();

      // give other tasks time to do something before we keep receiving
      // new messages.  also give other tasks time to pull the
      // TCP or UDP data before another message overwrites it.
      return;
   }
   */
   
   switch(_g_BGWF121.state)
   {
      default:
      case _BGWF121_STATE_INIT:
        #if defined(STACK_USE_UDP) || defined(STACK_USE_TCP)
         _TcpUdpSocketsReset();
        #endif
         memset(&_g_BGWF121, 0x00, sizeof(_g_BGWF121));
         // no break
      case _BGWF121_STATE_SEND_RESET:
         #if defined(BLUEGIGA_WF121_USES_UART)
         _BGWF121_UartFlowSetStop();
         #endif      
         disable_interrupts(__BGWF121_API_RX_ISR);
         BGWF121_ON_RESET();
         output_low(PIN_BGWF121_RESET);
         output_drive(PIN_BGWF121_RESET);
         _g_BGWF121.tick = TickGet();
         _g_BGWF121.state = _BGWF121_STATE_RESET_WAIT;
         break;
      
      case _BGWF121_STATE_RESET_WAIT:
         if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND/10)
         {
            output_high(PIN_BGWF121_RESET);
            #if defined(BLUEGIGA_WF121_USES_UART)
            delay_ms(10);
            while(kbhit(STREAM_UART_BGWF121))
               fgetc(STREAM_UART_BGWF121);
            memset(&_g_Bgwf121_uartrx, 0x00, sizeof(_g_Bgwf121_uartrx));
            _BGWF121_UartFlowSetGo();
            #endif
            clear_interrupt(__BGWF121_API_RX_ISR);
            enable_interrupts(__BGWF121_API_RX_ISR);
            _g_BGWF121.hdrRxIdx = 0;
            _g_BGWF121.state = _BGWF121_STATE_GET_BOOT;
            _g_BGWF121.tick = TickGet();
         }
         break;

      case _BGWF121_STATE_GET_BOOT:
         if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*3)
         {
            if (bit_test(_g_BGWF121.eventBitmap, _BGWF121_EVENT_BIT_BOOT))
            {
               if (_BGWF121_IsPutReady())
               {
                  i8 = 0;
                  
                  _g_BGWF121.tick = TickGet();
                  
                  _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_SYSTEM_SYNC, 0, 0);
                  _g_BGWF121.waitingForExpectedResponse = FALSE;
                  
                  _g_BGWF121.state = _BGWF121_STATE_SYNC_WAIT;
                  BGWF121_AFTER_RESET();
               }
            }
            else
            {
               _g_BGWF121.state = _BGWF121_STATE_SEND_RESET;
            }
         }
         break;

      case _BGWF121_STATE_SYNC_WAIT:
         if 
         (
            ((_g_BGWF121.eventBitmap & _BGWF121_EXPECTED_BITMAP_POST_SYNC) == _BGWF121_EXPECTED_BITMAP_POST_SYNC) &&
            (_g_BGWF121.syncEndpointsBitmap == 0xFFFFFFFF) &&
            _BGWF121_IsPutReady()
         )
         {
            debug_wf121_cmds(debug_putc, "BWF121 SET OP MODE %X\r\n", AppConfig.networkType);
            _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_WIFI_OP_MODE, 1, &AppConfig.networkType);
            _g_BGWF121.state = _BGWF121_STATE_WIFI_OP_MODE_WAIT;
         }
         else if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*9)
         {
            if (i8 < 3)
            {
               if (_BGWF121_IsPutReady())
               {
                  i8++;
                  _g_BGWF121.tick = TickGet();
                  
                  _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_SYSTEM_SYNC, 0, 0);
                  _g_BGWF121.eventBitmap = 0;
                  _g_BGWF121.syncEndpointsBitmap = 0;
                  
                  debug_wf121_cmds(debug_putc, "BWF121 SYNC TIMEOUT, TRY AGAIN\r\n");
               }
            }
            else
            {
               debug_wf121_cmds(debug_putc, "BWF121 SYNC FAIL %LX %LX\r\n", _g_BGWF121.eventBitmap, _g_BGWF121.syncEndpointsBitmap);
               _BGWF121_ResetState();
            }
         }
         break;

      case _BGWF121_STATE_WIFI_OP_MODE_WAIT:
         if (!_g_BGWF121.waitingForExpectedResponse && _BGWF121_IsPutReady())
         {
            _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_WIFI_ON, 0, 0);
            _g_BGWF121.wifiOn = _BGWF121_WIFI_POWER_UNKNOWN;
            _g_BGWF121.state = _BGWF121_STATE_WIFI_ON_WAIT1;         
         }
         else if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*3)
         {
            debug_wf121_cmds(debug_putc, "BWF121 OP MODE FAIL\r\n");
            _BGWF121_ResetState();         
         }
         break;

      case _BGWF121_STATE_WIFI_ON_WAIT1:
      case _BGWF121_STATE_DHCP_HOSTNAME_WAIT:      
      case _BGWF121_STATE_TCPIP_CONFIG_WAIT:
      case _BGWF121_STATE_TCPIP_SET_MAC_WAIT:
      case _BGWF121_STATE_PASSWORD_WAIT:
      case _BGWF121_STATE_SERVERS_DISABLE_WAIT:
      case _BGWF121_STATE_SERVERS_CONFIG_WAIT:
      case _BGWF121_STATE_DHCPS_IP_WAIT:
     #if defined(STACK_USE_DNS)
      case _BGWF121_STATE_DNS_IP_WAIT:
     #endif
     #if defined(DNSS_ONLY_URL)
      case _BGWF121_STATE_DNSS_ONLY_URL_ENA_WAIT:
      case _BGWF121_STATE_DNSS_ONLY_URL_VAL_WAIT:
     #endif
      case _BGWF121_STATE_PS_MAGIC_READ_WAIT:
      case _BGWF121_STATE_PS_MAGIC_WRITE_WAIT:
     #if definedinc(_BGWF121_STATE_DEBUG1_WAIT)
      case _BGWF121_STATE_DEBUG1_WAIT:
     #endif
     #if definedinc(_BGWF121_STATE_DEBUG2_WAIT)
      case _BGWF121_STATE_DEBUG2_WAIT:
     #endif
     #if definedinc(_BGWF121_STATE_DEBUG3_WAIT)
      case _BGWF121_STATE_DEBUG3_WAIT:
     #endif
         if (!_g_BGWF121.waitingForExpectedResponse)
         {
            if (_g_BGWF121.state == _BGWF121_STATE_PS_MAGIC_WRITE_WAIT)
            {
               _g_BGWF121.state++;
               
               SETTINGS_TCP_CLEAR_CHANGED();
               
               // i have a feeling that changing the ps keys requires a reboot
               // in order for the wf121 to use them.
               _BGWF121_ResetState();
            }
            else
            {
               _g_BGWF121.state++;
            }
         }
         else if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*3)
         {
            debug_wf121_cmds(debug_putc, "BWF121 CMD TIMEOUT sm%X\r\n", _g_BGWF121.state);
            _BGWF121_ResetState();
         }       
         break;
         
      case _BGWF121_STATE_SET_POWER_WAIT:
         if (!_g_BGWF121.waitingForExpectedResponse)
         {
           #if defined(STACK_USE_CCS_SCAN_TASK)
            if (_g_BGWF121.wifiScanDo)
            {
               _g_BGWF121.state = _BGWF121_STATE_SCAN_UNSORTED_START;
            }
            else
           #endif
            {
               _g_BGWF121.state = _BGWF121_STATE_PS_MAGIC_READ_START;
            }
         }      
         else if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*3)
         {
            debug_wf121_cmds(debug_putc, "BWF121 POWER_WAIT TIMEOUT\r\n");
            _BGWF121_ResetState();
         }       
         break;

   #if defined(STACK_USE_CCS_SCAN_TASK)
      case _BGWF121_STATE_SCAN_UNSORTED_START:
         if (_BGWF121_IsPutReady())
         {
            scr.w[0] = 0; //b[0] should be 0x00 for WIFI, b[1] whould be 0x00 to use default channels
            debug_wf121_cmds(debug_putc, "BWF121 SCAN NOW START\r\n");
            _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_WIFI_START_SCAN, 2, &scr.w[0]);
            _g_BGWF121.state = _BGWF121_STATE_SCAN_UNSORTED_WAIT;
         }
         break;

      case _BGWF121_STATE_SCAN_UNSORTED_WAIT:
      case _BGWF121_STATE_SCAN_SORTED_WAIT:
         if 
         (
            !_g_BGWF121.errorOnExpectedResponse &&
            !_g_BGWF121.waitingForExpectedResponse &&
            (
               (
                  (_g_BGWF121.state == _BGWF121_STATE_SCAN_UNSORTED_WAIT) &&
                  bit_test(_g_BGWF121.eventBitmap, _BGWF121_EVENT_BIT_SCANNED)
               ) ||
               (
                  (_g_BGWF121.state == _BGWF121_STATE_SCAN_SORTED_WAIT) &&
                  bit_test(_g_BGWF121.eventBitmap, _BGWF121_EVENT_BIT_SCAN_SORT)
               )
            )
         )
         {
            debug_wf121_cmds(debug_putc, "BWF121 SCAN DONE\r\n");
            if (_g_BGWF121.state == _BGWF121_STATE_SCAN_UNSORTED_WAIT)
               _g_BGWF121.state = _BGWF121_STATE_SCAN_SORTED_START;
            else
            {
               _g_BGWF121.wifiScanDo = FALSE;
               _g_BGWF121.wifiScanValid = TRUE;
               _g_BGWF121.state = _BGWF121_STATE_PS_MAGIC_READ_START;
            }
         }
         else if
         (
            // wait 3s for a response to the command, wait 30s for scan results
            _g_BGWF121.errorOnExpectedResponse ||
            (
               _g_BGWF121.waitingForExpectedResponse &&
               ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND * 3)
            ) ||
            (
               !_g_BGWF121.waitingForExpectedResponse &&
               ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND * 30)
            )
         )
         {
            debug_wf121_cmds(debug_putc, "BWF121 SCAN TIMEOUT\r\n");
            
            // reset WIFI and go back to connecting state.
            // this will also make WIFIScanIsBusy() return FALSE and
            // WIFIScanIsValid() return FALSE
            _g_BGWF121.state = _BGWF121_STATE_INIT;
         }
         break;
         
      case _BGWF121_STATE_SCAN_SORTED_START:
         if (_BGWF121_IsPutReady())
         {
            debug_wf121_cmds(debug_putc, "BWF121 SCAN SORT START\r\n");
            i8 = -1;  //number of results to send
            i8 = CCS_WIFISCAN_EE_NUM;
            //#warning !!! temp test code below
            //i8 = 1;
            _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_WIFI_SCAN_SORT, 1, &i8);
            _g_BGWF121.state = _BGWF121_STATE_SCAN_SORTED_WAIT;      
         }
         break;
   #endif
        
      case _BGWF121_STATE_WIFI_ON_WAIT2:
         if (_g_BGWF121.wifiOn == _BGWF121_WIFI_POWER_ON)
         {
            _g_BGWF121.state++;
         }
         else if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*20)
         {
            debug_wf121_cmds(debug_putc, "BWF121 WIFI_ON_WAIT2 TIMEOUT\r\n");
            _BGWF121_ResetState();
         }         
         break;

      case _BGWF121_STATE_SET_POWER_START:
         if (_BGWF121_IsPutReady())
         {
            i8 = 0;  //max power, low latency
            //i8 = 1;  //save power, goto sleep after 600ms of inactivity
            debug_wf121_cmds(debug_putc, "BGWF121 TX SET POWER START\r\n");
            _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_SYSTEM_POWER_SAVE, 1, &i8);
            _g_BGWF121.state++;
         }
         break;

      case _BGWF121_STATE_PS_MAGIC_READ_START:
         if (_BGWF121_IsPutReady())
         {
            debug_wf121_cmds(debug_putc, "BGWF121 TX READ MAGIC IN PS\r\n");
            scr.w[0] = __BGWF121_PS_MAGIC_LOC;
            _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_PS_LOAD, 2, &scr);
            _g_BGWF121.state++;
         }
         break;
      
      case _BGWF121_STATE_TCPIP_CONFIG_START:
         if (_BGWF121_IsPutReady())
         {
            _BGWF121_BgapiTxTcpipConfig();
         }
         break;

      case _BGWF121_STATE_TCPIP_SET_MAC_START:
         {
            unsigned int8 mac[] = DEF_MICROCHIP_DEFAULT_MAC;
            memcpy(scr.b, mac, 6);
         }         
         if (memcmp(scr.b, &AppConfig.MyMACAddr, 6) == 0)
         {
            _g_BGWF121.state += 2;  //skip this
         }
         else if (_BGWF121_IsPutReady())
         {
            debug_wf121_cmds(debug_putc, "BGWF121 TX SET MAC ");
            debug_wf121_cmds_array(&AppConfig.MyMACAddr, 6);
            debug_wf121_cmds(debug_putc, "\r\n");
            scr.b[0] = 0;  //wifi
            _BGWF121_BgapiTxPacket2(_BGAPI_COMMAND_CONFIG_SET_MAC, 1, scr.b, 6, &AppConfig.MyMACAddr);
            _g_BGWF121.state++;         
         }
         break;


      case _BGWF121_STATE_DHCP_HOSTNAME_START:
         if (_BGWF121_IsPutReady()) 
         {
           #if defined(MY_UNIT_HOSTNAME)
            debug_wf121_cmds(debug_putc, "BGWF121 TX DHCP HOSTNAME\r\n");
            _BGWF121_BgapiTxPacketROM2(_BGAPI_COMMAND_TCPIP_DHCP_HOSTNAME, (rom char*)MY_UNIT_HOSTNAME);
            _g_BGWF121.state++;
           #else
            _g_BGWF121.state += 2;
           #endif
         }
         break;

      case _BGWF121_STATE_SERVERS_DISABLE_START:
      case _BGWF121_STATE_SERVERS_CONFIG_START:
         if (_BGWF121_IsPutReady())
         {
            //scr.b[0] = https, scr.b[1] = dhcps, scr.b[2] = dnss
            memset(&scr, 0, sizeof(scr));
            if (_g_BGWF121.state == _BGWF121_STATE_SERVERS_CONFIG_START)
            {
               debug_wf121_cmds(debug_putc, "BGWF121 TX SERVERS ENABLE\r\n");
              #if defined(STACK_USE_DHCP_SERVER)
               scr.b[1] = bDHCPServerEnabled && !AppConfig.Flags.bIsDHCPEnabled;
              #endif
              #if 1
               #warning !! DNS Server disabled !!
              #else
               #if defined(STACK_USE_DNS_SERVER) && defined(STACK_EXT_MODULE_HAS_DNSS)
               scr.b[2] = AppConfig.networkType != WF_INFRASTRUCTURE;
               #endif
              #endif
            }
            else
            {
               debug_wf121_cmds(debug_putc, "BGWF121 TX SERVERS DISABLE\r\n");
            }
            _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_HTTPS_ENABLE, 3, &scr);
            _g_BGWF121.state++;
         }
         break;

     #if definedinc(_BGWF121_STATE_DEBUG1_START)
      case _BGWF121_STATE_DEBUG1_START:
         if (_BGWF121_IsPutReady())
         {
            debug_wf121_cmds(debug_putc, "BGWF121 PS READ 35\r\n");
            scr.w[0] = 35;
            _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_PS_LOAD, 2, &scr);
            _g_BGWF121.state++;
         }      
         break;
     #endif
     
     #if definedinc(_BGWF121_STATE_DEBUG2_START)
      case _BGWF121_STATE_DEBUG2_START:
         if (_BGWF121_IsPutReady())
         {
            debug_wf121_cmds(debug_putc, "BGWF121 PS READ 36\r\n");
            scr.w[0] = 36;
            _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_PS_LOAD, 2, &scr);
            _g_BGWF121.state++;
         }      
         break;
     #endif
     
     #if definedinc(_BGWF121_STATE_DEBUG3_START)
      case _BGWF121_STATE_DEBUG3_START:
         if (_BGWF121_IsPutReady())
         {
            debug_wf121_cmds(debug_putc, "BGWF121 PS READ 37\r\n");
            scr.w[0] = 37;
            _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_PS_LOAD, 2, &scr);
            _g_BGWF121.state++;
         }      
         break;
     #endif

      case _BGWF121_STATE_DHCPS_IP_START:
        #if defined(STACK_USE_DHCP_SERVER)
         if (_g_BGWF121.magicValid)
         {
            debug_wf121_cmds(debug_putc, "BGWF121 DHCP SERVER ALREADY CONFIGURED (MAGIC), SKIP\r\n");
            _g_BGWF121.state += 2;  //skip
         }
         else if (_BGWF121_IsPutReady())
         {
            ip.Val = AppConfig.MyIPAddr.Val;
            ip.v[3] += 1;         
            
            debug_wf121_cmds(debug_putc, "BGWF121 TX SET DHCP RESPONSE IP %u.%u.%u.%u\r\n", ip.v[0], ip.v[1], ip.v[2], ip.v[3]);
               
            scr.w[0] = 31; //pskey 31 is dhcps ip
            scr.b[2] = 4;  //length of ip
            _BGWF121_BgapiTxPacket2(_BGAPI_COMMAND_PS_SAVE, 3, &scr, 4, &ip);
            _g_BGWF121.state++;
         }
        #else
         debug_wf121_cmds(debug_putc, "BGWF121 NO DHCP SERVER\r\n");
         _g_BGWF121.state += 2;  //skip
        #endif
         break;
   
     #if defined(DNSS_ONLY_URL)
      case _BGWF121_STATE_DNSS_ONLY_URL_ENA_START: 
         if (_g_BGWF121.magicValid)
         {
            debug_wf121_cmds(debug_putc, "BGWF121 DNSS_VAL SKIP (MAGIC)\r\n");
            _g_BGWF121.state += 2;  //skip            
         }      
         else if (_BGWF121_IsPutReady())
         {
            debug_wf121_cmds(debug_putc, "BGWF121 TX DNSS ONLY URL ENA\r\n");
            scr.w[0] = 37; //FLASH_PS_KEY_DNSS_ANY_URL
            scr.b[2] = 1;  //length of value
            scr.b[3] = 0;   //0 new value
            _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_PS_SAVE, 4, &scr);
            _g_BGWF121.state++;
         }
         break;

      case _BGWF121_STATE_DNSS_ONLY_URL_VAL_START:
         if (_g_BGWF121.magicValid)
         {
            debug_wf121_cmds(debug_putc, "BGWF121 DNSS_URL SKIP (MAGIC)\r\n");
            _g_BGWF121.state += 2;  //skip            
         }
         else if (_BGWF121_IsPutReady())
         {
           #if defined(GENERIC_SCRATCH_BUFFER)
            #define wf121DnsOnlyPtr GENERIC_SCRATCH_BUFFER
            sprintf(wf121DnsOnlyPtr, DNSS_ONLY_URL);
           #else
            char *wf121DnsOnlyPtr = DNSS_ONLY_URL;
           #endif
           
            debug_wf121_cmds(debug_putc, "BGWF121 TX DNSS ONLY URL VAL '%s'\r\n", wf121DnsOnlyPtr);
            scr.w[0] = 36; //FLASH_PS_KEY_DNSS_URL
            scr.b[2] = strlen(wf121DnsOnlyPtr);  //length of value
            _BGWF121_BgapiTxPacket2(_BGAPI_COMMAND_PS_SAVE, 3, &scr, scr.b[2], wf121DnsOnlyPtr);
            _g_BGWF121.state++;
         }
         break;
     #endif
     
     #if defined(STACK_USE_DNS)
      case _BGWF121_STATE_DNS_IP_START:
        #if defined(STACK_USE_DHCP_CLIENT)
         if (!AppConfig.Flags.bIsDHCPEnabled)
         {
            if (_BGWF121_IsPutReady())
            {
               debug_wf121_cmds(debug_putc, "BGWF121 TX SET DNS IP\r\n");
               scr.b[0] = 0;  //set primary dns
               _BGWF121_BgapiTxPacket2(_BGAPI_COMMAND_TCPIP_DNS_CONFIG, 1, &scr.b[0], 4, &AppConfig.PrimaryDNSServer);
               _g_BGWF121.state++;
            }
         }
         else
        #endif
         {
            _g_BGWF121.state += 2;  //skip
         }
         break;
     #endif
         
#warning set channels based on WIFI_channelList[]

      case _BGWF121_STATE_PASSWORD_START:
         if 
         (
            (AppConfig.SecurityMode != WF_SECURITY_OPEN) &&
            ((AppConfig.SecurityKeyLength == 5) || (AppConfig.SecurityKeyLength >= 8))
         )
         {
            if (_BGWF121_IsPutReady())
            {
               debug_wf121_cmds(debug_putc, "BGWF121 TX WIFI PWD '%s' (%X)\r\n", AppConfig.SecurityKey, AppConfig.SecurityKeyLength);
               if (AppConfig.networkType == WF_SOFT_AP)
               {
                  _BGWF121_BgapiTxPacket2(_BGAPI_COMMAND_WIFI_AP_PWD, 1, &AppConfig.SecurityKeyLength, AppConfig.SecurityKeyLength, AppConfig.SecurityKey);
               }
               else
               {
                  _BGWF121_BgapiTxPacket2(_BGAPI_COMMAND_WIFI_PASSWORD, 1, &AppConfig.SecurityKeyLength, AppConfig.SecurityKeyLength, AppConfig.SecurityKey);
               }
               _g_BGWF121.state = _BGWF121_STATE_PASSWORD_WAIT;
            }
         }
         else
         {
            debug_wf121_cmds(debug_putc, "BGWF121 SKIP AP KEY (mode=%X keyLen=%X)\r\n", AppConfig.SecurityMode, AppConfig.SecurityKeyLength);
            _g_BGWF121.state += 2;
         }
         break;

      case _BGWF121_STATE_PS_MAGIC_WRITE_START:
         if (!_g_BGWF121.magicValid)
         {
               debug_wf121_cmds(debug_putc, "BGWF121 TX WRITE PS MAGIC\r\n");
               scr.w[0] = __BGWF121_PS_MAGIC_LOC;
               scr.b[2] = 1;  //length of val
               scr.b[3] = __BGWF121_PS_MAGIC_VAL;
               _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_PS_SAVE, 4, &scr);
               _g_BGWF121.state++;            
         }
         else
         {
            debug_wf121_cmds(debug_putc, "BGWF121 SKIP WRITING MAGIC (ALREADY VALID)\r\n");
            _g_BGWF121.state += 2;  //skip
         }
         break;

      case _BGWF121_STATE_CONNECT_START:
         if (_BGWF121_IsPutReady())
         {
            debug_wf121_cmds(debug_putc, "BGWF121 TX CONNECT SSID '%s' (%X)\r\n", AppConfig.MySSID, AppConfig.SsidLength);
            if (AppConfig.networkType == WF_SOFT_AP)
            {
               //scr.b[0] = channel, scr.b[1] = security, scr.b[2] = len of ssid
               if (WIFI_numChannelsInList == 0)
                  scr.b[0] = 6;
               else
                  scr.b[0] = WIFI_channelList[0];
               
               if (AppConfig.SecurityMode == WF_SECURITY_OPEN)
                  scr.b[1] = 0;
               else if ((AppConfig.SecurityMode == WF_SECURITY_WEP_40) || (AppConfig.SecurityMode == WF_SECURITY_WEP_104))
                  scr.b[1] = 3;
               else if ((AppConfig.SecurityMode == WF_SECURITY_WPA_WITH_KEY) || (AppConfig.SecurityMode == WF_SECURITY_WPA_WITH_PASS_PHRASE))
                  scr.b[1] = 1;
               else
                  scr.b[1] = 2;
   
               scr.b[2] = AppConfig.SsidLength;
               
               _BGWF121_BgapiTxPacket2(_BGAPI_COMMAND_WIFI_AP_START, 3, &scr, AppConfig.SsidLength, AppConfig.MySSID);   //Connect SSID--sme--WIFI
            }
            else
            {
               _BGWF121_BgapiTxPacket2(_BGAPI_COMMAND_WIFI_CONNECT_SSID, 1, &AppConfig.SsidLength, AppConfig.SsidLength, AppConfig.MySSID);   //Connect SSID--sme--WIFI
            }
            _g_BGWF121.state = _BGWF121_STATE_CONNECT_WAIT;
         }
         break;
         
      case _BGWF121_STATE_CONNECT_WAIT:
         if 
         (
            !_g_BGWF121.waitingForExpectedResponse &&
            (
               (AppConfig.networkType == WF_INFRASTRUCTURE) ||
               bit_test(_g_BGWF121.eventBitmap, _BGWF121_EVENT_BIT_AP_STARTED)
            )
         )
         {
            _g_BGWF121.state = _BGWF121_STATE_IDLE;
         }
         else if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*10)
         {
            debug_wf121_cmds(debug_putc, "BWF121 CONNECT_WAIT TIMEOUT\r\n");
            _BGWF121_ResetState();
         } 
         break;
                 
      case _BGWF121_STATE_IDLE:
         if (!_g_BGWF121.waitingForExpectedResponse && _g_BGWF121.uninitiatedEndpoint)
         {
            debug_wf121_cmds(debug_putc, "BGWF121 GOING TO CLOSE UNINITATED %LX\r\n", _g_BGWF121.uninitiatedEndpoint);
            _g_BGWF121.state = _BGWF121_STATE_CLOSE_UNINITIATED_ENDPOINT_START;
         }
        #if defined(STACK_USE_UDP) || defined(STACK_USE_TCP)
         else if (_g_BGWF121.isLinked)
         {
            _TcpUdpSocketTask();
         }
        #endif
         break;
         
      case _BGWF121_STATE_CLOSE_UNINITIATED_ENDPOINT_START:
         if (_BGWF121_IsPutReady())
         {
            for(i8=0; i8<32; i8++)
            {
               if (bit_test(_g_BGWF121.uninitiatedEndpoint, i8))
               {
                  break;
               }
            }
            if (i8 >= 32)
            {
               _g_BGWF121.state = _BGWF121_STATE_IDLE;
            }
            else
            {
               debug_wf121_cmds(debug_putc, "BGWF121 DO CLOSE UNINITATED ep%X\r\n", i8);
               bit_clear(_g_BGWF121.uninitiatedEndpoint, i8);
               bit_clear(_g_BGWF121.endpointsClosedBitmap, i8);
               _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_ENDPOINT_CLOSE, 1, &i8);
               _g_BGWF121.state = _BGWF121_STATE_CLOSE_UNINITIATED_ENDPOINT_WAIT;
            }
         }
         break;
         
      case _BGWF121_STATE_CLOSE_UNINITIATED_ENDPOINT_WAIT:
         if 
         (
            !_g_BGWF121.waitingForExpectedResponse &&
            bit_test(_g_BGWF121.endpointsClosedBitmap, i8)
         )
         {
            _g_BGWF121.state = _BGWF121_STATE_IDLE;
            debug_wf121_cmds(debug_putc, "BGWF121 CLOSE UNINITATED DONE\r\n");
         }      
         else if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*8)
         {
            debug_wf121_cmds(debug_putc, "BWF121 CLOSE_UNITIATED TIMEOUT\r\n");
            _BGWF121_ResetState();
         }  
         break;
         
      case _BGWF121_STATE_RECONNECT_START:
         _g_BGWF121.tick = TickGet();
         _g_BGWF121.state = _BGWF121_STATE_RECONNECT_WAIT;
         break;

      case _BGWF121_STATE_RECONNECT_WAIT:
         if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*2)
         {
            _g_BGWF121.state = _BGWF121_STATE_TCPIP_CONFIG_START;
         }
         break;
   }
}

#if defined(STACK_USE_CCS_SCAN_TASK)
void WIFIConnectStart(void)
{
   // don't need to do anything - after scan is complete it will re-connect to AP.
}

void WIFIScanEnableBackgroundRSSI(int1 enable)
{
}

int1 WIFIRSSIIsValid(void)
{
   return(FALSE);
   #warning !! DO !!
}

unsigned int8 WIFIRSSIGet(void)
{
   return(0);
   #warning !! DO !!
}

void WIFIScanStartDelayed(TICK t)
{
   _g_BGWF121.wifiScanValid = FALSE;
   _g_BGWF121.wifiScanDo = TRUE;
   _g_BGWF121.wifiScanNum = 0;
   
   if (t)
   {
      _g_BGWF121.wifiScanDelayed = TRUE;
      _g_BGWF121.wifiScanDelayDuration = t;
      _g_BGWF121.wifiScanDelayStart = TickGet();
   }
   else
   {
      _g_BGWF121.wifiScanDelayed = FALSE;   
   }
}

void WIFIScanStart(void)
{
  #if defined(STACK_USE_UDP) || defined(STACK_USE_TCP)
   _TcpUdpSocketsReset();
  #endif

   memset(&_g_BGWF121, 0x00, sizeof(_g_BGWF121));

   WIFIScanStartDelayed(0);
   
   _g_BGWF121.state = _BGWF121_STATE_SEND_RESET;
}

int1 WIFIScanIsBusy(void)
{
   return(_g_BGWF121.wifiScanDo);
}

int1 WIFIScanIsValid(void)
{
   return(_g_BGWF121.wifiScanValid);
}

unsigned int8 WIFIScanGetNum(void)
{
   return(_g_BGWF121.wifiScanNum);
}

void WIFIScanGetResult(unsigned int8 index, tWFScanResult *pResult)
{
   EEPROM_ADDRESS ee;
   
   ee = CCS_WIFISCAN_EE_LOC;
   
   ee += (unsigned int32)index * CCS_WIFISCAN_EE_SIZE;
   
   EEReadBytes(pResult, ee, sizeof(tWFScanResult));
}

// use _BGWF121_SpiGetBytes() to read result from WF121 over SPI.
// 'spiBytes' was the length of the packet, and this function should read
// that many bytes from the packet.
static void _WIFIScanSetResult(unsigned int8 spiBytes)
{
   tWFScanResult result;
   unsigned int8 security;
   EEPROM_ADDRESS ee;
   
   memset(&result, 0, sizeof(result));
   
   _BGWF121_GetBytes(result.bssid, 6);
   _BGWF121_GetBytes(&result.channel, 1);
   _BGWF121_GetBytes(&result.rssi, 2);
   _BGWF121_GetBytes(&result.snr, 1);
   _BGWF121_GetBytes(&security, 1);
   _BGWF121_GetBytes(&result.ssidLen, 1);
   spiBytes -= 12;
   
   if (bit_test(security, 1))
   {
      bit_set(result.apConfig, 4);
   }
   
   if (result.ssidLen > WF_MAX_SSID_LENGTH)
      result.ssidLen = WF_MAX_SSID_LENGTH;
      
   _BGWF121_GetBytes(result.ssid, result.ssidLen);
   spiBytes -= result.ssidLen;
   
   result.ssid[WF_MAX_SSID_LENGTH-1] = 0;
   result.ssidLen = strlen(result.ssid);  //do this because I have seen it return zeros for an SSID
   
   if (spiBytes)
   {
      _BGWF121_GetBytes(NULL, spiBytes);
   }
   
   debug_wf121_cmds(debug_putc, "WIFI SCAN RESULT %02X:%02X:%02X:%02X:%02X:%02X '%s' RSSI=%LX SNR=%X SEC=%X CHAN=%X\r\n",
         result.bssid[0],
         result.bssid[1],
         result.bssid[2],
         result.bssid[3],
         result.bssid[4],
         result.bssid[5],
         result.ssid,
         result.rssi,
         result.snr,
         security,
         result.channel
      );
   
   if (_g_BGWF121.wifiScanNum < CCS_WIFISCAN_EE_NUM)
   {
      ee = CCS_WIFISCAN_EE_LOC;
      
      ee += (unsigned int32)_g_BGWF121.wifiScanNum * CCS_WIFISCAN_EE_SIZE;
      
      EEWriteBytes(ee, &result, sizeof(tWFScanResult));
      
      _g_BGWF121.wifiScanNum += 1;
   }
}
#endif //if defined(STACK_USE_CCS_SCAN_TASK)

#endif   //once
