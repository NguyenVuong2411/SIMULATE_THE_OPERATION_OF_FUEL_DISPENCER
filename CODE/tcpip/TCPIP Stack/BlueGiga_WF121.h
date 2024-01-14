// BlueGiga_WF121.h
//
// Library for using this module in the Microchip TCP/IP stack.
//
// This device has a few limitations/differences:
//  * No RAM access to any TCP/UDP TX/RX buffers.  That means everything
//    is done using PIC's memory.  See WF121_TCPUDP_SCRATCH_RAM_TX and 
//    WF121_TCPUDP_SCRATCH_RAM_RX documentation below for more info.
//  * Device will not allow you to open multiple server sockets that are
//    using the same port.  That means a server can only have one
//    simultaneous connection.
//  * tWFScanResult contents are different due to differen scan results
//    from this device.  'DtimPeriod', 'numRates', 'basicRateSet[]', 
//    'atimWindow', 'beaconPeriod', 'bssType' and 'reserved' are removed.  
//    'rssi' is now a 16bit value.  'snr' is added.
//  * The WIFI scan results don't tell you if it's WPA or WEP security (only
//    that security is enabled), so the WPA and WPA2 bits in tWFScanResult's 
//    apConfig will always be clear even if the AP is using WPA or WPA2.
//  * If UDPIsGetReady() or TCPIsGetReady() returns non-zero, you have one
//    task time to read that data else the next call to StackTask() may 
//    ovewrite it with data for a different socket.
//  * Device only has 4 UDP sockets.  DHCP and DNS internally use 2 sockets,
//    leaving 2 UDP sockets for the user.  Creating a listen/server UDP socket
//    is not bi-directional, that means it takes 2 UDP sockets to create
//    a listen/server UDP socket that can transmit.
//  * WF_ADHOC and WF_P2P defines not provided, since those functionality is
//    not provided by the WF121.  You can only use WF_INFRASTRUCTURE and 
//    WF_SOFT_AP.  The values of WF_SOFT_AP and WF_INFRASTRUCTURE changed to
//    better match the command-set of the WF121.
//
// Requires:
// --------------------------------------------------------------
// STREAM_SPI_BGWF121 - A stream identifier created by #use spi() for talking
//    to the BG WF121 device over SPI.  Fastest SPI clock rate is 25MHz.
//
// PIN_BGWF121_CS - A pin #defined to what's connected the BG WF121's SPI chip 
//    select.
//
// PIN_BGWF121_INT - A pin #defined to what's connected to the BG WF121's SPI
//    notify.  This pin will be low when idle, high when the WF121 has 
//    data to send.
//
// PIN_BGWF121_RESET - A pin #defined to what's connected to BG WFI121's
//    MCLR pin.  Pulsing this pin low will reset the BG WF121.
//
//
// CONFIGS:
// ----------------------------------------------------------------
// WF121_TCPUDP_SCRATCH_RAM_TX - the maximum transmit size for UDP or TCP.
//    TCPIsPutReady() and UDPIsPutReady() will never be larger than this.
//
// WF121_TCPUDP_SCRATCH_RAM_RX - the maximum receive size for UDP or TCP.
//    The maximum size for TCP is 255, UDP can be larger.  TCPIsGetReady()
//    will never be larger than this (or 255, which ever is smaller).
//    UDPIsGetReady() will never be larger than this.  Any TCP packets
//    that are larger than this will be split into multiple TCPIsGetReady()
//    messages.  Any UDP packets that are larger than this will only save the
//    first WF121_TCPUDP_SCRATCH_RAM_RX, the rest will be discarded.


#ifndef __BLUEGIGA_WF121_H__
#define __BLUEGIGA_WF121_H__

void MACInit(void);

int1 MACIsLinked(void);

// empty macros for compatiblity with the Microchip TCP/IP stack.
// they are empty because they are automatically handled by
// the module.
#define ARPInit()

int1 WIFIRSSIIsValid(void);
unsigned int8 WIFIRSSIGet(void);


// some defines that match the MRF24 needed by the stack.
// most of these aren't used but left for backwards code compatability.
#define WF_DOMAIN_FCC   0
#define MY_DEFAULT_WEP_KEY_INDEX 0
#define WF_INFRASTRUCTURE  1
#define WF_SOFT_AP         2
//#define WF_ADHOC_CONNECT_THEN_START    0
//#define WF_ADHOC_CONNECT_ONLY          1
//#define WF_ADHOC_START_ONLY            2
#define WF_SECURITY_OPEN                         0
#define WF_SECURITY_WEP_40                       1
#define WF_SECURITY_WEP_104                      2
#define WF_SECURITY_WPA_WITH_KEY                 3
#define WF_SECURITY_WPA_WITH_PASS_PHRASE         4
#define WF_SECURITY_WPA2_WITH_KEY                5
#define WF_SECURITY_WPA2_WITH_PASS_PHRASE        6
#define WF_SECURITY_WPA_AUTO_WITH_KEY            7
#define WF_SECURITY_WPA_AUTO_WITH_PASS_PHRASE    8
#define WF_SECURITY_WEP_AUTO                     9

#define WF_MAX_SSID_LENGTH 32

#if defined(STACK_USE_UDP) || defined(STACK_USE_TCP)
typedef union
{
   struct
   {
      unsigned char isUDP: 1;
      unsigned char useDNS: 1;
      unsigned char bRemoteHostIsROM : 1;   // Remote host is stored in ROM
      unsigned char isServer: 1;
      unsigned char doClose: 1;
      unsigned char clientConnected: 1;   //if this is a server socket, this gets set TRUE if we switched from listen endpoint to a client endpoint.  you still need to check endpointEnabled in conjunction with this to see if socket is connected
      unsigned char makingTempEndpoint :1;
      unsigned char dnsResolving :1;
   };
   unsigned int8 b;
} TCPUDP_SOCKET_FLAGS;
// if the stack is being reset, preserve these flags so the socket can return
// to it's normally configured state.
#define TCPUDP_SOCKET_FLAGS_PRESERVE_ON_RESET   0b1111

#if 1
// what BGWF121GetVersionInfo() returns
typedef struct
{
   uint16_t major;
   uint16_t minor;
   uint16_t patch;
   uint16_t build;
   uint16_t bootloader;
   uint16_t tcpip;
   uint16_t hw;
} bgwf121_version_t;

// If the WF121 hasn't been initialized correctly then this will return a
//    NULL pointer.
// Otherwise it will return a pointer to 14 bytes of version information.
bgwf121_version_t * BGWF121GetVersionInfo(void);
#else
#define __WF121_NO_VERSION
#define BGWF121GetVersionInfo() (0)
#endif

typedef struct
{
   union
   {
      NODE_INFO   remoteNode;      // 10 bytes for MAC and IP address
      DWORD      remoteHost;      // RAM or ROM pointer to a hostname string (ex: "www.microchip.com")
   } remote;
   
   unsigned int16    remotePort;      // Remote node's UDP/TCP port number
   unsigned int16    localPort;      // Local UDP/TCP port number
   
   unsigned int8 sm;            // State of this socket
   //TICK tick;
   unsigned int8 endpoint;    //endpoint on the external module
   //unsigned int8 oldEndpoint; //endpoint that needs closing because a TCP client connected to our old listen endpoint
   unsigned int8 tempEndpoint;   //a temporary endpoint being made to transmit a UDP message on a socket that was opened as a server
      
   TCPUDP_SOCKET_FLAGS flags;
} TCPUDP_SOCKET_INFO;

#ifndef MAX_UDP_SOCKETS
#define MAX_UDP_SOCKETS 0
#endif

#ifndef TCP_CONFIGURATION
#define TCP_CONFIGURATION 0
#endif

#define MAX_UDP_TCP_SOCKETS   (TCP_CONFIGURATION+MAX_UDP_SOCKETS)

#if (MAX_UDP_TCP_SOCKETS>25)
   #error more than the WF121 can handle
#endif

extern TCPUDP_SOCKET_INFO _g_TcpUdpSocketInfo[MAX_UDP_TCP_SOCKETS];
#define UDPSocketInfo _g_TcpUdpSocketInfo
#endif //defined(STACK_USE_UDP) || defined(STACK_USE_TCP)

/*--------------*/
/* Scan Results */
/*--------------*/
typedef struct __PACKED
{
    UINT8      bssid[6]; // Network BSSID value
    UINT8      ssid[WF_MAX_SSID_LENGTH]; // Network SSID value

    /**
      Access point configuration
      <table>
        Bit 7       Bit 6       Bit 5       Bit 4       Bit 3       Bit 2       Bit 1       Bit 0
        -----       -----       -----       -----       -----       -----       -----       -----
        Reserved    Reserved    Preamble    Privacy     Reserved    Reserved    Reserved    Reserved
      </table>
      
      <table>
      Privacy   0 : AP is open (no security)
                 1: AP using security (WEP, WPA or WPA2)
      </table>
      */

    UINT8      apConfig;
    //UINT8      reserved;
    //UINT16     beaconPeriod; // Network beacon interval          
    //UINT16     atimWindow; // Only valid if bssType = WF_INFRASTRUCTURE

    /**
      List of Network basic rates.  Each rate has the following format:
      
      Bit 7
      * 0 – rate is not part of the basic rates set
      * 1 – rate is part of the basic rates set

      Bits 6:0 
      Multiple of 500kbps giving the supported rate.  For example, a value of 2 
      (2 * 500kbps) indicates that 1mbps is a supported rate.  A value of 4 in 
      this field indicates a 2mbps rate (4 * 500kbps).
      */
    //UINT8      basicRateSet[8]; 
    
    /**
       Signal Strength RSSI
       
       MRF24WB : RSSI_MAX (200) , RSSI_MIN (106)
       MRF24WG : RSSI_MAX (128) , RSSI_MIN (43)
      */    
    UINT16      rssi; // Signal strength of received frame beacon or probe response
    UINT8       snr;
    //UINT8      numRates; // Number of valid rates in basicRates
    //UINT8      DtimPeriod; // Part of TIM element
    //UINT8      bssType; // WF_INFRASTRUCTURE or WF_ADHOC
    UINT8      channel; // Channel number
    UINT8      ssidLen; // Number of valid characters in ssid

} tWFScanResult; 
#endif
