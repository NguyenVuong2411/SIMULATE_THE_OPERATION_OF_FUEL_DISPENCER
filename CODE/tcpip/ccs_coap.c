/*
   ccs_coap.c
   
   CoAP implementation.
   
   Work in progress
*/

#ifndef __CCS_COAP_C__
#define __CCS_COAP_C__

#ifndef debug_coap
   #define debug_coap(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)
#else
   #define __DO_DEBUG_COAP
#endif

#ifndef COAP_NOTIFY_NO_SOCKETS
#define COAP_NOTIFY_NO_SOCKETS(port)
#endif

#include <stdint.h>

// these values taken from RFC7252
#define _COAP_ACK_TIMEOUT  2  //seconds
#define _COAP_ACK_RANDOM_FACTOR  1.5
#ifndef _COAP_MAX_RETRANSMIT
#define _COAP_MAX_RETRANSMIT  4
#endif
#define _COAP_NSTART 1 
#define _COAP_DEFAULT_LEISURE  5 //seconds
#define _COAP_PROBING_RATE   1   //byte/second

#define _COAP_END_OF_OPTIONS_MARKER 0xFF

// valid values are: 16, 32, 64, 128, 256, 512, 1024
#ifndef COAP_TX_BLOCK_SIZE
#define COAP_TX_BLOCK_SIZE   64
#endif

#ifndef COAP_RETRY_TICKS
#define COAP_RETRY_TICKS   ((TICK)3*TICKS_PER_SECOND)
#endif

// won't attempt to do a UDPPut() a packet unless there is this much space available to transmit.
#define _COAP_MIN_TX_SIZE_NEEDED (COAP_TX_BLOCK_SIZE+128)   //bytes

#define _COAP_TX_ATTEMPT_TIMEOUT  ((TICK)5*TICKS_PER_SECOND)

typedef enum
{
   _COAP_STATE_IDLE = 0,
   _COAP_STATE_START_TX = 1,  //start a new request saved in _g_Coap.hdr, using previously configured parameters with CoapClientSet****()
   _COAP_STATE_START_RX = 2,  //a confirmable message was sent, wait for response
   _COAP_STATE_OBSERVING = 3, //listen for any observe notifications from the server
   _COAP_STATE_OBSERVE_ACK = 4   //send an ACK to an observe notification from the server
   //_COAP_STATE_TX_RST = 5  //send a RST then go back to IDLE
} _coap_state_t;

static void _CoapGotoDoneState(uint16_t response);
static void _CoapGotoIntermediateState(_coap_state_t newState, uint16_t response);
static void _CoapCloseSocket(void);
static void _CoapResetState(void);
static void _CoapPrepareResponse(uint8_t type);

typedef enum
{
   _COAP_HDR_TYPE_CON = 0,    //confirmable
   _COAP_HDR_TYPE_NON = 1,    //non-confirmable
   _COAP_HDR_TYPE_ACK = 2,
   _COAP_HDR_TYPE_RST = 3     //reset
} _coap_hdr_types_t;

typedef enum
{
   _COAP_OPT_OBSERVE = 6,
   _COAP_OPT_URI_PATH = 11,
   _COAP_OPT_URI_CONTENT_FORMAT = 12,
   _COAP_OPT_URI_URI_QUERY = 15,
   _COAP_OPT_BLOCK2 = 23,
   _COAP_OPT_BLOCK1 = 27
} _coap_opt_t;

typedef struct __PACKED
{
   //byte 0
   uint8_t tokenLength:4;  //0-8 are valid, 9-15 are reserved
   _coap_hdr_types_t type:2;
   uint8_t version:2;
   
   //byte 1
   union __PACKED
   {
      struct __PACKED
      {
         uint8_t class:3;      
         uint8_t detail:5;
      };
      uint8_t val;
   } code;
   
   //bytes 2-3
   uint16_t messageId;
} _coap_hdr_t;

typedef struct
{
   union
   {
      char * pRam;
      ROM char * pRom;
   };
   int1 romPointer;
} _coap_string_ptr_t;

#define _COAPPTR_ASSIGNED(p) ((p.romPointer && p.pRom) || p.pRam)

#define _COAPPTR_INC(p, x) if (p.romPointer) p.pRom += x; else p.pRam += x

#define _COAPPTR_DEC(p, x) if (p.romPointer) p.pRom -= x; else p.pRam -= x

static unsigned char _coapptr_deref(_coap_string_ptr_t p)
{
   if (p.romPointer && p.pRom)
   {
      return(*p.pRom);
   }
   else if (p.pRam)
   {
      return(*p.pRam);
   }
   else
   {
      return(0);
   }
}

static unsigned int16 _coapptr_strlen(_coap_string_ptr_t p)
{
   if (p.romPointer && p.pRom)
   {
      return(strlenpgm(p.pRom));
   }
   else if (p.pRam)
   {
      return(strlen(p.pRam));
   }
   else
   {
      return(0);
   }
}

typedef enum
{
   // this maps directly into the value put into the opt field
   _COAP_REQ_OBSERVE_REGISTER = 0,
   _COAP_REQ_OBSERVE_DEREGISTER = 1,
   // these values are internal to our statemachine
   _COAP_REQ_OBSERVE_NONE
} _coap_req_obsv_t;

typedef struct
{
   _coap_hdr_t hdr;
   _coap_req_obsv_t observe;
   uint8_t tries;
   uint16_t numBlock;
   _coap_string_ptr_t pPostNext;
   size_t postRemain;
   size_t postDid;
   uint16_t messageId;
   uint16_t token;
} _coap_statemachine_tx_t;

typedef struct
{
   // state status
   UDP_SOCKET socket;
   _coap_state_t state;
   TICK tick;
   TICK duration;
   uint8_t lastOptValue;
   
   // packet being tx
   _coap_statemachine_tx_t tx;

   // packet being rx
   struct
   {
      _coap_hdr_t hdr;
      coap_response_code_t response;
      uint8_t *pPayload;
      size_t payloadMax;
      size_t payloadSize;
      uint16_t token;
      int1 observing;
   } rx;
   
   
  #if defined(__DO_DEBUG_COAP)
   _coap_state_t debug;
  #endif
  
   // remote host
   UDP_PORT port;
   _coap_string_ptr_t hostname;
   _coap_string_ptr_t uriPath;
   _coap_string_ptr_t uriQuery;
   _coap_string_ptr_t pPostData;
   size_t postLen;
   int1 forceNewUdp;
} _coap_statemachine_t;

#if (COAP_MAX_CONNECTIONS > 1)
   _coap_statemachine_t * _gp_Coap;
   #define _g_Coap   (*_gp_Coap)
   #if defined(COAP_USES_MALLOC)
      _coap_statemachine_t * _gp_Coaps[COAP_MAX_CONNECTIONS];
   #else
      _coap_statemachine_t _g_Coaps[COAP_MAX_CONNECTIONS];
   #endif   
#else
   #if defined(COAP_USES_MALLOC)
      _coap_statemachine_t * _gp_Coap;
      #define _g_Coap   (*_gp_Coap)
   #else
      _coap_statemachine_t _g_Coap;
   #endif
#endif

#if (COAP_TX_BLOCK_SIZE==1024)
   #define _COAP_TX_BLOCK_SZX 6
#elif (COAP_TX_BLOCK_SIZE==512)
   #define _COAP_TX_BLOCK_SZX 5
#elif (COAP_TX_BLOCK_SIZE==256)
   #define _COAP_TX_BLOCK_SZX 4
#elif (COAP_TX_BLOCK_SIZE==128)
   #define _COAP_TX_BLOCK_SZX 3
#elif (COAP_TX_BLOCK_SIZE==64)
   #define _COAP_TX_BLOCK_SZX 2
#elif (COAP_TX_BLOCK_SIZE==32)
   #define _COAP_TX_BLOCK_SZX 1
#elif (COAP_TX_BLOCK_SIZE==16)
   #define _COAP_TX_BLOCK_SZX 0
#else
   #error invalid size used
#endif
   
void CoapClientSetHostPort(unsigned int16 port)
{
  #if defined(COAP_USES_MALLOC)
   if (!CoapIsMalloced())
      return;
  #endif
  
   debug_coap(debug_putc, "CoapClientSetHostPort(%lu)\r\n", port);
   
   _g_Coap.port = port;
   
   _g_Coap.forceNewUdp = TRUE;
}

void CoapClientSetHostName(char *hostname)
{
  #if defined(COAP_USES_MALLOC)
   if (!CoapIsMalloced())
      return;
  #endif
  
   debug_coap(debug_putc, "CoapClientSetHostName('%s')\r\n", hostname);
   
   _g_Coap.hostname.pRam = hostname;
   _g_Coap.hostname.romPointer = FALSE;
   _g_Coap.forceNewUdp = TRUE;
}

void CoapClientSetHostNameROM(ROM char *hostname)
{
  #if defined(COAP_USES_MALLOC)
   if (!CoapIsMalloced())
      return;
  #endif
  
   debug_coap(debug_putc, "CoapClientSetHostNameROM('%s')\r\n", hostname);
   
   _g_Coap.hostname.pRom = hostname;
   _g_Coap.hostname.romPointer = TRUE;
   _g_Coap.forceNewUdp = TRUE;
}

void CoapClientSetUriPath(char *uriPath)
{
  #if defined(COAP_USES_MALLOC)
   if (!CoapIsMalloced())
      return;
  #endif
  
   debug_coap(debug_putc, "CoapClientSetUriPath('%s')\r\n", uriPath);
   
   _g_Coap.uriPath.pRam = uriPath;
   _g_Coap.uriPath.romPointer = FALSE;
}

void CoapClientSetUriPathROM(ROM char *uriPath)
{
  #if defined(COAP_USES_MALLOC)
   if (!CoapIsMalloced())
      return;
  #endif
  
   debug_coap(debug_putc, "CoapClientSetUriPathROM('%s')\r\n", uriPath);
   
   _g_Coap.uriPath.pRom = uriPath;
   _g_Coap.uriPath.romPointer = TRUE;
}

void CoapClientSetUriQuery(char *uriQuery)
{
  #if defined(COAP_USES_MALLOC)
   if (!CoapIsMalloced())
      return;
  #endif
  
   debug_coap(debug_putc, "CoapClientSetUriQuery('%s')\r\n", uriQuery);
   
   _g_Coap.uriQuery.pRam = uriQuery;
   _g_Coap.uriQuery.romPointer = FALSE;
}

void CoapClientSetUriQueryROM(ROM char *uriQuery)
{
  #if defined(COAP_USES_MALLOC)
   if (!CoapIsMalloced())
      return;
  #endif
  
   debug_coap(debug_putc, "CoapClientSetUriQueryROM('%s')\r\n", uriQuery);
   
   _g_Coap.uriQuery.pRom = uriQuery;
   _g_Coap.uriQuery.romPointer = TRUE;
}

void CoapClientSetPostString(char *data)
{
  #if defined(COAP_USES_MALLOC)
   if (!CoapIsMalloced())
      return;
  #endif
  
   _g_Coap.pPostData.pRam = data;
   _g_Coap.pPostData.romPointer = FALSE;
   _g_Coap.postLen = _coapptr_strlen(_g_Coap.pPostData);
   
   debug_coap(debug_putc, "CoapClientSetPostString('%s') len=%lu\r\n", data, _g_Coap.postLen);
}

void CoapClientSetPostStringROM(ROM char *data)
{
  #if defined(COAP_USES_MALLOC)
   if (!CoapIsMalloced())
      return;
  #endif
  
   _g_Coap.pPostData.pRam = data;
   _g_Coap.pPostData.romPointer = TRUE;
   _g_Coap.postLen = _coapptr_strlen(_g_Coap.pPostData);
   
   debug_coap(debug_putc, "CoapClientSetPostStringROM('%s') len=%lu\r\n", data, _g_Coap.postLen);
}

void CoapClientSetPostBytes(uint8_t *data, size_t num)
{
  #if defined(COAP_USES_MALLOC)
   if (!CoapIsMalloced())
      return;
  #endif
  
   _g_Coap.pPostData.pRam = data;
   _g_Coap.pPostData.romPointer = FALSE;
   _g_Coap.postLen = num;
   
   debug_coap(debug_putc, "CoapClientSetPostBytes(%LX, %lu)\r\n", data, _g_Coap.postLen);
}

void CoapClientSetPostBytesROM(ROM uint8_t *data, size_t num)
{
  #if defined(COAP_USES_MALLOC)
   if (!CoapIsMalloced())
      return;
  #endif
  
   _g_Coap.pPostData.pRam = data;
   _g_Coap.pPostData.romPointer = TRUE;
   _g_Coap.postLen = num;
   
   debug_coap(debug_putc, "CoapClientSetPostStringROM(%LX, %lu)\r\n", data, _g_Coap.postLen);
}

void CoapClientSetUri(char *data)
{
   char *p;
   char *path = NULL;
   char *query = NULL;
   char *post = NULL;
   
   debug_printf(debug_putc, "CoapClientSetUri(%s)\r\n", data);
   
   p = data;
   
   if (p != NULL)
   {
      path = strchr(p, '/');
   }
   if (path != NULL)
   {
      *path = 0;
      path++;
      p = path;
   }
   
   if (p != NULL)
   {
      query = strchr(p, '?');
   }
   if (query != NULL)
   {
      *query = 0;
      query++;
      p = query;
   }
   
   if (p != NULL)
   {
      post = strchr(p, ':');
   }
   if (post != NULL)
   {
      *post = 0;
      post++;
   }
   
   CoapClientSetHostName(data);
   CoapClientSetUriPath(path);
   CoapClientSetUriQuery(query);
   CoapClientSetPostString(post);

debug_coap(debug_putc, "COAP ? '%s' '%s' '%s'\r\n", data, path, _g_Coap.hostname.pRam);
}

void CoapClientClearParams(void)
{
   CoapClientSetHostPort(5683);
   CoapClientSetPostString(NULL);
   CoapClientSetHostName(NULL);
   CoapClientSetUri(NULL);
   CoapClientSetResponsePointer(NULL, 0);
}

void CoapClientSetResponsePointer(uint8_t *p, size_t max)
{
   _g_Coap.rx.pPayload = p;
   _g_Coap.rx.payloadMax = max;
   _g_Coap.rx.payloadSize = 0;
}

int1 CoapClientIsBusy(void)
{
  #if defined(COAP_USES_MALLOC)
   if (!CoapIsMalloced())
      return(FALSE);
  #endif
  
   return(_g_Coap.state != _COAP_STATE_IDLE);
}

void CoapCancel(void)
{
   debug_coap(debug_putc, "CoapCancel()\r\n");
   _CoapResetState();
}

int1 _CoapCliendSendRequest(uint8_t code, _coap_req_obsv_t observeMethod)
{
   _coap_hdr_t hdr;
   uint8_t state;
   
  #if defined(COAP_USES_MALLOC)
   if (!CoapIsMalloced())
      return(FALSE);
  #endif

   state = _g_Coap.state;

   if ((state != _COAP_STATE_IDLE) && (state != _COAP_STATE_OBSERVING) && (state != _COAP_STATE_OBSERVE_ACK))
      return(FALSE);

   //if (_g_Coap.response != COAP_RESP_CODE_NOT_STARTED)
   //   return(FALSE);

   debug_coap(debug_putc, "_CoapCliendSendRequest(%u,%u)\r\n", code, observeMethod);

   if (_g_Coap.forceNewUdp)
   {
      _g_Coap.forceNewUdp = FALSE;
      _CoapCloseSocket();
      _g_Coap.tx.token = LFSRRand();
   }
   
   if (_g_Coap.socket == INVALID_UDP_SOCKET)
   {
      if (_g_Coap.hostname.romPointer && _g_Coap.hostname.pRom)
      {
         _g_Coap.socket = UDPOpenEx(_g_Coap.hostname.pRom, UDP_OPEN_ROM_HOST, 0, _g_Coap.port);
         debug_coap(debug_putc, "_CoapCliendSendRequest() SOCK '%s':%lu sock=%u\r\n", _g_Coap.hostname.pRom, _g_Coap.port, _g_Coap.socket);
      }
      else if (_g_Coap.hostname.pRam)
      {
         _g_Coap.socket = UDPOpenEx(_g_Coap.hostname.pRam, UDP_OPEN_RAM_HOST, 0, _g_Coap.port);
         debug_coap(debug_putc, "_CoapCliendSendRequest() SOCK '%s':%lu sock=%u\r\n", _g_Coap.hostname.pRam, _g_Coap.port, _g_Coap.socket);
      }
      
      if (_g_Coap.socket == INVALID_UDP_SOCKET)
      {
         COAP_NOTIFY_NO_SOCKETS(_g_Coap.port);
         debug_coap(debug_putc, "_CoapCliendSendRequest() NO_SOCK ???\r\n");
         _g_Coap.rx.response = COAP_RESP_PHY_ERR;
         return(FALSE);
      }
   }
   
   memset(&hdr, 0, sizeof(hdr));
   hdr.version = 1;
   hdr.code.val = code;
   _g_Coap.tx.messageId += 1;
   hdr.messageId = swaps(_g_Coap.tx.messageId);

   _g_Coap.tx.pPostNext = _g_Coap.pPostData;
   _g_Coap.tx.postRemain = _g_Coap.postLen;
   _g_Coap.tx.numBlock = 0;
   
   _g_Coap.tx.observe = observeMethod;
   if (observeMethod != _COAP_REQ_OBSERVE_NONE)
   {
      hdr.tokenLength = 2; //sizeof(_g_Coap.tx.token)
   }

   _g_Coap.tick = TickGet();
   _g_Coap.tx.tries = 0;
   
   memcpy(&_g_Coap.tx.hdr, &hdr, sizeof(hdr));
   
   _g_Coap.state = _COAP_STATE_START_TX;
   _g_Coap.rx.response = COAP_RESP_CODE_BUSY;
   
   return(TRUE);
}

int1 CoapCliendSendRequest(uint8_t code)
{
   return(_CoapCliendSendRequest(code, _COAP_REQ_OBSERVE_NONE));
}

int1 CoapClientSendObserveRequest(uint8_t enable)
{
   if (_g_Coap.rx.pPayload != NULL)
   {
      memset(_g_Coap.rx.pPayload, 0, _g_Coap.rx.payloadMax);
   }
   _g_Coap.rx.payloadSize = 0;
   
   if (enable)
   {
      if (enable == 1)
      {
         _g_Coap.tx.token = LFSRRand();
      }
      return(_CoapCliendSendRequest(COAP_METHOD_GET, _COAP_REQ_OBSERVE_REGISTER));
   }
   else
   {
      return(_CoapCliendSendRequest(COAP_METHOD_GET, _COAP_REQ_OBSERVE_DEREGISTER));
   }
}

coap_response_code_t CoapClientGetResponse(void)
{
   coap_response_code_t ret;
   
   ret = _g_Coap.rx.response;
   
   if (ret != COAP_RESP_CODE_BUSY)
   {
      if ((_g_Coap.state == _COAP_STATE_OBSERVING) || (_g_Coap.state == _COAP_STATE_OBSERVE_ACK))
      {
         _g_Coap.rx.response = COAP_RESP_CODE_BUSY;
      }
      else
      {
         _g_Coap.rx.response = COAP_RESP_CODE_NOT_STARTED;
      }
      debug_coap(debug_putc, "CoapClientGetResponse() 0x%LX -> 0x%LX\r\n", ret, _g_Coap.rx.response);
   }
   
   return(ret);
}

size_t CoapClientGetResponseSize(void)
{
   return(_g_Coap.rx.payloadSize);
}

// reset the opt delta
static void _CoapResetOpt(void)
{
   _g_Coap.lastOptValue = 0;
}

// put 'n' bytes from 'p' to UDP.
static void _CoapPutPointer(_coap_string_ptr_t p, size_t n)
{
   while(n--)
   {
      UDPPut(_coapptr_deref(p));
      _COAPPTR_INC(p, 1);
   }
}

static void _CoapPutOptHeader(uint16_t optVal, uint16_t optLen)
{
   uint8_t b = 0;
   uint16_t delta;
   uint16_t deltaExtended;
   uint16_t lengthExtended;
   uint8_t deltaExtendNum;
   uint8_t lengthExtendNum;
   
   delta = optVal - (uint16_t)_g_Coap.lastOptValue;
   _g_Coap.lastOptValue = optVal;
   
   //debug_coap(debug_putc, "_CoapPutOptHeader(%lu,%lu) ", optVal, optLen);
   
   if (delta >= (uint16_t)269)
   {
      deltaExtended = delta - (uint16_t)269;
      deltaExtendNum = 2;
      delta = 14;
      swaps(deltaExtended);
   }
   else if (delta >= (uint16_t)13)
   {
      deltaExtended = delta - (uint16_t)13;
      deltaExtendNum = 1;
      delta = 13;
   }
   else
      deltaExtendNum = 0;
   
   if (optLen >= (uint16_t)269)
   {
      lengthExtended = optLen - (uint16_t)269;
      lengthExtendNum = 2;
      optLen = 14;
      swaps(lengthExtended);
   }
   else if (optLen >= (uint16_t)13)
   {
      lengthExtended = optLen - 13;
      lengthExtendNum = 1;
      optLen = 13;
   }
   else
      lengthExtendNum = 0;
   
   b = delta * 16;
   b &= 0xF0;
   b += optLen;

   //debug_coap(debug_putc, "delta=%lu deltaext=%LX deltalen=%u ", delta, deltaExtended, deltaExtendNum);
   //debug_coap(debug_putc, "optlen=%lu lenext=%LX lenlen=%u\r\n", optLen, lengthExtended, lengthExtendNum);
 
   UDPPut(b);
   
   if (deltaExtendNum != 0)
   {
      UDPPutArray(&deltaExtended, deltaExtendNum);
   }
   
   if (lengthExtendNum != 0)
   {
      UDPPutArray(&lengthExtended, lengthExtendNum);
   }
}

// split 'p' at 'divider', at each split send new coap opt 'optVal' to UDP.
static void _CoapPutOptTokenize(uint16_t optVal, _coap_string_ptr_t p, char divider)
{
   char c;
   size_t n = 0;
   
   for(;;)
   {
      c = _coapptr_deref(p);
      
      if ((c == divider) || (c == 0))
      {
         if (n)
         {
            _COAPPTR_DEC(p, n);
            
            _CoapPutOptHeader(optVal, n);
   
            _CoapPutPointer(p, n);
            
            _COAPPTR_INC(p, n);
            
            n = 0;
         }
         
         if (c == 0)
            break;
      }
      else
         n++;
      
      _COAPPTR_INC(p, 1);
   }
}

// send coap opt 'optVal' with payload 'p'
static void _CoapPutOpt(uint16_t optVal, _coap_string_ptr_t p)
{
   size_t n;
   
   n = _coapptr_strlen(p);
   
   _CoapPutOptHeader(optVal, n);
   
   _CoapPutPointer(p, n);
}

// send coap opt 'optVal' with payload 'n' bytes from 'p'
static void _CoapPutOptRAM(uint16_t optVal, uint8_t *p, size_t n)
{
   _CoapPutOptHeader(optVal, n);
   
   if (n > 0)
   {
      UDPPutArray(p, n);
   }
}

// transmit the message in _g_Coap.tx
// returns TRUE if sent, FALSE if UDP isn't ready (try again).
static int1 _CoapTx(void)
{
   size_t txSize;
   uint32_t block1;
   int n;
   int1 more;
   uint16_t w;
   
   if 
   (
      !UDPIsOpened(_g_Coap.socket) ||
      (UDPIsPutReady(_g_Coap.socket) < _COAP_MIN_TX_SIZE_NEEDED)
   )
   {
      //if (TickIsExpired(_g_Coap.tick, _COAP_TX_ATTEMPT_TIMEOUT))
      if ((TickGet() - _g_Coap.tick) >= _COAP_TX_ATTEMPT_TIMEOUT)
      {
         _CoapGotoDoneState(COAP_RESP_PHY_ERR);
      }
      return(FALSE);
   }
   
   debug_coap(debug_putc, "_CoapTx()\r\n");

   if (_g_Coap.tx.postRemain > COAP_TX_BLOCK_SIZE)
   {
      more = TRUE;
      txSize = COAP_TX_BLOCK_SIZE;
   }
   else
   {
      more = FALSE;
      txSize = _g_Coap.tx.postRemain;
   }
   _g_Coap.tx.postDid = txSize;
  
   UDPPutArray(&_g_Coap.tx.hdr, sizeof(_coap_hdr_t));

   n = _g_Coap.tx.hdr.tokenLength;
   if (n > 0)
   {
      UDPPutArray(&_g_Coap.tx.token, n);
   }
  
   _CoapResetOpt();
      
   n = _g_Coap.tx.observe;
   if (n != _COAP_REQ_OBSERVE_NONE)
   {
      if (n == 0)
      {
         _CoapPutOptRAM(_COAP_OPT_OBSERVE, NULL, 0);
      }
      else
      {
         _CoapPutOptRAM(_COAP_OPT_OBSERVE, &n, 1);
      }
   }
   
   _CoapPutOptTokenize(_COAP_OPT_URI_PATH, _g_Coap.uriPath, '/');
   
   if (_COAPPTR_ASSIGNED(_g_Coap.uriQuery))
   {
      _CoapPutOpt(_COAP_OPT_URI_URI_QUERY, _g_Coap.uriQuery);
   }

   if (more || (_g_Coap.tx.numBlock!=0))
   {
      w = _g_Coap.tx.numBlock;
      
      block1 = w * (uint16_t)16;  //shift left 4 times
      block1 += _COAP_TX_BLOCK_SZX;
      if (more)
      {
         bit_set(block1, 3);  //more
      }
      
      if (w >= 4096)
      {
         n = 3;
         block1 = swap24(block1);
      }
      else if (w >= 16)
      {
         n = 2;
         block1 = swaps(block1);
      }
      else
      {
         n = 1;
      }
   
      _CoapPutOptRAM(_COAP_OPT_BLOCK1, &block1, n);
   }
   
   if (txSize > 0)
   {
      UDPPut(_COAP_END_OF_OPTIONS_MARKER);
   
      _CoapPutPointer(_g_Coap.tx.pPostNext, txSize);
   }
   
   UDPFlush();
   
   return(TRUE);
}

// receive a response to the message sent _g_Coap.
// returns TRUE if a message was received and _g_Coap.rx.response was updated
static int1 _CoapRx(void)
{
   uint8_t tkl;
   uint16_t optVal;
   uint16_t optDelta;
   uint8_t optHdr;
   uint16_t optLen;
   size_t num;
   
   int1 ok;
   
   if (UDPIsGetReady(_g_Coap.socket) == 0)
      return(FALSE);

   _g_Coap.rx.observing = FALSE;

   debug_coap(debug_putc, "_CoapRx()\r\n");
      
   if (UDPGetArray(&_g_Coap.rx.hdr, sizeof(_coap_hdr_t)) != sizeof(_coap_hdr_t))
   {
      debug_coap(debug_putc, "_CoapRx() BAD_HDR\r\n");
      return(FALSE);
   }
   
   tkl = _g_Coap.rx.hdr.tokenLength;
   if (tkl > 0)
   {
      if (tkl >= 2)
      {
         tkl -= 2;
         UDPGetArray(&_g_Coap.rx.token, 2);
      }
      else
      {
         UDPGetArray(&_g_Coap.rx.token, tkl);
         tkl = 0;
      }
      if (tkl > 0)
      {
         if (UDPGetArray(NULL, tkl) != tkl)
         {
            debug_coap(debug_putc, "_CoapRx() BAD_TOKEN\r\n");
            return(FALSE);
         }
      }
   }
   
   optVal = 0;
   
   while (UDPIsGetReady(_g_Coap.socket))
   {
      UDPGet(&optHdr);
         
      if (optHdr == _COAP_END_OF_OPTIONS_MARKER)
         break;

      ok = TRUE;

      optDelta = (optHdr / 16) & 0x0F;
      if (optDelta == 14)
      {
         ok = (UDPGetArray(&optDelta, 2) == 2);
         optDelta = swaps(optDelta);
         optDelta += 269;
      }
      else if (optDelta == 13)
      {
         ok = (UDPGetArray(&optDelta, 1) == 1);
         optDelta &= 0xFF;
         optDelta += 13;
      }
      optVal += optDelta;
      if (!ok)
      {
         debug_coap(debug_putc, "_CoapRx() BAD_OPT_DELTA\r\n");
         return(FALSE);
      }
      
      optLen = optHdr & 0x0F;
      if (optLen == 14)
      {
         ok = (UDPGetArray(&optLen, 2) == 2);
         optLen = swaps(optLen);
         optLen += 269;
      }
      else if (optLen == 13)
      {
         ok = (UDPGetArray(&optLen, 1) == 1);
         optLen &= 0xFF;
         optLen += 13;
      }
      if (!ok)
      {
         debug_coap(debug_putc, "_CoapRx() BAD_OPT_LEN\r\n");
         return(FALSE);
      }
      
      if (UDPGetArray(NULL, optLen) != optLen) 
      {
         debug_coap(debug_putc, "_CoapRx() BAD_OPT_DATA\r\n");
         return(FALSE);
      }
      
      // got the opt, now parse it.
      // be aware that we threw away the payload with the UPDGetArray(NULL) above.
      if (optVal == _COAP_OPT_OBSERVE)
      {
         debug_coap(debug_putc, "_CoapRx() _COAP_OPT_OBSERVE\r\n");
         _g_Coap.rx.observing = TRUE;
      }
   }
   
   num = UDPIsGetReady(_g_Coap.socket);
   if (num > 0)
   {
      if (num > _g_Coap.rx.payloadMax)
         num = _g_Coap.rx.payloadMax;
      
      _g_Coap.rx.payloadSize = UDPGetArray(_g_Coap.rx.pPayload, num);
   }
   
   UDPDiscard();
   
   debug_coap(debug_putc, "_CoapRX() OK!\r\n");
   
   //_g_Coap.rx.hdr.messageId = swaps(_g_Coap.rx.hdr.messageId);
   
   return(TRUE);
}

static void _CoapSetRetryDuration(void)
{
   _g_Coap.duration = COAP_RETRY_TICKS + COAP_RETRY_TICKS*_g_Coap.tx.tries;
   _g_Coap.tick = TickGet();
}

static void _CoapClientTaskRxAck(void)
{
   if (!_CoapRx())
   {
      //if (TickIsExpired(_g_Coap.tick, _g_Coap.duration))
      if ((TickGet() - _g_Coap.tick) >= _g_Coap.duration)
      {
         if (++_g_Coap.tx.tries >= _COAP_MAX_RETRANSMIT)
         {
            debug_coap(debug_putc, "_CoapClientTaskRxAck() GIVEUP\r\n");
            _CoapGotoDoneState(COAP_RESP_CODE_TIMEOUT);
         }
         else
         {
            debug_coap(debug_putc, "_CoapClientTaskRxAck() RETRY\r\n");
            _CoapTx();
            _CoapSetRetryDuration();
         }
      }
      return;
   }
   
   if (_g_Coap.tx.hdr.messageId != _g_Coap.rx.hdr.messageId)
   {
      debug_coap(debug_putc, "_CoapClientTaskRxAck() INV_MSGID %LX vs %LX\r\n", _g_Coap.tx.hdr.messageId, _g_Coap.rx.hdr.messageId);
      return;
   }
   
   if (_g_Coap.rx.hdr.version != 1)
   {
      debug_coap(debug_putc, "_CoapClientTaskRxAck() INV_VER %u\r\n", _g_Coap.rx.hdr.version);
      return;
   }

   if (_g_Coap.rx.hdr.type != _COAP_HDR_TYPE_ACK)
   {
      debug_coap(debug_putc, "_CoapClientTaskRxAck() NOT_ACK %u\r\n", _g_Coap.rx.hdr.type);
      return;
   }
  
   _g_Coap.tx.postRemain -= _g_Coap.tx.postDid;
  
   if 
   (
      (
         (_g_Coap.tx.hdr.code.val == COAP_METHOD_POST) ||
         (_g_Coap.tx.hdr.code.val == COAP_METHOD_PUT)
      ) &&
      (_g_Coap.tx.postRemain != 0)
   )
   {
      _COAPPTR_INC(_g_Coap.tx.pPostNext, _g_Coap.tx.postDid);
      _g_Coap.tx.messageId += 1;
      _g_Coap.tx.numBlock += 1;
      _g_Coap.tx.hdr.messageId = swaps(_g_Coap.tx.messageId);
      _g_Coap.state = _COAP_STATE_START_TX;
      debug_coap(debug_putc, "_CoapClientTaskRxAck() TX_NEXT_BLOCK\r\n");
   }
   else if 
   (
      _g_Coap.rx.observing &&
      COAP_CODE_IS_SUCCESS(_g_Coap.rx.hdr.code.val)
   )
   {
      _CoapGotoIntermediateState(_COAP_STATE_OBSERVING, _g_Coap.rx.hdr.code.val);
   }
   else
   {
      _CoapGotoDoneState(_g_Coap.rx.hdr.code.val);
   }
}

static void _CoapClientTaskRxObserve(void)
{
   uint8_t respCodeVal;
   _coap_state_t newState;
   
   if (!_CoapRx())
      return;

   respCodeVal = _g_Coap.rx.hdr.code.val;

   debug_coap(debug_putc, "_CoapClientTaskRxObserve() obs=%u val=%X\r\n", _g_Coap.rx.observing, respCodeVal);

   if (_g_Coap.rx.observing && (_g_Coap.tx.token == _g_Coap.rx.token))
   {
      if (COAP_CODE_IS_SUCCESS(respCodeVal))
      {
         newState = _COAP_STATE_OBSERVING;
         if (_g_Coap.rx.hdr.type == _COAP_HDR_TYPE_CON)
         {
            newState = _COAP_STATE_OBSERVE_ACK;
            _CoapPrepareResponse(_COAP_HDR_TYPE_ACK);
         }
         
         _CoapGotoIntermediateState(newState, respCodeVal);
      }
      else
      {
         _CoapGotoDoneState(respCodeVal);
      }
   }
   /*
   else
   {
      _CoapPrepareResponse(_COAP_HDR_TYPE_RST);
      _CoapGotoIntermediateState(_COAP_STATE_TX_RST, COAP_RESP_CODE_BUSY);
   }
   */
}

static void _CoapResetState(void)
{
   _g_Coap.rx.response = COAP_RESP_CODE_NOT_STARTED;
   _g_Coap.state = 0;
}

static void _CoapInit(void)
{
   debug_coap(debug_putc, "_CoapInit()\r\n");
   
   memset(&_g_Coap, 0, sizeof(_g_Coap));
   _g_Coap.socket = INVALID_UDP_SOCKET;
   _CoapResetState();
   _g_Coap.tx.messageId = LFSRRand();
   CoapClientClearParams();
}

static void _CoapCloseSocket(void)
{
   debug_coap(debug_putc, "_CoapCloseSocket()\r\n");
   
   if (_g_Coap.socket != INVALID_UDP_SOCKET)
   {
      UDPClose(_g_Coap.socket);
   }
   _g_Coap.socket = INVALID_UDP_SOCKET;
}

static void _CoapGotoIntermediateState(_coap_state_t newState, uint16_t response)
{
   debug_coap(debug_putc, "_CoapGotoIntermediateState(%X, %LX)\r\n", newState, response);
   _g_Coap.state = newState;
   _g_Coap.rx.response = response;
}

static void _CoapGotoDoneState(uint16_t response)
{
   debug_coap(debug_putc, "_CoapGotoDoneState(%LX)\r\n", response);
   _CoapGotoIntermediateState(_COAP_STATE_IDLE, response);
   //_CoapCloseSocket();
   
   /*
   debug_coap(debug_putc, "_CoapGotoDoneState(%LX)\r\n", response);
   _g_Coap.state = _COAP_STATE_IDLE;
   _CoapCloseSocket();
   _g_Coap.rx.response = response;
   */
}

static void _CoapPrepareResponse(uint8_t type)
{
   _coap_hdr_t hdr;
   uint16_t messageId;
   uint16_t token;
   
   debug_coap(debug_putc, "_CoapTxPrepareType(%X) oldTxMsgId=%LX\r\n", type, _g_Coap.tx.messageId);

   memset(&hdr, 0, sizeof(hdr));
   hdr.version = 1;
   hdr.type = type;
   hdr.messageId = _g_Coap.rx.hdr.messageId;
   
   messageId = _g_Coap.tx.messageId;
   token = _g_Coap.tx.token;
   memset(&_g_Coap.tx, 0, sizeof(_coap_statemachine_tx_t));
   _g_Coap.tx.messageId = messageId;
   _g_Coap.tx.token = token;

   //debug_coap(debug_putc, "_CoapPrepareResponse() oldTxMsgId=%LX newTxMsgId=%LX\r\n", messageId, _g_Coap.tx.messageId);
  
   memcpy(&_g_Coap.tx.hdr, &hdr, sizeof(hdr));
   
   _g_Coap.tick = TickGet();
}

#if (COAP_MAX_CONNECTIONS > 1)
void CoapClientUseSocket(int8 which)
{
  #if defined(COAP_USES_MALLOC)
   _gp_Coap = _gp_Coaps[which];
  #else
   _gp_Coap = &_g_Coaps[which];
  #endif
}
#endif

#if defined(COAP_USES_MALLOC)
int1 CoapMalloc(void)
{
  #if (COAP_MAX_CONNECTIONS > 1)
   int i;
   void *p;
  #endif
   int1 error = FALSE;  

   debug_coap(debug_putc, "CoapMalloc()\r\n");
   
   if (CoapIsMalloced())
      return(TRUE);

  #if (COAP_MAX_CONNECTIONS > 1)
   for(i=0; i<COAP_MAX_CONNECTIONS; i++)
   {
      p = malloc(sizeof(_coap_statemachine_t));
      if (!p)
      {
         error = TRUE;
         break;
      }
      _gp_Coaps[i] = p;
   }
   for(i=0; i<COAP_MAX_CONNECTIONS; i++)
   {
      if (error)
      {
         if (_gp_Coaps[i] != NULL)
         {
            free(_gp_Coaps[i]);
            _gp_Coaps[i] = NULL;
         }
      }
      else
      {
         CoapClientUseSocket(i);
         _CoapInit();
      }
   }
   CoapClientUseSocket(0);
  #else
   _gp_Coap = malloc(sizeof(_coap_statemachine_t));
   
   if (_gp_Coap == NULL)
      return(FALSE);  

   _CoapInit();
  #endif
   
   
   return(!error);
}
   
void CoapFree(void)
{
  #if (COAP_MAX_CONNECTIONS > 1)
   int i;
  #endif

   debug_coap(debug_putc, "CoapFree()\r\n");

  #if (COAP_MAX_CONNECTIONS > 1)
   for(i=0; i<COAP_MAX_CONNECTIONS; i++)  
   {
      CoapClientUseSocket(i);
      
      if (_gp_Coap != NULL)
      {
         _CoapCloseSocket();
         
         free(_gp_Coap);
      }
      
      _gp_Coaps[i] = NULL;
   }
   CoapClientUseSocket(0);
  #else
   if (_gp_Coap != NULL)
   {
      _CoapCloseSocket();
      
      free(_gp_Coap);
   }
   
   _gp_Coap = NULL;
  #endif
}

int1 CoapIsMalloced(void)
{
   return(_gp_Coap != NULL);
}
#endif

void CoapClientInit(void)
{
#if (COAP_MAX_CONNECTIONS > 1)
   int i;
#endif

   debug_coap(debug_putc, "CoapClientInit()\r\n");

#if (COAP_MAX_CONNECTIONS > 1)
   for(i=0; i<COAP_MAX_CONNECTIONS; i++)
   {
     #if defined(COAP_USES_MALLOC)
      _gp_Coaps[i] = NULL;
     #else
      CoapClientUseSocket(i);
      _CoapInit();
     #endif
   }
   CoapClientUseSocket(0);
#else
  #if defined(COAP_USES_MALLOC)
   _gp_Coap = NULL;
  #else
   _CoapInit();
  #endif
#endif
}

#define _COAP_GOTO_DONE_STATE(response)   \
   _g_Coap.state = _COAP_STATE_IDLE;

void CoapClientTask(void)
{
  #if (COAP_MAX_CONNECTIONS > 1)
   int i;
   void *pLast;
  #endif
  
  #if defined(COAP_USES_MALLOC)
   if (!CoapIsMalloced())
      return;
  #endif

  #if defined(__DO_DEBUG_COAP)
   if (_g_Coap.debug != _g_Coap.state)
   {
      debug_coap(debug_putc, "COAP_STATE %u -> %u\r\n", _g_Coap.debug, _g_Coap.state);
      _g_Coap.debug = _g_Coap.state;
   }
  #endif

  #if (COAP_MAX_CONNECTIONS > 1)
   pLast = _gp_Coap;
   for(i=0; i<COAP_MAX_CONNECTIONS; i++)
   {
   CoapClientUseSocket(i);
  #endif
   switch(_g_Coap.state)
   {
      case _COAP_STATE_IDLE:
         break;
         
      case _COAP_STATE_START_TX:
         if (_CoapTx())
         {
            _g_Coap.state = _COAP_STATE_START_RX;
            _CoapSetRetryDuration();
         }
         else if ((TickGet() - _g_Coap.tick) >= _COAP_TX_ATTEMPT_TIMEOUT)
         {
            if (!UDPIsOpened(_g_Coap.socket))
            {
               _CoapGotoDoneState(COAP_RESP_CODE_DNS_LOOKUP_FAIL);
            }
            else
            {
               _CoapGotoDoneState(COAP_RESP_PHY_ERR);
            }
         }
         break;   //_COAP_STATE_START_TX

      case _COAP_STATE_START_RX:
         _CoapClientTaskRxAck();  //listen for a response, send retries if needed
         break;   //_COAP_STATE_START_RX

      case _COAP_STATE_OBSERVING:
         _CoapClientTaskRxObserve();
         break;

      case _COAP_STATE_OBSERVE_ACK:
         if (_CoapTx())
         {
            _g_Coap.state = _COAP_STATE_OBSERVING;
         }
         else if ((TickGet() - _g_Coap.tick) >= TICKS_PER_SECOND)
         {
            _CoapGotoDoneState(COAP_RESP_PHY_ERR);
         }
         break;

      /*case _COAP_STATE_TX_RST:
         if (_CoapTx())
         {
            _CoapGotoDoneState(COAP_RESP_CODE_NOT_STARTED);
         }
         else if ((TickGet() - _g_Coap.tick) >= TICKS_PER_SECOND)
         {
            _CoapGotoDoneState(COAP_RESP_PHY_ERR);
         }         
         break;*/
   }
  #if (COAP_MAX_CONNECTIONS > 1)
   }
   _gp_Coap = pLast;
  #endif
}

#endif   //__CCS_COAP_C__
