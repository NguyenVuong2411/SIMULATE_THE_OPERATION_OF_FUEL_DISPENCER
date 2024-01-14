// BlueGiga_WF121.c
//
// library for using this module in the Microchip TCP/IP stack.

#ifndef __BLUEGIGA_WF121_C__
#define __BLUEGIGA_WF121_C__

#if !defined(HTTP_SAVE_CONTEXT_IN_PIC_RAM)
#error invalid stack configuration option
#endif

// on June 25 2015, Jeff @ Bluegiga sent me a beta/modified firmware where
// a TCP server socket would only accept one simulataneous server socket.
// with this beta/modified firmware in place we can remove the code that I
// added that closed the server socket on a client connection.  the define
// below will remove the code that closes the server socket on a client 
// connection and should be defined if we are using Jeff's June 25 firmware.
#define __BGWF121_TCP_SERVER_WONT_SIMULTANEOUS__

typedef enum
{
   BGAPI_TCPUDP_UNUSED = 0,
   BGAPI_TCPUDP_DNS_START,
   BGAPI_TCPUDP_DNS_WAIT1,
   BGAPI_TCPUDP_DNS_WAIT2,
   BGAPI_TCPUDP_OPEN_START,
   BGAPI_TCPUDP_OPEN_WAIT1,
   BGAPI_TCPUDP_OPEN_WAIT2,
   BGAPI_TCPUDP_UDP_BIND_WAIT,
   BGAPI_TCPUDP_IDLE,
   BGAPI_TCPUDP_TX_START,
   BGAPI_TCPUDP_TX_WAIT,
   BGAPI_TCPUDP_OPEN_UDP_TX_SOCKET_START,
   BGAPI_TCPUDP_OPEN_UDP_TX_SOCKET_WAIT,
   BGAPI_TCPUDP_OPEN_UDP_TX_BIND_WAIT,
   BGAPI_TCPUDP_CLOSE_START
   //BGAPI_TCPUDP_CLOSE_WAIT,
} _bgapi_tcpudp_state_t;

#if defined(STACK_USE_UDP) || defined(STACK_USE_TCP)
#ifndef WF121_TCPUDP_SCRATCH_RAM_TX
#define WF121_TCPUDP_SCRATCH_RAM_TX   255
#endif

#ifndef WF121_TCPUDP_SCRATCH_RAM_RX
#define WF121_TCPUDP_SCRATCH_RAM_RX   255
#endif
struct
{
   unsigned int8 socket;      //which socket is currently using the buffer, -1 if no one
  #if (WF121_TCPUDP_SCRATCH_RAM_TX>=0x100)
   unsigned int16 num;
  #else
   unsigned int8 num;
  #endif
   unsigned int8 b[WF121_TCPUDP_SCRATCH_RAM_TX];
} _g_BgapiTcpUdpScratchTx = {-1};

struct
{
   unsigned int8 socket;      //which socket is currently using the buffer, -1 if no one
  #if (WF121_TCPUDP_SCRATCH_RAM_RX>=0x100)
   unsigned int16 num;        //number of bytes put into buffer
   unsigned int16 idx;
  #else
   unsigned int8 num;        //number of bytes put into buffer
   unsigned int8 idx;
  #endif
   unsigned int8 b[WF121_TCPUDP_SCRATCH_RAM_RX];
} _g_BgapiTcpUdpScratchRx = {-1};

typedef unsigned int8 TCPUDP_SOCKET;   //index into _g_TcpUdpSocketInfo
static TCPUDP_SOCKET _BgapiEndpointToSocket(unsigned int8 ep, int1 onlyIfConnected);
static static TCPUDP_SOCKET _BgapiTempEndpointToSocket(unsigned int8 ep);
//static TCPUDP_SOCKET _BgapiEndpointToTCPServerSocket(unsigned int8 ep);
#endif   //defined(STACK_USE_UDP) || defined(STACK_USE_TCP)

static void _TcpUdpSocketsInit(void);
static void _TcpUdpSocketsReset(void);
void _TcpUdpSocketTask(void);

#include "BlueGiga_WF121_Commands.c"
#include "BlueGiga_WF121_Stack.c"

#endif
