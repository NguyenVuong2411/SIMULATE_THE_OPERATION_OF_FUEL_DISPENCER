// BlueGiga_WF121_Stack.c
//
// A Microchip TCP/IP stack equivalent library for sending/receiving 
// TCP, UDDP and other packets.

#ifndef __BLUEGIGA_WF121_STACK_C__
#define __BLUEGIGA_WF121_STACK_C__

#ifndef debug_tcpudp
#define debug_tcpudp(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z)
#endif

#if defined(STACK_USE_DHCP_SERVER)
BOOL bDHCPServerEnabled = FALSE;

void DHCPServerTask(void)
{
}

void DHCPServer_Disable(void)
{
   bDHCPServerEnabled = FALSE;
}

void DHCPServer_Enable(void)
{
   bDHCPServerEnabled = TRUE;
}
#endif

#if defined(STACK_USE_DNS_SERVER) && defined(STACK_EXT_MODULE_HAS_DNSS)
void DNSServerTask(void)
{
}
#endif

#if defined(STACK_USE_DHCP_CLIENT)
void DHCPInit(BYTE vInterface)
{
   debug_printf(debug_putc, "DHCPInit()\r\n");
   //DHCPDisable(vInterface);
}

void DHCPEnable(unsigned int8 nic)
{
   debug_printf(debug_putc, "DHCPEnable()\r\n");
   AppConfig.Flags.bIsDHCPEnabled = TRUE;
}

void DHCPDisable(unsigned int8 nic)
{
   debug_printf(debug_putc, "DHCPDisable()\r\n");
   AppConfig.Flags.bIsDHCPEnabled = FALSE;
}

BOOL DHCPIsEnabled(BYTE vInterface)
{
   if (_g_BGWF121.state >= _BGWF121_STATE_TCPIP_CONFIG_WAIT)
      return(_g_BGWF121_dhcpEnabled);
   return(AppConfig.Flags.bIsDHCPEnabled);
}

BOOL DHCPIsBound(BYTE vInterface)
{
   return(_g_BGWF121.dhcpBound);
}
#endif

#if defined(STACK_USE_UDP) || defined(STACK_USE_TCP)
TCPUDP_SOCKET_INFO _g_TcpUdpSocketInfo[MAX_UDP_TCP_SOCKETS];

#define _BGAPI_ENDPOINT_DESTINATION -1
#define _BGAPI_ENDPOINT_ROUTING 0

static void _TcpUdpScratchTxSetFree(void)
{
   debug_tcpudp(debug_putc, "_TcpUdpScratchTxSetFree() s%X t%LX\r\n", _g_BgapiTcpUdpScratchTx.socket, TickGet());
   memset(&_g_BgapiTcpUdpScratchTx, 0, sizeof(_g_BgapiTcpUdpScratchTx));
   _g_BgapiTcpUdpScratchTx.socket = -1;
}

static void _TcpUdpScratchRxSetFree(void)
{
   debug_tcpudp(debug_putc, "_TcpUdpScratchRxSetFree() t%LX\r\n", TickGet());
   memset(&_g_BgapiTcpUdpScratchRx, 0, sizeof(_g_BgapiTcpUdpScratchRx));
   _g_BgapiTcpUdpScratchRx.socket = -1;
}

static void _TcpUdpSocketInitFromPtr(TCPUDP_SOCKET_INFO *p)
{
   memset(p, 0x00, sizeof(TCPUDP_SOCKET_INFO));
   p->endpoint = -1;     //WF121 will give us the endpoint
   //p->oldEndpoint = -1; //WF121 will give us the endpoint
   p->tempEndpoint = -1;   //WF121 will give us the endpoint
}

static void _TcpUdpSocketInitFromSocket(TCPUDP_SOCKET s)
{
   _TcpUdpSocketInitFromPtr(&_g_TcpUdpSocketInfo[s]);
}

static void _TcpUdpSocketsInit(void)
{
   int i;
   for(i=0; i<MAX_UDP_TCP_SOCKETS; i++)
   {
      _TcpUdpSocketInitFromSocket(i);
   }
   
   _TcpUdpScratchRxSetFree();
   _TcpUdpScratchTxSetFree();
}

static void _TcpUdpSocketResetFromPtr(TCPUDP_SOCKET_INFO *p)
{
   if (p->sm != BGAPI_TCPUDP_UNUSED)
   {
      p->flags.b &= TCPUDP_SOCKET_FLAGS_PRESERVE_ON_RESET;
      p->endpoint = -1;     //WF121 will give us the endpoint
      //p->oldEndpoint = -1; //WF121 will give us the endpoint
      p->tempEndpoint = -1;   //WF121 will give us the endpoint
      p->sm = BGAPI_TCPUDP_DNS_START;
   }
   else
   {
      _TcpUdpSocketInitFromPtr(p);
   }
}

static void _TcpUdpSocketResetFromSocket(TCPUDP_SOCKET s)
{
   _TcpUdpSocketResetFromPtr(&_g_TcpUdpSocketInfo[s]);
}

static void _TcpUdpSocketsReset(void)
{
   int i;
   for(i=0; i<MAX_UDP_TCP_SOCKETS; i++)
   {
      _TcpUdpSocketResetFromSocket(i);
   }
   
   _TcpUdpScratchRxSetFree();
   _TcpUdpScratchTxSetFree();
}

// returns -1 if not found
/*static TCPUDP_SOCKET _BgapiEndpointToTCPServerSocket(unsigned int8 ep)
{
   int i;
   
   for(i=0; i<MAX_UDP_TCP_SOCKETS; i++)
   {
      if 
      (
         _g_TcpUdpSocketInfo[i].flags.isServer &&
         (_g_TcpUdpSocketInfo[i].tcpRemoteEndpoint == ep)
      )
      {
         return(i);
      }
   }
   
   return(-1); //not found
}*/

BOOL _TcpUdpIsConnected(TCPUDP_SOCKET s, int1 ingoreClosingFlag)
{
   TCPUDP_SOCKET_FLAGS flags;
   uint8_t ep;
   
   ep = _g_TcpUdpSocketInfo[s].endpoint;
   
   if 
   (
      (s >= MAX_UDP_TCP_SOCKETS) || 
      (_g_TcpUdpSocketInfo[s].sm == BGAPI_TCPUDP_UNUSED) || 
      (ep == -1)
   )
   {
      return(0);
   }
      
   flags.b = _g_TcpUdpSocketInfo[s].flags.b;

   return
   (
      (
         flags.isUDP ||      //if UDP, ignore isServer flag
         !flags.isServer ||
         flags.clientConnected
      ) &&
      bit_test(_g_BGWF121.endpointsActiveBitmap, ep) &&
      (
         ingoreClosingFlag ||
         !flags.doClose
      )
   );
}

// search all of our sockets and see if this endpoint is a temporary endpoing
// returns -1 if not found.
static TCPUDP_SOCKET _BgapiTempEndpointToSocket(unsigned int8 ep)
{
   int i;
   
   for(i=0; i<MAX_UDP_TCP_SOCKETS; i++)
   {
      if (_g_TcpUdpSocketInfo[i].tempEndpoint == ep)
      {
         return(i);
      }
   }
   
   return(-1); //not found
}


// returns -1 if not found
static TCPUDP_SOCKET _BgapiEndpointToSocket(unsigned int8 ep, int1 onlyIfConnected)
{
   int i;
   
   for(i=0; i<MAX_UDP_TCP_SOCKETS; i++)
   {
      if 
      (
         (_g_TcpUdpSocketInfo[i].endpoint == ep) &&
         (
            !onlyIfConnected ||
            _TcpUdpIsConnected(i, FALSE)
         )
      )
      {
         return(i);
      }
   }
   
   return(-1); //not found
}

typedef enum
{
   _TCPUDP_HOST_TYPE_SERVER = 0, //ignore 'host', open a server on 'port'
  #if defined(STACK_USE_DNS)
   _TCPUDP_HOST_TYPE_RAM_HOSTNAME = 1, //'host' points to a RAM string that needs DNS resolved, open client to 'port'
   _TCPUDP_HOST_TYPE_ROM_HOSTNAME = 2, //'host' points to a ROM string that needs DNS resolved, open client to 'port'
  #endif
   _TCPUDP_HOST_TYPE_IP_ADDRESS = 3,   //'host' is a 4 byte IP address, open client to 'port'
   _TCPUDP_HOST_TYPE_NODE_INFO = 4     //'host' is a pointer to a NODE_INFO data type that contains IP address (MAC is not important), open client to 'port'
} _tcpudp_host_type_t;

//TCPUDP_SOCKET _TcpUdpSocketOpen(DWORD remoteHost, BYTE remoteHostType, unsigned int16 localPort, unsigned int16 remotePort)
TCPUDP_SOCKET _TcpUdpSocketOpen(DWORD host, _tcpudp_host_type_t hostType, unsigned int16 localPort, unsigned int16 remotePort)
{
   int i;
   int1 repeat = FALSE;
   TCPUDP_SOCKET_INFO *ptr;
      
   if
   (
      (host == 0) ||
      (hostType == _TCPUDP_HOST_TYPE_SERVER) ||
      (remotePort == 0)
   )
   {
      host = 0;
      hostType = _TCPUDP_HOST_TYPE_SERVER;
   }
  
   // if we are attempting to create a server socket, first check
   // that one already hasn't been created with this port.  creating two
   // endpoints with the same port seems to confuse the WF121.
   if (hostType == _TCPUDP_HOST_TYPE_SERVER)
   {
      ptr = &_g_TcpUdpSocketInfo[0];
      
      for(i=0; i<MAX_UDP_TCP_SOCKETS; i++)
      {
         if 
         (
            (ptr->sm != BGAPI_TCPUDP_UNUSED) && 
            (ptr->flags.isServer) &&
            (ptr->localPort == localPort)
         )
         {
            // open the socket but put it un-connected and idle state
            debug_tcpudp(debug_putc, "_TcpUdpSocketOpen() ignored because repeat server\r\n");
            repeat = TRUE;
            break;
         }
         ptr++;
      }
   }
   
   // look for unused socket
   ptr = &_g_TcpUdpSocketInfo[0];
   for(i=0; i<MAX_UDP_TCP_SOCKETS; i++)
   {
      if (ptr->sm == BGAPI_TCPUDP_UNUSED)
         break;
      ptr++;
   }
   
   if (i >= MAX_UDP_TCP_SOCKETS)
   {
      //debug_tcpudp(debug_putc, "NO_SOCK\r\n");
      return(-1);
   }
   
   debug_tcpudp(debug_putc, "_TcpUdpSocketOpen(%LX, %X, %LX, %LX) ", host, hostType, localPort, remotePort);
   debug_tcpudp(debug_putc, "s=%X ", i);

   _TcpUdpSocketInitFromPtr(ptr);
   
   if (repeat)
   {
      debug_tcpudp(debug_putc, "REPEAT\r\n");
      ptr->sm = BGAPI_TCPUDP_IDLE;
      return(i);
   }
   
   if (hostType == _TCPUDP_HOST_TYPE_SERVER)
   {
      debug_tcpudp(debug_putc, "SERVER ");
      ptr->remotePort = 0;
      ptr->flags.isServer = TRUE;
   }
   else
   {
      debug_tcpudp(debug_putc, "CLIENT ");
      ptr->remotePort = remotePort;
   }
     
   ptr->localPort = localPort;
   
  #if defined(STACK_USE_DNS)
   if 
   (
      (hostType == _TCPUDP_HOST_TYPE_RAM_HOSTNAME) ||
      (hostType == _TCPUDP_HOST_TYPE_ROM_HOSTNAME)
   )
   {
      if (hostType == _TCPUDP_HOST_TYPE_ROM_HOSTNAME)
      {
         ptr->flags.bRemoteHostIsROM = TRUE;
      }
      ptr->flags.useDNS = TRUE;
      ptr->remote.remoteHost = host;
   }
   else
  #endif
   if (hostType == _TCPUDP_HOST_TYPE_IP_ADDRESS)
   {
      ptr->remote.remoteNode.IPAddr = host;
   }
   else  //(hostType == _TCPUDP_HOST_TYPE_NODE_INFO)
   {
      ptr->remote.remoteNode.IPAddr.Val = ((NODE_INFO*)host)->IPAddr.Val;
   }
   
   ptr->sm = BGAPI_TCPUDP_DNS_START;
   
   debug_tcpudp(debug_putc, "server%U\r\n", ptr->flags.isServer);
   
   return(i);
}

void _TcpUdpSocketClose(TCPUDP_SOCKET s)
{
   if (s == -1)
      return;
      
   debug_tcpudp(debug_putc, "_TcpUdpSocketClose(%X)\r\n", s);
   
   if (_g_TcpUdpSocketInfo[s].sm != BGAPI_TCPUDP_UNUSED)
   {
      _g_TcpUdpSocketInfo[s].flags.doClose = TRUE;
   }
}

WORD _TcpUdpSocketIsPutReady(TCPUDP_SOCKET s)
{
   if 
   (
      (s != -1) &&
      _TcpUdpIsConnected(s, FALSE) &&
      (_g_TcpUdpSocketInfo[s].sm == BGAPI_TCPUDP_IDLE) &&
      (
         (_g_BgapiTcpUdpScratchTx.socket == s) || 
         (_g_BgapiTcpUdpScratchTx.socket == -1) ||
         (_g_BgapiTcpUdpScratchTx.num == 0)
      )
   )
   {
      _g_BgapiTcpUdpScratchTx.socket = s;
      return(WF121_TCPUDP_SCRATCH_RAM_TX - _g_BgapiTcpUdpScratchTx.num);
   }
   
   return(0);
}

BOOL _TcpUdpSocketPut(TCPUDP_SOCKET s, BYTE v)
{
   if 
   (
      _TcpUdpSocketIsPutReady(s)
   )
   {
      _g_BgapiTcpUdpScratchTx.b[_g_BgapiTcpUdpScratchTx.num++] = v;
      return(TRUE);
   }
   
   return(FALSE);
}


WORD _TcpUdpSocketPutArray(TCPUDP_SOCKET s, BYTE *cData, WORD wDataLen)
{
   unsigned int16 max;

   max = _TcpUdpSocketIsPutReady(s);
  
   if (max > 0)
   {
      if (wDataLen > max)
         wDataLen = max;
         
      memcpy(&_g_BgapiTcpUdpScratchTx.b[_g_BgapiTcpUdpScratchTx.num], cData, wDataLen);
      
      _g_BgapiTcpUdpScratchTx.num += wDataLen;
      
      return(wDataLen);
   }
   
   return(0);
}

WORD _TcpUdpSocketIsGetReady(TCPUDP_SOCKET s)
{
   if
   (
      (s != -1) && 
      (_g_BgapiTcpUdpScratchRx.socket == s)
   )
   {
      return(_g_BgapiTcpUdpScratchRx.num - _g_BgapiTcpUdpScratchRx.idx);
   }
   
   return(0);
}

static void _TcpUdpSocketDiscard(TCPUDP_SOCKET s)
{
   if (_TcpUdpSocketIsGetReady(s))
   {
      _TcpUdpScratchRxSetFree();
   }
}

void _TcpUdpSocketRxSeek(TCPUDP_SOCKET s, uint16_t pos)
{
   if 
   (
      _TcpUdpSocketIsGetReady(s) &&
      (pos < _g_BgapiTcpUdpScratchRx.num)
   )
   {
      _g_BgapiTcpUdpScratchRx.idx = pos;
   }
}

BOOL _TcpUdpSocketGet(TCPUDP_SOCKET s, BYTE *v)
{
   if (_TcpUdpSocketIsGetReady(s))
   {
      *v = _g_BgapiTcpUdpScratchRx.b[_g_BgapiTcpUdpScratchRx.idx++];
      
      if (_g_BgapiTcpUdpScratchRx.idx == _g_BgapiTcpUdpScratchRx.num)
      {
         _TcpUdpScratchRxSetFree();
      }
      
      return(TRUE);
   }
   
   return(FALSE);
}

WORD _TcpUdpSocketGetArray(TCPUDP_SOCKET s, BYTE *cData, WORD wDataLen)
{
   unsigned int16 num;
   
   num = _TcpUdpSocketIsGetReady(s);
   
   if (num > 0)
   {
      if (num < wDataLen)
         wDataLen = num;
         
      memcpy(cData, &_g_BgapiTcpUdpScratchRx.b[_g_BgapiTcpUdpScratchRx.idx], wDataLen);
      
      _g_BgapiTcpUdpScratchRx.idx += wDataLen;
      
      if (_g_BgapiTcpUdpScratchRx.idx == _g_BgapiTcpUdpScratchRx.num)
      {
         _TcpUdpScratchRxSetFree();
      }
      
      return(wDataLen);
   }
   return(0);
}

#define _WF121_UDP_BIND_DELAY_   (TICKS_PER_SECOND/5)

#if 0
int1 _Wf121WorkingSocketDelayed(void)
{
   TCPUDP_SOCKET_INFO scr;
   
   memcpy(&scr, _g_BGWF121.pWorkingSocket, sizeof(TCPUDP_SOCKET_INFO));
   
   if (scr.flags.delayDo)
   {
      if ((TickGet() - scr.delayStart) >= scr.delayDuration)
      {
         scr.flags.delayDo = FALSE;
         
         debug_tcpudp(debug_putc, "WF121 SOCKET %LX DELAY END AT %LX\r\n", _g_BGWF121.pWorkingSocket, TickGet());
         
         memcpy(_g_BGWF121.pWorkingSocket, &scr, sizeof(TCPUDP_SOCKET_INFO));
      }
   }
   
   return(scr.flags.delayDo);
}

void _Wf121WorkingSocketDelay(TICK duration)
{
   TCPUDP_SOCKET_INFO scr;
   
   memcpy(&scr, _g_BGWF121.pWorkingSocket, sizeof(TCPUDP_SOCKET_INFO));
     
   scr.flags.delayDo = TRUE;
   scr.delayDuration = duration;
   scr.delayStart = TickGet();
   
   debug_tcpudp(debug_putc, "WF121 SOCKET %LX DELAY START AT %LX\r\n", _g_BGWF121.pWorkingSocket, scr.delayStart);
   
   memcpy(_g_BGWF121.pWorkingSocket, &scr, sizeof(TCPUDP_SOCKET_INFO));
}
#else
   #warning !!! remove the timing variables from the socket info struct
   #define _Wf121WorkingSocketDelayed() FALSE
   #define _Wf121WorkingSocketDelay(t)
#endif

void _TcpUdpSocketTask(void)
{
   static unsigned int8 tries;
   int i=0;
   TCPUDP_SOCKET_INFO scr;
   
   union
   {
      struct __PACKED
      {
         unsigned int8 ip[4];
         unsigned int16 port;
         unsigned int8 routing;
      } udpConnect;
      
      struct __PACKED
      {
         unsigned int8 endpoint;
         unsigned int16 port;
      } udpBind;
      
      struct __PACKED
      {
         unsigned int16 port;
         unsigned int8 destination;
      } udpServer;
      
      struct __PACKED
      {
         unsigned int8 ip[4];
         unsigned int16 port;
         unsigned int8 routing;
      } tcpConnect;
      
      struct __PACKED
      {
         unsigned int16 port;
         unsigned int8 destination;      
      } tcpServer;
      
      struct __PACKED
      {
         unsigned int8 endpoint;
         unsigned int8 len;
      } endpointSend;
   } packet;
   
   if (_g_BGWF121.pWorkingSocket && _Wf121WorkingSocketDelayed())
   {
      _g_BGWF121.pWorkingSocket = NULL;
   }
   
   if (!_g_BGWF121.pWorkingSocket)
   {
      _g_BGWF121.pWorkingSocket = _g_TcpUdpSocketInfo;
      
      // first see if anyone needs to tx or close a socket, these are
      // higher priority than attempting to open a new socket.
      
      for (i=0; i<MAX_UDP_TCP_SOCKETS; i++)
      {
         memcpy(&scr, _g_BGWF121.pWorkingSocket, sizeof(TCPUDP_SOCKET_INFO));
         
         if (_Wf121WorkingSocketDelayed())
         {
            _g_BGWF121.pWorkingSocket++;
         }
         else if
         (
            _TcpUdpIsConnected(i, TRUE) &&
            (_g_BgapiTcpUdpScratchTx.num > 0) &&
            (_g_BgapiTcpUdpScratchTx.socket == i)
         )
         {
            if 
            (
               scr.flags.isUDP && 
               scr.flags.isServer &&
               (scr.tempEndpoint == -1)
            )
            {
               // need to make a temporary client socket to transmit
               if (!scr.flags.makingTempEndpoint)
               {
                  _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_OPEN_UDP_TX_SOCKET_START;
                  debug_tcpudp(debug_putc, "%LX TCP/UDP TASK TEMP_SOCKET_TX s%X ep%X sm%X\r\n", TickGet(), i, _g_BGWF121.pWorkingSocket->endpoint, scr.sm);
               }
            }
            else
            {
               _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_TX_START;
               debug_tcpudp(debug_putc, "TCP/UDP TASK TX s%X ep%X sm%X\r\n", i, _g_BGWF121.pWorkingSocket->endpoint, scr.sm);
            }
            break;
         }
         else if
         (
            _g_BGWF121.pWorkingSocket->flags.doClose &&
            (
               (scr.sm == BGAPI_TCPUDP_IDLE) ||
               (scr.sm == BGAPI_TCPUDP_OPEN_WAIT2)
            )
         )
         {
            _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_CLOSE_START;
            debug_tcpudp(debug_putc, "TCP/UDP TASK CLOSE s%X ep%X sm%X fl%X tx_s%X tx_n%LX\r\n", i, _g_BGWF121.pWorkingSocket->endpoint, scr.sm, scr.flags.b, _g_BgapiTcpUdpScratchTx.socket, _g_BgapiTcpUdpScratchTx.num);
            break;
         }
         else
         {
            _g_BGWF121.pWorkingSocket++;
         }
      }
      
      if (i >= MAX_UDP_TCP_SOCKETS)
         _g_BGWF121.pWorkingSocket = NULL;      
   }
      
   if (!_g_BGWF121.pWorkingSocket)
   {
      _g_BGWF121.pWorkingSocket = _g_TcpUdpSocketInfo;
      
      for(i=0; i<MAX_UDP_TCP_SOCKETS; i++)
      {
         memcpy(&scr, _g_BGWF121.pWorkingSocket, sizeof(TCPUDP_SOCKET_INFO));

         if (_Wf121WorkingSocketDelayed())
         {
            _g_BGWF121.pWorkingSocket++;
         }
         else if 
         (
            (scr.sm != BGAPI_TCPUDP_UNUSED) && 
            (scr.sm != BGAPI_TCPUDP_IDLE)
         )
         {
            debug_tcpudp(debug_putc, "TCP/UDP TASK USING s%X ep%X sm%X\r\n", i, _g_BGWF121.pWorkingSocket->endpoint, scr.sm);
            break;
         }
         else
         {
            _g_BGWF121.pWorkingSocket++;
         }
      }
      
      if (i >= MAX_UDP_TCP_SOCKETS)
         _g_BGWF121.pWorkingSocket = NULL;
   }
      
   if (!_g_BGWF121.pWorkingSocket)
      return;  //no pending sockets
      
   memcpy(&scr, _g_BGWF121.pWorkingSocket, sizeof(TCPUDP_SOCKET_INFO));

   switch(scr.sm)
   {
      default:
      case BGAPI_TCPUDP_UNUSED:
      case BGAPI_TCPUDP_IDLE:
         debug_tcpudp(debug_putc, "TCP/UDP TASK DONE WITH ep%X sm%X\r\n", _g_BGWF121.pWorkingSocket->endpoint, _g_BGWF121.pWorkingSocket->sm);
         _g_BGWF121.pWorkingSocket = NULL;
         break;
         
      case BGAPI_TCPUDP_DNS_START:
         tries = 0;
         if (!scr.flags.useDNS)
         {
            _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_OPEN_START;
         }
         else if (_BGWF121_IsPutReady())
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask() start DNS resolve\r\n");
            if (scr.flags.bRemoteHostIsROM)
            {
               _BGWF121_BgapiTxPacketROM2(
                     _BGAPI_COMMAND_TCPIP_DNS_RESOLVE,
                     scr.remote.remoteHost
                  );
            }
            else
            {
               i = strlen(scr.remote.remoteHost);
               
               _BGWF121_BgapiTxPacket2(
                     _BGAPI_COMMAND_TCPIP_DNS_RESOLVE,
                     1,
                     &i,
                     i,
                     scr.remote.remoteHost
                  );
            }
            scr.flags.dnsResolving = TRUE;
            scr.sm++;
            memcpy(_g_BGWF121.pWorkingSocket, &scr, sizeof(TCPUDP_SOCKET_INFO));
         }
         break;
      
      //wait for command response
      case BGAPI_TCPUDP_DNS_WAIT1:
         if (!_g_BGWF121.waitingForExpectedResponse)
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask() got DNS response, wait for result\r\n");
            _g_BGWF121.tick = TickGet();
            _g_BGWF121.pWorkingSocket->sm += 1;  //BGAPI_TCPUDP_DNS_WAIT2
         }
         else if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*3)
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask() no DNS response\r\n");
            _g_BGWF121.pWorkingSocket->flags.dnsResolving = FALSE;
            _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_IDLE;
            _g_BGWF121.waitingForExpectedResponse = FALSE;
         }
         break;
         
      //wait for dns event
      case BGAPI_TCPUDP_DNS_WAIT2:
         if (!scr.flags.dnsResolving)
         {
            _g_BGWF121.pWorkingSocket->sm += 1;
         }
         if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*20)
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask() no DNS result\r\n");
            _g_BGWF121.pWorkingSocket->flags.dnsResolving = FALSE;
            _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_IDLE;
         }      
         break;
         
      case BGAPI_TCPUDP_OPEN_START:
         if (_BGWF121_IsPutReady())
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask() create endpoint listen%U udp%U lp%LX rp%LX\r\n", scr.flags.isServer, scr.flags.isUDP, scr.localPort, scr.remotePort);
            if (scr.flags.isUDP)
            {
               if (scr.flags.isServer)
               {
                  packet.udpServer.port = scr.localPort;
                  packet.udpServer.destination = _BGAPI_ENDPOINT_DESTINATION;
                  _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_TCPIP_UDP_LISTEN, 3, &packet.udpServer);
               }
               else
               {
                  memcpy(packet.udpConnect.ip, &scr.remote.remoteNode.IPAddr, 4);
                  packet.udpConnect.port = scr.remotePort;
                  packet.udpConnect.routing = _BGAPI_ENDPOINT_ROUTING;
                  _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_TCPIP_UDP_CONNECT, 7, &packet.udpConnect);
               }
            }
            else  //tcp
            {
               if (scr.flags.isServer)
               {
                  packet.tcpServer.port = scr.localPort;
                  packet.tcpServer.destination = _BGAPI_ENDPOINT_DESTINATION;
                  _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_TCPIP_TCP_LISTEN, 3, &packet.tcpServer);
               }
               else
               {
                  memcpy(packet.tcpConnect.ip, &scr.remote.remoteNode.IPAddr, 4);
                  packet.tcpConnect.port = scr.remotePort;
                  packet.tcpConnect.routing = _BGAPI_ENDPOINT_DESTINATION;
                  _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_TCPIP_TCP_CONNECT, 7, &packet.tcpConnect);
               }
            }
            _g_BGWF121.pWorkingSocket->sm += 1; //BGAPI_TCPUDP_OPEN_WAIT1
         }
         break;

      case BGAPI_TCPUDP_OPEN_WAIT1:
         if (!_g_BGWF121.waitingForExpectedResponse && !_g_BGWF121.errorOnExpectedResponse)
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask() endpoint created\r\n");
            _g_BGWF121.pWorkingSocket->sm += 1; //BGAPI_TCPUDP_OPEN_WAIT2
         }
         else if 
         (
            (
               !_g_BGWF121.waitingForExpectedResponse &&
               ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND/2)
            ) ||
            (
               _g_BGWF121.waitingForExpectedResponse &&
               ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*3)
            )
         )
         {
            if (!_g_BGWF121.waitingForExpectedResponse && (++tries < 20))
            {
               debug_tcpudp(debug_putc, "_TcpUdpSocketTask() endpoint create retry\r\n");
               _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_OPEN_START;
            }
            else
            {
               debug_tcpudp(debug_putc, "_TcpUdpSocketTask() no endpoint response\r\n");
               //_g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_IDLE;
               _BGWF121_ResetState();
            }
         }
         break;
        
      case BGAPI_TCPUDP_OPEN_WAIT2:
         if 
         (
            //scr.flags.endpointEnabled
            bit_test(_g_BGWF121.endpointsEnabledBitmap, scr.endpoint) &&
            _BGWF121_IsPutReady()
         )
         {
            if ((scr.flags.isUDP) && (!scr.flags.isServer) && (scr.localPort != 0))
            {
               debug_tcpudp(debug_putc, "%LX _TcpUdpSocketTask() binding endpoint\r\n", TickGet());
               packet.udpBind.port = scr.localPort;
               packet.udpBind.endpoint = scr.endpoint;
               _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_TCPIP_UDP_BIND, 3, &packet.udpBind);
               _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_UDP_BIND_WAIT;
            }
            else
            {
               debug_tcpudp(debug_putc, "_TcpUdpSocketTask() endpoint active\r\n");
               _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_IDLE;
            }
         }
         else if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*3)
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask() no endpoint active event\r\n");
            _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_IDLE;
         }
         break;
      
      case BGAPI_TCPUDP_UDP_BIND_WAIT:
         if (!_g_BGWF121.waitingForExpectedResponse)
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask() bind response\r\n");
            _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_IDLE;
            _Wf121WorkingSocketDelay(_WF121_UDP_BIND_DELAY_);
         }
         else if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND)
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask() no bind response\r\n");
            //_g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_IDLE;
            //let it go on if it didn't bind to the requested port. the local port usually isn't important
            _g_BGWF121.waitingForExpectedResponse = FALSE;
            _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_IDLE;
         }
         break;
         
      case BGAPI_TCPUDP_TX_START:
         if (_BGWF121_IsPutReady())
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask() TX ");
            if ((scr.tempEndpoint != -1) && (scr.flags.isUDP))
            {
               debug_tcpudp(debug_putc, "TEMPEP ");
               packet.endpointSend.endpoint = scr.tempEndpoint;
            }
            else
               packet.endpointSend.endpoint = scr.endpoint;
            debug_tcpudp(debug_putc, "ep%X n%X\r\n", packet.endpointSend.endpoint, _g_BgapiTcpUdpScratchTx.num);
            packet.endpointSend.len = _g_BgapiTcpUdpScratchTx.num;
            #if defined(STACK_USE_CCS_TX_EVENT)
            STACK_USE_CCS_TX_EVENT();
            #endif
            _BGWF121_BgapiTxPacket2(_BGAPI_COMMAND_ENDPOINT_SEND, 2, &packet.endpointSend, _g_BgapiTcpUdpScratchTx.num, _g_BgapiTcpUdpScratchTx.b);
            _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_TX_WAIT;
            _TcpUdpScratchTxSetFree();
         }
         break;
         
      case BGAPI_TCPUDP_TX_WAIT:
         if 
         (
            (!_g_BGWF121.waitingForExpectedResponse) ||
            ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*3)
         )
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask TX ok%U ep%X tep%X ", !_g_BGWF121.waitingForExpectedResponse, scr.endpoint, scr.tempEndpoint);
            _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_IDLE;
            if ((scr.tempEndpoint != -1) && (scr.flags.isUDP))
            {
               debug_tcpudp(debug_putc, "free_temp ");
               bit_set(_g_BGWF121.uninitiatedEndpoint, scr.tempEndpoint);
               scr.tempEndpoint = -1;
               memcpy(_g_BGWF121.pWorkingSocket, &scr, sizeof(TCPUDP_SOCKET_INFO));
            }
            _g_BGWF121.waitingForExpectedResponse = FALSE;
            debug_tcpudp(debug_putc, "\r\n");
         }
         break;
      
      case BGAPI_TCPUDP_CLOSE_START:
         debug_tcpudp(debug_putc, "_TcpUdpSocketTask() close ep%X s%X ", scr.endpoint, _BgapiEndpointToSocket(scr.endpoint, FALSE));
         if (scr.endpoint != -1)
         {
            if (_g_BgapiTcpUdpScratchTx.socket == _BgapiEndpointToSocket(scr.endpoint, FALSE))
            {
               _TcpUdpScratchTxSetFree();
            }
            if (_g_BgapiTcpUdpScratchRx.socket == _BgapiEndpointToSocket(scr.endpoint, FALSE))
            {
               _TcpUdpScratchRxSetFree();
            }

           #if defined(__BGWF121_TCP_SERVER_WONT_SIMULTANEOUS__)
            if (!scr.flags.isUDP && scr.flags.clientConnected)
            {
               // When a client connected, we set the .clientConnected flag and overwrote the .endpoint with the client endpoint.
               // Now make .endpoint back to the original server/listen endpoint.
               debug_tcpudp(debug_putc, "clientConnected,ep=%X ", scr.endpoint);
               scr.flags.clientConnected = FALSE;
               bit_set(_g_BGWF121.uninitiatedEndpoint, scr.endpoint);
               scr.endpoint = scr.tempEndpoint;
               scr.tempEndpoint = -1;
            }
           #endif
            
            memcpy(_g_BGWF121.pWorkingSocket, &scr, sizeof(TCPUDP_SOCKET_INFO));
           
            if (scr.flags.isServer && !scr.flags.isUDP)
            {
               debug_tcpudp(debug_putc, "RESET ");
              #if defined(__BGWF121_TCP_SERVER_WONT_SIMULTANEOUS__)
               _g_BGWF121.pWorkingSocket->flags.doClose = FALSE;
               _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_IDLE;
              #else
               _TcpUdpSocketResetFromPtr(_g_BGWF121.pWorkingSocket);
              #endif
            }
            else
            {
               debug_tcpudp(debug_putc, "INIT ");
               bit_set(_g_BGWF121.uninitiatedEndpoint, scr.endpoint);
               _TcpUdpSocketInitFromPtr(_g_BGWF121.pWorkingSocket);
            }       
         }
         else
         {
            // shouldn't have gotten here... but ok.
            debug_tcpudp(debug_putc, "CONFUSED_INIT ");
            _TcpUdpSocketInitFromPtr(_g_BGWF121.pWorkingSocket);
         }
         debug_tcpudp(debug_putc, "\r\n");
         _g_BGWF121.pWorkingSocket = NULL;
         break;         
         
         /*
      case BGAPI_TCPUDP_CLOSE_START:
         if (scr.endpoint != -1)
         {
            if (_BGWF121_IsPutReady())
            {
               debug_tcpudp(debug_putc, "_TcpUdpSocketTask() close ep%X s%X\r\n", scr.endpoint, _BgapiEndpointToSocket(scr.endpoint, FALSE));
               if (_g_BgapiTcpUdpScratchTx.socket == _BgapiEndpointToSocket(scr.endpoint, FALSE))
               {
                  _TcpUdpScratchTxSetFree();
               }
               if (_g_BgapiTcpUdpScratchRx.socket == _BgapiEndpointToSocket(scr.endpoint, FALSE))
               {
                  _TcpUdpScratchRxSetFree();
               }
               #error do .clientConnected here
               _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_ENDPOINT_CLOSE, 1, &scr.endpoint);
               _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_CLOSE_WAIT;
            }
         }
         else
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask() close unused ep\r\n");
            _TcpUdpSocketInitFromPtr(_g_BGWF121.pWorkingSocket);
            _g_BGWF121.pWorkingSocket = NULL;         
         }
         break;
         
      case BGAPI_TCPUDP_CLOSE_WAIT:
         if 
         (
            !_g_BGWF121.waitingForExpectedResponse && 
            !bit_test(_g_BGWF121.endpointsEnabledBitmap, scr.endpoint)
         )
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask close response server%U\r\n", scr.flags.isServer);
            
            if (scr.flags.isServer && !scr.flags.isUDP)
            {
               _TcpUdpSocketResetFromPtr(_g_BGWF121.pWorkingSocket);
            }
            else
            {
               _TcpUdpSocketInitFromPtr(_g_BGWF121.pWorkingSocket);
            }
            
            _g_BGWF121.pWorkingSocket = NULL;
         }
         else if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*3)
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask close no response\r\n");
            _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_CLOSE_START;
         } 
         break;        
         */
         
      case BGAPI_TCPUDP_OPEN_UDP_TX_SOCKET_START:
         if (_BGWF121_IsPutReady())
         {
            debug_tcpudp(debug_putc, "%LX _TcpUdpSocketTask creating temp tx endpoint\r\n", TickGet());
            memcpy(packet.udpConnect.ip, &scr.remote.remoteNode.IPAddr, 4);
            packet.udpConnect.port = scr.remotePort;
            packet.udpConnect.routing = _BGAPI_ENDPOINT_ROUTING;
   
            _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_TCPIP_UDP_CONNECT, 7, &packet.udpConnect);      
            _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_OPEN_UDP_TX_SOCKET_WAIT;
            _g_BGWF121.pWorkingSocket->flags.makingTempEndpoint = 1;
           #if 0
            #warning !! FORCING ERROR
            _g_BGWF121.waitingForExpectedResponse = FALSE;
           #endif
         }
         break;
         
      case BGAPI_TCPUDP_OPEN_UDP_TX_SOCKET_WAIT:
         if 
         (
            (!_g_BGWF121.waitingForExpectedResponse) && 
            (scr.tempEndpoint != -1) &&
            bit_test(_g_BGWF121.endpointsEnabledBitmap, scr.tempEndpoint) &&
            _BGWF121_IsPutReady()
         )
         {
            debug_tcpudp(debug_putc, "%LX _TcpUdpSocketTask temp tx created ep%X, now TX\r\n", TickGet(), scr.tempEndpoint);
            
            //_g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_TX_START;

            debug_tcpudp(debug_putc, "_TcpUdpSocketTask() binding TEMP endpoint to %LX\r\n", scr.localPort);
            packet.udpBind.port = scr.localPort;
            packet.udpBind.endpoint = scr.tempEndpoint;
            _BGWF121_BgapiTxPacket(_BGAPI_COMMAND_TCPIP_UDP_BIND, 3, &packet.udpBind);
            _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_OPEN_UDP_TX_BIND_WAIT;
         }
         else if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*3)
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask create temp tx NO response\r\n");
            if (scr.tempEndpoint != -1)
               bit_set(_g_BGWF121.uninitiatedEndpoint, scr.tempEndpoint);
            _g_BGWF121.pWorkingSocket->flags.makingTempEndpoint = 0;
            _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_IDLE;
            _TcpUdpScratchTxSetFree();
            _g_BGWF121.waitingForExpectedResponse = FALSE;
         } 
         break;

      case BGAPI_TCPUDP_OPEN_UDP_TX_BIND_WAIT:
         if (!_g_BGWF121.waitingForExpectedResponse)
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask bind tx response\r\n");
            _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_TX_START;
            _Wf121WorkingSocketDelay(_WF121_UDP_BIND_DELAY_);
         }
         else if ((TickGet() - _g_BGWF121.tick) >= TICKS_PER_SECOND*3)
         {
            debug_tcpudp(debug_putc, "_TcpUdpSocketTask bind tx NO response\r\n");
            _g_BGWF121.waitingForExpectedResponse = FALSE;
            bit_set(_g_BGWF121.uninitiatedEndpoint, scr.tempEndpoint);
            _g_BGWF121.pWorkingSocket->sm = BGAPI_TCPUDP_IDLE;
            _TcpUdpScratchTxSetFree();
         }
         break;
   }
}
#endif   //defined(STACK_USE_UDP) || defined(STACK_USE_TCP)

#if defined(STACK_USE_UDP)
void UDPInit()
{
}

UDP_SOCKET UDPOpenEx(DWORD remoteHost, BYTE remoteHostType, UDP_PORT localPort, UDP_PORT remotePort)
{
   UDP_SOCKET s;
   //UDP_PORT port;

   // as of this writing, the remoteHostType #defines in UDP.H match 
   // the _tcpudp_host_type_t, so no translation needed.

   s = _TcpUdpSocketOpen(remoteHost, remoteHostType, localPort, remotePort);
   
   if (s != -1)
   {
      _g_TcpUdpSocketInfo[s].flags.isUDP = TRUE;
   }
   else
   {
      s = INVALID_UDP_SOCKET;
   }
   
   return(s);
}

void UDPClose(UDP_SOCKET s)
{
   if (s < MAX_UDP_TCP_SOCKETS)
   {
      _g_TcpUdpSocketInfo[s].flags.isServer = FALSE;
      
      _TcpUdpSocketClose(s);
   }
}

static BOOL _UDPSocketIsTxValid(void)
{
   return
   (
      (_g_BgapiTcpUdpScratchTx.socket != -1) &&
      (_g_BgapiTcpUdpScratchTx.socket < MAX_UDP_TCP_SOCKETS) &&
      (_g_TcpUdpSocketInfo[_g_BgapiTcpUdpScratchTx.socket].sm == BGAPI_TCPUDP_IDLE)
   );
}

WORD UDPIsPutReady(UDP_SOCKET s)
{
   return(_TcpUdpSocketIsPutReady(s));
}

BOOL UDPPut(BYTE v)
{
   if (_UDPSocketIsTxValid())
   {
      _TcpUdpSocketPut(_g_BgapiTcpUdpScratchTx.socket, v);
      return(TRUE);
   }
   
   return(FALSE);
}

WORD UDPPutArray(BYTE *cData, WORD wDataLen)
{
   if (_UDPSocketIsTxValid())
   {
      return(_TcpUdpSocketPutArray(_g_BgapiTcpUdpScratchTx.socket, cData, wDataLen));
   }
   
   return(0);
}

BYTE* UDPPutString(BYTE *strData)
{
   unsigned int16 len;
   
   len = strlen(strData);
   
   strData += UDPPutArray(strData, len);
   
   return(strData);
}

void UDPFlush(void)
{
}

WORD UDPIsGetReady(UDP_SOCKET s)
{
   return(_TcpUdpSocketIsGetReady(s));
}

BOOL UDPGet(BYTE *v)
{
   return(_TcpUdpSocketGet(_g_BgapiTcpUdpScratchRx.socket, v));
}

void UDPSetRxBuffer(WORD wOffset)
{
   _TcpUdpSocketRxSeek(_g_BgapiTcpUdpScratchRx.socket, wOffset);
}

WORD UDPGetArray(BYTE *cData, WORD wDataLen)
{
   return(_TcpUdpSocketGetArray(_g_BgapiTcpUdpScratchRx.socket, cData, wDataLen));
}

void UDPDiscard(void)
{
   _TcpUdpSocketDiscard(_g_BgapiTcpUdpScratchRx.socket);
}

BOOL UDPIsOpened(UDP_SOCKET socket)
{
   return(_TcpUdpIsConnected(socket, FALSE));
}
#endif

#if defined(STACK_USE_TCP)
void TCPInit(void)
{
}

//SOCKET_INFO* TCPGetRemoteInfo(TCP_SOCKET hTCP) {}
//BOOL TCPWasReset(TCP_SOCKET hTCP) {}
BOOL TCPIsConnected(TCP_SOCKET hTCP) 
{
   return(_TcpUdpIsConnected(hTCP, FALSE));
}

void TCPDisconnect(TCP_SOCKET hTCP)    //close connection, but re-init listen
{
   _TcpUdpSocketClose(hTCP);
}

void TCPClose(TCP_SOCKET hTCP)   //close all connections
{
   if (hTCP < MAX_UDP_TCP_SOCKETS)
   {
      _g_TcpUdpSocketInfo[hTCP].flags.isServer = FALSE;
      _TcpUdpSocketClose(hTCP);
   }
}

WORD TCPIsPutReady(TCP_SOCKET hTCP)
{
   return(_TcpUdpSocketIsPutReady(hTCP));
}

BOOL TCPPut(TCP_SOCKET hTCP, BYTE byte)
{
   return(_TcpUdpSocketPut(hTCP, byte));
}

WORD TCPPutArray(TCP_SOCKET hTCP, BYTE* Data, WORD Len)
{
   return(_TcpUdpSocketPutArray(hTCP, Data, Len));
}

BYTE* TCPPutString(TCP_SOCKET hTCP, BYTE* Data) 
{
   WORD len;
   
   len = strlen(Data);
   
   len = TCPPutArray(hTCP, Data, len);
   
   return(Data + len);
}

WORD TCPIsGetReady(TCP_SOCKET hTCP) 
{
   return(_TcpUdpSocketIsGetReady(hTCP));
}

//WORD TCPGetRxFIFOFree(TCP_SOCKET hTCP) {}
BOOL TCPGet(TCP_SOCKET hTCP, BYTE* byte) 
{
   return(_TcpUdpSocketGet(hTCP, byte));
}

WORD TCPGetArray(TCP_SOCKET hTCP, BYTE* buffer, WORD count) 
{
   return(_TcpUdpSocketGetArray(hTCP, buffer, count));
}

//BYTE TCPPeek(TCP_SOCKET hTCP, WORD wStart) {}
//WORD TCPPeekArray(TCP_SOCKET hTCP, BYTE *vBuffer, WORD wLen, WORD wStart) {}
//WORD TCPFindEx(TCP_SOCKET hTCP, BYTE cFind, WORD wStart, WORD wSearchLen, BOOL bTextCompare) {}
//WORD TCPFindArrayEx(TCP_SOCKET hTCP, BYTE* cFindArray, WORD wLen, WORD wStart, WORD wSearchLen, BOOL bTextCompare) {}
void TCPDiscard(TCP_SOCKET hTCP) 
{
   _TcpUdpSocketDiscard(hTCP);
}

//BOOL TCPProcess(NODE_INFO* remote, IP_ADDR* localIP, WORD len) {}
void TCPFlush(TCP_SOCKET hTCP) 
{
}

BOOL TCPWasReset(TCP_SOCKET hTCP)
{
   return(FALSE);
}

BOOL TCPAdjustFIFOSize(TCP_SOCKET hTCP, WORD wMinRXSize, WORD wMinTXSize, BYTE vFlags)
{
   return(FALSE);
}

TCP_SOCKET TCPOpen(DWORD dwRemoteHost, BYTE vRemoteHostType, WORD wPort, BYTE vSocketPurpose) 
{
   TCP_SOCKET s;
   
   // as of this writing, the vRemoteHostType #defines in TCP.H match 
   // the _tcpudp_host_type_t, so no translation needed.
   
   s = _TcpUdpSocketOpen(dwRemoteHost, vRemoteHostType, wPort, wPort);
   
   if (s == -1)
   {
      s = INVALID_SOCKET;
   }
   
   return(s);
}

#if defined(__18CXX)
   //WORD TCPFindROMArrayEx(TCP_SOCKET hTCP, ROM BYTE* cFindArray, WORD wLen, WORD wStart, WORD wSearchLen, BOOL bTextCompare) {}
   //WORD TCPPutROMArray(TCP_SOCKET hTCP, ROM BYTE* Data, WORD Len) {}
   ROM BYTE* TCPPutROMString(TCP_SOCKET hTCP, ROM BYTE* Data) 
   {
      int1 ok;
      char c;
      
      for(;;)
      {
         c = *Data;
         if (c == 0)
            return(Data);
         ok = TCPPut(hTCP, *Data);
         if (!ok)
            return(Data);
         Data++;
      }
   }
#endif
//BOOL TCPAdjustFIFOSize(TCP_SOCKET hTCP, WORD wMinRXSize, WORD wMinTXSize, BYTE vFlags) {}
void TCPTouch(TCP_SOCKET s) 
{
   #warning !! TODO ADD DISCONNECT TIMER !!
}
WORD TCPGetTxFIFOFull(TCP_SOCKET hTCP) {return(WF121_TCPUDP_SCRATCH_RAM_TX - TCPIsPutReady(hTCP));}
#endif

#endif
